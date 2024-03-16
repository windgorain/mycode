/*********************************************************
*   Copyright (C), Xingang.Li
*   Author:      LiXingang  Version: 1.0  Date: 2017-10-1
*   Description: simple bpf file
*   在某些场景,如openwrt/iot/sdwan等资源有限场景,可以使用spf代替elf
*
********************************************************/
#include <sys/mman.h>
#include "bs.h"
#include "types.h"
#include "utl/file_utl.h"
#include "utl/elf_utl.h"
#include "utl/time_utl.h"
#include "utl/mem_utl.h"
#include "utl/mybpf_utl.h"
#include "utl/umap_def.h"
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
#include "mybpf_def_inner.h"
#include "mybpf_simple_func.h"

#define MYBPF_SIMPLE_PRIVATE_NAME_MAX_LEN 255


typedef struct {
    UINT sec_size; 
    UCHAR sec_type;
}MYBPF_SIMPLE_END_S;


static int _mybpf_simple_write_zero(VBUF_S *vbuf, int len)
{
    UCHAR zero[2] = {0};
    int i;

    for (i=0; i<len; i++) {
        if (VBUF_CatFromBuf(vbuf, zero, 1) < 0) {
            RETURNI(BS_CAN_NOT_WRITE, "Can't write buf");
        }
    }

    return 0;
}

static int _mybpf_simple_write(VBUF_S *vbuf, void *mem, int size)
{
    if (VBUF_CatFromBuf(vbuf, mem, size) < 0) {
        RETURNI(BS_CAN_NOT_WRITE, "Can't write buf");
    }
    return 0;
}

static int _mybpf_simple_write_header(U8 aot_mode, U16 app_ver, VBUF_S *vbuf)
{
    MYBPF_SIMPLE_HDR_S hdr = {0};

    hdr.magic = htonl(MYBPF_SIMPLE_MAGIC);
    hdr.ver = MYBPF_SIMPLE_VER;
    hdr.app_ver = htons(app_ver);
    hdr.utc_sec = TM_SecondsFromUTC();
    hdr.utc_sec = htonl(hdr.utc_sec);
    hdr.aot_mode = aot_mode;

    
    return _mybpf_simple_write(vbuf, &hdr, sizeof(hdr));
}

static int _mybpf_simple_write_totle_size(VBUF_S *vbuf)
{
    MYBPF_SIMPLE_HDR_S *hdr;
    UINT size = VBUF_GetDataLength(vbuf);

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

static int _mybpf_simpe_add_map(OUT VBUF_S *vbuf, MYBPF_MAPS_SEC_S *s, int padding_len)
{
    int i;
    int ret = 0;
    char *map;

    map = s->maps;

    for (i=0; i<s->map_count; i++) {
        ret |= _mybpf_simple_write(vbuf, map, s->map_def_size);
        ret |= _mybpf_simple_write_zero(vbuf, padding_len);
        map = map + s->map_def_size;
    }

    return ret;
}


static int _mybpf_simple_add_map_sec_hdr(VBUF_S *vbuf, int map_count, int map_def_size, U8 flags)
{
    MYBPF_SIMPLE_MAP_HDR_S hdr = {0};

    hdr.sec_type = MYBPF_SIMPLE_SEC_TYPE_MAP;
    hdr.sec_size = sizeof(MYBPF_SIMPLE_MAP_HDR_S) + (map_count * map_def_size);
    hdr.map_count = map_count;
    hdr.flags = flags;
    hdr.sec_size = htonl(hdr.sec_size);

    
    return _mybpf_simple_write(vbuf, &hdr, sizeof(hdr));
}

static int _mybpf_simple_write_maps_sec(VBUF_S *vbuf, ELF_S *elf,
        ELF_GLOBAL_DATA_S *global_data, MYBPF_MAPS_SEC_S *map_sec)
{
    int ret = 0;
    int map_count;
    int padding = 0;
    int map_def_size = map_sec->map_def_size;
    U8 flags = 0;

    if (map_def_size < UMAP_ELF_MAP_MIN_SIZE) {
        map_def_size = UMAP_ELF_MAP_MIN_SIZE;
        padding = map_def_size - map_sec->map_def_size;
    }

    map_count = global_data->sec_count + map_sec->map_count;

    if (global_data->have_bss) {
        flags |= MYBPF_SIMPLE_MAP_FLAG_BSS;
    }
    if (global_data->have_data) {
        flags |= MYBPF_SIMPLE_MAP_FLAG_DATA;
    }
    if (global_data->rodata_count) {
        flags |= MYBPF_SIMPLE_MAP_FLAG_RODATA;
    }

    
    ret |= _mybpf_simple_add_map_sec_hdr(vbuf, map_count, map_def_size, flags);

    
    ret |= _mybpf_simple_write_global_data(vbuf, global_data, map_def_size);

    
    ret |= _mybpf_simpe_add_map(vbuf, map_sec, padding);

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

static int _mybpf_simple_write_maps_name_sec_hdr(VBUF_S *vbuf, int map_count, int data_size)
{
    MYBPF_SIMPLE_MAP_NAME_HDR_S hdr = {0};

    hdr.sec_type = MYBPF_SIMPLE_SEC_TYPE_MAP_NAME;
    hdr.map_count = map_count;
    hdr.sec_size = sizeof(hdr) + data_size;
    hdr.sec_size = htonl(hdr.sec_size);

    
    return _mybpf_simple_write(vbuf, &hdr, sizeof(hdr));
}

static int _mybpf_simple_write_maps_name_sec(VBUF_S *vbuf, ELF_S *elf, ELF_GLOBAL_DATA_S *data, MYBPF_MAPS_SEC_S *map_sec)
{
    int i;
    int ret = 0;
    int data_size = 0;
    int map_count;
    char *maps_name[MYBPF_LOADER_MAX_MAPS];

    BS_DBGASSERT(map_sec->map_count <= MYBPF_LOADER_MAX_MAPS);

    if (data->have_bss) data_size += sizeof(".bss");
    if (data->have_data) data_size += sizeof(".data");
    for (i=0; i<data->rodata_count; i++) {
        data_size += sizeof(".rodata");
    }

    
    for (i=0; i<map_sec->map_count; i++) {
        maps_name[i] = ELF_GetSecSymbolName(elf, map_sec->sec_id, 0, i);
        if (! maps_name[i]) {
            RETURNI(BS_ERR, "Can't get map name");
        }
        data_size += (strlen(maps_name[i]) + 1);
    }

    map_count = map_sec->map_count + data->sec_count;

    
    ret |= _mybpf_simple_write_maps_name_sec_hdr(vbuf, map_count, data_size);

    
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

static int _mybpf_simple_write_maps(VBUF_S *vbuf, ELF_S *elf,
        OUT MYBPF_RELO_MAP_S *maps_relo, MYBPF_SIMPLE_CONVERT_PARAM_S *p)
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

    _MYBPF_SIMPLE_BuildReloMaps(&global_data, &map_sec, maps_relo);

    return map_count;
}


static int _mybpf_simple_write_prog_hdr(VBUF_S *vbuf, int prog_size, int jit_arch)
{
    MYBPF_SIMPLE_PROG_HDR_S hdr = {0};

    hdr.sec_type = MYBPF_SIMPLE_SEC_TYPE_PROG;
    hdr.sec_size = sizeof(hdr) + prog_size;
    hdr.sec_size = htonl(hdr.sec_size);
    hdr.jit_arch = jit_arch;

    
    return _mybpf_simple_write(vbuf, &hdr, sizeof(hdr));
}

static int _mybpf_simple_write_prog_sec(VBUF_S *vbuf, void *mem, int mem_size, int jit_arch)
{
    int ret = 0;

    ret |= _mybpf_simple_write_prog_hdr(vbuf, mem_size, jit_arch);
    ret |= _mybpf_simple_write(vbuf, mem, mem_size);

    return ret;
}

static int _mybpf_simple_write_prog_info_hdr(VBUF_S *vbuf, int info_size, int func_count)
{
    MYBPF_SIMPLE_PROG_INFO_HDR_S hdr = {0};

    hdr.sec_type = MYBPF_SIMPLE_SEC_TYPE_PROG_INFO;
    hdr.func_count = htons(func_count);
    hdr.sec_size = sizeof(hdr) + info_size;
    hdr.sec_size = htonl(hdr.sec_size);

    return _mybpf_simple_write(vbuf, &hdr, sizeof(hdr));
}


static int _mybpf_simple_write_func_info(VBUF_S *vbuf, ELF_PROG_INFO_S *progs_info, int prog_count)
{
    int ret = 0;
    int i;
    MYBPF_SIMPLE_PROG_OFF_S info;

    for (i=0; i<prog_count; i++) {
        info.offset = htonl(progs_info[i].prog_offset);
        info.len = htonl(progs_info[i].size);
        ret |= _mybpf_simple_write(vbuf, &info, sizeof(info));
    }

    return ret;
}


static int _mybpf_simple_write_progs_name(VBUF_S *vbuf, ELF_PROG_INFO_S *progs, int prog_count)
{
    char *str;
    int len;
    int i;
    int ret = 0;

    
    for (i=0; i<prog_count; i++) {
        str = progs[i].sec_name;
        len = strlen(str) + 1;
        ret |= _mybpf_simple_write(vbuf, str, len);
        str = progs[i].func_name;
        len = strlen(str) + 1;
        ret |= _mybpf_simple_write(vbuf, str, len);
    }

    return ret;
}

static int _mybpf_simple_write_prog_info(VBUF_S *vbuf, ELF_PROG_INFO_S *progs_info, int prog_count)
{
    MYBPF_SIMPLE_PROG_OFF_S info;
    int i;
    int ret = 0;
    int info_size;

    
    info_size = sizeof(info) * prog_count;
    for (i=0; i<prog_count; i++) {
        info_size += strlen(progs_info[i].sec_name) + 1;
        info_size += strlen(progs_info[i].func_name) + 1;
    }

    
    ret |= _mybpf_simple_write_prog_info_hdr(vbuf, info_size, prog_count);

    
    ret |= _mybpf_simple_write_func_info(vbuf, progs_info, prog_count);

    
    ret |= _mybpf_simple_write_progs_name(vbuf, progs_info, prog_count);

    return ret;
}

static int _mybpf_simple_jit_progs(void *insts, int insts_len, void *progs, int prog_count,
        void **jitted, MYBPF_SIMPLE_CONVERT_PARAM_S *p)
{
    MYBPF_JIT_INSN_S jit_insn = {0};
    MYBPF_JIT_CFG_S cfg = {0};
    int ret;

    cfg.translate_mode_aot = p->translate_mode_aot;
    cfg.aot_map_index_to_ptr = p->aot_map_index_to_ptr;
    cfg.param_6th = p->param_6th;
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

static int _mybpf_simple_get_sec_size(void *hdr)
{
    MYBPF_SIMPLE_COMMON_HDR_S *sec = hdr;
    return ntohl(sec->sec_size);
}

typedef struct {
    void *progs;
    void *progs_info;
    int progs_size;
    int progs_count;
}_MYBPF_SIMPLE_PROG_INFO_S;

static int _mybpf_simple_get_progs_info(FILE_MEM_S *m, OUT _MYBPF_SIMPLE_PROG_INFO_S *info)
{
    info->progs_size = MYBPF_SIMPLE_GetProgsSize(m);
    info->progs_count = MYBPF_SIMPLE_GetProgsCount(m);

    info->progs = MYBPF_SIMPLE_GetProgs(m);
    info->progs_info = MEM_ZMalloc(sizeof(ELF_PROG_INFO_S) * info->progs_count);

    if ((! info->progs) ||  (! info->progs_info)) {
        MEM_SafeFree(info->progs);
        MEM_SafeFree(info->progs_info);
        info->progs = info->progs_info = NULL;
        RETURN(BS_NO_MEMORY);
    }

    MYBPF_SIMPLE_GetProgsInfo(m, info->progs_info, info->progs_count);

    return 0;
}

static void _mybpf_simple_clear_progs_info(_MYBPF_SIMPLE_PROG_INFO_S *info)
{
    MEM_SafeFree(info->progs_info);
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

    ret = MYBPF_INSN_WalkExternCalls(progs_mem, len, _MYBPF_SIMPLE_AddHelperDepends, &d);
    if (ret < 0) {
        return ret;
    }

    if (d.count == 0) {
        return 0;
    }

    return _mybpf_simple_write_helper_depends_sec(vbuf, &d);
}


static void _mybpf_simple_drop_prog_name(INOUT ELF_PROG_INFO_S *progs, int count)
{
    for (int i=0; i<count; i++) {
        progs[i].func_name = "";
    }
}

static int _mybpf_simple_write_progs(VBUF_S *vbuf, MYBPF_SIMPLE_CONVERT_PARAM_S *p,
        void *mem, int mem_size,
        ELF_PROG_INFO_S *progs_info, int prog_count)
{
    int ret;

    if ((p) && (p->jit_arch) && (! p->with_func_name)) {
        prog_count = _MYBPF_SIMPLE_DropSubProgInfo(progs_info, prog_count);
    }

    if ((p) && (p->with_func_name == 0)) {
        _mybpf_simple_drop_prog_name(progs_info, prog_count);
    }

    if ((ret = _mybpf_simple_write_prog_sec(vbuf, mem, mem_size, p->jit_arch)) < 0) {
        return ret;
    }

    if ((ret = _mybpf_simple_write_prog_info(vbuf, progs_info, prog_count)) < 0) {
        return ret;
    }

    return 0;
}

static int _mybpf_simple_jit_and_write(VBUF_S *vbuf, MYBPF_SIMPLE_CONVERT_PARAM_S *p,
        void *mem, int mem_size,
        ELF_PROG_INFO_S *progs_info, int prog_count)
{
    int ret;
    void *jitted = NULL;
    int jitted_len;

    jitted_len = _mybpf_simple_jit_progs(mem, mem_size, progs_info, prog_count, &jitted, p);
    if (jitted_len < 0) {
        ErrCode_Print();
        PRINTLN_HYELLOW("Jit failed");
        return jitted_len;
    } 

    ret = _mybpf_simple_write_helper_depends(vbuf, mem, mem_size);
    ret |= _mybpf_simple_write_progs(vbuf, p, jitted, jitted_len, progs_info, prog_count);

    MEM_Free(jitted);

    return ret;
}

static int _mybpf_simple_write_prog_mem(VBUF_S *vbuf, MYBPF_SIMPLE_CONVERT_PARAM_S *p,
        void *mem, int mem_size,
        ELF_PROG_INFO_S *progs_info, int prog_count)
{
    int ret;

    if (p->jit_arch) {
        ret = _mybpf_simple_jit_and_write(vbuf, p, mem, mem_size, progs_info, prog_count);
    }else {
        ret =_mybpf_simple_write_progs(vbuf, p, mem, mem_size, progs_info, prog_count);
    }

    return ret;
}


static int _mybpf_simple_move_text_sec(ELF_S *elf, void *mem, int mem_size, ELF_PROG_INFO_S *progs_info, int prog_count)
{
    ELF_SECTION_S text_sec = {0};
    int text_progs_count;
    int text_size;
    int ret;
    int i;

    ELF_GetSecByName(elf, ".text", &text_sec);

    
    if (text_sec.data) {
        text_size = text_sec.data->d_size;
        ret = MEM_SwapByOff(mem, mem_size, text_size);
        if (ret < 0) {
            return ret;
        }

        MYBPF_INSN_ModifyTextOff(mem, mem_size, mem_size - text_size);

        text_progs_count = ELF_GetSecProgsInfoCount(progs_info, prog_count, ".text");
        if (text_progs_count) {
            ret = MEM_SwapByOff(progs_info, prog_count * sizeof(ELF_PROG_INFO_S), text_progs_count * sizeof(ELF_PROG_INFO_S));
            if (ret < 0) {
                return ret;
            }
            for (i=0; i<(prog_count - text_progs_count); i++) {
                progs_info[i].sec_offset -= text_size;
                progs_info[i].prog_offset -= text_size;
            }
            for (; i<prog_count; i++) {
                progs_info[i].sec_offset += (mem_size - text_size);
                progs_info[i].prog_offset += (mem_size - text_size);
            }
        }

    }

    return 0;
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

    if (! p->keep_text_pos) {
        if ((ret = _mybpf_simple_move_text_sec(elf, mem, mem_size, progs_info, prog_count)) < 0) {
            goto Out;
        }
    }

    if ((p) && (p->helper_map)) {
        if (MYBPF_INSN_FixupExtCalls(mem, mem_size, _MYBPF_SIMPLE_GetHelperOffset, p->helper_map) < 0) {
            ret = ERR_Set(BS_ERR, "Fixup ext calls error");
            goto Out;
        }
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

static int _mybpf_simple_del_sec(VBUF_S *vbuf, int type, int id)
{
    FILE_MEM_S m;
    void *sec;
    int sec_size;

    m.data = VBUF_GetData(vbuf);
    m.len = VBUF_GetDataLength(vbuf);

    sec = mybpf_simple_get_type_sec(&m, type, id);
    if (! sec) {
        return 0;
    }

    sec_size = _mybpf_simple_get_sec_size(sec);

    VBUF_Cut(vbuf, VBUF_Ptr2Offset(vbuf, sec), sec_size);

    return 0;
}

static int _mybpf_simple_elf2spfbuf(void *elf, MYBPF_SIMPLE_CONVERT_PARAM_S *p, OUT VBUF_S *vbuf)
{
    int maps_count;
    MYBPF_RELO_MAP_S maps_relo[MYBPF_LOADER_MAX_MAPS];
    ELF_S *pelf = elf;

    if (_mybpf_simple_write_header(p->aot_mode, p->app_ver, vbuf) < 0) {
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

    if (_mybpf_simple_write_totle_size(vbuf) < 0) {
        return -1;
    }

    return 0;
}


static int _mybpf_simple_merge_global_data(FILE_MEM_S *m, OUT VBUF_S *vbuf, int map_index_off)
{
    int i;
    int ret = 0;
    int global_map_count;
    MYBPF_SIMPLE_MAP_DATA_S *sec;

    global_map_count = MYBPF_SIMPLE_GetTypeSecCount(m, MYBPF_SIMPLE_SEC_TYPE_GLOBAL_DATA);

    for (i=0; i<global_map_count; i++) {
        sec = MYBPF_SIMPLE_GetSec(m, MYBPF_SIMPLE_SEC_TYPE_GLOBAL_DATA, i);
        if (sec) {
            int sec_size = _mybpf_simple_get_sec_size(sec);
            sec->map_index += map_index_off;
            ret |= _mybpf_simple_write(vbuf, sec, sec_size);
        }
    }

    return ret;
}


static int _mybpf_simple_merge_map_name(FILE_MEM_S *m1, FILE_MEM_S *m2, OUT VBUF_S *vbuf)
{
    MYBPF_SIMPLE_MAP_NAME_HDR_S *name_sec1, *name_sec2 = NULL;
    int ret;
    int map_count;
    int data_size1, data_size2 = 0;
    void *data1, *data2 = NULL;

    name_sec1 = mybpf_simple_get_type_sec(m1, MYBPF_SIMPLE_SEC_TYPE_MAP_NAME, 0);
    if (! name_sec1) {
        return 0;
    }

    if (m2) {
        name_sec2 = mybpf_simple_get_type_sec(m2, MYBPF_SIMPLE_SEC_TYPE_MAP_NAME, 0);
        if (! name_sec2) {
            return 0;
        }
    }

    data1 = mybpf_simple_get_sec_data(name_sec1);
    data_size1 = mybpf_simple_get_sec_data_size(name_sec1);
    map_count = name_sec1->map_count;

    if (m2) {
       map_count += name_sec2->map_count;
       data2 = mybpf_simple_get_sec_data(name_sec2);
       data_size2 = mybpf_simple_get_sec_data_size(name_sec2);
    }

    ret = _mybpf_simple_write_maps_name_sec_hdr(vbuf, map_count, data_size1 + data_size2);
    ret |= _mybpf_simple_write(vbuf, data1, data_size1);
    if (m2) {
        ret |= _mybpf_simple_write(vbuf, data2, data_size2);
    }

    return ret;
}

static int _mybpf_simple_merge_maps(FILE_MEM_S *m1, FILE_MEM_S *m2, OUT VBUF_S *vbuf, int with_map_name)
{
    MYBPF_MAPS_SEC_S map1 = {0}, map2 = {0};
    MYBPF_SIMPLE_MAP_HDR_S *map_hdr;
    int ret = 0;
    int map_count;
    int map_size_def;
    U8 flags = 0;

    MYBPF_SIMPLE_GetMapsSection(m1, &map1);

    map_hdr = mybpf_simple_get_type_sec(m1, MYBPF_SIMPLE_SEC_TYPE_MAP, 0);
    if (map_hdr) {
        flags = map_hdr->flags;
    }

    if (m2) {
        MYBPF_SIMPLE_GetMapsSection(m2, &map2);

        map_hdr = mybpf_simple_get_type_sec(m2, MYBPF_SIMPLE_SEC_TYPE_MAP, 0);
        if (map_hdr) {
            flags |= map_hdr->flags;
        }
    }

    map_count = map1.map_count + map2.map_count;
    if (map_count == 0) {
        return 0;
    }
    
    map_size_def = MAX(map1.map_def_size, map2.map_def_size);

    
    ret |= _mybpf_simple_add_map_sec_hdr(vbuf, map_count, map_size_def, flags);

    
    ret |= _mybpf_simpe_add_map(vbuf, &map1, map_size_def - map1.map_def_size);
    ret |= _mybpf_simpe_add_map(vbuf, &map2, map_size_def - map2.map_def_size);

    
    ret |= _mybpf_simple_merge_global_data(m1, vbuf, 0);
    if (m2) {
        ret |= _mybpf_simple_merge_global_data(m2, vbuf, map1.map_count);
    }

    
    if (with_map_name) {
        ret |= _mybpf_simple_merge_map_name(m1, m2, vbuf);
    }

    return ret;
}

static int _mybpf_simple_merge_modify_prog(void *insts, int insn_index, void *ud)
{
    MYBPF_MAPS_SEC_S *map1 = ud;
    MYBPF_INSN_S *insn = insts;
    int src_reg;

    src_reg = insn[insn_index].src_reg;

    if ((src_reg == BPF_PSEUDO_MAP_FD) || (src_reg == BPF_PSEUDO_MAP_VALUE)) {
        insn[insn_index].imm += map1->map_count;
        return 0;
    }

    return 0;
}

static int _mybpf_simple_merge_progs(FILE_MEM_S *m1, FILE_MEM_S *m2, OUT VBUF_S *vbuf)
{
    int ret;
    int i;
    ELF_PROG_INFO_S *info;
    MYBPF_MAPS_SEC_S map1, map2;
    _MYBPF_SIMPLE_PROG_INFO_S info1 = {0};
    _MYBPF_SIMPLE_PROG_INFO_S info2 = {0};

    MYBPF_SIMPLE_GetMapsSection(m1, &map1);
    MYBPF_SIMPLE_GetMapsSection(m2, &map2);

    if ((0 != MYBPF_SIMPLE_GetJitArch(m1)) || (0 != MYBPF_SIMPLE_GetJitArch(m2))) {
        
        RETURN(BS_NOT_SUPPORT);
    }

    ret = _mybpf_simple_get_progs_info(m1, &info1);
    ret |= _mybpf_simple_get_progs_info(m2, &info2);
    if (ret < 0) {
        goto _OUT;
    }

    
    MYBPF_INSN_WalkLddw(info2.progs, info2.progs_size, _mybpf_simple_merge_modify_prog, &map1);
    
    
    info = info2.progs_info;
    for (i=0; i<info2.progs_count; i++) {
        info[i].prog_offset += info1.progs_size;
    }

    ret = _mybpf_simple_write_prog_hdr(vbuf, info1.progs_size + info2.progs_size, 0);
    ret |= _mybpf_simple_write(vbuf, info1.progs, info1.progs_size);
    ret |= _mybpf_simple_write(vbuf, info2.progs, info2.progs_size);

    ret |= _mybpf_simple_write_prog_info_hdr(vbuf,
            MYBPF_SIMPLE_GetProgsInfoSize(m1) + MYBPF_SIMPLE_GetProgsInfoSize(m2),
            info1.progs_count + info2.progs_count);
    ret |= _mybpf_simple_write_func_info(vbuf, info1.progs_info, info1.progs_count);
    ret |= _mybpf_simple_write_func_info(vbuf, info2.progs_info, info2.progs_count);

    ret |= _mybpf_simple_write_progs_name(vbuf, info1.progs_info, info1.progs_count);
    ret |= _mybpf_simple_write_progs_name(vbuf, info2.progs_info, info2.progs_count);

_OUT:
    _mybpf_simple_clear_progs_info(&info1);
    _mybpf_simple_clear_progs_info(&info2);

    return ret;
}

static int _mybpf_simple_find_depend(MYBPF_HELPER_DEPENDS_S *d, int helper_id)
{
    int i;

    for (i = 0; i<d->count; i++) {
        if (d->helpers[i] == helper_id) {
            return i;
        }
    }

    return -1;
}

static int _mybpf_simple_merge_depends(FILE_MEM_S *m1, FILE_MEM_S *m2, OUT VBUF_S *vbuf)
{
    int ret = 0;
    void *sec1, *sec2 = NULL;
    int count1 = 0, count2 = 0, i;
    int *helpers1, *helpers2;
    MYBPF_HELPER_DEPENDS_S d = {0};

    sec1 = mybpf_simple_get_type_sec(m1, MYBPF_SIMPLE_SEC_TYPE_HELPER_DEPENDS, 0);
    if (m2) {
        sec2 = mybpf_simple_get_type_sec(m2, MYBPF_SIMPLE_SEC_TYPE_HELPER_DEPENDS, 0);
    }

    if (sec1) {
        count1 = mybpf_simple_get_sec_data_size(sec1) / sizeof(int);
        helpers1 = mybpf_simple_get_sec_data(sec1);
    }

    if (sec2) {
        count2 = mybpf_simple_get_sec_data_size(sec2) / sizeof(int);
        helpers2 = mybpf_simple_get_sec_data(sec2);
    }

    if (count1) {
        memcpy(d.helpers, helpers1, count1 * sizeof(int));
        d.count = count1;
    }
    
    for (i=0; i<count2; i++) {
        if (_mybpf_simple_find_depend(&d, helpers2[i]) < 0) {
            if (d.count >= ARRAY_SIZE(d.helpers)) {
                RETURN(BS_OUT_OF_RANGE);
            }
            d.helpers[d.count] = helpers2[i];
            d.count ++;
        }
    }

    if (d.count) {
        ret = _mybpf_simple_write_helper_depends_sec(vbuf, &d);
    }

    return ret;
}

static int _mybpf_simple_merge_private(FILE_MEM_S *m, OUT VBUF_S *vbuf)
{
    void *sec = NULL;
    int ret = 0;

    while ((sec = mybpf_simple_get_next_type_sec(m, MYBPF_SIMPLE_SEC_TYPE_PRIVATE, sec))) {
        ret |= _mybpf_simple_write(vbuf, sec, _mybpf_simple_get_sec_size(sec));
    }

    return ret;
}

static int _mybpf_simple_convert_prog(FILE_MEM_S *m, MYBPF_SIMPLE_CONVERT_PARAM_S *p, OUT VBUF_S *dst)
{
    int ret = 0;
    int old_jit_arch;
    _MYBPF_SIMPLE_PROG_INFO_S info = {0};

    if ((ret = _mybpf_simple_get_progs_info(m, &info)) < 0) {
        return ret;
    }

    old_jit_arch = MYBPF_SIMPLE_GetJitArch(m);

    if ((! old_jit_arch) && (p->jit_arch)) {
        ret = _mybpf_simple_jit_and_write(dst, p, info.progs, info.progs_size, info.progs_info, info.progs_count);
    }else {
        ret |= _mybpf_simple_write_progs(dst, p, info.progs, info.progs_size, info.progs_info, info.progs_count);
    }

    _mybpf_simple_clear_progs_info(&info);

    return ret;
}


static int _mybpf_simple_open_elf_file(char *elf_file, MYBPF_SIMPLE_CONVERT_PARAM_S *p, OUT FILE_MEM_S *m)
{
    int ret = 0;
    VBUF_S vbuf;

    VBUF_Init(&vbuf);

    if (MYBPF_SIMPLE_Bpf2SpfBuf(elf_file, p, &vbuf) >= 0) {
        ret = FILE_MemByData(VBUF_GetData(&vbuf), VBUF_GetDataLength(&vbuf), m);
    }

    VBUF_Finit(&vbuf);

    return ret;
}


static int _mybpf_simple_convert_file_2_spf_buf(char *src_filename, MYBPF_SIMPLE_CONVERT_PARAM_S *p, OUT VBUF_S *vbuf)
{
    FILE_MEM_S m = {0};
    int ret;

    ret = MYBPF_SIMPLE_OpenFile(src_filename, &m);
    if (ret < 0) {
        RETURNI(BS_ERR, "Can't open %s", src_filename);
    }

    ret = MYBPF_SIMPLE_Convert(&m, vbuf, p);

    MYBPF_SIMPLE_Close(&m);

    return ret;
}


int MYBPF_SIMPLE_Elf2SpfBuf(void *elf, MYBPF_SIMPLE_CONVERT_PARAM_S *p, OUT VBUF_S *vbuf)
{
    return _mybpf_simple_elf2spfbuf(elf, p, vbuf);
}


int MYBPF_SIMPLE_Bpf2SpfBuf(char *bpf_file, MYBPF_SIMPLE_CONVERT_PARAM_S *p, OUT VBUF_S *vbuf)
{
    ELF_S elf = {0};

    if (ELF_Open(bpf_file, &elf) < 0) {
        RETURNI(BS_ERR, "Can't open file %s", bpf_file);
    }

    int ret = _mybpf_simple_elf2spfbuf(&elf, p, vbuf);

    ELF_Close(&elf);

    return ret;
}

int MYBPF_SIMPLE_DelSec(VBUF_S *vbuf, int type, int id)
{
    return _mybpf_simple_del_sec(vbuf, type, id);
}


int MYBPF_SIMPLE_Merge(VBUF_S *src1, VBUF_S *src2, OUT VBUF_S *dst)
{
    int ret = 0;
    FILE_MEM_S m1, m2;
    MYBPF_SIMPLE_HDR_S *h1, *h2;
    U16 app_ver1, app_ver2;

    m1.data= VBUF_GetData(src1);
    m1.len = VBUF_GetDataLength(src1);
    m2.data= VBUF_GetData(src2);
    m2.len = VBUF_GetDataLength(src2);

    h1 = (void*)m1.data;
    h2 = (void*)m2.data;

    if (h1->aot_mode != h2->aot_mode) {
        RETURNI(BS_NOT_MATCH, "aot mode not matched");
    }

    app_ver1 = ntohs(h1->app_ver);
    app_ver2 = ntohs(h2->app_ver);

    ret |= _mybpf_simple_write_header(h1->aot_mode, MAX(app_ver1, app_ver2), dst);
    ret |= _mybpf_simple_merge_maps(&m1, &m2, dst, 1);
    ret |= _mybpf_simple_merge_progs(&m1, &m2, dst);
    ret |= _mybpf_simple_merge_depends(&m1, &m2, dst);
    ret |= _mybpf_simple_merge_private(&m1, dst);
    ret |= _mybpf_simple_merge_private(&m2, dst);
    ret |= _mybpf_simple_write_totle_size(dst);

    return ret;
}

int MYBPF_SIMPLE_Convert(FILE_MEM_S *spf, OUT VBUF_S *dst, MYBPF_SIMPLE_CONVERT_PARAM_S *p)
{
    int ret = 0;

    ret |= _mybpf_simple_write_header(p->aot_mode, p->app_ver, dst);
    ret |= _mybpf_simple_merge_maps(spf, NULL, dst, p->with_map_name);
    ret |= _mybpf_simple_convert_prog(spf, p, dst);
    ret |= _mybpf_simple_merge_depends(spf, NULL, dst);
    ret |= _mybpf_simple_merge_private(spf, dst);
    ret |= _mybpf_simple_write_totle_size(dst);

    return ret;
}


int MYBPF_SIMPLE_Convert2Buf(char *src_filename, MYBPF_SIMPLE_CONVERT_PARAM_S *p, OUT VBUF_S *vbuf)
{
    return _mybpf_simple_convert_file_2_spf_buf(src_filename, p, vbuf);
}


int MYBPF_SIMPLE_Convert2File(char *src_filename, char *dst_filename, MYBPF_SIMPLE_CONVERT_PARAM_S *p)
{
    VBUF_S vbuf;
    int ret = 0;

    VBUF_Init(&vbuf);

    ret = _mybpf_simple_convert_file_2_spf_buf(src_filename, p, &vbuf);
    if (ret == 0) {
        ret = VBUF_WriteFile(dst_filename, &vbuf);
    }

    VBUF_Finit(&vbuf);

    return ret;
}

int MYBPF_SIMPLE_BuildFileInfo(char *file, OUT char *buf, int buf_size)
{
    FILE_MEM_S m;
    int ret;

    ret = MYBPF_SIMPLE_OpenFile(file, &m);
    if (ret < 0) {
        RETURNI(BS_ERR, "Can't open %s", file);
    }

    ret = MYBPF_SIMPLE_BuildInfo(&m, buf, buf_size);

    MYBPF_SIMPLE_Close(&m);

    return ret;
}

BOOL_T MYBPF_SIMPLE_IsSpfFile(char *simple_file)
{
    MYBPF_SIMPLE_HDR_S hdr = {0};

    int len = FILE_MemTo(simple_file, &hdr, sizeof(hdr));
    if (len != sizeof(hdr)) {
        return FALSE;
    }

    if ((hdr.ver != MYBPF_SIMPLE_VER) || (hdr.magic != htonl(MYBPF_SIMPLE_MAGIC))) {
        return FALSE;
    }

    return TRUE;
}

int MYBPF_SIMPLE_OpenFile(char *file, OUT FILE_MEM_S *m)
{
    if (MYBPF_SIMPLE_IsSpfFile(file)) {
        return MYBPF_SIMPLE_OpenSpf(file, m);
    } else {
        MYBPF_SIMPLE_CONVERT_PARAM_S p = {0};

        p.with_func_name = 1;
        p.with_map_name = 1;

        return _mybpf_simple_open_elf_file(file, &p, m);
    }
}


int MYBPF_SIMPLE_OpenFileRaw(char *file, OUT FILE_MEM_S *m)
{
    if (MYBPF_SIMPLE_IsSpfFile(file)) {
        return MYBPF_SIMPLE_OpenSpf(file, m);
    } else {
        MYBPF_SIMPLE_CONVERT_PARAM_S p = {0};

        p.with_func_name = 1;
        p.with_map_name = 1;
        p.keep_text_pos = 1;

        return _mybpf_simple_open_elf_file(file, &p, m);
    }
}

int MYBPF_SIMPLE_WriteFile(FILE_MEM_S *m, char *filename)
{
    return FILE_WriteFile(filename, m->data, m->len);
}
