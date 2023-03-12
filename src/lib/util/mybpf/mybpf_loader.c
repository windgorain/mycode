/*================================================================
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2017-10-1
* Description: 
*
================================================================*/
#include <sys/mman.h>
#include "bs.h"
#include "utl/mmap_utl.h"
#include "utl/elf_utl.h"
#include "utl/bit_opt.h"
#include "utl/mybpf_utl.h"
#include "utl/mybpf_loader.h"
#include "utl/mybpf_prog.h"
#include "utl/mybpf_vm.h"
#include "utl/mybpf_runtime.h"
#include "utl/mybpf_hookpoint.h"
#include "utl/mybpf_elf.h"
#include "utl/mybpf_relo.h"
#include "utl/mybpf_simple.h"
#include "utl/mybpf_jit.h"
#include "utl/mybpf_insn.h"
#include "utl/mybpf_dbg.h"
#include "utl/map_utl.h"
#include "utl/exec_utl.h"
#include "utl/ufd_utl.h"
#include "utl/umap_utl.h"
#include "utl/file_utl.h"
#include "mybpf_def_inner.h"
#include "mybpf_osbase.h"

enum {
    MYBPF_FILE_TYPE_ELF = 1,
    MYBPF_FILE_TYPE_SIMPLE,
    MYBPF_FILE_TYPE_SIMPLE_MEM,
};

static void _mybpf_loader_copy_maps_fd(MYBPF_RUNTIME_S *runtime,
        MYBPF_LOADER_NODE_S *node, MYBPF_LOADER_NODE_S *old)
{
    int i;

    node->map_count = old->map_count;
    node->map_def_size = old->map_def_size;
    memcpy(node->maps, old->maps, sizeof(node->maps));
    memcpy(node->map_fds, old->map_fds, sizeof(node->map_fds));
    for (i=0; i<node->map_count; i++) {
        UFD_IncRef(runtime->ufd_ctx, node->map_fds[i]);
    }
}

static int _mybpf_loader_fixup(MYBPF_RUNTIME_S *runtime, MYBPF_LOADER_NODE_S *node)
{
    int ret;

    ret = MYBPF_PROG_ReplaceMapFdWithMapPtr(runtime, node->maps, node);
    if (ret < 0) {
        return ret;
    }

    ret = MYBPF_PROG_FixupExtCalls(node->insts, node->insts_len);
    if (ret < 0) {
        return ret;
    }

    return 0;
}

static int _mybpf_loader_load_one_prog(MYBPF_RUNTIME_S *runtime, char *func_name,
        MYBPF_LOADER_NODE_S *node, uint32_t offset, int insn_len, char *sec_name)
{
    int fd;
    MYBPF_PROG_NODE_S *prog;
    int main_prog_index = node->main_prog_count;

    if (! func_name) {
        RETURNI(BS_ERR, "Can't get function name");
    }

    if (node->main_prog_count >= MYBPF_LOADER_MAX_PROGS) {
        RETURNI(BS_ERR, "Reach prog max number");
    }

    prog = MYBPF_PROG_Alloc((char*)node->insts + offset, insn_len, sec_name, func_name);
    if (! prog) {
        RETURNI(BS_ERR, "Can't load %s:%s \r\n", node->param.filename, func_name);
    }
    prog->loader_node = node;

#if 0
    ret = MYBPF_PROG_ConvertCtxAccess(prog);
    if (ret < 0) {
        MYBPF_PROG_Free(runtime, prog);
        RETURNI(BS_ERR, "Can't load %s:%s", node->param.filename, func_name);
    }
#endif

    fd = MYBPF_PROG_Add(runtime, prog);
    if (fd < 0) {
        MYBPF_PROG_Free(runtime, prog);
        RETURNI(BS_ERR, "Can't load %s:%s", node->param.filename, func_name);
    }

    node->main_prog_fd[main_prog_index] = fd;
    node->main_prog_count ++;

    return 0;
}

static int _mybpf_loader_load_progs(MYBPF_RUNTIME_S *runtime, MYBPF_LOADER_NODE_S *node)
{
    int i;
    int ret = 0;

    for (i=0; i<node->progs_count; i++) {
        ELF_PROG_INFO_S *prog = &node->progs[i];

        if (strcmp(prog->sec_name, ".text") == 0) {
            continue;
        }

        ret = _mybpf_loader_load_one_prog(runtime, prog->func_name, node,
                prog->offset, prog->size, prog->sec_name);
        if (ret < 0) {
            break;
        }
    }

    return ret;
}

static int _mybpf_loader_attach(MYBPF_RUNTIME_S *runtime, int type, int fd, MYBPF_PROG_NODE_S *prog)
{
    if (MYBPF_HookPointAttach(runtime, &runtime->hp_list[type], fd) < 0) {
        RETURN(BS_CAN_NOT_CONNECT);
    }

    prog->attached |= (1 << type);

    return 0;
}

static int _mybpf_loader_auto_attach_prog(MYBPF_RUNTIME_S *runtime, MYBPF_LOADER_NODE_S *node)
{
    int i;
    MYBPF_PROG_NODE_S *prog;

    for (i=0; i<node->main_prog_count; i++) {
        prog = MYBPF_PROG_GetByFD(runtime, node->main_prog_fd[i]);
        if (! prog) {
            continue;
        }

        if (strncmp("tcmd/", prog->sec_name, 5) == 0) {
            _mybpf_loader_attach(runtime, MYBPF_HP_TCMD, node->main_prog_fd[i], prog);
        } else if (strncmp("xdp/", prog->sec_name, 4) == 0) {
            _mybpf_loader_attach(runtime, MYBPF_HP_XDP, node->main_prog_fd[i], prog);
        }
    }

    return 0;
}

/* 校验是否可以replace时保留map, maps定义必须一致 */
static BOOL_T _mybpf_loader_check_may_keep_map(MYBPF_RUNTIME_S *runtime,
        MYBPF_MAPS_SEC_S *map_sec, MYBPF_LOADER_NODE_S *old_node)
{
    UMAP_ELF_MAP_S *elfmap;
    UMAP_HEADER_S *hdr;
    char *map;
    int i;

    if (map_sec->map_count != old_node->map_count) {
        return FALSE;
    }

    if (map_sec->map_def_size != old_node->map_def_size) {
        return FALSE;
    }

    map = map_sec->maps;

    for (i=0; i<map_sec->map_count; i++) {
        elfmap = (void*)map;

        hdr = old_node->maps[i];
        if (! hdr) {
            return FALSE;
        }

        if ((elfmap->type != hdr->type)
                || (elfmap->max_elem != hdr->max_elem)
                || (elfmap->flags != hdr->flags)
                || (elfmap->size_key != hdr->size_key)
                || (elfmap->size_value != hdr->size_value)) {
            return FALSE;
        }

        map = map + map_sec->map_def_size;
    }

    return TRUE;
}

static int _mybpf_get_file_type(MYBPF_LOADER_PARAM_S *p)
{
    char *filename = p->filename;

    if (p->simple_mem) {
        return MYBPF_FILE_TYPE_SIMPLE_MEM;
    }

    if (MYBPF_SIMPLE_IsSimpleFormatFile(filename)) {
        return MYBPF_FILE_TYPE_SIMPLE;
    }

    return MYBPF_FILE_TYPE_ELF;
}

static int _mybpf_loader_deattach(MYBPF_RUNTIME_S *runtime, int type, int fd, MYBPF_PROG_NODE_S *prog)
{
    MYBPF_HookPointDetach(runtime, &runtime->hp_list[type], fd);
    BIT_CLR(prog->attached, (1 << type));
    return 0;
}

static void _mybpf_detach_all_type(MYBPF_RUNTIME_S *runtime, MYBPF_PROG_NODE_S *prog, int fd)
{
    int type;

    for (type=0; type<MYBPF_HP_MAX; type++) {
        if (prog->attached & (1 << type)) {
            _mybpf_loader_deattach(runtime, type, fd, prog);
        }
    }
}

static void _mybpf_loader_deattach_prog_all(MYBPF_RUNTIME_S *runtime, MYBPF_LOADER_NODE_S *node)
{
    MYBPF_PROG_NODE_S *prog;

    for (int i=0; i<node->main_prog_count; i++) {
        prog = MYBPF_PROG_GetByFD(runtime, node->main_prog_fd[i]);
        if (! prog) {
            continue;
        }
        _mybpf_detach_all_type(runtime, prog, node->main_prog_fd[i]);
    }
}


static void _mybpf_loader_free_node(MYBPF_RUNTIME_S *runtime, MYBPF_LOADER_NODE_S *node)
{
    int i;

    for (i=0; i<node->main_prog_count; i++) {
        MYBPF_PROG_Close(runtime, node->main_prog_fd[i]);
    }

    for (i=0; i<node->map_count; i++) {
        UMAP_Close(runtime->ufd_ctx, node->map_fds[i]);
    }

    if (node->param.filename) {
        MEM_Free(node->param.filename);
    }

    if (node->param.instance) {
        MEM_Free(node->param.instance);
    }

    if (node->insts) {
        if (node->jitted) {
            munmap(node->insts, node->insts_len);
        } else {
            MEM_Free(node->insts);
        }
    }

    if (node->progs) {
        ELF_ClearProgsInfo(node->progs, node->progs_count);
        MEM_Free(node->progs);
    }

    MEM_Free(node);
}

static void _mybpf_laoder_free_node_rcu(void *rcu_node)
{
    MYBPF_LOADER_NODE_S *node = container_of(rcu_node, MYBPF_LOADER_NODE_S, rcu);
    _mybpf_loader_free_node(node->runtime, node);
}

static void _mybpf_loader_unload_node(MYBPF_RUNTIME_S *runtime, MYBPF_LOADER_NODE_S *node)
{
    _mybpf_loader_deattach_prog_all(runtime, node);
    MAP_Del(runtime->loader_map, node->param.instance, strlen(node->param.instance));
    RcuEngine_Call(&node->rcu, _mybpf_laoder_free_node_rcu);
}

static int _mybpf_loader_open_global_data_map(MYBPF_RUNTIME_S *runtime, MYBPF_LOADER_NODE_S *node,
        char *name, void *data, int size)
{
    int fd;
    int key = 0;
    UMAP_HEADER_S *map;
    UMAP_ELF_MAP_S map_def;

    MYBPF_DBG_OUTPUT(MYBPF_DBG_ID_LOADER, MYBPF_DBG_FLAG_LOADER_PROCESS,
            "Open map:%s key_size:4 value_size:%d max_elem:1 flags:0x0 \n", name, size);

    map_def.type = BPF_MAP_TYPE_ARRAY;
    map_def.size_key = sizeof(int);
    map_def.max_elem = 1;
    map_def.size_value = size;

    fd = UMAP_Open(runtime->ufd_ctx, &map_def, name);
    if (fd < 0) {
        RETURN(BS_CAN_NOT_OPEN);
    }

    map = UMAP_GetByFd(runtime->ufd_ctx, fd);

    if (data) {
        UMAP_UpdateElem(map, &key, data, 0);
    }

    node->map_fds[node->map_count] = fd;
    node->maps[node->map_count] = map;
    node->map_count ++;

    node->global_data[node->global_data_count] = UMAP_LookupElem(map, &key);
    node->global_data_count ++;

    return 0;
}

static int _mybpf_loader_open_elf_global_data_maps(MYBPF_RUNTIME_S *runtime,
        MYBPF_LOADER_NODE_S *node, ELF_GLOBAL_DATA_S *data)
{
    int ret = 0;

    if (data->have_bss) {
        ret |= _mybpf_loader_open_global_data_map(runtime, node,
                ".bss", NULL, data->bss_sec.data->d_size);
    }

    if (data->have_data) {
        ret |= _mybpf_loader_open_global_data_map(runtime, node,
                ".data", data->data_sec.data->d_buf, data->data_sec.data->d_size);
    }

    for (int i=0; i<data->rodata_count; i++) {
        ret |= _mybpf_loader_open_global_data_map(runtime, node,
                ".rodata", data->rodata_sec[i].data->d_buf, data->rodata_sec[i].data->d_size);
    }

    return ret;
}

static int _mybpf_loader_open_elf_maps(MYBPF_RUNTIME_S *runtime, ELF_S *elf, MYBPF_LOADER_NODE_S *node,
        ELF_GLOBAL_DATA_S *data, MYBPF_MAPS_SEC_S *map_sec)
{
    int ret;
    int fd;
    int i;

    node->map_def_size = UMAP_ELF_MAP_MIN_SIZE;
    if (map_sec->map_def_size) {
        node->map_def_size = map_sec->map_def_size;
    }

    if (node->map_def_size < UMAP_ELF_MAP_MIN_SIZE) {
        RETURN(BS_ERR);
    }

    ret = _mybpf_loader_open_elf_global_data_maps(runtime, node, data);
    if (ret < 0) {
        return ret;
    }

    UMAP_ELF_MAP_S *map = map_sec->maps;
    for (i=0; i<map_sec->map_count; i++) {
        char *map_name = ELF_GetSecSymbolName(elf, map_sec->sec_id, 0, i);

        MYBPF_DBG_OUTPUT(MYBPF_DBG_ID_LOADER, MYBPF_DBG_FLAG_LOADER_PROCESS,
                "Open map:%s key_size:%d value_size:%d max_elem:%d flags:0x%x \n",
                map_name, map->size_key, map->size_value, map->max_elem, map->flags);

        fd = UMAP_Open(runtime->ufd_ctx, map, map_name);
        if (fd < 0) {
            RETURN(BS_CAN_NOT_OPEN);
        }
        node->map_fds[node->map_count] = fd;
        node->maps[node->map_count] = UMAP_GetByFd(runtime->ufd_ctx, fd);
        node->map_count ++;
        map = (void*)((char*)map + map_sec->map_def_size);
    }

    return 0;
}

static inline void _mybpf_loader_fill_relo_one_map(int sec_id, int type,
        int offset, int value, OUT MYBPF_RELO_MAP_S *relo_map)
{
    relo_map->sec_id = sec_id;
    relo_map->type = type;
    relo_map->offset = offset;
    relo_map->value = value;
}

static void _mybpf_loader_fill_relo_maps(ELF_GLOBAL_DATA_S *data, MYBPF_MAPS_SEC_S *map_sec,
        OUT MYBPF_RELO_MAP_S *relo_maps)
{
    int index = 0;

    if (data->have_bss) {
        _mybpf_loader_fill_relo_one_map(data->bss_sec.sec_id, MYBPF_RELO_MAP_BSS, 0, index, &relo_maps[index]);
        index ++;
    }

    if (data->have_data) {
        _mybpf_loader_fill_relo_one_map(data->data_sec.sec_id, MYBPF_RELO_MAP_DATA, 0, index, &relo_maps[index]);
        index ++;
    }

    for (int i=0; i<data->rodata_count; i++) {
        _mybpf_loader_fill_relo_one_map(data->rodata_sec[i].sec_id, MYBPF_RELO_MAP_RODATA, 0, index, &relo_maps[index]);
        index ++;
    }

    for (int i=0; i<map_sec->map_count; i++) {
        _mybpf_loader_fill_relo_one_map(map_sec->sec_id,
                MYBPF_RELO_MAP_BPFMAP, map_sec->map_def_size * i, index, &relo_maps[index]);
        index ++;
    }
}

static int _mybpf_loader_load_elf_maps(MYBPF_RUNTIME_S *runtime, ELF_S *elf,
        MYBPF_LOADER_NODE_S *node, OUT MYBPF_RELO_MAP_S *relo_maps)
{
    int ret;
    int map_count;
    MYBPF_MAPS_SEC_S map_sec = {0};
    ELF_GLOBAL_DATA_S global_data = {0};

    ELF_GetGlobalData(elf, &global_data);
    MYBPF_ELF_GetMapsSection(elf, &map_sec);

    map_count = global_data.sec_count + map_sec.map_count;

    MYBPF_DBG_OUTPUT(MYBPF_DBG_ID_LOADER, MYBPF_DBG_FLAG_LOADER_PROCESS,
            "global_map_count:%d, bpf_map_count:%d \n", global_data.sec_count, map_sec.map_count);

    /* 不存在map */
    if (map_count <= 0) {
        return 0;
    }

    if (map_count > MYBPF_LOADER_MAX_MAPS) {
        BS_DBGASSERT(0);
        RETURN(BS_OUT_OF_RANGE);
    }

    ret = _mybpf_loader_open_elf_maps(runtime, elf, node, &global_data, &map_sec);
    if (ret < 0) {
        return ret;
    }

    _mybpf_loader_fill_relo_maps(&global_data, &map_sec, relo_maps);

    return 0;
}

static void _mybpf_load_jit(MYBPF_LOADER_NODE_S *node)
{
    MYBPF_JIT_INSN_S jit_insn = {0};
    MYBPF_JIT_CFG_S cfg = {0};

    jit_insn.insts = node->insts;
    jit_insn.insts_len = node->insts_len;
    jit_insn.progs = node->progs;
    jit_insn.progs_count = node->progs_count;

    MYBPF_DBG_OUTPUT(MYBPF_DBG_ID_LOADER, MYBPF_DBG_FLAG_LOADER_PROCESS, "jit prog to local arch\n");

    cfg.helper_mode = MYBPF_JIT_HELPER_MODE_BASE;
    if (MYBPF_Jit(&jit_insn, &cfg) < 0) {
        PRINT_HYELLOW("Jit failed, to use raw code");
        return;
    }

    void *code = MMAP_MapExec(jit_insn.insts, jit_insn.insts_len);
    if (! code) {
        return;
    }

    MEM_Free(node->insts);
    node->insts = code;
    node->insts_len = jit_insn.insts_len;
    node->jitted = 1;
}

static int _mybpf_dup_progs_info_names(ELF_PROG_INFO_S *progs, int prog_count)
{
    int ret = 0;

    for (int i=0; i<prog_count; i++) {
        if (progs[i].func_name) {
            progs[i].func_name = TXT_Strdup(progs[i].func_name);
        }
        if (progs[i].sec_name) {
            progs[i].sec_name = TXT_Strdup(progs[i].sec_name);
            if (! progs[i].sec_name) {
                ret = BS_NO_MEMORY;
            }
        }
    }

    return ret;
}

static int _mybpf_loader_load_elf_progs(MYBPF_RUNTIME_S *runtime, ELF_S *elf,
        MYBPF_LOADER_NODE_S *node, MYBPF_RELO_MAP_S *relo_maps)
{
    int ret = 0;

    node->progs_count = ELF_GetProgsCount(elf);
    if (node->progs_count == 0) {
        RETURNI(BS_ERR, "Prog count is 0");
    }

    node->progs = MEM_ZMalloc(sizeof(ELF_PROG_INFO_S) * node->progs_count);
    if (! node->progs) {
        RETURNI(BS_ERR, "Can't alloc memory for progs");
    }
    ELF_GetProgsInfo(elf, node->progs, node->progs_count);
    if (_mybpf_dup_progs_info_names(node->progs,node->progs_count) < 0) {
        RETURN(BS_NO_MEMORY);
    }

    node->insts = ELF_DupProgs(elf);
    if (! node->insts) {
        RETURNI(BS_ERR, "Can't alloc memory for prog");
    }
    node->insts_len = ELF_GetProgsSize(elf);

    if (MYBPF_DBG_IS_SWITCH_ON(MYBPF_DBG_ID_LOADER, MYBPF_DBG_FLAG_LOADER_PROGS)) {
        for (int i=0; i<node->progs_count; i++) {
            ELF_PROG_INFO_S *p = &node->progs[i];
            MYBPF_DBG_OUTPUT(MYBPF_DBG_ID_LOADER, MYBPF_DBG_FLAG_LOADER_PROGS,
                    "sec_name:%s funcname:%s offset:%d addr:0x%p \n",
                    p->sec_name, p->func_name, p->offset, (char*)node->insts + p->offset);
        }
    }

    if ((ret = MYBPF_RELO_ProgRelo(elf, node->insts,
                    relo_maps, node->map_count, node->progs, node->progs_count)) < 0) {
        return ret;
    }

    if ((ret = _mybpf_loader_fixup(runtime, node)) < 0) {
        return ret;
    }

    if (node->param.flag & MYBPF_LOADER_FLAG_JIT) {
        _mybpf_load_jit(node);
    }

    return _mybpf_loader_load_progs(runtime, node);
}

static int _mybpf_loader_load_ok(MYBPF_RUNTIME_S *runtime, MYBPF_LOADER_NODE_S *node, MYBPF_LOADER_NODE_S *old)
{
    if (old) {
        _mybpf_loader_unload_node(runtime, old);
    }

    char *instance = node->param.instance;

    int ret = MAP_AddNode(runtime->loader_map, instance, strlen(instance), node, &node->link_node, 0);
    if (ret < 0) {
        return ret;
    }

    if (node->param.flag & MYBPF_LOADER_FLAG_AUTO_ATTACH) {
        _mybpf_loader_auto_attach_prog(runtime, node);
    }

    return 0;
}

static int _mybpf_loader_load_by_elf(MYBPF_RUNTIME_S *runtime, ELF_S *elf,
        MYBPF_LOADER_NODE_S *node, MYBPF_LOADER_NODE_S *old, UINT flag)
{
    int ret;
    MYBPF_RELO_MAP_S relo_maps[MYBPF_LOADER_MAX_MAPS];

    if (old && (flag & MYBPF_LOADER_FLAG_KEEP_MAP)) {
        _mybpf_loader_copy_maps_fd(runtime, node, old);
    } else {
        ret = _mybpf_loader_load_elf_maps(runtime, elf, node, relo_maps);
        if (ret < 0) {
            return ret;
        }
    }

    ret = _mybpf_loader_load_elf_progs(runtime, elf, node, relo_maps);
    if (ret < 0) {
        return ret;
    }

    return _mybpf_loader_load_ok(runtime, node, old);
}

/* 校验是否可以replace时保留map, maps定义必须一致 */
static BOOL_T _mybpf_loader_check_may_keep_map_elf(MYBPF_RUNTIME_S *runtime,
        ELF_S *new_elf, MYBPF_LOADER_NODE_S *old_node)
{
    MYBPF_MAPS_SEC_S map_sec;

    if (MYBPF_ELF_GetMapsSection(new_elf, &map_sec) < 0) {
        return FALSE;
    }

    return _mybpf_loader_check_may_keep_map(runtime, &map_sec, old_node);
}

/* 检查是否允许替换 */
static int _mybpf_loader_check_may_replace_elf(MYBPF_RUNTIME_S *runtime,
        MYBPF_LOADER_PARAM_S *p, MYBPF_LOADER_NODE_S *old_node)
{
    ELF_S elf = {0};
    int ret;
    BOOL_T check = TRUE;

    /* 判断能否打开文件 */
    ret = ELF_Open(p->filename, &elf);
    if (ret < 0) {
        RETURNI(BS_CAN_NOT_OPEN, "Can't open file %s", p->filename);
    }

    if (p->flag & MYBPF_LOADER_FLAG_KEEP_MAP) {
        /* 判断map def是否一致 */
        check = _mybpf_loader_check_may_keep_map_elf(runtime, &elf, old_node);
    }

    ELF_Close(&elf);

    if (! check) {
        RETURNI(BS_NOT_MATCHED, "Map not match");
    }

    return 0;
}

static int _mybpf_loader_load_node_elf(MYBPF_RUNTIME_S *runtime,
        MYBPF_LOADER_NODE_S *node, MYBPF_LOADER_NODE_S *old, UINT flag)
{
    int ret;
    ELF_S elf = {0};

    ret = ELF_Open(node->param.filename, &elf);
    if (ret < 0) {
        RETURNI(BS_CAN_NOT_OPEN, "Can't open file %s \r\n", node->param.filename);
    }

    ret = _mybpf_loader_load_by_elf(runtime, &elf, node, old, flag);

    ELF_Close(&elf);

    return ret;
}

static int _mybpf_loader_load_simple_maps(MYBPF_RUNTIME_S *runtime, MYBPF_LOADER_NODE_S *node)
{
    UMAP_ELF_MAP_S *map;
    int i;
    int ret;
    MYBPF_MAPS_SEC_S map_sec;

    ret = MYBPF_SIMPLE_GetMapsSection(node->param.simple_mem, &map_sec);
    if (ret < 0) {
        return ret;
    }

    if (map_sec.map_count == 0) {
        return 0;
    }

    if (map_sec.map_def_size < sizeof(UMAP_ELF_MAP_S)) {
        RETURN(BS_ERR);
    }

    if (map_sec.map_count > MYBPF_LOADER_MAX_MAPS) {
        BS_DBGASSERT(0);
        RETURN(BS_OUT_OF_RANGE);
    }

    map = map_sec.maps;

    node->map_def_size = map_sec.map_def_size;
    node->map_count = 0;

    for (i=0; i<map_sec.map_count; i++) {
        char *map_name = MYBPF_SIMPLE_GetMapName(node->param.simple_mem, i);

        if (! map_name) {
            map_name = "";
        }

        MYBPF_DBG_OUTPUT(MYBPF_DBG_ID_LOADER, MYBPF_DBG_FLAG_LOADER_PROCESS,
                "Open map:%s key_size:%d value_size:%d max_elem:%d flags:0x%x \n",
                map_name, map->size_key, map->size_value, map->max_elem, map->flags);

        int fd = UMAP_Open(runtime->ufd_ctx, map, map_name);
        if (fd < 0) {
            RETURN(BS_CAN_NOT_OPEN);
        }
        node->map_fds[i] = fd;
        node->maps[i] = UMAP_GetByFd(runtime->ufd_ctx, fd);
        node->map_count ++;
        map = (void*)((char*)map + map_sec.map_def_size);
    }

    /* 向global data map中填充数据 */
    int map_data_count = MYBPF_SIMPLE_GetTypeSecCount(node->param.simple_mem, MYBPF_SIMPLE_SEC_TYPE_GLOBAL_DATA);
    for (i=0; i<map_data_count; i++) {
        MYBPF_SIMPLE_MAP_DATA_S *sec = MYBPF_SIMPLE_GetSec(node->param.simple_mem, MYBPF_SIMPLE_SEC_TYPE_GLOBAL_DATA, i);
        if (sec)  {
            int key = 0;
            void *data = MYBPF_SIMPLE_GetSecData(sec);
            if (data) {
                UMAP_UpdateElem(node->maps[sec->map_index], &key, data, 0);
            }
            node->global_data[node->global_data_count] = UMAP_LookupElem(node->maps[sec->map_index], &key);
            node->global_data_count ++;
        }
    }

    return 0;
}

static int _mybpf_loader_simple_prog_load(MYBPF_RUNTIME_S *runtime, FILE_MEM_S *f, MYBPF_LOADER_NODE_S *node)
{
    int jit_arch = MYBPF_SIMPLE_GetJitArch(f);

    if (jit_arch != MYBPF_JIT_ARCH_NONE) {
        MYBPF_DBG_OUTPUT(MYBPF_DBG_ID_LOADER, MYBPF_DBG_FLAG_LOADER_PROCESS,
                "Prog has been jitted, arch=%s\n", MYBPF_JIT_GetArchName(jit_arch));

        if (jit_arch != MYBPF_JIT_LocalArch()) {
            RETURNI(BS_NOT_SUPPORT, "Can't support jitted arch %s", MYBPF_JIT_GetArchName(jit_arch));
        }

        node->insts = MMAP_MapExec(MYBPF_SIMPLE_GetProgs(f), node->insts_len);
        if (! node->insts) {
            RETURNI(BS_ERR, "Can't alloc memory for progs");
        }
        node->jitted = 1;

        return 0;
    } else {
        node->insts = MYBPF_SIMPLE_DupPorgs(f);
        if (! node->insts) {
            RETURNI(BS_ERR, "Can't alloc memory for prog");
        }

        int ret = _mybpf_loader_fixup(runtime, node);
        if (ret < 0) {
            return ret;
        }

        if (node->param.flag & MYBPF_LOADER_FLAG_JIT) {
            _mybpf_load_jit(node);
        }

        return 0;
    }
}

static int _mybpf_loader_load_simple_progs(MYBPF_RUNTIME_S *runtime, MYBPF_LOADER_NODE_S *node)
{
    int ret = 0;
    FILE_MEM_S *f = node->param.simple_mem;
    int size = MYBPF_SIMPLE_GetProgsSize(f);

    node->insts_len = size;

    node->progs_count = MYBPF_SIMPLE_GetProgsCount(f);
    if (node->progs_count == 0) {
        RETURNI(BS_ERR, "Prog count is 0");
    }

    node->progs = MEM_ZMalloc(sizeof(ELF_PROG_INFO_S) * node->progs_count);
    if (! node->progs) {
        RETURNI(BS_ERR, "Can't alloc memory for progs");
    }
    MYBPF_SIMPLE_GetProgsInfo(f, node->progs, node->progs_count);
    if (_mybpf_dup_progs_info_names(node->progs,node->progs_count) < 0) {
        RETURN(BS_NO_MEMORY);
    }

    ret = _mybpf_loader_simple_prog_load(runtime, f, node);
    if (ret < 0) {
        return ret;
    }

    if (MYBPF_DBG_IS_SWITCH_ON(MYBPF_DBG_ID_LOADER, MYBPF_DBG_FLAG_LOADER_PROGS)) {
        for (int i=0; i<node->progs_count; i++) {
            ELF_PROG_INFO_S *p = &node->progs[i];
            MYBPF_DBG_OUTPUT(MYBPF_DBG_ID_LOADER, MYBPF_DBG_FLAG_LOADER_PROGS,
                    "sec_name:%s funcname:%s offset:%d addr:0x%p \n",
                    p->sec_name, p->func_name, p->offset, (char*)node->insts + p->offset);
        }
    }

    return _mybpf_loader_load_progs(runtime, node);
}

static int _mybpf_loader_load_simple(MYBPF_RUNTIME_S *runtime,
        MYBPF_LOADER_NODE_S *node, MYBPF_LOADER_NODE_S *old, UINT flag)
{
    int ret;

    if (old && (flag & MYBPF_LOADER_FLAG_KEEP_MAP)) {
        _mybpf_loader_copy_maps_fd(runtime, node, old);
    } else {
        ret = _mybpf_loader_load_simple_maps(runtime, node);
        if (ret < 0) {
            return ret;
        }
    }

    ret = _mybpf_loader_load_simple_progs(runtime, node);
    if (ret < 0) {
        return ret;
    }

    return _mybpf_loader_load_ok(runtime, node, old);
}

/* 校验是否可以replace时保留map, maps定义必须一致 */
static BOOL_T _mybpf_loader_check_may_keep_map_simple(MYBPF_RUNTIME_S *runtime, FILE_MEM_S *f, MYBPF_LOADER_NODE_S *old)
{
    MYBPF_MAPS_SEC_S map_sec;

    if (MYBPF_SIMPLE_GetMapsSection(f, &map_sec) < 0) {
        return FALSE;
    }

    return _mybpf_loader_check_may_keep_map(runtime, &map_sec, old);
}

static int _mybpf_loader_check_may_replace_simple(MYBPF_RUNTIME_S *r, MYBPF_LOADER_PARAM_S *p, MYBPF_LOADER_NODE_S *old)
{
    BOOL_T check = TRUE;

    FILE_MEM_S *f = p->simple_mem;

    BS_DBGASSERT(f);

    if (p->flag & MYBPF_LOADER_FLAG_KEEP_MAP) {
        /* 判断map def是否一致 */
        check = _mybpf_loader_check_may_keep_map_simple(r, f, old);
    }

    if (! check) {
        RETURNI(BS_NOT_MATCHED, "Map not match");
    }

    return 0;
}

static int _mybpf_loader_check_may_replace(MYBPF_RUNTIME_S *runtime, MYBPF_LOADER_PARAM_S *p, MYBPF_LOADER_NODE_S *old)
{
    if (p->simple_mem) {
        return _mybpf_loader_check_may_replace_simple(runtime, p, old);
    }

    return _mybpf_loader_check_may_replace_elf(runtime, p, old);
}

static int _mybpf_loader_load_node(MYBPF_RUNTIME_S *runtime,
        MYBPF_LOADER_NODE_S *node, MYBPF_LOADER_NODE_S *old, UINT flag)
{
    if (node->param.simple_mem) {
        return _mybpf_loader_load_simple(runtime, node, old, flag);
    }

    return _mybpf_loader_load_node_elf(runtime, node, old, flag);
}

static inline MYBPF_LOADER_NODE_S * _mybpf_loader_get_node(MYBPF_RUNTIME_S *runtime, char *instance)
{
    return MAP_Get(runtime->loader_map, instance, strlen(instance));
}

static inline MYBPF_LOADER_NODE_S * _mybpf_loader_get_first_node(MYBPF_RUNTIME_S *runtime)
{
    return MAP_GetFirst(runtime->loader_map);
}

static MYBPF_LOADER_NODE_S * _mybpf_loader_create_node(MYBPF_RUNTIME_S *runtime, MYBPF_LOADER_PARAM_S *p)
{
    MYBPF_LOADER_NODE_S *node;

    node = MEM_ZMalloc(sizeof(MYBPF_LOADER_NODE_S));
    if (! node) {
        ERR_VSet(BS_NO_MEMORY, "Can't alloc memory");
        return NULL;
    }

    node->param.flag = p->flag;

    node->param.simple_mem = p->simple_mem;

    node->param.filename = TXT_Strdup(p->filename);
    if (node->param.filename == NULL) {
        ERR_VSet(BS_NO_MEMORY, "Can't alloc memory");
        _mybpf_loader_free_node(runtime, node);
        return NULL;
    }

    node->param.instance = TXT_Strdup(p->instance);
    if (node->param.instance == NULL) {
        ERR_VSet(BS_NO_MEMORY, "Can't alloc memory");
        _mybpf_loader_free_node(runtime, node);
        return NULL;
    }

    node->runtime = runtime;

    return node;
}

static int _mybpf_loader_load_to_runtime(MYBPF_RUNTIME_S *r, MYBPF_LOADER_PARAM_S *p, MYBPF_LOADER_NODE_S *old)
{
    MYBPF_LOADER_NODE_S *new_node;
    int ret;

    new_node = _mybpf_loader_create_node(r, p);
    if (! new_node) {
        RETURN(BS_ERR);
    }

    ret = _mybpf_loader_load_node(r, new_node, old, p->flag);
    if (ret < 0) {
        _mybpf_loader_free_node(r, new_node);
        return ret;
    }

    return 0;
}

static int _mybpf_loader_load(MYBPF_RUNTIME_S *runtime, MYBPF_LOADER_PARAM_S *p)
{
    int ret;
    MYBPF_LOADER_NODE_S *old_node;

    old_node = _mybpf_loader_get_node(runtime, p->instance);
    if (old_node) {
        ret = _mybpf_loader_check_may_replace(runtime, p, old_node);
        if (ret < 0) {
            return ret;
        }
    }

    return _mybpf_loader_load_to_runtime(runtime, p, old_node);
}

int MYBPF_LoaderLoad(MYBPF_RUNTIME_S *runtime, MYBPF_LOADER_PARAM_S *p)
{
    if (! p->filename) {
        p->filename = "";
    }

    int type = _mybpf_get_file_type(p);

    if (type == MYBPF_FILE_TYPE_SIMPLE) {
        p->simple_mem = MYBPF_SIMPLE_OpenFile(p->filename);
        if (! p->simple_mem) {
            RETURNI(BS_CAN_NOT_OPEN, "Can't open file %s", p->filename);
        }
    }

    int ret = _mybpf_loader_load(runtime, p);

    if (type == MYBPF_FILE_TYPE_SIMPLE) {
        MYBPF_SIMPLE_Close(p->simple_mem);
        p->simple_mem = NULL;
    }

    return ret;
}

int MYBPF_DetachAll(MYBPF_RUNTIME_S *runtime, char *instance)
{
    MYBPF_LOADER_NODE_S *node;

    node = _mybpf_loader_get_node(runtime, instance);
    if (! node) {
        RETURN(BS_NO_SUCH);
    }

    _mybpf_loader_deattach_prog_all(runtime, node);

    return 0;
}

int MYBPF_AttachAuto(MYBPF_RUNTIME_S *runtime, char *instance)
{
    MYBPF_LOADER_NODE_S *node;

    node = _mybpf_loader_get_node(runtime, instance);
    if (! node) {
        RETURN(BS_NO_SUCH);
    }

    _mybpf_loader_auto_attach_prog(runtime, node);
    node->param.flag |= MYBPF_LOADER_FLAG_AUTO_ATTACH;

    return 0;
}

void MYBPF_LoaderUnload(MYBPF_RUNTIME_S *runtime, char *instance)
{
    MYBPF_LOADER_NODE_S *node;

    node = _mybpf_loader_get_node(runtime, instance);
    if (node) {
        _mybpf_loader_unload_node(runtime, node);
    }
}

void MYBPF_LoaderUnloadAll(MYBPF_RUNTIME_S *runtime)
{
    MYBPF_LOADER_NODE_S *node;

    while ((node = _mybpf_loader_get_first_node(runtime))) {
        _mybpf_loader_unload_node(runtime, node);
    }
}

MYBPF_LOADER_NODE_S * MYBPF_LoaderGet(MYBPF_RUNTIME_S *runtime, char *instance)
{
    return _mybpf_loader_get_node(runtime, instance);
}

/* *iter=NULL表示获取第一个, return NULL表示结束 */
MYBPF_LOADER_NODE_S * MYBPF_LoaderGetNext(MYBPF_RUNTIME_S *runtime, INOUT void **iter)
{
    MAP_ELE_S *ele;
    MYBPF_LOADER_NODE_S *node;

    ele = MAP_GetNextEle(runtime->loader_map, *iter);
    *iter = ele;
    if (! ele) {
        return NULL;
    }

    node = ele->pData;

    return node;
}

