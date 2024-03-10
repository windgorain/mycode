/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/elf_utl.h"
#include "utl/qsort_utl.h"

#define DATA_SEC ".data"
#define BSS_SEC ".bss"
#define RODATA_SEC ".rodata"
#define KCONFIG_SEC ".kconfig"
#define KSYMS_SEC ".ksyms"
#define STRUCT_OPS_SEC ".struct_ops"

static Elf_Data *elf_sec_data(Elf_Scn *scn)
{
	Elf_Data *data;

	if (!scn)
		return NULL;

	data = elf_getdata(scn, 0);
	if (!data) {
		return NULL;
	}

	return data;
}

static char * elf_sym_str(ELF_S *elf, int off)
{
	return elf_strptr(elf->elf_info, elf->ehdr.e_shstrndx, off);
}

static int elf_get_sec_by_scn(ELF_S *elf, Elf_Scn *scn, OUT ELF_SECTION_S *sec)
{
    GElf_Shdr *shdr = &sec->shdr;

	sec->sec_id = elf_ndxscn(scn);

	if (gelf_getshdr(scn, shdr) != shdr) {
        return -1;
    }

	sec->shname = elf_sym_str(elf, shdr->sh_name);
	if (!sec->shname || !shdr->sh_size) {
        return -1;
    }

	sec->data = elf_sec_data(scn);
	if (!sec->data || elf_getdata(scn, sec->data) != NULL) {
        return -1;
    }

	return 0;
}

static int elf_get_sec_by_id(ELF_S *elf, int id, OUT ELF_SECTION_S *sec)
{
	Elf_Scn *scn;

	scn = elf_getscn(elf->elf_info, id);
	if (!scn) {
        RETURN(BS_ERR);
    }

    return elf_get_sec_by_scn(elf, scn, sec);
}

static Elf64_Shdr *elf_sec_hdr(Elf_Scn *scn)
{
	Elf64_Shdr *shdr;

	if (!scn)
		return NULL;

	shdr = elf64_getshdr(scn);
	if (!shdr) {
		return NULL;
	}

	return shdr;
}

static int elf_get_symbols(ELF_S *elf)
{
	Elf_Scn *scn = NULL;
	int idx;
	Elf64_Shdr *sh;
	Elf_Data *data;

	while ((scn = elf_nextscn(elf->elf_info, scn)) != NULL) {
		sh = elf_sec_hdr(scn);
		if (!sh) {
			return -1;
        }

		if (sh->sh_type == SHT_SYMTAB) {
			if (elf->symbols) {
                RETURN(BS_ALREADY_EXIST);
			}

			data = elf_sec_data(scn);
			if (!data) {
				return -1;
            }

			idx = elf_ndxscn(scn);

			elf->symbols = data;
			elf->symbols_shndx = idx;
			elf->strtabidx = sh->sh_link;
		}
	}

    return 0;
}

static int elf_get_elf_info(ELF_S *elf)
{
	elf->elf_info = elf_begin(elf->fd, ELF_C_READ, NULL);
    if (! elf->elf_info) {
        RETURN(BS_ERR);
    }

	if (gelf_getehdr(elf->elf_info, &elf->ehdr) != &elf->ehdr) {
        RETURN(BS_ERR);
    }

    return elf_get_symbols(elf);
}

static Elf64_Sym *elf_sym_by_id(Elf_Data *symbols, int sym_id)
{
    int count = symbols->d_size / sizeof(Elf64_Sym);

    if (sym_id >= count) {
        return NULL;
    }

	return (Elf64_Sym *)symbols->d_buf + sym_id;
}

int ELF_Open(char *path, OUT ELF_S *elf)
{
    int ret;

    BS_DBGASSERT(elf);
    BS_DBGASSERT(path);

    if (elf_version(EV_CURRENT) == EV_NONE) {
        RETURN(BS_NOT_READY);
    }

    elf->fd = open(path, O_RDONLY, 0);

    if (elf->fd < 0) {
        RETURN(BS_CAN_NOT_OPEN);
    }

    ret = elf_get_elf_info(elf);
    if (ret < 0) {
        close(elf->fd);
        elf->fd = -1;
        return ret;
    }

    return 0;
}

void ELF_Close(ELF_S *elf)
{
    if (elf->elf_info) {
        elf_end(elf->elf_info);
        elf->elf_info = NULL;
    }
    if (elf->fd >= 0) {
        close(elf->fd);
        elf->fd = -1;
    }
}


void * ELF_GetNextSection(ELF_S *elf, void *iter, OUT ELF_SECTION_S *sec)
{
	Elf_Scn *scn = iter;

    while ((scn = elf_nextscn(elf->elf_info, scn))) {
        if (elf_get_sec_by_scn(elf, scn, sec) == 0) {
            break;
        }
    }

    return scn;
}

int ELF_SecCount(ELF_S *elf)
{
    
    return elf->ehdr.e_shnum - 1;
}


int ELF_GetSecByID(ELF_S *elf, int sec_id, OUT ELF_SECTION_S *sec)
{
    return elf_get_sec_by_id(elf, sec_id, sec);
}

int ELF_GetSecByName(ELF_S *elf, char *sec_name, OUT ELF_SECTION_S *sec)
{
    void *iter = NULL;

    while ((iter = ELF_GetNextSection(elf,iter, sec))) {
        if (strcmp(sec->shname, sec_name) == 0) {
            return 0;
        }
    }

    sec->shname = NULL;
    sec->data = NULL;

    return -1;
}


int ELF_GetSecIDByName(ELF_S *elf, char *sec_name)
{
    ELF_SECTION_S sec;

    if (ELF_GetSecByName(elf, sec_name, &sec) < 0) {
        return -1;
    }

    return sec.sec_id;
}


int ELF_SecSymbolCount(ELF_S *elf, int sec_id, int type)
{
	int i, count = 0;
	Elf_Data *symbols = elf->symbols;

    if (! symbols) {
        return 0;
    }

	int nr_syms = symbols->d_size / sizeof(Elf64_Sym);

	for (i = 0; i < nr_syms; i++) {
		Elf64_Sym *sym = elf_sym_by_id(symbols, i);

		if (sym->st_shndx != sec_id) {
			continue;
        }

		if (ELF64_ST_TYPE(sym->st_info) == STT_SECTION) {
			continue;
        }

        if ((type) && (ELF64_ST_TYPE(sym->st_info) != type)) {
            continue;
        }

		count ++;
	}

    return count;
}


Elf64_Sym * ELF_GetSymbol(ELF_S *elf, int id)
{
	Elf_Data *symbols = elf->symbols;

    return elf_sym_by_id(symbols, id);
}


Elf64_Sym * ELF_GetSecSymbol(ELF_S *elf, int sec_id, int type, int index)
{
	int i, count = 0;
	Elf_Data *symbols = elf->symbols;

	int nr_syms = symbols->d_size / sizeof(Elf64_Sym);

	for (i = 0; i < nr_syms; i++) {
		Elf64_Sym *sym = elf_sym_by_id(symbols, i);

		if (sym->st_shndx != sec_id) {
			continue;
        }

		if (ELF64_ST_TYPE(sym->st_info) == STT_SECTION) {
			continue;
        }

        if ((type) && (ELF64_ST_TYPE(sym->st_info) != type)) {
            continue;
        }

        if (count == index) {
            return sym;
        }

		count ++;
	}

    return NULL;
}

char * ELF_GetSecSymbolName(ELF_S *elf, int sec_id, int type, int index)
{
    Elf64_Sym *sym = ELF_GetSecSymbol(elf, sec_id, type, index);
    if (! sym) {
        return NULL;
    }
    return ELF_GetSymbolName(elf, sym);
}

char * ELF_GetSymbolName(ELF_S *elf, Elf64_Sym * sym)
{
    return elf_sym_str(elf, sym->st_name);
}


GElf_Rel * ELF_GetRel(Elf_Data *relo_data, int id, OUT GElf_Rel *rel)
{
    return gelf_getrel(relo_data, id, rel);
}

BOOL_T ELF_IsProgSection(ELF_SECTION_S *sec)
{
    if ((sec->shdr.sh_type != SHT_PROGBITS) || !(sec->shdr.sh_flags & SHF_EXECINSTR)) {
        return FALSE;
    }

    return TRUE;
}

BOOL_T ELF_IsDataSection(ELF_SECTION_S *sec)
{
    if (sec->shdr.sh_type != SHT_PROGBITS) {
        return FALSE;
    }

    if (sec->shdr.sh_flags & SHF_EXECINSTR) {
        return FALSE;
    }

    if (strcmp(sec->shname, DATA_SEC) != 0) {
        return FALSE;
    }

    return TRUE;
}

BOOL_T ELF_IsRoDataSection(ELF_SECTION_S *sec)
{
    if (sec->shdr.sh_type != SHT_PROGBITS) {
        return FALSE;
    }

    if (sec->shdr.sh_flags & SHF_EXECINSTR) {
        return FALSE;
    }

    if (strncmp(sec->shname, RODATA_SEC, STR_LEN(RODATA_SEC)) != 0) {
        return FALSE;
    }

    return TRUE;
}

BOOL_T ELF_IsBssSection(ELF_SECTION_S *sec)
{
    if ((sec->shdr.sh_type == SHT_NOBITS) && (strcmp(sec->shname, BSS_SEC) == 0)) {
        return TRUE;
    }
    return FALSE;
}


int ELF_GetGlobalData(ELF_S *elf, OUT ELF_GLOBAL_DATA_S *global_data)
{
    void *iter = NULL;
    ELF_SECTION_S the_sec;
    int count = 0;

    global_data->have_bss = 0;
    global_data->have_data = 0;
    global_data->rodata_count = 0;

    while ((iter = ELF_GetNextSection(elf, iter, &the_sec))) {
        if (ELF_IsBssSection(&the_sec)) {
            global_data->bss_sec = the_sec;
            global_data->have_bss = 1;
            count ++;
        } else if (ELF_IsDataSection(&the_sec)) {
            global_data->data_sec = the_sec;
            global_data->have_data = 1;
            count ++;
        } else if (ELF_IsRoDataSection(&the_sec)) {
            global_data->rodata_sec[global_data->rodata_count] = the_sec;
            global_data->rodata_count ++;
            count ++;
        }
    }
    
    global_data->sec_count = count;

    return count;
}

int ELF_GetProgsCount(ELF_S *elf)
{
    void *iter = NULL;
    ELF_SECTION_S sec;
    int count = 0;

    while ((iter = ELF_GetNextSection(elf, iter, &sec))) {
        if (! ELF_IsProgSection(&sec)) {
            continue;
        }
        count += ELF_SecSymbolCount(elf, sec.sec_id, STT_FUNC);
    }

    return count;
}


int ELF_GetProgsSize(ELF_S *elf)
{
    void *iter = NULL;
    ELF_SECTION_S sec;
    int size = 0;

    while ((iter = ELF_GetNextSection(elf, iter, &sec))) {
        if (! ELF_IsProgSection(&sec)) {
            continue;
        }
        size += sec.data->d_size;
    }

    return size;
}


int ELF_CopyProgs(ELF_S *elf, OUT void *mem, int mem_size)
{
    ELF_SECTION_S sec;
    void *iter = NULL;
    int func_size;
    uint32_t offset = 0;
    char *out = mem;

    while ((iter = ELF_GetNextSection(elf, iter, &sec))) {
        if (! ELF_IsProgSection(&sec)) {
            continue;
        }

        func_size = sec.data->d_size;
        if (offset + func_size > mem_size) {
            RETURN(BS_OUT_OF_RANGE);
        }

        memcpy(out + offset, sec.data->d_buf, func_size);
        offset += func_size;
    }

    return offset;
}


void * ELF_DupProgs(ELF_S *elf)
{
    int size = ELF_GetProgsSize(elf);

    if (size <= 0) {
        return NULL;
    }

    void *mem = MEM_Malloc(size);
    if (! mem) {
        return NULL;
    }

    ELF_CopyProgs(elf, mem, size);

    return mem;
}

static int _elf_prog_info_cmp(const void *n1, const void *n2)
{
    const ELF_PROG_INFO_S *p1 = n1;
    const ELF_PROG_INFO_S *p2 = n2;

    return (int)p1->prog_offset - (int)p2->prog_offset;
}

static void _elf_sort_progs_info(ELF_PROG_INFO_S *progs, int prog_count)
{
    QSORT_Do(progs, prog_count, sizeof(ELF_PROG_INFO_S), _elf_prog_info_cmp);
}

static int _elf_get_progs_info(ELF_S *elf, OUT ELF_PROG_INFO_S *progs, int max_prog_count)
{
    ELF_SECTION_S sec;
    void *iter = NULL;
    char *func_name;
    int count = 0;
    int i;
    int prog_sec_off = 0;

    while ((iter = ELF_GetNextSection(elf, iter, &sec))) {
        if (! ELF_IsProgSection(&sec)) {
            continue;
        }

        int func_count = ELF_SecSymbolCount(elf, sec.sec_id, STT_FUNC);
        for (i=0; i<func_count; i++) {
            if (count >= max_prog_count) {
                return count;
            }
            func_name = ELF_GetSecSymbolName(elf, sec.sec_id, STT_FUNC, i);
            Elf64_Sym *sym = ELF_GetSecSymbol(elf, sec.sec_id, STT_FUNC, i);
            progs[count].sec_offset = prog_sec_off;
            progs[count].prog_offset = sym->st_value + prog_sec_off;
            progs[count].size = sym->st_size;
            progs[count].sec_name = sec.shname;
            progs[count].func_name = func_name;
            progs[count].sec_id = sec.sec_id;
            count++;
        }

        prog_sec_off += sec.data->d_size;
    }

    return count;
}

int ELF_GetProgsInfo(ELF_S *elf, OUT ELF_PROG_INFO_S *progs, int max_prog_count)
{
    int count = _elf_get_progs_info(elf, progs, max_prog_count);
    _elf_sort_progs_info(progs, count);
    return count;
}

int ELF_GetSecProgsInfoCount(ELF_PROG_INFO_S *info, int prog_count, char *sec_name)
{
    int i;
    int count = 0;

    for (i=0; i<prog_count; i++) {
        if (strcmp(info[i].sec_name, sec_name) == 0) {
            count ++;
        }
    }

    return count;
}

int ELF_WalkProgs(ELF_S *elf, PF_ELF_WALK_PROG walk_func, void *ud)
{
    ELF_SECTION_S sec;
    ELF_PROG_INFO_S info = {0};
    void *iter = NULL;
    char *func_name;
    int ret = 0;
    int i;
    int sec_offset = 0;

    while ((iter = ELF_GetNextSection(elf, iter, &sec))) {
        if (! ELF_IsProgSection(&sec)) {
            continue;
        }

        int func_count = ELF_SecSymbolCount(elf, sec.sec_id, STT_FUNC);
        for (i=0; i<func_count; i++) {
            func_name = ELF_GetSecSymbolName(elf, sec.sec_id, STT_FUNC, i);
            Elf64_Sym *sym = ELF_GetSecSymbol(elf, sec.sec_id, STT_FUNC, i);

            info.sec_offset = sec_offset;
            info.prog_offset = sec_offset + sym->st_value;
            info.size = sym->st_size;
            info.sec_name = sec.shname;
            info.func_name = func_name;
            info.sec_id = sec.sec_id;

            ret = walk_func(sec.data->d_buf + sym->st_value, &info, ud);
            if (ret < 0) {
                break;
            }
        }

        sec_offset += sec.data->d_size;
    }

    return ret;
}

