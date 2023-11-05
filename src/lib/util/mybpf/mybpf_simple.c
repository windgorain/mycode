/*================================================================
*   Copyright (C), Xingang.Li
*   Author:      LiXingang  Version: 1.0  Date: 2017-10-1
*   Description: simple bpf file
*   在某些场景,如openwrt/iot/sdwan等资源有限场景,可以使用spf代替elf
*
================================================================*/
#include <sys/mman.h>
#include "bs.h"
#include "utl/file_utl.h"
#include "utl/elf_utl.h"
#include "utl/vbuf_utl.h"
#include "utl/bpf_helper_utl.h"
#include "utl/umap_utl.h"
#include "utl/mybpf_vm.h"
#include "utl/mybpf_runtime.h"
#include "utl/mybpf_loader.h"
#include "utl/mybpf_prog.h"
#include "utl/mybpf_elf.h"
#include "utl/mybpf_file.h"
#include "utl/mybpf_relo.h"
#include "utl/mybpf_simple.h"
#include "utl/mybpf_jit.h"
#include "utl/mybpf_insn.h"

#define MYBPF_SIMPLE_PRIVATE_NAME_MAX_LEN 255


typedef struct {
    UINT sec_size; 
    UCHAR sec_type;
}MYBPF_SIMPLE_END_S;

typedef struct {
    int helpers[1024];
    int count;
}MYBPF_HELPER_DEPENDS_S;

static int _mybpf_simple_write(VBUF_S *vbuf, void *mem, int size)
{
    if (VBUF_CatFromBuf(vbuf, mem, size) < 0) {
        RETURNI(BS_CAN_NOT_WRITE, "Can't write buf");
    }
    return 0;
}

static int _mybpf_simple_write_header(VBUF_S *vbuf)
{
    MYBPF_SIMPLE_HDR_S hdr = {0};

    hdr.magic = MYBPF_SIMPLE_MAGIC;
    hdr.ver = MYBPF_SIMPLE_VER;

    
    return _mybpf_simple_write(vbuf, &hdr, sizeof(hdr));
}

static int _mybpf_simple_write_totle_size(VBUF_S *vbuf, UINT size)
{
    MYBPF_SIMPLE_HDR_S *hdr;

    hdr = VBUF_GetData(vbuf);
    if (! hdr) {
        RETURN(BS_ERR);
    }

    hdr->totle_size = htonl(size);

    return 0;
}

static int _mybpf_simple_write_global_data(VBUF_S *vbuf, ELF_GLOBAL_DATA_S *data, int map_def_size)
{
    UMAP_ELF_MAP_S *map = MEM_ZMalloc(map_def_size);
    int ret = 0;

    if (! map) {
        RETURN(BS_NO_MEMORY);
    }

    map->type = BPF_MAP_TYPE_ARRAY;
    map->size_key = sizeof(int);
    map->max_elem = 1;

    

    if (data->have_bss) {
        map->size_value = data->bss_sec.data->d_size;
        ret |= _mybpf_simple_write(vbuf, map, map_def_size);
    }

    if (data->have_data) {
        map->size_value = data->data_sec.data->d_size;
        ret |= _mybpf_simple_write(vbuf, map, map_def_size);
    }

    for (int i=0; i<data->rodata_count; i++) {
        map->size_value = data->rodata_sec[i].data->d_size;
        ret |= _mybpf_simple_write(vbuf, map, map_def_size);
    }

    MEM_Free(map);

    return ret;
}

static int _mybpf_simple_write_maps_sec(VBUF_S *vbuf, ELF_S *elf, ELF_GLOBAL_DATA_S *data, MYBPF_MAPS_SEC_S *map_sec)
{
    MYBPF_SIMPLE_MAP_HDR_S hdr = {0};
    char *map;
    int i;
    int ret = 0;
    int map_count;
    int map_def_size;
    
    map_def_size = UMAP_ELF_MAP_MIN_SIZE;
    if (map_sec->map_def_size) {
        map_def_size = map_sec->map_def_size;
        BS_DBGASSERT(map_def_size >= UMAP_ELF_MAP_MIN_SIZE);
    }

    map_count = data->sec_count + map_sec->map_count;

    hdr.sec_type = MYBPF_SIMPLE_SEC_TYPE_MAP;
    hdr.sec_size = sizeof(MYBPF_SIMPLE_MAP_HDR_S) + (map_count * map_def_size);
    hdr.map_count = map_count;
    hdr.sec_size = htonl(hdr.sec_size);

    
    ret |= _mybpf_simple_write(vbuf, &hdr, sizeof(hdr));

    
    ret |= _mybpf_simple_write_global_data(vbuf, data, map_def_size);

    
    map = map_sec->maps;
    for (i=0; i<map_sec->map_count; i++) {
        ret |= _mybpf_simple_write(vbuf, map, map_def_size);
        map = map + map_def_size;
    }

    return ret;
}

static int _mybpf_simple_write_map_data_sec_one(VBUF_S *vbuf, int map_index, void *data, int data_size)
{
    MYBPF_SIMPLE_MAP_DATA_S hdr = {0};
    int ret = 0;

    hdr.sec_type = MYBPF_SIMPLE_SEC_TYPE_GLOBAL_DATA;
    hdr.map_index = map_index;
    hdr.sec_size = sizeof(MYBPF_SIMPLE_MAP_DATA_S) + data_size;
    hdr.sec_size = htonl(hdr.sec_size);

    ret |= _mybpf_simple_write(vbuf, &hdr, sizeof(hdr));
    if (data) {
        ret |= _mybpf_simple_write(vbuf, data, data_size);
    }

    return ret;
}


static int _mybpf_simple_write_global_data_secs(VBUF_S *vbuf, ELF_S *elf, ELF_GLOBAL_DATA_S *data)
{
    int ret = 0;
    int map_index = 0; 

    if (data->have_bss) {
        ret |= _mybpf_simple_write_map_data_sec_one(vbuf, map_index, NULL, 0);
        map_index ++;
    }

    if (data->have_data) {
        ret |= _mybpf_simple_write_map_data_sec_one(vbuf, map_index,
                data->data_sec.data->d_buf, data->data_sec.data->d_size);
        map_index ++;
    }

    for (int i=0; i<data->rodata_count; i++) {
        ret |= _mybpf_simple_write_map_data_sec_one(vbuf, map_index,
                data->rodata_sec[i].data->d_buf, data->rodata_sec[i].data->d_size);
        map_index ++;
    }

    return ret;
}

static int _mybpf_simple_write_maps_name_sec(VBUF_S *vbuf, ELF_S *elf, ELF_GLOBAL_DATA_S *data, MYBPF_MAPS_SEC_S *map_sec)
{
    int i;
    int ret = 0;
    MYBPF_SIMPLE_MAP_NAME_HDR_S hdr = {0};
    char *maps_name[MYBPF_LOADER_MAX_MAPS];

    BS_DBGASSERT(map_sec->map_count <= MYBPF_LOADER_MAX_MAPS);

    hdr.sec_type = MYBPF_SIMPLE_SEC_TYPE_MAP_NAME;
    hdr.sec_size = sizeof(MYBPF_SIMPLE_MAP_NAME_HDR_S);
    hdr.map_count = map_sec->map_count + data->sec_count;

    if (data->have_bss) hdr.sec_size += sizeof(".bss");
    if (data->have_data) hdr.sec_size += sizeof(".data");
    for (i=0; i<data->rodata_count; i++) {
        hdr.sec_size += sizeof(".rodata");
    }

    
    for (i=0; i<map_sec->map_count; i++) {
        maps_name[i] = ELF_GetSecSymbolName(elf, map_sec->sec_id, 0, i);
        if (! maps_name[i]) {
            RETURNI(BS_ERR, "Can't get map name");
        }
        hdr.sec_size += (strlen(maps_name[i]) + 1);
    }

    hdr.sec_size = htonl(hdr.sec_size);

    
    ret |= _mybpf_simple_write(vbuf, &hdr, sizeof(hdr));

    
    if (data->have_bss) ret |= _mybpf_simple_write(vbuf, ".bss", sizeof(".bss"));
    if (data->have_data) ret |= _mybpf_simple_write(vbuf, ".data", sizeof(".data"));
    for (i=0; i<data->rodata_count; i++) {
        ret |= _mybpf_simple_write(vbuf, ".rodata", sizeof(".rodata"));
    }

    
    
    for (i=0; i<map_sec->map_count; i++) {
        int map_name_size = strlen(maps_name[i]) + 1;
        ret |= _mybpf_simple_write(vbuf, maps_name[i], map_name_size);
    }

    return ret;
}

static void _mybpf_simple_fill_relo_map(int sec_id, int type, int offset, int value, OUT MYBPF_RELO_MAP_S *relo_map)
{
    relo_map->sec_id = sec_id;
    relo_map->type = type;
    relo_map->offset = offset;
    relo_map->value = value;
}


static void _mybpf_simple_build_relo_maps(ELF_GLOBAL_DATA_S *data, MYBPF_MAPS_SEC_S *map_sec,
        OUT MYBPF_RELO_MAP_S *maps_relo)
{
    int index = 0;
    int i;

    
    

    
    if (data->have_bss) {
        _mybpf_simple_fill_relo_map(data->bss_sec.sec_id, MYBPF_RELO_MAP_BSS, 0, index, &maps_relo[index]);
        index ++;
    }

    if (data->have_data) {
        _mybpf_simple_fill_relo_map(data->data_sec.sec_id, MYBPF_RELO_MAP_DATA, 0, index, &maps_relo[index]);
        index ++;
    }

    for (i=0; i<data->rodata_count; i++) {
        _mybpf_simple_fill_relo_map(data->rodata_sec[i].sec_id, MYBPF_RELO_MAP_RODATA, 0, index, &maps_relo[index]);
        index ++;
    }

    for (i=0; i<map_sec->map_count; i++) {
        
        int offset = map_sec->map_def_size * i;
        _mybpf_simple_fill_relo_map(map_sec->sec_id, MYBPF_RELO_MAP_BPFMAP, offset, index, &maps_relo[index]);
        index ++;
    }
}

static int _mybpf_simple_write_maps(VBUF_S *vbuf, ELF_S *elf, OUT MYBPF_RELO_MAP_S *maps_relo, MYBPF_SIMPLE_CONVERT_PARAM_S *p)
{
    MYBPF_MAPS_SEC_S map_sec = {0};
    ELF_GLOBAL_DATA_S global_data = {0};
    int map_count;

    MYBPF_ELF_GetGlobalDataUsed(elf, &global_data);

    MYBPF_ELF_GetMapsSection(elf, &map_sec);

    map_count = global_data.sec_count + map_sec.map_count;

    
    if (map_count <= 0) {
        return 0;
    }

    if (map_count > MYBPF_LOADER_MAX_MAPS) {
        RETURNI(BS_ERR, "map count exceed");
    }

    if (_mybpf_simple_write_maps_sec(vbuf, elf, &global_data, &map_sec) < 0) {
        return -1;
    }

    if (_mybpf_simple_write_global_data_secs(vbuf, elf, &global_data) < 0) {
        return -1;
    }

    if (p && p->with_map_name) {
        if (_mybpf_simple_write_maps_name_sec(vbuf, elf, &global_data, &map_sec) < 0) {
            return -1;
        }
    }

    _mybpf_simple_build_relo_maps(&global_data, &map_sec, maps_relo);

    return map_count;
}

static int _mybpf_simple_write_prog_sec(VBUF_S *vbuf, void *mem, int mem_size, void *jitted, int jit_arch)
{
    MYBPF_SIMPLE_PROG_HDR_S hdr = {0};
    int ret = 0;

    hdr.sec_type = MYBPF_SIMPLE_SEC_TYPE_PROG;
    hdr.sec_size = sizeof(hdr) + mem_size;
    hdr.sec_size = htonl(hdr.sec_size);

    if (jitted) {
        hdr.jit_arch = jit_arch;
    }

    
    ret |= _mybpf_simple_write(vbuf, &hdr, sizeof(hdr));
    ret |= _mybpf_simple_write(vbuf, mem, mem_size);

    return ret;
}


static int _mybpf_simple_drop_sub_prog_info(INOUT ELF_PROG_INFO_S *progs, int count)
{
    int new_count = count;

    for (int i=0; i<new_count; i++) {
        if (strcmp(".text", progs[i].sec_name) != 0) 
            continue;

        int cp_count = (new_count - 1) - i;
        if (cp_count) {
            memcpy(&progs[i], &progs[i+1], sizeof(progs[0]) * cp_count);
        }
        i--;
        new_count --;
    }

    return new_count;
}


static void _mybpf_simple_drop_prog_name(INOUT ELF_PROG_INFO_S *progs, int count)
{
    for (int i=0; i<count; i++) {
        progs[i].func_name = "";
    }
}

static int _mybpf_simple_write_prog_info(VBUF_S *vbuf, ELF_PROG_INFO_S *progs, int count)
{
    MYBPF_SIMPLE_PROG_INFO_HDR_S hdr = {0};
    MYBPF_SIMPLE_PROG_OFF_S info;
    int i;
    int ret = 0;

    hdr.sec_type = MYBPF_SIMPLE_SEC_TYPE_PROG_INFO;
    hdr.sec_size = sizeof(hdr);
    hdr.func_count = htons(count);

    
    hdr.sec_size += sizeof(info) * count;
    for (i=0; i<count; i++) {
        hdr.sec_size += strlen(progs[i].sec_name) + 1;
        hdr.sec_size += strlen(progs[i].func_name) + 1;
    }

    hdr.sec_size = htonl(hdr.sec_size);

    
    ret |= _mybpf_simple_write(vbuf, &hdr, sizeof(hdr));

    
    for (i=0; i<count; i++) {
        info.offset = htonl(progs[i].prog_offset);
        info.len = htonl(progs[i].size);
        ret |= _mybpf_simple_write(vbuf, &info, sizeof(info));
    }

    
    for (i=0; i<count; i++) {
        char *str = progs[i].sec_name;
        int len = strlen(str) + 1;
        ret |= _mybpf_simple_write(vbuf, str, len);
        str = progs[i].func_name;
        len = strlen(str) + 1;
        ret |= _mybpf_simple_write(vbuf, str, len);
    }

    return ret;
}

static int _mybpf_simple_jit(void *insts, int insts_len, void *progs, int prog_count,
        void **jitted, MYBPF_SIMPLE_CONVERT_PARAM_S *p)
{
    MYBPF_JIT_INSN_S jit_insn = {0};
    MYBPF_JIT_CFG_S cfg = {0};
    int ret;

    cfg.translate_mode_aot = p->translate_mode_aot;
    cfg.aot_map_index_to_ptr = p->aot_map_index_to_ptr;
    cfg.helper_mode = p->helper_mode;
    cfg.jit_arch = p->jit_arch;

    jit_insn.insts = insts;
    jit_insn.insts_len = insts_len;
    jit_insn.progs = progs;
    jit_insn.progs_count = prog_count;

    if ((ret = MYBPF_Jit(&jit_insn, &cfg)) < 0) {
        return ret;
    }

    *jitted = jit_insn.insts;

    return jit_insn.insts_len;
}

static char * _mybpf_simple_get_sec_name(void *hdr)
{
    MYBPF_SIMPLE_COMMON_HDR_S *sec = hdr;

    if (sec->name_size == 0) {
        return NULL;
    }
    
    return (void*)(sec + 1);
}

static int _mybpf_simple_get_sec_data_size(void *hdr)
{
    MYBPF_SIMPLE_COMMON_HDR_S *sec = hdr;
    int size;

    size = ntohl(sec->sec_size);
    size -= sizeof(*sec);
    size -= sec->name_size;

    return size;
}

static void * _mybpf_simple_get_sec_data(void *hdr)
{
    MYBPF_SIMPLE_COMMON_HDR_S *sec = hdr;
    char *d;

    if (_mybpf_simple_get_sec_data_size(sec) == 0) {
        return NULL;
    }

    d = (void*)(sec + 1);
    d += sec->name_size;

    return d;
}

static int _mybpf_simple_get_helper_offset(int imm, void *ud)
{
    MYBPF_SIMPLE_CONVERT_CALL_MAP_S *map = ud;

    while(map->imm) {
        if (map->imm == imm) {
            return map->new_imm;
        }
        map ++;
    }

    return 0;
}

static int _mybpf_simple_add_helper_depends(void *insts, int insn_index, void *ud)
{
    MYBPF_HELPER_DEPENDS_S *d = ud;
    MYBPF_INSN_S *insn = insts;
    int imm = insn[insn_index].imm;
    int i;

    imm = htonl(imm);

    
    for (i=0; i<d->count; i++) {
        if (d->helpers[i] == imm) {
            return 0;
        }
    }

    if (d->count >= ARRAY_SIZE(d->helpers)) {
        PRINTLN_HRED("helper depends record error");
        return -1;
    }

    d->helpers[d->count] = imm;
    d->count ++;

    return 0;
}

static int _mybpf_simple_write_helper_depends_sec(VBUF_S *vbuf, MYBPF_HELPER_DEPENDS_S *d)
{
    MYBPF_SIMPLE_HELPER_DENENDS_HDR_S hdr = {0};
    int ret = 0;

    hdr.sec_type = MYBPF_SIMPLE_SEC_TYPE_HELPER_DEPENDS;
    hdr.sec_size = sizeof(hdr) + (d->count * sizeof(int));
    hdr.sec_size = htonl(hdr.sec_size);

    
    ret |= _mybpf_simple_write(vbuf, &hdr, sizeof(hdr));
    ret |= _mybpf_simple_write(vbuf, d->helpers, d->count * sizeof(int));

    return ret;
}

static int _mybpf_simple_write_helper_depends(VBUF_S *vbuf, void *progs_mem, int len)
{
    MYBPF_HELPER_DEPENDS_S d;
    int ret;

    memset(&d, 0, sizeof(d));

    ret = MYBPF_INSN_WalkExternCalls(progs_mem, len, _mybpf_simple_add_helper_depends, &d);
    if (ret < 0) {
        return ret;
    }

    if (d.count == 0) {
        return 0;
    }

    return _mybpf_simple_write_helper_depends_sec(vbuf, &d);
}

static inline int _mybpf_simple_write_prog_mem(VBUF_S *vbuf, MYBPF_SIMPLE_CONVERT_PARAM_S *p,
        void *mem, int mem_size,
        ELF_PROG_INFO_S *progs_info, int prog_count)
{
    int ret;
    void *jitted = NULL;
    int jitted_len;
    int jit_arch = p ? p->jit_arch : MYBPF_JIT_ARCH_NONE;

    if (p->helper_map) {
        if (MYBPF_INSN_FixupExtCalls(mem, mem_size, _mybpf_simple_get_helper_offset, p->helper_map) < 0) {
            ret = ERR_Set(BS_ERR, "Fixup ext calls error");
            goto Out;
        }
    }

    if (jit_arch) {
        jitted_len = ret = _mybpf_simple_jit(mem, mem_size, progs_info, prog_count, &jitted, p);
        if (jitted_len <= 0) {
            jit_arch = MYBPF_JIT_ARCH_NONE;
            ErrCode_Print();
            PRINTLN_HYELLOW("Jit failed");
            goto Out;
        } 

        ret = _mybpf_simple_write_helper_depends(vbuf, mem, mem_size);
        if (ret < 0) {
            goto Out;
        }

        mem = jitted;
        mem_size = jitted_len;
    }

    if ((ret = _mybpf_simple_write_prog_sec(vbuf, mem, mem_size, mem, jit_arch)) < 0) {
        goto Out;
    }

    if ((jit_arch) && (! p->with_func_name)) {
        prog_count = _mybpf_simple_drop_sub_prog_info(progs_info, prog_count);
    }

    if ((!p) || (p->with_func_name == 0)) 
        _mybpf_simple_drop_prog_name(progs_info, prog_count);

    if ((ret = _mybpf_simple_write_prog_info(vbuf, progs_info, prog_count)) < 0) {
        goto Out;
    }

Out:
    MEM_SafeFree(jitted);
    return ret;
}

static int _mybpf_simple_write_prog(VBUF_S *vbuf, ELF_S *elf, MYBPF_SIMPLE_CONVERT_PARAM_S *p,
        MYBPF_RELO_MAP_S *relo_maps, int maps_count)
{
    int ret;
    int mem_size = ELF_GetProgsSize(elf);
    int prog_count = ELF_GetProgsCount(elf);
    void *mem = NULL;
    ELF_PROG_INFO_S *progs_info = NULL;

    mem = ELF_DupProgs(elf);
    if (! mem) {
        return ERR_Set(BS_NO_MEMORY, "Can't alloc memory");
    }

    progs_info = MEM_ZMalloc(sizeof(ELF_PROG_INFO_S) * prog_count);
    if (! progs_info) {
        ret = ERR_Set(BS_ERR, "Can't alloc memory for progs_info");
        goto Out;
    }

    ELF_GetProgsInfo(elf, progs_info, prog_count);

    if ((ret = MYBPF_RELO_ProgRelo(elf, mem, relo_maps, maps_count, progs_info, prog_count)) < 0) {
        goto Out;
    }

    ret = _mybpf_simple_write_prog_mem(vbuf, p, mem, mem_size, progs_info, prog_count);

Out:
    MEM_SafeFree(mem);
    MEM_SafeFree(progs_info);

    return ret;
}

static int _mybpf_simple_write_private_secs(VBUF_S *vbuf, ELF_S *elf)
{
    ELF_SECTION_S sec;
    void *iter = NULL;
    MYBPF_SIMPLE_PRIVATE_HDR_S hdr = {0};
    int len;
    int size;
    int ret;

    hdr.sec_type = MYBPF_SIMPLE_SEC_TYPE_PRIVATE;

    while ((iter = ELF_GetNextSection(elf, iter, &sec))) {

        if (ELF_IsProgSection(&sec)) {
            
            continue;
        }

        len = strlen(sec.shname);
        if (len <= strlen("klc/")) {
            continue;
        }

        if (strncmp(sec.shname, "klc/", STR_LEN("klc/")) != 0) {
            continue;
        }

        if (len > MYBPF_SIMPLE_PRIVATE_NAME_MAX_LEN) {
            PRINTLN_HYELLOW("Can't support %s", sec.shname);
            continue;
        }

        size = len + 1;

        hdr.name_size = size;
        hdr.sec_size = sizeof(hdr) + size + sec.data->d_size;
        hdr.sec_size = htonl(hdr.sec_size);

        ret = _mybpf_simple_write(vbuf, &hdr, sizeof(hdr));
        ret |= _mybpf_simple_write(vbuf, sec.shname, size);
        ret |= _mybpf_simple_write(vbuf, sec.data->d_buf, sec.data->d_size);
        if (ret != 0) {
            return ret;
        }
    }

    return 0;
}

static int _mybpf_simple_write_end_sec(VBUF_S *vbuf)
{
    MYBPF_SIMPLE_END_S hdr = {0};
    hdr.sec_type = MYBPF_SIMPLE_SEC_TYPE_END;
    return _mybpf_simple_write(vbuf, &hdr, sizeof(hdr));
}

static int _mybpf_simple_write_vbuf_2_file(VBUF_S *vbuf, char *simple_file)
{
    FILE *fp = NULL;

    fp = FILE_Open(simple_file, TRUE, "wb+");
    if (! fp) {
        RETURNI(BS_ERR, "Can't open file %s", simple_file);
    }

    int ret = fwrite(VBUF_GetData(vbuf), 1, VBUF_GetDataLength(vbuf), fp);

    FILE_Close(fp);

    if (ret < 0) {
        RETURNI(BS_CAN_NOT_WRITE, "Can't wiret to file");
    }

    return ret;
}

static inline MYBPF_SIMPLE_COMMON_HDR_S * _mybpf_simple_get_next_sec(FILE_MEM_S *m, void *cur_hdr)
{
    MYBPF_SIMPLE_COMMON_HDR_S *common_hdr = cur_hdr;

    if (! cur_hdr) {
        
        MYBPF_SIMPLE_HDR_S *hdr = (void*)m->pucFileData;
        return (void*)(hdr + 1);
    }

    
    if (common_hdr->sec_type == MYBPF_SIMPLE_SEC_TYPE_END) {
        return NULL;
    }

    return (void*)((UCHAR*)common_hdr + ntohl(common_hdr->sec_size));
}


static inline void * _mybpf_simple_get_next_type_sec(FILE_MEM_S *m, int type, void *cur_hdr)
{
    MYBPF_SIMPLE_COMMON_HDR_S *common_hdr = cur_hdr;

    while ((common_hdr = _mybpf_simple_get_next_sec(m, common_hdr))) {
        if (common_hdr->sec_type == type) {
            return common_hdr;
        }
    }

    return NULL;
}


static inline int _mybpf_simple_get_type_sec_count(FILE_MEM_S *m, int type)
{
    MYBPF_SIMPLE_COMMON_HDR_S *common_hdr = NULL;
    int count = 0;

    while ((common_hdr = _mybpf_simple_get_next_type_sec(m, type, common_hdr))) {
        count ++;
    }

    return count;
}


static inline void * _mybpf_simple_get_type_sec(FILE_MEM_S *m, int type, int id)
{
    MYBPF_SIMPLE_COMMON_HDR_S *common_hdr = NULL;
    int this_id = 0;

    while ((common_hdr = _mybpf_simple_get_next_type_sec(m, type, common_hdr))) {
        if (this_id == id) {
            return common_hdr;
        }
        this_id ++;
    }

    return NULL;
}

static inline int _mybpf_simple_get_prog_names(FILE_MEM_S *m, int id, OUT char **sec_name, OUT char **func_name)
{
    int i;
    char *fname;
    char *sname;
    int func_count;
    MYBPF_SIMPLE_PROG_OFF_S *off;

    MYBPF_SIMPLE_PROG_INFO_HDR_S *hdr = _mybpf_simple_get_type_sec(m, MYBPF_SIMPLE_SEC_TYPE_PROG_INFO, 0);
    if (! hdr) {
        RETURN(BS_NOT_FOUND);
    }

    func_count = ntohs(hdr->func_count);
    if (id >= func_count) {
        RETURN(BS_OUT_OF_RANGE);
    }

    off = _mybpf_simple_get_sec_data(hdr);

    sname = (void*)(off + func_count);
    fname = sname + strlen(sname) + 1;

    for (i=0; i<id; i++) {
        sname = fname + strlen(fname) + 1;
        fname = sname + strlen(sname) + 1;
    }

    if (sec_name) {
        *sec_name = sname;
    }

    if (func_name) {
        *func_name = fname;
    }

    return 0;
}

static inline char * _mybpf_simple_get_prog_sec_name(FILE_MEM_S *m, int id)
{
    char *sec_name;

    if (_mybpf_simple_get_prog_names(m, id, &sec_name, NULL) < 0) {
        return NULL;
    }

    return sec_name;
}

static inline char * _mybpf_simple_get_prog_func_name(FILE_MEM_S *m, int id)
{
    char *func_name;

    if (_mybpf_simple_get_prog_names(m, id, NULL, &func_name) < 0) {
        return NULL;
    }

    return func_name;
}

static inline MYBPF_SIMPLE_PROG_OFF_S * _mybpf_simple_get_prog_offs(FILE_MEM_S *m)
{
    MYBPF_SIMPLE_PROG_INFO_HDR_S *hdr = _mybpf_simple_get_type_sec(m, MYBPF_SIMPLE_SEC_TYPE_PROG_INFO, 0);
    if (! hdr) {
        return NULL;
    }

    return _mybpf_simple_get_sec_data(hdr);
}

static inline char * _mybpf_simple_get_map_name(FILE_MEM_S *m, int id)
{
    int i;
    char *map_name;

    MYBPF_SIMPLE_MAP_NAME_HDR_S *name_hdr = _mybpf_simple_get_type_sec(m, MYBPF_SIMPLE_SEC_TYPE_MAP_NAME, 0);
    if (! name_hdr) {
        return NULL;
    }

    if (id >= name_hdr->map_count) {
        return NULL;
    }

    map_name = _mybpf_simple_get_sec_data(name_hdr);

    for (i=0; i<id; i++) {
        map_name = map_name + strlen(map_name) + 1;
    }

    return map_name;
}

static int _mybpf_simple_get_map_def_size(MYBPF_SIMPLE_MAP_HDR_S *hdr)
{
    int map_size;

    if (hdr->map_count == 0) {
        return 0;
    }

    map_size = _mybpf_simple_get_sec_data_size(hdr);

    return map_size / hdr->map_count;
}

static int _mybpf_simple_format_map_info(FILE_MEM_S *m, void *common_hdr, OUT char *buf, int buf_size)
{
    int i;
    MYBPF_SIMPLE_MAP_HDR_S *hdr = (void*)common_hdr;

    BS_DBGASSERT(hdr->sec_type == MYBPF_SIMPLE_SEC_TYPE_MAP);

    int ret = SNPRINTF(buf, buf_size, "map_count:%u, map_def_size:%u \r\n",
            hdr->map_count, _mybpf_simple_get_map_def_size(hdr));
    if (ret < 0) {
        RETURNI(BS_NOT_COMPLETE, "buf size too small");
    }

    UMAP_ELF_MAP_S * map = _mybpf_simple_get_sec_data(hdr);

    for (i=0; i<hdr->map_count; i++) {
        int len = strlen(buf);
        ret = SNPRINTF(buf + len, buf_size - len,
                    "map_id:%u, type:%u, key_size:%u, value_size:%u, max_elem:%u, name:%s \r\n",
                    i, map->type, map->size_key, map->size_value, map->max_elem, _mybpf_simple_get_map_name(m, i));
        if (ret < 0) {
            RETURNI(BS_NOT_COMPLETE, "buf size too small");
        }
    }

    return 0;
}

static int _mybpf_simple_format_prog_info(FILE_MEM_S *m, void *common_hdr, OUT char *buf, int buf_size)
{
    MYBPF_SIMPLE_PROG_HDR_S *hdr = common_hdr;
    SNPRINTF(buf, buf_size, "prog_size:%d, jit_arch:%s \r\n",
            MYBPF_SIMPLE_GetSecDataSize(hdr), MYBPF_JIT_GetArchName(hdr->jit_arch));
    return 0;
}

static int _mybpf_simple_format_proginfo_info(FILE_MEM_S *m, void *common_hdr, OUT char *buf, int buf_size)
{
    MYBPF_SIMPLE_PROG_INFO_HDR_S *hdr = common_hdr;
    MYBPF_SIMPLE_PROG_OFF_S *off = _mybpf_simple_get_sec_data(hdr);
    int i;
    int ret;
    int func_count = ntohs(hdr->func_count);

    for (i=0; i<func_count; i++) {
        int len = strlen(buf);
        char *func_name = _mybpf_simple_get_prog_func_name(m, i);
        char *sec_name = _mybpf_simple_get_prog_sec_name(m, i);

        if ((func_name) && (func_name[0])) {
            ret = SNPRINTF(buf + len, buf_size - len, "sec:%s, func:%s, offset:%u, size:%u \r\n", 
                    sec_name, func_name, ntohl(off[i].offset), ntohl(off[i].len));
        } else {
            ret = SNPRINTF(buf + len, buf_size - len, "sec:%s, offset:%u, size:%u \r\n", 
                    sec_name, ntohl(off[i].offset), ntohl(off[i].len));
        }

        if (ret < 0) {
            RETURNI(BS_NOT_COMPLETE, "buf size too small");
        }
    }

    return 0;
}

static int _mybpf_simple_format_private_info(FILE_MEM_S *m, void *common_hdr, OUT char *buf, int buf_size)
{
    MYBPF_SIMPLE_PRIVATE_HDR_S *hdr = common_hdr;
    char *name = _mybpf_simple_get_sec_name(hdr);
    int sec_size;

    int len = strlen(buf);
    sec_size = ntohl(hdr->sec_size) - sizeof(*hdr);

    if (SNPRINTF(buf + len, buf_size - len, "sec:%s, size:%d \r\n", name, sec_size) < 0) {
        RETURNI(BS_NOT_COMPLETE, "buf size too small");
    }

    return 0;
}

static int _mybpf_simple_format_helper_depends_info(FILE_MEM_S *m, void *common_hdr, OUT char *buf, int buf_size)
{
    MYBPF_SIMPLE_HELPER_DENENDS_HDR_S *hdr = common_hdr;
    int sec_size;
    int helper_count;
    int i;
    int len;

    sec_size = ntohl(hdr->sec_size) - sizeof(*hdr);

    helper_count = sec_size / sizeof(int);

    int *helpers = _mybpf_simple_get_sec_data(hdr);

    len = strlen(buf);
    if (SNPRINTF(buf + len, buf_size - len, "helpers:") < 0) {
        RETURNI(BS_NOT_COMPLETE, "buf size too small");
    }

    for (i=0; i<helper_count; i++) {
        int len = strlen(buf);
        if (SNPRINTF(buf + len, buf_size - len, " %d", ntohl(helpers[i])) < 0) {
            RETURNI(BS_NOT_COMPLETE, "buf size too small");
        }
    }

    len = strlen(buf);
    if (SNPRINTF(buf + len, buf_size - len, "\r\n") < 0) {
        RETURNI(BS_NOT_COMPLETE, "buf size too small");
    }

    return 0;
}

typedef int (*PF_MYBPF_SIMPLE_FORMAT_SEC_FUNC)(FILE_MEM_S *m, void *common_hdr, OUT char *buf, int buf_size);

static PF_MYBPF_SIMPLE_FORMAT_SEC_FUNC g_mybpf_simple_format_sec_funcs[] = {
    [MYBPF_SIMPLE_SEC_TYPE_END] = NULL,
    [MYBPF_SIMPLE_SEC_TYPE_MAP] = _mybpf_simple_format_map_info,
    [MYBPF_SIMPLE_SEC_TYPE_PROG] = _mybpf_simple_format_prog_info,
    [MYBPF_SIMPLE_SEC_TYPE_PROG_INFO] = _mybpf_simple_format_proginfo_info,
    [MYBPF_SIMPLE_SEC_TYPE_PRIVATE] = _mybpf_simple_format_private_info,
    [MYBPF_SIMPLE_SEC_TYPE_HELPER_DEPENDS] = _mybpf_simple_format_helper_depends_info,
};

static int _mybpf_simple_format_file_info(FILE_MEM_S *m, OUT char *buf, int buf_size)
{
    int ret;
    PF_MYBPF_SIMPLE_FORMAT_SEC_FUNC func;
    MYBPF_SIMPLE_HDR_S *hdr = (void*)m->pucFileData;

    ret = SNPRINTF(buf, buf_size, "version:%u, total_size:%u \r\n", hdr->ver, ntohl(hdr->totle_size));
    if (ret < 0) {
        RETURNI(BS_NOT_COMPLETE, "buf size too small");
    }

    MYBPF_SIMPLE_COMMON_HDR_S *common_hdr = NULL;

    while ((common_hdr = _mybpf_simple_get_next_sec(m, common_hdr))) {
        if (common_hdr->sec_type > ARRAY_SIZE(g_mybpf_simple_format_sec_funcs)) {
            continue;
        }

        func = g_mybpf_simple_format_sec_funcs[common_hdr->sec_type];
        if (! func) {
            continue;
        }

        int len = strlen(buf);
        ret = func(m, common_hdr, buf + len, buf_size - len);
        if (ret < 0) {
            return ret;
        }
    }

    return 0;
}


int MYBPF_SIMPLE_Elf2SpfBuf(void *elf, MYBPF_SIMPLE_CONVERT_PARAM_S *p, OUT VBUF_S *vbuf)
{
    int maps_count;
    MYBPF_RELO_MAP_S maps_relo[MYBPF_LOADER_MAX_MAPS];
    ELF_S *pelf = elf;

    if (_mybpf_simple_write_header(vbuf) < 0) {
        return -1;
    }

    if ((maps_count = _mybpf_simple_write_maps(vbuf, pelf, maps_relo, p)) < 0) {
        return -1;
    }

    if (_mybpf_simple_write_prog(vbuf, pelf, p, maps_relo, maps_count) < 0) {
        return -1;
    }

    if (_mybpf_simple_write_private_secs(vbuf, pelf) < 0) {
        return -1;
    }

    if (_mybpf_simple_write_end_sec(vbuf) < 0) {
        return -1;
    }

    UINT size = VBUF_GetDataLength(vbuf);
    if (_mybpf_simple_write_totle_size(vbuf, size) < 0) {
        return -1;
    }

    return 0;
}


int MYBPF_SIMPLE_Bpf2SpfBuf(char *bpf_file, MYBPF_SIMPLE_CONVERT_PARAM_S *p, OUT VBUF_S *vbuf)
{
    ELF_S elf = {0};

    if (ELF_Open(bpf_file, &elf) < 0) {
        RETURNI(BS_ERR, "Can't open file %s", bpf_file);
    }

    int ret = MYBPF_SIMPLE_Elf2SpfBuf(&elf, p, vbuf);

    ELF_Close(&elf);

    return ret;
}


int MYBPF_SIMPLE_Bpf2SpfFile(char *bpf_file, char *simple_file, MYBPF_SIMPLE_CONVERT_PARAM_S *p)
{
    VBUF_S vbuf;

    VBUF_Init(&vbuf);

    int ret = MYBPF_SIMPLE_Bpf2SpfBuf(bpf_file, p, &vbuf);

    if (ret == 0) {
        ret = _mybpf_simple_write_vbuf_2_file(&vbuf, simple_file);
    }

    VBUF_Finit(&vbuf);

    return ret;
}

int MYBPF_SIMPLE_BuildInfo(FILE_MEM_S *m, OUT char *buf, int buf_size)
{
    return _mybpf_simple_format_file_info(m, buf, buf_size);
}

int MYBPF_SIMPLE_BuildFileInfo(char *simple_file, OUT char *buf, int buf_size)
{
    FILE_MEM_S *m = MYBPF_SIMPLE_OpenFile(simple_file);
    if (! m) {
        RETURNI(BS_ERR, "Can't open %s", simple_file);
    }

    int ret = _mybpf_simple_format_file_info(m, buf, buf_size);

    MYBPF_SIMPLE_Close(m);

    return ret;
}

BOOL_T MYBPF_SIMPLE_IsSimpleFormatFile(char *simple_file)
{
    MYBPF_SIMPLE_HDR_S hdr = {0};

    int len = FILE_MemTo(simple_file, &hdr, sizeof(hdr));
    if (len != sizeof(hdr)) {
        return FALSE;
    }

    if ((hdr.ver != MYBPF_SIMPLE_VER) || (hdr.magic != MYBPF_SIMPLE_MAGIC)) {
        return FALSE;
    }

    return TRUE;
}

FILE_MEM_S * MYBPF_SIMPLE_OpenFile(char *simple_file)
{
    FILE_MEM_S *m = FILE_Mem(simple_file);
    if (! m) {
        return NULL;
    }

    if (m->uiFileLen < sizeof(MYBPF_SIMPLE_HDR_S)) {
        FILE_MemFree(m);
        return NULL;
    }

    MYBPF_SIMPLE_HDR_S *hdr = (void*)m->pucFileData;

    if ((hdr->ver != MYBPF_SIMPLE_VER) || (hdr->magic != MYBPF_SIMPLE_MAGIC)) {
        FILE_MemFree(m);
        return NULL;
    }

    return m;
}

void MYBPF_SIMPLE_Close(FILE_MEM_S *f)
{
    FILE_MemFree(f);
}

int MYBPF_SIMPLE_GetMapsSection(FILE_MEM_S *m, OUT MYBPF_MAPS_SEC_S *map_sec)
{
    MYBPF_SIMPLE_MAP_HDR_S *hdr;

    map_sec->map_count = 0;
    map_sec->map_def_size = 0;
    map_sec->maps = NULL;

    hdr = _mybpf_simple_get_type_sec(m, MYBPF_SIMPLE_SEC_TYPE_MAP, 0);
    if (! hdr) {
        return 0;
    }

    map_sec->map_count = hdr->map_count;
    map_sec->map_def_size = _mybpf_simple_get_map_def_size(hdr);
    map_sec->maps = _mybpf_simple_get_sec_data(hdr);

    return 0;
}


char * MYBPF_SIMPLE_GetMapName(FILE_MEM_S *m, int id)
{
    return _mybpf_simple_get_map_name(m, id);
}

int MYBPF_SIMPLE_GetJitArch(FILE_MEM_S *m)
{
    MYBPF_SIMPLE_PROG_HDR_S *hdr;

    hdr = _mybpf_simple_get_type_sec(m, MYBPF_SIMPLE_SEC_TYPE_PROG, 0);
    if (! hdr) {
        return MYBPF_JIT_ARCH_NONE;
    }

    return hdr->jit_arch;
}

int MYBPF_SIMPLE_GetTypeSecCount(FILE_MEM_S *m, int type)
{
    return _mybpf_simple_get_type_sec_count(m, type);
}


void * MYBPF_SIMPLE_GetSec(FILE_MEM_S *m, int type, int id)
{
    return _mybpf_simple_get_type_sec(m, type, id);
}


void * MYBPF_SIMPLE_GetSecByName(FILE_MEM_S *m, int type, char *name)
{
    MYBPF_SIMPLE_PRIVATE_HDR_S *hdr = NULL;

    while ((hdr = _mybpf_simple_get_next_type_sec(m, type, hdr))) {
        char *sec_name;

        if (hdr->sec_size == 0) { 
            continue;
        }

        sec_name = _mybpf_simple_get_sec_name(hdr);

        if (strcmp(name, sec_name) == 0) {
            return hdr;
        }
    }

    return NULL;
}

char * MYBPF_SIMPLE_GetSecName(void *sec)
{
    if (! sec) {
        return NULL;
    }

    return _mybpf_simple_get_sec_name(sec);
}

int MYBPF_SIMPLE_GetSecDataSize(void *sec)
{
    if (! sec) {
        return 0;
    }

    return _mybpf_simple_get_sec_data_size(sec);
}

void * MYBPF_SIMPLE_GetSecData(void *sec)
{
    if (! sec) {
        return NULL;
    }

    return _mybpf_simple_get_sec_data(sec);
}

int MYBPF_SIMPLE_CopySecData(void *sec, OUT void *mem, int mem_size)
{
    if (! sec) {
        RETURN(BS_NO_SUCH);
    }

    int size = MYBPF_SIMPLE_GetSecDataSize(sec);
    size = MIN(size, mem_size);
    if (size) {
        memcpy(mem, MYBPF_SIMPLE_GetSecData(sec), size);
    }

    return size;
}

void * MYBPF_SIMPLE_DupSecData(void *sec)
{
    if (! sec) {
        return NULL;
    }

    int size = MYBPF_SIMPLE_GetSecDataSize(sec);
    if (size <= 0) {
        return NULL;
    }

    void *mem = MEM_Malloc(size);
    if (! mem) {
        return NULL;
    }

    MYBPF_SIMPLE_CopySecData(sec, mem, size);

    return mem;
}


int MYBPF_SIMPLE_GetProgsCount(FILE_MEM_S *m)
{
    MYBPF_SIMPLE_PROG_INFO_HDR_S *hdr;

    hdr = _mybpf_simple_get_type_sec(m, MYBPF_SIMPLE_SEC_TYPE_PROG_INFO, 0);
    if (! hdr) {
        return 0;
    }

    return ntohs(hdr->func_count);
}


int MYBPF_SIMPLE_GetProgsSize(FILE_MEM_S *m)
{
    void *sec = MYBPF_SIMPLE_GetSec(m, MYBPF_SIMPLE_SEC_TYPE_PROG, 0);
    return MYBPF_SIMPLE_GetSecDataSize(sec);
}

void * MYBPF_SIMPLE_GetProgs(FILE_MEM_S *m)
{
    void *sec = MYBPF_SIMPLE_GetSec(m, MYBPF_SIMPLE_SEC_TYPE_PROG, 0);
    return MYBPF_SIMPLE_GetSecData(sec);
}

int MYBPF_SIMPLE_CopyPorgs(FILE_MEM_S *m, OUT void *mem, int mem_size)
{
    void *sec = MYBPF_SIMPLE_GetSec(m, MYBPF_SIMPLE_SEC_TYPE_PROG, 0);
    return MYBPF_SIMPLE_CopySecData(sec, mem, mem_size);
}

void * MYBPF_SIMPLE_DupPorgs(FILE_MEM_S *m)
{
    void *sec = MYBPF_SIMPLE_GetSec(m, MYBPF_SIMPLE_SEC_TYPE_PROG, 0);
    return MYBPF_SIMPLE_DupSecData(sec);
}

int MYBPF_SIMPLE_GetProgInfo(FILE_MEM_S *m, int id, OUT ELF_PROG_INFO_S *progs)
{
    int count = MYBPF_SIMPLE_GetProgsCount(m);
    MYBPF_SIMPLE_PROG_OFF_S *offs = _mybpf_simple_get_prog_offs(m);

    if (id >= count) {
        return BS_NO_SUCH;
    }

    progs->prog_offset = ntohl(offs[id].offset);
    progs->size = ntohl(offs[id].len);
    progs->sec_name = _mybpf_simple_get_prog_sec_name(m, id);
    progs->func_name = _mybpf_simple_get_prog_func_name(m, id);

    return 0;
}

int MYBPF_SIMPLE_GetProgsInfo(FILE_MEM_S *m, OUT ELF_PROG_INFO_S *progs, int max_prog_count)
{
    int i;
    char *sec_name, *func_name;

    int count = MYBPF_SIMPLE_GetProgsCount(m);
    MYBPF_SIMPLE_PROG_OFF_S *offs = _mybpf_simple_get_prog_offs(m);

    count = MIN(count, max_prog_count);

    for (i=0; i<count; i++) {
        progs[i].prog_offset = ntohl(offs[i].offset);
        progs[i].size = ntohl(offs[i].len);
        sec_name = _mybpf_simple_get_prog_sec_name(m, i);
        func_name = _mybpf_simple_get_prog_func_name(m, i);
        progs[i].sec_name = TXT_Strdup(sec_name);
        progs[i].func_name = TXT_Strdup(func_name);
    }

    return count;
}


int MYBPF_SIMPLE_GetMainProgsCount(FILE_MEM_S *m)
{
    int i;
    int index = 0;

    int count = MYBPF_SIMPLE_GetProgsCount(m);

    for (i=0; i<count; i++) {
        char *sec_name;

        sec_name = _mybpf_simple_get_prog_sec_name(m, i);
        if (strcmp(".text", sec_name) == 0) {
            continue;
        }

        index ++;
    }

    return index;
}


int MYBPF_SIMPLE_GetMainProgInfo(FILE_MEM_S *m, int id, OUT ELF_PROG_INFO_S *info)
{
    int i;
    int index = 0;

    int count = MYBPF_SIMPLE_GetProgsCount(m);
    MYBPF_SIMPLE_PROG_OFF_S *offs = _mybpf_simple_get_prog_offs(m);

    for (i=0; i<count; i++) {
        char *sec_name;

        sec_name = _mybpf_simple_get_prog_sec_name(m, i);
        if (strcmp(".text", sec_name) == 0) {
            continue;
        }

        if (id == index) {
            info->prog_offset = ntohl(offs[i].offset);
            info->size = ntohl(offs[i].len);
            info->sec_name = sec_name;
            info->func_name = _mybpf_simple_get_prog_func_name(m, i);
            return 0;
        }

        index ++;
    }

    return -1;
}


int MYBPF_SIMPLE_GetMainProgsInfo(FILE_MEM_S *m, OUT ELF_PROG_INFO_S *progs, int max_prog_count)
{
    int i;
    int index = 0;

    int count = MYBPF_SIMPLE_GetProgsCount(m);
    MYBPF_SIMPLE_PROG_OFF_S *offs = _mybpf_simple_get_prog_offs(m);

    for (i=0; i<count; i++) {
        char *sec_name;

        if (index >= max_prog_count) {
            break;
        }

        sec_name = _mybpf_simple_get_prog_sec_name(m, i);
        if (strcmp(".text", sec_name) == 0) {
            continue;
        }

        progs[index].prog_offset = ntohl(offs[i].offset);
        progs[index].size = ntohl(offs[i].len);
        progs[index].sec_name = sec_name;
        progs[index].func_name = _mybpf_simple_get_prog_func_name(m, i);
        index ++;
    }

    return index;
}

int MYBPF_SIMPLE_WalkProg(FILE_MEM_S *m, PF_ELF_WALK_PROG walk_func, void *ud)
{
    ELF_PROG_INFO_S info = {0};
    char *progs;

    int count = MYBPF_SIMPLE_GetProgsCount(m);
    progs = MYBPF_SIMPLE_GetProgs(m);

    for (int i=0; i<count; i++) {
        MYBPF_SIMPLE_GetProgInfo(m, i, &info);
        walk_func(progs + info.prog_offset, info.prog_offset, info.size, info.sec_name, info.func_name, ud);
    }

    return 0;
}


UINT MYBPF_SIMPLE_GetSimpleSizeByHdr(void *hdr)
{
    MYBPF_SIMPLE_HDR_S *h = hdr;

    if ((h->magic != MYBPF_SIMPLE_MAGIC) || (h->ver != 0)) {
        return 0;
    }

    return ntohl(h->totle_size);
}

