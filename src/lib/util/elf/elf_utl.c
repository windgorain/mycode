/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/elf_utl.h"

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
        RETURN(BS_ERR);
    }

	sec->shname = elf_sym_str(elf, shdr->sh_name);
	if (!sec->shname || !shdr->sh_size) {
        RETURN(BS_ERR);
    }

	sec->data = elf_sec_data(scn);
	if (!sec->data || elf_getdata(scn, sec->data) != NULL) {
        RETURN(BS_ERR);
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

    memset(elf, 0, sizeof(ELF_S));

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

/* 返回 iter, NULL表示结束 */
void * ELF_GetNextSection(ELF_S *elf, void *iter/* NULL表示获取第一个 */, OUT ELF_SECTION_S *sec)
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
    /* sec从1开始, 所以需要减掉1 */
    return elf->ehdr.e_shnum - 1;
}

/* 获取第几个section, id就是section的序号 */
int ELF_GetSecByID(ELF_S *elf, int id, OUT ELF_SECTION_S *sec)
{
    return elf_get_sec_by_id(elf, id, sec);
}

int ELF_GetSecByName(ELF_S *elf, char *sec_name, OUT ELF_SECTION_S *sec)
{
    void *iter = NULL;

    while ((iter = ELF_GetNextSection(elf,iter, sec))) {
        if (strcmp(sec->shname, sec_name) == 0) {
            return 0;
        }
    }

    return -1;
}

/* 返回对应sec的id */
int ELF_GetSecIDByName(ELF_S *elf, char *sec_name)
{
    ELF_SECTION_S sec;

    if (ELF_GetSecByName(elf, sec_name, &sec) < 0) {
        return -1;
    }

    return sec.sec_id;
}

/* 获取指定SecID中指定类型的symbols数目, 如果type为0则忽略type */
int ELF_SecSymbolCount(ELF_S *elf, int sec_id, int type)
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

		count ++;
	}

    return count;
}

/* 根据id获取symbol */
Elf64_Sym * ELF_GetSymbol(ELF_S *elf, int id)
{
	Elf_Data *symbols = elf->symbols;

    return elf_sym_by_id(symbols, id);
}

/* 获取指定Sec中的symbol. index表示是第几个section中匹配的symbol的索引, type为0则表示忽略 */
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

/* 通过指定的重定位索引获取重定位项 */
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

