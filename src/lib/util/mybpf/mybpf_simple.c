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

#define MYBPF_SIMPLE_VER 0 
#define MYBPF_SIMPLE_MAGIC 0xae 

typedef struct {
    UCHAR magic1;
    UCHAR magic2;
    UCHAR ver;       /* 版本号 */
    UCHAR reserved;
    UINT totle_size;
}MYBPF_SIMPLE_HDR_S;

typedef struct {
    UINT sec_size; /* net order, 包含hdr */
    UCHAR sec_type;
    UCHAR resereved[3];
}MYBPF_SIMPLE_COMMON_HDR_S;

typedef struct {
    UINT sec_size; /* net order, 包含hdr */
    UCHAR sec_type;
    UCHAR map_count; /* map个数 */
    UCHAR reserved1;
    UCHAR reserved2;
}MYBPF_SIMPLE_MAP_HDR_S;

typedef struct {
    UINT sec_size; /* net order, 包含hdr */
    UCHAR sec_type;
    UCHAR map_count; /* map个数 */
    UCHAR reserved1;
    UCHAR reserved2;
}MYBPF_SIMPLE_MAP_NAME_HDR_S;

typedef struct {
    UINT sec_size; /* net order, 包含hdr */
    UCHAR sec_type;
    UCHAR jit_arch;
    UCHAR reserved1;
    UCHAR reserved2;
}MYBPF_SIMPLE_PROG_HDR_S;

typedef struct {
    UINT sec_size; /* net order, 包含hdr */
    UCHAR sec_type;
    UCHAR reserved;
    USHORT func_count;
}MYBPF_SIMPLE_PROG_INFO_HDR_S;

typedef struct {
    UINT sec_size; /* net order, 包含hdr */
    UCHAR sec_type;
    UCHAR reserved0;
    UCHAR reserved1;
    UCHAR reserved2;
}MYBPF_SIMPLE_BOOTSTRAP_HDR_S;

/* END Section HDR */
typedef struct {
    UINT sec_size; /* net order, 包含hdr */
    UCHAR sec_type;
}MYBPF_SIMPLE_END_S;

static int _mybpf_simple_write(FILE *fp, void *mem, int size)
{
    if (fwrite(mem, 1, size, fp) != size) {
        RETURNI(BS_CAN_NOT_WRITE, "Can't wiret to file");
    }
    return 0;
}

static int _mybpf_simple_write_header(FILE *fp)
{
    MYBPF_SIMPLE_HDR_S hdr = {0};

    hdr.ver = MYBPF_SIMPLE_VER;
    hdr.magic1 = MYBPF_SIMPLE_MAGIC;
    hdr.magic2 = MYBPF_SIMPLE_MAGIC;

    /* 写入header */
    return _mybpf_simple_write(fp, &hdr, sizeof(hdr));
}

static int _mybpf_simple_write_totle_size(FILE *fp, UINT size)
{
    int ret;

    fseek(fp, offsetof(MYBPF_SIMPLE_HDR_S, totle_size), SEEK_SET);

    size = htonl(size);
    ret = _mybpf_simple_write(fp, &size, sizeof(int));
    fseek(fp, 0, SEEK_END);

    return ret;
}

static int _mybpf_simple_write_global_data(FILE *fp, ELF_GLOBAL_DATA_S *data, int map_def_size)
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
        ret |= _mybpf_simple_write(fp, map, map_def_size);
    }

    if (data->have_data) {
        map->size_value = data->data_sec.data->d_size;
        ret |= _mybpf_simple_write(fp, map, map_def_size);
    }

    if (data->have_rodata) {
        map->size_value = data->rodata_sec.data->d_size;
        ret |= _mybpf_simple_write(fp, map, map_def_size);
    }

    MEM_Free(map);

    return ret;
}

static int _mybpf_simple_write_maps_sec(FILE *fp, ELF_S *elf, ELF_GLOBAL_DATA_S *data, MYBPF_MAPS_SEC_S *map_sec)
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

    /* 写入header */
    ret |= _mybpf_simple_write(fp, &hdr, sizeof(hdr));
    ret |= _mybpf_simple_write_global_data(fp, data, map_def_size);

    /* 写入maps */
    map = map_sec->maps;
    for (i=0; i<map_sec->map_count; i++) {
        ret |= _mybpf_simple_write(fp, map, map_def_size);
        map = map + map_def_size;
    }

    return ret;
}

static int _mybpf_simple_write_maps_name_sec(FILE *fp, ELF_S *elf, ELF_GLOBAL_DATA_S *data, MYBPF_MAPS_SEC_S *map_sec)
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
    if (data->have_rodata) hdr.sec_size += sizeof(".rodata");

    /* 获取maps name */
    for (i=0; i<map_sec->map_count; i++) {
        maps_name[i] = ELF_GetSecSymbolName(elf, map_sec->sec_id, 0, i);
        if (! maps_name[i]) {
            RETURNI(BS_ERR, "Can't get map name");
        }
        hdr.sec_size += (strlen(maps_name[i]) + 1);
    }

    hdr.sec_size = htonl(hdr.sec_size);

    /* 写入header */
    ret |= _mybpf_simple_write(fp, &hdr, sizeof(hdr));

    /* 写入global data name */
    if (data->have_bss) ret |= _mybpf_simple_write(fp, ".bss", sizeof(".bss"));
    if (data->have_data) ret |= _mybpf_simple_write(fp, ".bss", sizeof(".data"));
    if (data->have_rodata) ret |= _mybpf_simple_write(fp, ".robss", sizeof(".rodata"));

    /* 写入maps-name */
    /* 格式为: name1\0name2\0...namex\0 */
    for (i=0; i<map_sec->map_count; i++) {
        int map_name_size = strlen(maps_name[i]) + 1;
        ret |= _mybpf_simple_write(fp, maps_name[i], map_name_size);
    }

    return ret;
}

static int _mybpf_simple_write_maps(FILE *fp, ELF_S *elf, OUT MYBPF_RELO_MAP_S *maps, MYBPF_SIMPLE_CONVERT_PARAM_S *p)
{
    MYBPF_MAPS_SEC_S map_sec = {0};
    ELF_GLOBAL_DATA_S global_data = {0};
    int i;
    int map_count;
    int index;

    ELF_GetGlobalData(elf, &global_data);

    MYBPF_ELF_GetMapsSection(elf, &map_sec);

    map_count = global_data.sec_count + map_sec.map_count;

    /* 不存在map, 不用写入map sec */
    if (map_count <= 0) {
        return 0;
    }

    if (map_count > MYBPF_LOADER_MAX_MAPS) {
        RETURNI(BS_ERR, "map count exceed");
    }

    if (_mybpf_simple_write_maps_sec(fp, elf, &global_data, &map_sec) < 0) {
        return -1;
    }

    if (p && p->with_map_name) {
        if (_mybpf_simple_write_maps_name_sec(fp, elf, &global_data, &map_sec) < 0) {
            return -1;
        }
    }

    index = 0;

    if (global_data.have_bss) {
        maps[index].sec_id = global_data.bss_sec.sec_id;
        maps[index].type = MYBPF_RELO_MAP_BSS;
        maps[index].offset = 0;
        maps[index].value = index;
        index ++;
    }

    if (global_data.have_data) {
        maps[index].sec_id = global_data.data_sec.sec_id;
        maps[index].type = MYBPF_RELO_MAP_DATA;
        maps[index].offset = 0;
        maps[index].value = index;
        index ++;
    }

    if (global_data.have_rodata) {
        maps[index].sec_id = global_data.rodata_sec.sec_id;
        maps[index].type = MYBPF_RELO_MAP_RODATA;
        maps[index].offset = 0;
        maps[index].value = index;
        index ++;
    }

    for (i=0; i<map_sec.map_count; i++) {
        maps[index].sec_id = map_sec.sec_id;
        maps[index].type = MYBPF_RELO_MAP_BPFMAP;
        maps[index].offset = (map_sec.map_def_size * i);
        maps[index].value = index;
        index ++;
    }

    return map_count;
}

static int _mybpf_simple_write_prog_sec(FILE *fp, void *mem, int mem_size, void *jitted, int jit_arch)
{
    MYBPF_SIMPLE_PROG_HDR_S hdr = {0};
    int ret = 0;

    hdr.sec_type = MYBPF_SIMPLE_SEC_TYPE_PROG;
    hdr.sec_size = sizeof(hdr) + mem_size;
    hdr.sec_size = htonl(hdr.sec_size);

    if (jitted) {
        hdr.jit_arch = jit_arch;
    }

    /* 写入header */
    ret |= _mybpf_simple_write(fp, &hdr, sizeof(hdr));
    ret |= _mybpf_simple_write(fp, mem, mem_size);

    return ret;
}

/* 对于jit后,可以不需要sub prog info了 */
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

/* 对于不携带name的情况,精简掉sec_name的后缀 */
static void _mybpf_simple_drop_prog_name(INOUT ELF_PROG_INFO_S *progs, int count)
{
    for (int i=0; i<count; i++) {
        progs[i].func_name = "";

        char *split = strchr(progs[i].sec_name, '/');
        if (split) {
            split[1] = '\0';
        }
    }
}

static int _mybpf_simple_write_prog_info(FILE *fp, ELF_PROG_INFO_S *progs, int count)
{
    MYBPF_SIMPLE_PROG_INFO_HDR_S hdr = {0};
    MYBPF_SIMPLE_PROG_OFF_S info;
    int i;
    int ret = 0;

    hdr.sec_type = MYBPF_SIMPLE_SEC_TYPE_PROG_INFO;
    hdr.sec_size = sizeof(hdr);
    hdr.func_count = htons(count);

    /* 计算sec size */
    hdr.sec_size += sizeof(info) * count;
    for (i=0; i<count; i++) {
        hdr.sec_size += strlen(progs[i].sec_name) + 1;
        hdr.sec_size += strlen(progs[i].func_name) + 1;
    }

    hdr.sec_size = htonl(hdr.sec_size);

    /* 写入header */
    ret |= _mybpf_simple_write(fp, &hdr, sizeof(hdr));

    /* 写入func info */
    for (i=0; i<count; i++) {
        info.offset = htonl(progs[i].offset);
        info.len = htonl(progs[i].size);
        ret |= _mybpf_simple_write(fp, &info, sizeof(info));
    }

    /* 写入sec_name和func_name:
       格式: sec_name1\0func_name1\0...sec_namex\0\func_namex\0
     */
    for (i=0; i<count; i++) {
        char *str = progs[i].sec_name;
        int len = strlen(str) + 1;
        ret |= _mybpf_simple_write(fp, str, len);
        str = progs[i].func_name;
        len = strlen(str) + 1;
        ret |= _mybpf_simple_write(fp, str, len);
    }

    return ret;
}

static int _mybpf_simple_jit(void *insts, int insts_len, void *progs, int prog_count,
        void **jitted, MYBPF_SIMPLE_CONVERT_PARAM_S *p)
{
    MYBPF_JIT_INSN_S jit_insn = {0};
    int ret;
    UINT flag = 0;

    if (p->ext_call_agent) {
        flag |= MYBPF_JIT_FLAG_USE_AGENT;
    }

    jit_insn.insts = insts;
    jit_insn.insts_len = insts_len;
    jit_insn.progs = progs;
    jit_insn.progs_count = prog_count;

    if ((ret = MYBPF_JitArch(p->jit_arch, &jit_insn, flag)) < 0) {
        return ret;
    }

    *jitted = jit_insn.insts;

    return jit_insn.insts_len;
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

static int _mybpf_simple_write_prog(FILE *fp, ELF_S *elf, MYBPF_SIMPLE_CONVERT_PARAM_S *p)
{
    int ret;
    int mem_size = ELF_GetProgsSize(elf);
    int prog_count = ELF_GetProgsCount(elf);
    void *jitted = NULL;
    int jitted_len;
    void *mem = NULL;
    ELF_PROG_INFO_S *progs = NULL;
    int jit_arch = p ? p->jit_arch : MYBPF_JIT_ARCH_NONE;

    mem = ELF_DupProgs(elf);
    if (! mem) {
        ret = ERR_Set(BS_NO_MEMORY, "Can't alloc memory");
        goto Out;
    }

    progs = MEM_Malloc(sizeof(ELF_PROG_INFO_S) * prog_count);
    if (! progs) {
        ret = ERR_Set(BS_ERR, "Can't alloc memory for progs");
        goto Out;
    }

    ELF_GetProgsInfo(elf, progs, prog_count);

    if (p->convert_calls_map) {
        if (MYBPF_INSN_FixupExtCalls(mem, mem_size, _mybpf_simple_get_helper_offset, p->convert_calls_map) < 0) {
            ret = ERR_Set(BS_ERR, "Fixup ext calls error");
            goto Out;
        }
    }

    MYBPF_INSN_RelativeBpfCalls(mem, mem_size);

    if (jit_arch) {
        jitted_len = _mybpf_simple_jit(mem, mem_size, progs, prog_count, &jitted, p);
        if (jitted_len <= 0) {
            jit_arch = MYBPF_JIT_ARCH_NONE;
        } else {
            MEM_Free(mem);
            mem = jitted;
            mem_size = jitted_len;
        }
    }

    if ((ret = _mybpf_simple_write_prog_sec(fp, mem, mem_size, mem, jit_arch)) < 0) {
        goto Out;
    }

    if (jit_arch)
        prog_count = _mybpf_simple_drop_sub_prog_info(progs, prog_count);

    if ((!p) || (p->with_func_name == 0)) 
        _mybpf_simple_drop_prog_name(progs, prog_count);

    if ((ret = _mybpf_simple_write_prog_info(fp, progs, prog_count)) < 0) {
        goto Out;
    }

Out:
    if (jitted) {
        munmap(mem, mem_size);
    } else {
        MEM_FREE_NULL(mem);
    }
    MEM_FREE_NULL(progs);

    return ret;
}

static int _mybpf_simple_write_end_sec(FILE *fp)
{
    MYBPF_SIMPLE_END_S hdr = {0};
    hdr.sec_type = MYBPF_SIMPLE_SEC_TYPE_END;
    return _mybpf_simple_write(fp, &hdr, sizeof(hdr));
}

static inline MYBPF_SIMPLE_COMMON_HDR_S * _mybpf_simple_get_next_sec(FILE_MEM_S *m, void *cur_hdr)
{
    MYBPF_SIMPLE_COMMON_HDR_S *common_hdr = cur_hdr;

    if (! cur_hdr) {
        /* 获取第一个sec */
        MYBPF_SIMPLE_HDR_S *hdr = (void*)m->pucFileData;
        return (void*)(hdr + 1);
    }

    /* 已经是最后一个sec了 */
    if (common_hdr->sec_type == MYBPF_SIMPLE_SEC_TYPE_END) {
        return NULL;
    }

    return (void*)((UCHAR*)common_hdr + ntohl(common_hdr->sec_size));
}

/* 获取指定type的下一个sec */
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

/* 获取指定type+id的sec */
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

static inline void * _mybpf_simple_get_data(MYBPF_SIMPLE_COMMON_HDR_S *sec)
{
    return sec + 1;
}

static inline int _mybpf_simple_get_prog_names(FILE_MEM_S *m, int id, OUT char **sec_name, OUT char **func_name)
{
    int i;
    char *fname;
    char *sname;

    MYBPF_SIMPLE_PROG_INFO_HDR_S *hdr = _mybpf_simple_get_type_sec(m, MYBPF_SIMPLE_SEC_TYPE_PROG_INFO, 0);
    if (! hdr) {
        RETURN(BS_NOT_FOUND);
    }

    int func_count = ntohs(hdr->func_count);
    if (id >= func_count) {
        RETURN(BS_OUT_OF_RANGE);
    }

    MYBPF_SIMPLE_PROG_OFF_S *off = (void*)(hdr + 1);

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

    return (void*)(hdr + 1);
}

static inline char * _mybpf_simple_get_map_name(FILE_MEM_S *m, int id)
{
    int i;
    MYBPF_SIMPLE_MAP_NAME_HDR_S *name_hdr = _mybpf_simple_get_type_sec(m, MYBPF_SIMPLE_SEC_TYPE_MAP_NAME, 0);
    if (! name_hdr) {
        return NULL;
    }

    if (id >= name_hdr->map_count) {
        return NULL;
    }

    char *map_name = (void*)(name_hdr + 1);

    for (i=0; i<id; i++) {
        map_name = map_name + strlen(map_name) + 1;
    }

    return map_name;
}

static int _mybpf_simple_get_map_def_size(MYBPF_SIMPLE_MAP_HDR_S *hdr)
{
    if (hdr->map_count == 0) {
        return 0;
    }

    int sec_size = ntohl(hdr->sec_size);
    int map_size = sec_size - sizeof(MYBPF_SIMPLE_MAP_HDR_S);

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

    UMAP_ELF_MAP_S * map = (void*)(hdr + 1);

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
    MYBPF_SIMPLE_PROG_OFF_S *off = (void*)(hdr + 1);
    int i;
    int func_count = ntohs(hdr->func_count);

    for (i=0; i<func_count; i++) {
        int len = strlen(buf);
        if (SNPRINTF(buf + len, buf_size - len, "prog:%s, sec:%s, offset:%u, size:%u \r\n", 
                    _mybpf_simple_get_prog_func_name(m, i),
                    _mybpf_simple_get_prog_sec_name(m, i),
                    ntohl(off[i].offset), ntohl(off[i].len)) < 0) {
            RETURNI(BS_NOT_COMPLETE, "buf size too small");
        }
    }

    return 0;
}

typedef int (*PF_MYBPF_SIMPLE_FORMAT_SEC_FUNC)(FILE_MEM_S *m, void *common_hdr, OUT char *buf, int buf_size);

static PF_MYBPF_SIMPLE_FORMAT_SEC_FUNC g_mybpf_simple_format_sec_funcs[] = {
    [MYBPF_SIMPLE_SEC_TYPE_END] = NULL,
    [MYBPF_SIMPLE_SEC_TYPE_MAP] = _mybpf_simple_format_map_info,
    [MYBPF_SIMPLE_SEC_TYPE_PROG] = _mybpf_simple_format_prog_info,
    [MYBPF_SIMPLE_SEC_TYPE_PROG_INFO] = _mybpf_simple_format_proginfo_info,
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

/* p: 控制参数,可以为NULL */
int MYBPF_SIMPLE_ConvertBpf2Simple(char *bpf_file, char *simple_file, MYBPF_SIMPLE_CONVERT_PARAM_S *p)
{
    int maps_count;
    FILE *fp = NULL;
    ELF_S elf;
    MYBPF_RELO_MAP_S maps[MYBPF_LOADER_MAX_MAPS];

    if (ELF_Open(bpf_file, &elf) < 0) {
        RETURNI(BS_ERR, "Can't open file %s", bpf_file);
    }

    fp = FILE_Open(simple_file, TRUE, "wb+");
    if (! fp) {
        ERR_VSet(BS_ERR, "Can't open file %s", simple_file);
        goto err;
    }

    if (_mybpf_simple_write_header(fp) < 0) {
        goto err;
    }

    if ((maps_count = _mybpf_simple_write_maps(fp, &elf, maps, p)) < 0) {
        goto err;
    }

    if (MYBPF_RELO_ProgRelo(&elf, maps, maps_count) < 0) {
        goto err;
    }

    if (_mybpf_simple_write_prog(fp, &elf, p) < 0) {
        goto err;
    }

    if (_mybpf_simple_write_end_sec(fp) < 0) {
        goto err;
    }

    UINT size = ftell(fp);
    if (_mybpf_simple_write_totle_size(fp, size) < 0) {
        goto err;
    }

    ELF_Close(&elf);
    FILE_Close(fp);

    return 0;

err:
    ELF_Close(&elf);
    if (fp) {
        FILE_Close(fp);
        FILE_DelFile(simple_file);
    }
    return -1;
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

    if ((hdr->ver != MYBPF_SIMPLE_VER) || (hdr->magic1 != MYBPF_SIMPLE_MAGIC) || (hdr->magic2 != MYBPF_SIMPLE_MAGIC)) {
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
    map_sec->map_count = 0;
    map_sec->map_def_size = 0;
    map_sec->maps = NULL;

    MYBPF_SIMPLE_MAP_HDR_S *hdr = _mybpf_simple_get_type_sec(m, MYBPF_SIMPLE_SEC_TYPE_MAP, 0);
    if (! hdr) {
        return 0;
    }

    map_sec->map_count = hdr->map_count;
    map_sec->map_def_size = _mybpf_simple_get_map_def_size(hdr);
    map_sec->maps = (void*)(hdr + 1);

    return 0;
}

/* id: map_id, 从0开始 */
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

void * MYBPF_SIMPLE_GetSec(FILE_MEM_S *m, int type, int id)
{
    return _mybpf_simple_get_type_sec(m, type, id);
}

void * MYBPF_SIMPLE_GetSecData(void *sec)
{
    if (! sec) {
        return NULL;
    }

    return _mybpf_simple_get_data(sec);
}

int MYBPF_SIMPLE_GetSecDataSize(void *sec)
{
    if (! sec) {
        return 0;
    }

    MYBPF_SIMPLE_COMMON_HDR_S *hdr = sec;
    int size = ntohl(hdr->sec_size);
    size -= sizeof(*hdr);

    return size;
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


/* 获取prog个数 */
int MYBPF_SIMPLE_GetProgsCount(FILE_MEM_S *m)
{
    MYBPF_SIMPLE_PROG_INFO_HDR_S *hdr;

    hdr = _mybpf_simple_get_type_sec(m, MYBPF_SIMPLE_SEC_TYPE_PROG_INFO, 0);
    if (! hdr) {
        return 0;
    }

    return ntohs(hdr->func_count);
}

/* 获取progs总字节数 */
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

/* 获取第id个prog的offset
   成功返回offset, 失败返回<0.
   off参数可以为NULL */
int MYBPF_SIMPLE_GetProgOffset(FILE_MEM_S *m, int id, OUT MYBPF_SIMPLE_PROG_OFF_S *off)
{
    if (id >= MYBPF_SIMPLE_GetProgsCount(m)) {
        return -1;
    }

    MYBPF_SIMPLE_PROG_OFF_S *offs = _mybpf_simple_get_prog_offs(m);

    int offset = ntohl(offs[id].offset);

    if (off) {
        off->offset = offset;
        off->len = ntohl(offs[id].len);
    }

    return offset;
}

int MYBPF_SIMPLE_GetProgsInfo(FILE_MEM_S *m, OUT ELF_PROG_INFO_S *progs, int max_prog_count)
{
    int i;

    int count = MYBPF_SIMPLE_GetProgsCount(m);
    MYBPF_SIMPLE_PROG_OFF_S *offs = _mybpf_simple_get_prog_offs(m);

    count = MIN(count, max_prog_count);

    for (i=0; i<count; i++) {
        progs[i].offset = ntohl(offs[i].offset);
        progs[i].size = ntohl(offs[i].len);
        progs[i].sec_name = _mybpf_simple_get_prog_sec_name(m, i);
        progs[i].func_name = _mybpf_simple_get_prog_func_name(m, i);
    }

    return count;
}

/* 忽略掉.text中的普通func, 获取第id个main prog的offset
   成功返回offset, 失败返回<0.
   off参数可以为NULL */
int MYBPF_SIMPLE_GetMainProgOffset(FILE_MEM_S *m, int id, OUT MYBPF_SIMPLE_PROG_OFF_S *off)
{
    int i;
    int index = 0;
    int offset;

    int count = MYBPF_SIMPLE_GetProgsCount(m);
    MYBPF_SIMPLE_PROG_OFF_S *offs = _mybpf_simple_get_prog_offs(m);

    for (i=0; i<count; i++) {
        if (strcmp(".text", _mybpf_simple_get_prog_sec_name(m, i)) == 0) {
            continue;
        }

        if (id == index) {
            offset = ntohl(offs[i].offset);
            if (off) {
                off->offset = offset;
                off->len = ntohl(offs[i].len);
            }
            return offset;
        }

        index ++;
    }

    return -1;
}

/* 忽略掉.text中的普通func */
int MYBPF_SIMPLE_GetMainProgsInfo(FILE_MEM_S *m, OUT ELF_PROG_INFO_S *progs, int max_prog_count)
{
    int i;
    int index = 0;

    int count = MYBPF_SIMPLE_GetProgsCount(m);
    MYBPF_SIMPLE_PROG_OFF_S *offs = _mybpf_simple_get_prog_offs(m);

    for (i=0; i<count; i++) {
        if (index >= max_prog_count) {
            break;
        }

        char *sec_name = _mybpf_simple_get_prog_sec_name(m, i);
        if (strcmp(".text", sec_name) == 0) {
            continue;
        }

        progs[index].offset = ntohl(offs[i].offset);
        progs[index].size = ntohl(offs[i].len);
        progs[index].sec_name = sec_name;
        progs[index].func_name = _mybpf_simple_get_prog_func_name(m, i);
        index ++;
    }

    return index;
}

/* 获取整个simple文件大小 */
UINT MYBPF_SIMPLE_GetSimpleSizeByHdr(void *hdr)
{
    MYBPF_SIMPLE_HDR_S *h = hdr;

    if ((h->magic1 != MYBPF_SIMPLE_MAGIC) || (h->magic2 != MYBPF_SIMPLE_MAGIC) || (h->ver != 0)) {
        return 0;
    }

    return ntohl(h->totle_size);
}

