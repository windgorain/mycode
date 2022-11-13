/*================================================================
*   Created by LiXingang
*   Date: 2017.10.1
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/elf_utl.h"
#include "utl/bit_opt.h"
#include "utl/mybpf_utl.h"
#include "utl/mybpf_prog.h"
#include "utl/mybpf_vm.h"
#include "utl/mybpf_loader.h"
#include "utl/mybpf_runtime.h"
#include "utl/mybpf_hookpoint.h"
#include "utl/map_utl.h"
#include "utl/exec_utl.h"
#include "utl/ufd_utl.h"
#include "utl/umap_utl.h"
#include "mybpf_def.h"
#include "mybpf_osbase.h"

typedef struct {
    int sec_id;
    int map_def_size;
    int map_count;
    void *maps;
}MYBPF_LOADER_MAPS_SEC_S;

static int _mybpf_loader_get_maps_section(ELF_S *elf, OUT MYBPF_LOADER_MAPS_SEC_S *map_sec)
{
    ELF_SECTION_S sec;

    map_sec->map_count = 0;
    map_sec->map_def_size = 0;
    map_sec->maps = NULL;

    if (ELF_GetSecByName(elf, "maps", &sec) < 0) {
        return 0;
    }

    map_sec->map_count = ELF_SecSymbolCount(elf, sec.sec_id, 0);
    if (map_sec->map_count <= 0) {
        return 0;
    }

    /* 计算map结构体的大小 */
    map_sec->map_def_size = sec.data->d_size / map_sec->map_count;
    if (map_sec->map_def_size < sizeof(UMAP_ELF_MAP_S)) {
        RETURN(BS_ERR);
    }

    map_sec->maps = sec.data->d_buf;
    map_sec->sec_id = sec.sec_id;

    return 0;
}

static int _mybpf_loader_load_maps(ELF_S *elf, MYBPF_LOADER_NODE_S *node)
{
    char *map;
    char *map_name;
    int i;
    int ret;
    int map_def_offset;
    MYBPF_LOADER_MAPS_SEC_S map_sec;

    ret = _mybpf_loader_get_maps_section(elf, &map_sec);
    if (ret < 0) {
        return ret;
    }

    if (map_sec.map_count == 0) {
        return 0;
    }

    if (map_sec.map_count > MYBPF_LOADER_MAX_MAPS) {
        BS_DBGASSERT(0);
        RETURN(BS_OUT_OF_RANGE);
    }

    map = map_sec.maps;
    map_def_offset = 0;

    node->map_def_size = map_sec.map_def_size;
    node->map_count = 0;

    for (i=0; i<map_sec.map_count; i++) {
        map_name = ELF_GetSecSymbolName(elf, map_sec.sec_id, 0, i);
        node->map_fd[i] = UMAP_Open((void*)map, map_def_offset, map_name);
        if (node->map_fd[i] < 0) {
            RETURN(BS_CAN_NOT_OPEN);
        }
        node->map_count ++;
        map = map + map_sec.map_def_size;
        map_def_offset += map_sec.map_def_size;
    }

    return 0;
}

static int _mybpf_loader_load_prog(MYBPF_RUNTIME_S *runtime, ELF_S *elf, MYBPF_LOADER_NODE_S *node, ELF_SECTION_S *sec)
{
    char *name;
    int fd;
    int ret;
    MYBPF_PROG_NODE_S *prog;
    int prog_index = node->prog_count;

    if (! ELF_IsProgSection(sec)) {
        return 0;
    }

    if (node->prog_count >= MYBPF_LOADER_MAX_PROGS) {
        RETURNI(BS_ERR, "Reach prog max number");
    }

    name = ELF_GetSecSymbolName(elf, sec->sec_id, STT_FUNC, 0);
    prog = MYBPF_PROG_Alloc(sec->data->d_buf, sec->data->d_size, sec->shname, name);
    if (! prog) {
        EXEC_OutInfo("Can't load %s:%s \r\n", node->param.filename, name);
        RETURN(BS_NO_MEMORY);
    }

    ret = MYBPF_PROG_ReplaceMapFdWithMapPtr(prog);
    ret |= MYBPF_PROG_ConvertCtxAccess(prog);
    ret |= MYBPF_PROG_FixupBpfCalls(prog);
    if (ret < 0) {
        MYBPF_PROG_Free(prog);
        RETURNI(BS_ERR, "Can't load %s:%s", node->param.filename, name);
    }

    fd = MYBPF_PROG_Add(prog);
    if (fd < 0) {
        MYBPF_PROG_Free(prog);
        RETURNI(BS_ERR, "Can't load %s:%s", node->param.filename, name);
    }

    node->prog_fd[prog_index] = fd;
    node->prog_count ++;

    return 0;
}

static int _mybpf_loader_attach_xdp(MYBPF_RUNTIME_S *runtime, int fd, MYBPF_PROG_NODE_S *prog)
{
    if (MYBPF_XdpAttach(&runtime->xdp_list, fd) < 0) {
        RETURN(BS_CAN_NOT_CONNECT);
    }

    prog->attached |= (1 << MYBPF_ATTACH_TYPE_XDP);

    return 0;
}

static int _mybpf_loader_deattach_xdp(MYBPF_RUNTIME_S *runtime, int fd, MYBPF_PROG_NODE_S *prog)
{
    MYBPF_XdpDetach(&runtime->xdp_list, fd);
    BIT_CLR(prog->attached, (1 << MYBPF_ATTACH_TYPE_XDP));
    return 0;
}

static int _mybpf_loader_auto_attach_prog(MYBPF_RUNTIME_S *runtime, MYBPF_LOADER_NODE_S *node)
{
    int i;
    MYBPF_PROG_NODE_S *prog;

    for (i=0; i<node->prog_count; i++) {
        prog = MYBPF_PROG_GetByFD(node->prog_fd[i]);
        if (! prog) {
            continue;
        }
        if (strncmp("xdp/", prog->sec_name, 4) == 0) {
            _mybpf_loader_attach_xdp(runtime, node->prog_fd[i], prog);
        }
    }

    return 0;
}

static int _mybpf_loader_deattach_prog_all(MYBPF_RUNTIME_S *runtime, MYBPF_LOADER_NODE_S *node)
{
    int i;
    MYBPF_PROG_NODE_S *prog;

    for (i=0; i<node->prog_count; i++) {
        prog = MYBPF_PROG_GetByFD(node->prog_fd[i]);
        if (! prog) {
            continue;
        }
        if (strncmp("xdp/", prog->sec_name, 4) == 0) {
            _mybpf_loader_deattach_xdp(runtime, node->prog_fd[i], prog);
        }
    }

    return 0;
}

static int _mybpf_loader_load_progs(MYBPF_RUNTIME_S *runtime, ELF_S *elf, MYBPF_LOADER_NODE_S *node)
{
    void *iter = NULL;
    ELF_SECTION_S sec;
    int ret = 0;

    while ((iter = ELF_GetNextSection(elf, iter, &sec))) {
        ret = _mybpf_loader_load_prog(runtime, elf, node, &sec);
        if (ret < 0) {
            break;
        }
    }

    return ret;
}

static int _mybpf_loader_parse_relo_and_apply(ELF_S *elf, MYBPF_LOADER_NODE_S *node, ELF_SECTION_S *relo_sec, ELF_SECTION_S *prog_sec)
{
    Elf_Data *data = relo_sec->data;
    GElf_Shdr *shdr = &relo_sec->shdr;
    MYBPF_INSN_S *insn = (MYBPF_INSN_S *) prog_sec->data->d_buf;
	int nrels = shdr->sh_size / shdr->sh_entsize;
    int i;
    UMAP_HEADER_S *hdr;

	for (i = 0; i < nrels; i++) {
		GElf_Sym *sym;
		GElf_Rel rel;
		unsigned int insn_idx;
		BOOL_T match = FALSE;
		int map_idx;

        ELF_GetRel(data, i, &rel);

		insn_idx = rel.r_offset / sizeof(MYBPF_INSN_S);

		sym = ELF_GetSymbol(elf, GELF_R_SYM(rel.r_info));

		if (insn[insn_idx].code != (BPF_LD | BPF_IMM | BPF_DW)) {
			return -1;
		}

		insn[insn_idx].src_reg = BPF_PSEUDO_MAP_FD;

		/* Match FD relocation against recorded map_data[] offset */
		for (map_idx = 0; map_idx < node->map_count; map_idx++) {
            hdr = UMAP_GetByFd(node->map_fd[map_idx]);
			if (hdr->map_def_offset == sym->st_value) {
				match = TRUE;
                insn[insn_idx].imm = node->map_fd[map_idx];
                break;
			}
		}

		if (! match) {
			return -1;
		}
	}

	return 0;
}

static int _mybpf_loader_process_relo(ELF_S *elf, MYBPF_LOADER_NODE_S *node)
{
    void *iter = NULL;
    ELF_SECTION_S relo_sec, prog_sec;

    while((iter = ELF_GetNextSection(elf, iter, &relo_sec))) {
        if (relo_sec.shdr.sh_type != SHT_REL) {
            continue;
        }

        /* locate prog sec that need map fixup (relocations) */
        if (ELF_GetSecByID(elf, relo_sec.shdr.sh_info, &prog_sec) < 0) {
            continue;
        }

        if (! ELF_IsProgSection(&prog_sec)) {
            continue;
        }

        _mybpf_loader_parse_relo_and_apply(elf, node, &relo_sec, &prog_sec);
    }

    return 0;
}

static int _mybpf_loader_load_by_file(MYBPF_RUNTIME_S *runtime, ELF_S *elf,
        MYBPF_LOADER_NODE_S *node, MYBPF_LOADER_NODE_S *to_replace, UINT flag)
{
    int ret;
    int i;

    if (flag & MYBPF_LOADER_FLAG_KEEP_MAP) {
        node->map_count = to_replace->map_count;
        node->map_def_size = to_replace->map_def_size;
        memcpy(node->map_fd, to_replace->map_fd, sizeof(node->map_fd));
        for (i=0; i<node->map_count; i++) {
            UFD_IncRef(node->map_fd[i]);
        }
    } else {
        ret = _mybpf_loader_load_maps(elf, node);
        if (ret < 0) {
            return ret;
        }
    }

    _mybpf_loader_process_relo(elf, node);

    ret = _mybpf_loader_load_progs(runtime, elf, node);
    if (ret < 0) {
        return ret;
    }

    if (flag & MYBPF_LOADER_FLAG_AUTO_ATTACH) {
        _mybpf_loader_auto_attach_prog(runtime, node);
    }

    return 0;
}

/* 校验是否可以replace时保留map, maps定义必须一致 */
static BOOL_T _mybpf_loader_check_may_keep_map(ELF_S *new_elf, MYBPF_LOADER_NODE_S *old_node)
{
    MYBPF_LOADER_MAPS_SEC_S map_sec;
    UMAP_ELF_MAP_S *elfmap;
    UMAP_HEADER_S *hdr;
    char *map;
    int i;

    if (_mybpf_loader_get_maps_section(new_elf, &map_sec) < 0) {
        return FALSE;
    }

    if (map_sec.map_count != old_node->map_count) {
        return FALSE;
    }

    if (map_sec.map_def_size != old_node->map_def_size) {
        return FALSE;
    }

    map = map_sec.maps;

    for (i=0; i<map_sec.map_count; i++) {
        elfmap = (void*)map;

        hdr = UMAP_GetByFd(old_node->map_fd[i]);
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

        map = map + map_sec.map_def_size;
    }

    return TRUE;
}

/* 检查是否允许替换 */
static int _mybpf_loader_check_may_replace(MYBPF_RUNTIME_S *runtime,
        MYBPF_LOADER_PARAM_S *p, MYBPF_LOADER_NODE_S *old_node)
{
    ELF_S elf;
    int ret;
    BOOL_T check = TRUE;

    /* 判断能否打开文件 */
    ret = ELF_Open(p->filename, &elf);
    if (ret < 0) {
        RETURNI(BS_CAN_NOT_OPEN, "Can't open file %s", p->filename);
    }

    if (p->flag & MYBPF_LOADER_FLAG_KEEP_MAP) {
        /* 判断map def是否一致 */
        check = _mybpf_loader_check_may_keep_map(&elf, old_node);
    }

    ELF_Close(&elf);

    if (! check) {
        RETURNI(BS_NOT_MATCHED, "Map not match");
    }

    return 0;
}

static int _mybpf_loader_load_node(MYBPF_RUNTIME_S *runtime,
        MYBPF_LOADER_NODE_S *node, MYBPF_LOADER_NODE_S *to_replace, UINT flag)
{
    int ret;
    ELF_S elf;

    ret = ELF_Open(node->param.filename, &elf);
    if (ret < 0) {
        RETURNI(BS_CAN_NOT_OPEN, "Can't open file %s \r\n", node->param.filename);
    }

    ret = _mybpf_loader_load_by_file(runtime, &elf, node, to_replace, flag);

    ELF_Close(&elf);

    return ret;
}

static inline MYBPF_LOADER_NODE_S * _mybpf_loader_get_node(MYBPF_RUNTIME_S *runtime, char *instance)
{
    return MAP_Get(runtime->loader_map, instance, strlen(instance));
}

static void _mybpf_loader_free_node(MYBPF_RUNTIME_S *runtime, MYBPF_LOADER_NODE_S *node)
{
    int i;
    MYBPF_PROG_NODE_S * prog;

    for (i=0; i<node->prog_count; i++) {
        prog = MYBPF_PROG_GetByFD(node->prog_fd[i]);
        if (! prog) {
            continue;
        }
        if (prog->attached & (1 << MYBPF_ATTACH_TYPE_XDP)) {
            MYBPF_XdpDetach(&runtime->xdp_list, node->prog_fd[i]);
        }
        MYBPF_PROG_Close(node->prog_fd[i]);
    }

    for (i=0; i<node->map_count; i++) {
        UMAP_Close(node->map_fd[i]);
    }

    if (node->param.filename) {
        MEM_Free(node->param.filename);
    }

    if (node->param.instance) {
        MEM_Free(node->param.instance);
    }

    if (node->param.sec_name) {
        MEM_Free(node->param.sec_name);
    }

    MEM_Free(node);
}

static MYBPF_LOADER_NODE_S * _mybpf_loader_create_node(MYBPF_RUNTIME_S *runtime, MYBPF_LOADER_PARAM_S *p)
{
    int ret;
    MYBPF_LOADER_NODE_S *node;

    node = MEM_ZMalloc(sizeof(MYBPF_LOADER_NODE_S));
    if (! node) {
        ERR_VSet(BS_NO_MEMORY, "Can't alloc memory");
        return NULL;
    }

    node->param.flag = p->flag;

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

    if (p->sec_name) {
        node->param.sec_name = TXT_Strdup(p->sec_name);
        if (node->param.sec_name == NULL) {
            ERR_VSet(BS_NO_MEMORY, "Can't alloc memory");
            _mybpf_loader_free_node(runtime, node);
            return NULL;
        }
    }

    ret = MAP_AddNode(runtime->loader_map, node->param.instance, strlen(p->instance),
            node, &node->link_node, 0);
    if (ret < 0) {
        _mybpf_loader_free_node(runtime, node);
        return NULL;
    }

    return node;
}

static void _mybpf_loader_unload_node(MYBPF_RUNTIME_S *runtime, MYBPF_LOADER_NODE_S *node)
{
    MAP_Del(runtime->loader_map, node->param.instance, strlen(node->param.instance));
    _mybpf_loader_free_node(runtime, node);
}

static int _mybpf_loader_load(MYBPF_RUNTIME_S *runtime, MYBPF_LOADER_PARAM_S *p, MYBPF_LOADER_NODE_S *to_replace)
{
    MYBPF_LOADER_NODE_S *new_node;
    int ret;

    new_node = _mybpf_loader_create_node(runtime, p);
    if (! new_node) {
        return BS_ERR;
    }

    ret = _mybpf_loader_load_node(runtime, new_node, to_replace, p->flag);
    if (ret < 0) {
        _mybpf_loader_unload_node(runtime, new_node);
        return ret;
    }

    return 0;
}

static void _mybpf_loader_disable_prog(MYBPF_LOADER_NODE_S *node)
{
    int i;
    for (i=0; i<node->prog_count; i++) {
        MYBPF_PROG_Disable(node->prog_fd[i]);
    }
}

static void _mybpf_loader_enable_prog(MYBPF_LOADER_NODE_S *node)
{
    int i;
    for (i=0; i<node->prog_count; i++) {
        MYBPF_PROG_Enable(node->prog_fd[i]);
    }
}

static void _mybpf_loader_disable_node(MYBPF_RUNTIME_S *runtime, MYBPF_LOADER_NODE_S *node)
{
    if (node) {
        /* 先将old node从map中摘除 */
        MAP_Del(runtime->loader_map, node->param.instance, strlen(node->param.instance));
        _mybpf_loader_disable_prog(node);
    }
}

static void _mybpf_loader_enable_node(MYBPF_RUNTIME_S *runtime, MYBPF_LOADER_NODE_S *node)
{
    if (node ) {
        MAP_AddNode(runtime->loader_map, node->param.instance, strlen(node->param.instance), node, &node->link_node, 0);
        _mybpf_loader_enable_prog(node);
    }
}

int MYBPF_LoaderLoad(MYBPF_RUNTIME_S *runtime, MYBPF_LOADER_PARAM_S *p)
{
    int ret;
    MYBPF_LOADER_NODE_S *old_node;

    old_node = _mybpf_loader_get_node(runtime, p->instance);
    if (old_node) {
        ret = _mybpf_loader_check_may_replace(runtime, p, old_node);
        if (ret < 0) {
            return ret;
        }
        _mybpf_loader_disable_node(runtime, old_node);
    } else {
        BIT_CLR(p->flag, MYBPF_LOADER_FLAG_KEEP_MAP);
    }

    ret = _mybpf_loader_load(runtime, p, old_node);
    if (ret < 0) {
        if (old_node) {
            _mybpf_loader_enable_node(runtime, old_node);
        }
        return ret;
    }

    if (old_node) {
        _mybpf_loader_free_node(runtime, old_node);
    }

    return 0;
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

MYBPF_LOADER_NODE_S * MYBPF_LoaderGet(MYBPF_RUNTIME_S *runtime, char *instance)
{
    return _mybpf_loader_get_node(runtime, instance);
}

/* *iter=NULL表示获取第一个, return NULL表示结束 */
MYBPF_LOADER_NODE_S * MYBPF_LoaderGetNext(MYBPF_RUNTIME_S *runtime, INOUT void **iter)
{
    MAP_ELE_S *ele;
    MYBPF_LOADER_NODE_S *node;

    ele = MAP_GetNext(runtime->loader_map, *iter);
    *iter = ele;
    if (! ele) {
        return NULL;
    }

    node = ele->pData;

    return node;
}

