/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/elf_utl.h"

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

    if (strcmp(sec->shname, RODATA_SEC) != 0) {
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

/* 获取data sections */
int ELF_GetGlobalData(ELF_S *elf, OUT ELF_GLOBAL_DATA_S *global_data)
{
    void *iter = NULL;
    ELF_SECTION_S the_sec;
    int count = 0;

    global_data->have_bss = 0;
    global_data->have_data = 0;
    global_data->have_rodata = 0;

    while ((iter = ELF_GetNextSection(elf, iter, &the_sec))) {
        if (ELF_IsDataSection(&the_sec)) {
            global_data->data_sec = the_sec;
            global_data->have_data = 1;
            count ++;
        } else if (ELF_IsRoDataSection(&the_sec)) {
            global_data->rodata_sec = the_sec;
            global_data->have_rodata = 1;
            count ++;
        } else if (ELF_IsBssSection(&the_sec)) {
            global_data->bss_sec = the_sec;
            global_data->have_bss = 1;
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

/* 获取ELF中所有的prog的size之和 */
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

/* 把elf中的progs copy到mem; 如果mem_size不够会返回错误,
 成功: 返回copy了多长. 
 失败: return < 0 */
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

/* 申请内存并copy progs */
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

int ELF_GetProgsInfo(ELF_S *elf, OUT ELF_PROG_INFO_S *progs, int max_prog_count)
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
            progs[count].offset = sym->st_value + prog_sec_off;
            progs[count].size = sym->st_size;
            progs[count].sec_name = sec.shname;
            progs[count].func_name = func_name;
            count++;
        }

        prog_sec_off += sec.data->d_size;
    }

    return count;
}

