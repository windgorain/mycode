/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/elf_utl.h"
#include "../h/ulc_def.h"
#include "../h/ulc_map.h"
#include "../h/ulc_prog.h"
#include "../h/ulc_fd.h"
#include "../h/ulc_hookpoint.h"
#include "../h/ulc_osbase.h"
#include "../h/ulc_verifier.h"

#define ULC_LOADER_MAX_MAPS 32

enum {
    ULC_LOAD_TYPE_XDP = 0,
};

typedef struct {
    int type;
    char *filename;
    char *sec_name;
    ELF_S *elf;
    int map_count;
    int map_fd[ULC_LOADER_MAX_MAPS];
}ULC_LOADER_S;

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

    if ((loader->sec_name) && (strcmp(loader->sec_name, sec->shname) == 0)) {
        return TRUE;
    }

    if ((loader->type == ULC_LOAD_TYPE_XDP) && (strncmp("xdp/", sec->shname, 4) == 0)) {
        return TRUE;
    }

    return FALSE;
}

static int _ulcloader_load_prog(ULC_LOADER_S *loader)
{
    void *iter = NULL;
    ELF_SECTION_S sec;
    ELF_S *elf = loader->elf;
    char *name;
    int fd = BS_NO_SUCH;
    ULC_PROG_NODE_S *prog;

    while ((iter = ELF_GetNextSection(elf, iter, &sec))) {
        if (_ulcloader_should_load_prog(loader, &sec)) {
            name = ELF_GetSecSymbolName(elf, sec.sec_id, STT_FUNC, 0);
            prog = ULC_PROG_Alloc(sec.data->d_buf, sec.data->d_size, name);
            if (! prog) {
                RETURN(BS_ERR);
            }
            ULC_VERIFIER_ReplaceMapFdWithMapPtr(prog);
            ULC_VERIFIER_ConvertCtxAccess(prog);
            ULC_VERIFIER_FixupBpfCalls(prog);
            fd = ULC_PROG_Add(prog);
            if (fd < 0) {
                ULC_PROG_Free(prog);
                return fd;
            }
            break;
        }
    }

    return fd;
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
    int fd;

    ret = _ulcloader_load_maps(loader);
    if (ret < 0) {
        return ret;
    }

    _ulcloader_process_relo(loader);

    fd = _ulcloader_load_prog(loader);
    if (fd < 0) {
        return fd;
    }

    ULC_HookPoint_XdpAttach(fd);

    return fd;
}

int ULC_LOADER_LoadXdp(char *filename, char *sec_name)
{
    int ret;
    ELF_S elf;
    ULC_LOADER_S load = {0};

    ret = ELF_Open(filename, &elf);
    if (ret < 0) {
        return -1;
    }

    load.type = ULC_LOAD_TYPE_XDP;
    load.filename = filename;
    load.sec_name = sec_name;
    load.elf = &elf;

    ret = _ulcloader_load_by_file(&load);

    ELF_Close(&elf);

    return ret;
}

