/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/elf_utl.h"
#include "utl/ulc_utl.h"
#include "utl/map_utl.h"
#include "utl/exec_utl.h"
#include "../h/ulc_def.h"
#include "../h/ulc_map.h"
#include "../h/ulc_prog.h"
#include "../h/ulc_fd.h"
#include "../h/ulc_hookpoint.h"
#include "../h/ulc_osbase.h"
#include "../h/ulc_verifier.h"

#define ULC_LOADER_MAX_MAPS 32
#define ULC_LOADER_MAX_PROGS 32

enum {
    ULC_LOAD_TYPE_XDP = 0,
};

typedef struct {
    ULC_LOADER_PARAM_S param;
    int type;
    ELF_S *elf;
    int prog_count;
    int prog_fd[ULC_LOADER_MAX_PROGS];
    int map_count;
    int map_fd[ULC_LOADER_MAX_MAPS];
}ULC_LOADER_S;

static MAP_HANDLE g_ulc_loader_map;

static int _ulcloader_load_maps(ULC_LOADER_S *loader)
{
    ELF_SECTION_S sec;
    int map_count;
    int map_def_size;
    char *map;
    char *map_name;
    int i;
    int map_offset;

    if (ELF_GetSecByName(loader->elf, "maps", &sec) < 0) {
        return 0;
    }

    map_count = ELF_SecSymbolCount(loader->elf, sec.sec_id, 0);
    if (map_count <= 0) {
        return 0;
    }

    if (map_count > ULC_LOADER_MAX_MAPS) {
        BS_DBGASSERT(0);
        return -1;
    }

    /* 计算map结构体的大小 */
    map_def_size = sec.data->d_size / map_count;

    if (map_def_size < sizeof(ULC_ELF_MAP_S)) {
        return -1;
    }

    map = sec.data->d_buf;
    map_offset = 0;

    loader->map_count = 0;

    for (i=0; i<map_count; i++) {
        map_name = ELF_GetSecSymbolName(loader->elf, sec.sec_id, 0, i);
        loader->map_fd[i] = ULC_MAP_Open((void*)map, map_offset, map_name);
        if (loader->map_fd[i] < 0) {
            return -1;
        }
        loader->map_count ++;
        map = map + map_def_size;
        map_offset += map_def_size;
    }

    return 0;
}

static BOOL_T _ulcloader_should_load_prog(ULC_LOADER_S *loader, ELF_SECTION_S *sec)
{
    if (! ELF_IsProgSection(sec)) {
        return FALSE;
    }

    if (loader->param.sec_name) {
        if (strcmp(loader->param.sec_name, sec->shname) == 0) {
            return TRUE;
        }

        return FALSE;
    }

    if ((loader->type == ULC_LOAD_TYPE_XDP) && (strncmp("xdp/", sec->shname, 4) == 0)) {
        return TRUE;
    }

    return FALSE;
}

static int _ulcloader_load_prog(ULC_LOADER_S *loader, ELF_SECTION_S *sec)
{
    ELF_S *elf = loader->elf;
    char *name;
    int fd;

    ULC_PROG_NODE_S *prog;
    int prog_index = loader->prog_count;

    if (loader->prog_count >= ULC_LOADER_MAX_PROGS) {
        RETURN(BS_REACH_MAX);
    }

    name = ELF_GetSecSymbolName(elf, sec->sec_id, STT_FUNC, 0);
    prog = ULC_PROG_Alloc(sec->data->d_buf, sec->data->d_size, name);
    if (! prog) {
        EXEC_OutInfo("Can't load %s:%s \r\n", loader->param.filename, name);
        RETURN(BS_NO_MEMORY);
    }
    ULC_VERIFIER_ReplaceMapFdWithMapPtr(prog);
    ULC_VERIFIER_ConvertCtxAccess(prog);
    ULC_VERIFIER_FixupBpfCalls(prog);
    fd = ULC_PROG_Add(prog);
    if (fd < 0) {
        EXEC_OutInfo("Can't load %s:%s \r\n", loader->param.filename, name);
        ULC_PROG_Free(prog);
        RETURN(BS_ERR);
    }

    if (ULC_HookPoint_XdpAttach(fd) < 0) {
        ULC_PROG_Close(fd);
        RETURN(BS_CAN_NOT_CONNECT);
    }

    prog->attached |= (1 << ULC_LOAD_TYPE_XDP);
    loader->prog_fd[prog_index] = fd;
    loader->prog_count ++;

    return 0;
}

static int _ulcloader_load_progs(ULC_LOADER_S *loader)
{
    void *iter = NULL;
    ELF_SECTION_S sec;
    ELF_S *elf = loader->elf;

    while ((iter = ELF_GetNextSection(elf, iter, &sec))) {
        if (_ulcloader_should_load_prog(loader, &sec)) {
            _ulcloader_load_prog(loader, &sec);
        }
    }

    return 0;
}

static int _ulcloader_parse_relo_and_apply(ULC_LOADER_S *loader, ELF_SECTION_S *relo_sec, ELF_SECTION_S *prog_sec)
{
    Elf_Data *data = relo_sec->data;
    GElf_Shdr *shdr = &relo_sec->shdr;
    ULC_INSN_S *insn = (ULC_INSN_S *) prog_sec->data->d_buf;
	int nrels = shdr->sh_size / shdr->sh_entsize;
    int i;
    ULC_MAP_HEADER_S *hdr;

	for (i = 0; i < nrels; i++) {
		GElf_Sym *sym;
		GElf_Rel rel;
		unsigned int insn_idx;
		BOOL_T match = FALSE;
		int map_idx;

        ELF_GetRel(data, i, &rel);

		insn_idx = rel.r_offset / sizeof(ULC_INSN_S);

		sym = ELF_GetSymbol(loader->elf, GELF_R_SYM(rel.r_info));

		if (insn[insn_idx].code != (BPF_LD | BPF_IMM | BPF_DW)) {
			return -1;
		}

		insn[insn_idx].src_reg = BPF_PSEUDO_MAP_FD;

		/* Match FD relocation against recorded map_data[] offset */
		for (map_idx = 0; map_idx < loader->map_count; map_idx++) {
            hdr = ULC_MAP_GetByFd(loader->map_fd[map_idx]);
			if (hdr->map_offset == sym->st_value) {
				match = TRUE;
                insn[insn_idx].imm = loader->map_fd[map_idx];
                break;
			}
		}

		if (! match) {
			return -1;
		}
	}

	return 0;
}

static int _ulcloader_process_relo(ULC_LOADER_S *loader)
{
    ELF_S *elf = loader->elf;
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

        _ulcloader_parse_relo_and_apply(loader, &relo_sec, &prog_sec);
    }

    return 0;
}

static int _ulcloader_load_by_file(ULC_LOADER_S *loader)
{
    int ret;

    ret = _ulcloader_load_maps(loader);
    if (ret < 0) {
        return ret;
    }

    _ulcloader_process_relo(loader);

    ret = _ulcloader_load_progs(loader);
    if (ret < 0) {
        return ret;
    }

    return 0;
}

static int _ulc_loader_load(ULC_LOADER_S *loader)
{
    int ret;
    ELF_S elf;

    ret = ELF_Open(loader->param.filename, &elf);
    if (ret < 0) {
        return -1;
    }

    loader->type = ULC_LOAD_TYPE_XDP;
    loader->elf = &elf;

    ret = _ulcloader_load_by_file(loader);

    ELF_Close(&elf);

    loader->elf = NULL;

    return ret;
}

static void _ulc_loader_free(ULC_LOADER_S *loader)
{
    int i;
    ULC_PROG_NODE_S * prog;

    for (i=0; i<loader->prog_count; i++) {
        prog = ULC_PROG_GetByFD(loader->prog_fd[i]);
        if (! prog) {
            continue;
        }
        if (prog->attached & (1 << ULC_LOAD_TYPE_XDP)) {
            ULC_HookPoint_XdpDetach(loader->prog_fd[i]);
        }
        ULC_PROG_Close(loader->prog_fd[i]);
    }

    for (i=0; i<loader->map_count; i++) {
        ULC_MAP_Close(loader->map_fd[i]);
    }

    if (loader->param.filename) {
        MEM_Free(loader->param.filename);
    }
    if (loader->param.sec_name) {
        MEM_Free(loader->param.sec_name);
    }

    MEM_Free(loader);
}

static ULC_LOADER_S * _ulc_loader_create_node(ULC_LOADER_PARAM_S *p)
{
    int ret;
    ULC_LOADER_S * loader;

    if (MAP_Get(g_ulc_loader_map, p->filename, strlen(p->filename))) {
        return NULL;
    }

    loader = MEM_ZMalloc(sizeof(ULC_LOADER_S));
    if (! loader) {
        return NULL;
    }

    loader->param.filename = TXT_Strdup(p->filename);
    if (loader->param.filename == NULL) {
        _ulc_loader_free(loader);
        return NULL;
    }

    if (p->sec_name) {
        loader->param.sec_name = TXT_Strdup(p->sec_name);
        if (loader->param.sec_name == NULL) {
            _ulc_loader_free(loader);
            return NULL;
        }
    }

    ret = MAP_Add(g_ulc_loader_map, loader->param.filename, strlen(p->filename), loader, 0);
    if (ret < 0) {
        _ulc_loader_free(loader);
        return NULL;
    }

    return loader;
}

static void _ulc_loader_unload_node(ULC_LOADER_S *loader)
{
    MAP_Del(g_ulc_loader_map, loader->param.filename, strlen(loader->param.filename));
    _ulc_loader_free(loader);
}

int ULC_Loader_Init()
{
    g_ulc_loader_map = MAP_HashCreate(NULL);
    if (! g_ulc_loader_map) {
        RETURN(BS_NO_MEMORY);
    }

    return 0;
}

int ULC_Loader_Load(ULC_LOADER_PARAM_S *p)
{
    int ret;
    ULC_LOADER_S *loader = _ulc_loader_create_node(p);
    if (! loader) {
        RETURN(BS_ERR);
    }

    ret = _ulc_loader_load(loader);
    if (ret < 0) {
        _ulc_loader_unload_node(loader);
        return ret;
    }

    return 0;
}

void ULC_Loader_UnLoad(char *filename)
{
    ULC_LOADER_S * loader;

    loader = MAP_Get(g_ulc_loader_map, filename, strlen(filename));
    if (loader) {
        _ulc_loader_unload_node(loader);
    }
}

/* *iter=NULL表示获取第一个, return NULL表示结束 */
ULC_LOADER_PARAM_S * ULC_Loader_GetNext(INOUT void **iter)
{
    MAP_ELE_S *ele;
    ULC_LOADER_S *loader;

    ele = MAP_GetNext(g_ulc_loader_map, *iter);
    *iter = ele;
    if (! ele) {
        return NULL;
    }

    loader = ele->pData;

    return &loader->param;
}

