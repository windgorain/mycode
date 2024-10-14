/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0
* Description:
******************************************************************************/
#include "bs.h"
#include "utl/elf_utl.h"
#include "utl/elf_lib.h"
#include "utl/qsort_utl.h"
#include "utl/ldata_utl.h"
#include "utl/ulc_user.h"


static Elf64_Shdr * _elflib_get_sec_hdr_by_id(LLDATA_S *d, U32 id)
{
    Elf64_Ehdr *ehdr = (void*)d->data;
    int section_count = ehdr->e_shnum;

    if (id >= section_count) {
        return NULL;
    }

    U64 off = ehdr->e_shoff + id * ehdr->e_shentsize;

    return LLDATA_BoundsCheck(d, off, sizeof(Elf64_Shdr));
}


static Elf64_Shdr * _elflib_get_strtab_sec(LLDATA_S *d)
{
    Elf64_Ehdr *ehdr = (void*)d->data;
    return _elflib_get_sec_hdr_by_id(d, ehdr->e_shstrndx);
}

static char * _elflib_get_str(LLDATA_S *d, U64 off)
{
    Elf64_Shdr *sec_hdr = _elflib_get_strtab_sec(d);
    if (!sec_hdr) {
        return NULL;
    }

    if (off >= sec_hdr->sh_size) {
        return NULL;
    }

    char *data = LLDATA_BoundsCheck(d, sec_hdr->sh_offset, sec_hdr->sh_size);

    return data + off;
}

static Elf64_Shdr * _elflib_get_next_section(LLDATA_S *d, Elf64_Shdr *cur)
{
    Elf64_Ehdr *ehdr = (void*)d->data;
    int section_count = ehdr->e_shnum;
    U64 off;

    if (cur) {
        off = (char*)cur - (char*)d->data; 
        off += ehdr->e_shentsize; 
    } else {
        off = ehdr->e_shoff; 
    }

    if ((off - ehdr->e_shoff) >= (section_count * ehdr->e_shentsize)) {
        return NULL;
    }

    Elf64_Shdr *shdr = LLDATA_BoundsCheck(d, off, sizeof(Elf64_Shdr));

    return shdr;
}


static char * _elflib_get_strtab_str(LLDATA_S *d, Elf64_Shdr *strtab_shdr, U64 off)
{
    if (! strtab_shdr) {
        return NULL;
    }

    if (off >= strtab_shdr->sh_size) {
        return NULL;
    }

    char *data = ELFLIB_GetSecData(d, strtab_shdr);
    if (! data) {
        return NULL;
    }

    return data + off;
}

static int _elflib_get_func_info(LLDATA_S *d, OUT ELF_PROG_INFO_S *progs, int max_prog_count)
{
    Elf64_Shdr *shdr = NULL;
    char *func_name;
    int count = 0;
    int i;
    int prog_sec_off = 0;

    while ((shdr = ELFLIB_GetNextSec(d, shdr))) {
        if (! ELFLIB_IsProgSection(shdr)) {
            continue;
        }

        int sec_id = ELFLIB_GetSecID(d, shdr);
        char *sec_name = ELFLIB_GetSecName(d, shdr);
        int func_count = ELFLIB_SecSymbolCount(d, sec_id, STT_FUNC);

        for (i=0; i<func_count; i++) {
            if (count >= max_prog_count) {
                return count;
            }
            func_name = ELFLIB_GetSecSymNameByID(d, sec_id, STT_FUNC, i);
            Elf64_Sym *sym = ELFLIB_GetSecSymByID(d, sec_id, STT_FUNC, i);
            progs[count].sec_offset = prog_sec_off;
            progs[count].func_offset = sym->st_value + prog_sec_off;
            progs[count].func_size = sym->st_size;
            progs[count].sec_name = sec_name;
            progs[count].func_name = func_name;
            progs[count].sec_id = sec_id;
            count++;
        }

        prog_sec_off += shdr->sh_size;
    }

    return count;
}

static int _elflib_func_info_cmp(const void *n1, const void *n2)
{
    const ELF_PROG_INFO_S *p1 = n1;
    const ELF_PROG_INFO_S *p2 = n2;

    return (int)p1->func_offset - (int)p2->func_offset;
}

static void _elflib_sort_funcs_info(ELF_PROG_INFO_S *funcs, int func_count)
{
    QSORT_Do(funcs, func_count, sizeof(ELF_PROG_INFO_S), _elflib_func_info_cmp);
}

int ELFLIB_CheckHeader(LLDATA_S *d)
{
    Elf64_Ehdr *ehdr = LLDATA_BoundsCheck(d, 0, sizeof(*ehdr));
    if (! ehdr) {
        RETURNI(BS_TOO_SMALL, "too small");
    }

    char TXT_STRING_ARRAY(magic_name, ELFMAG);
    if (memcmp(ehdr->e_ident, magic_name, SELFMAG)) {
        RETURNI(BS_ERR, "wrong magic");
    }

    if (ehdr->e_ident[EI_CLASS] != ELFCLASS64) {
        RETURNI(BS_ERR, "wrong class");
    }

    if (ehdr->e_ident[EI_DATA] != ELFDATA2LSB) {
        RETURNI(BS_ERR, "wrong byte order");
    }

    if (ehdr->e_ident[EI_VERSION] != 1) {
        RETURNI(BS_ERR, "wrong version");
    }

    if (ehdr->e_ident[EI_OSABI] != ELFOSABI_NONE) {
        RETURNI(BS_ERR, "wrong OS ABI");
    }

    if (ehdr->e_type != ET_REL) {
        RETURNI(BS_ERR, "wrong type, expected relocatable");
    }

    

    if (ehdr->e_shnum > ELF_MAX_SECTIONS) {
        RETURNI(BS_ERR, "too many sections");
    }

    return 0;
}

Elf64_Ehdr * ELFLIB_GetHeader(LLDATA_S *d)
{
    if (ELFLIB_CheckHeader(d) < 0) {
        return NULL;
    }

    return (void*)d->data;
}


int ELFLIB_CountSection(LLDATA_S *d)
{
    Elf64_Ehdr *ehdr = (void*)d->data;
    return ehdr->e_shnum;
}


Elf64_Shdr * ELFLIB_GetSecByID(LLDATA_S *d, U32 id)
{
    if (! id) {
        return NULL;
    }
    return _elflib_get_sec_hdr_by_id(d, id);
}


int ELFLIB_GetSecID(LLDATA_S *d, Elf64_Shdr *shdr)
{
    Elf64_Ehdr *ehdr = (void*)d->data;
    char *shdr_1st = (char*)d->data + ehdr->e_shoff;
    return (U32)((char*)shdr - shdr_1st) / (U32)ehdr->e_shentsize;
}


int ELFLIB_GetSecIDByName(LLDATA_S *d, char *sec_name)
{
    Elf64_Shdr *shdr = ELFLIB_GetSecByName(d, sec_name);
    if (! shdr) {
        return -1;
    }
    return ELFLIB_GetSecID(d, shdr);
}

void * ELFLIB_GetSecData(LLDATA_S *d, Elf64_Shdr *shdr)
{
    if (! shdr) {
        return NULL;
    }
    return LLDATA_BoundsCheck(d, shdr->sh_offset, shdr->sh_size);
}


Elf64_Shdr * ELFLIB_GetNextSec(LLDATA_S *d, Elf64_Shdr *cur)
{
    return _elflib_get_next_section(d, cur);
}


Elf64_Shdr * ELFLIB_GetNextTypeSec(LLDATA_S *d, U32 type, Elf64_Shdr *cur)
{
    Elf64_Shdr *shdr = cur;

    while ((shdr = ELFLIB_GetNextSec(d, shdr))) {
        if (shdr->sh_type == type) {
            return shdr;
        }
    }

    return NULL;
}


int ELFLIB_GetTypeSecCount(LLDATA_S *d, U32 type)
{
    int count = 0;
    Elf64_Shdr *shdr = NULL;

    while ((shdr = ELFLIB_GetNextTypeSec(d, type, shdr))) {
        count ++;
    }

    return count;
}


Elf64_Shdr * ELFLIB_GetTypeSec(LLDATA_S *d, U32 type, int index)
{
    int type_index = 0;
    Elf64_Shdr *shdr = NULL;

    while ((shdr = ELFLIB_GetNextTypeSec(d, type, shdr))) {
        if (type_index == index) {
            return shdr;
        }
        type_index ++;
    }

    return NULL;
}


Elf64_Sym * ELFLIB_GetSymByRel(LLDATA_S *d, Elf64_Rel *rel)
{
    U32 sym_id = ELF64_R_SYM(rel->r_info);
    return ELFLIB_GetSymByID(d, sym_id);
}


Elf64_Shdr * ELFLIB_GetTargetSecByRel(LLDATA_S *d, Elf64_Rel *rel)
{
    return ELFLIB_GetSecBySym(d, ELFLIB_GetSymByRel(d, rel));
}


Elf64_Shdr * ELFLIB_GetRelSecByMdySecID(LLDATA_S *d, int mdy_sec_id)
{
    Elf64_Shdr *shdr = NULL;

    while ((shdr = ELFLIB_GetNextTypeSec(d, SHT_REL, shdr))) {
        if (shdr->sh_info == mdy_sec_id) {
            return shdr;
        }
    }

    return NULL;
}


Elf64_Rel * ELFLIB_GetRelByMdy(LLDATA_S *d, int mdy_sec_id, U64 mdy_offset)
{
    Elf64_Shdr *rel_shdr = NULL;

    while ((rel_shdr = ELFLIB_GetNextTypeSec(d, SHT_REL, rel_shdr))) {
        if (rel_shdr->sh_info != mdy_sec_id) {
            continue;
        }
        Elf64_Rel *rs = ELFLIB_GetSecData(d, rel_shdr);
        if (! rs) {
            continue;
        }
        int nrels = rel_shdr->sh_size / rel_shdr->sh_entsize;
        for (int i = 0; i < nrels; i++) {
            if (rs[i].r_offset == mdy_offset) {
                return &rs[i];
            }
        }
    }

    return NULL;
}


Elf64_Rel * ELFLIB_GetRelByMdyRange(LLDATA_S *d, int mdy_sec_id, U64 mdy_begin, U64 mdy_end)
{
    Elf64_Shdr *rel_shdr = NULL;

    while ((rel_shdr = ELFLIB_GetNextTypeSec(d, SHT_REL, rel_shdr))) {
        if (rel_shdr->sh_info != mdy_sec_id) {
            continue;
        }
        Elf64_Rel *rs = ELFLIB_GetSecData(d, rel_shdr);
        if (! rs) {
            continue;
        }
        int nrels = rel_shdr->sh_size / rel_shdr->sh_entsize;
        for (int i = 0; i < nrels; i++) {
            if (rs[i].r_info == 0) {
                continue;
            }
            if ((rs[i].r_offset >= mdy_begin) && (rs[i].r_offset < mdy_end)){
                return &rs[i];
            }
        }
    }

    return NULL;
}


Elf64_Rel * ELFLIB_GetRelByTarget(LLDATA_S *d, int target_sec_id, U64 target_offset)
{
    Elf64_Shdr *rel_shdr = NULL;

    while ((rel_shdr = ELFLIB_GetNextTypeSec(d, SHT_REL, rel_shdr))) {
        Elf64_Rel *rs = ELFLIB_GetSecData(d, rel_shdr);
        if (! rs) {
            continue;
        }
        int nrels = rel_shdr->sh_size / rel_shdr->sh_entsize;
        for (int i = 0; i < nrels; i++) {
            Elf64_Sym *sym = ELFLIB_GetSymByRel(d, &rs[i]);
            if (! sym) {
                continue;
            }
            if ((sym->st_shndx == target_sec_id) && (sym->st_value == target_offset)) {
                return &rs[i];
            }
        }
    }

    return NULL;
}


Elf64_Rel * ELFLIB_GetRelBySymID(LLDATA_S *d, U32 sym_id)
{
    Elf64_Shdr *rel_shdr = NULL;
    char TXT_STRING_ARRAY(debug_name, ".rel.debug");
    char TXT_STRING_ARRAY(btf_name, ".rel.BTF");

    while ((rel_shdr = ELFLIB_GetNextTypeSec(d, SHT_REL, rel_shdr))) {
        char *sec_name = ELFLIB_GetSecName(d, rel_shdr);
        if (! sec_name) {
            continue;
        }

        
        if (TXT_STRNCMP(sec_name, debug_name) == 0) {
            continue;
        }

        
        if (TXT_STRNCMP(sec_name, btf_name) == 0) {
            continue;
        }

        Elf64_Rel *rs = ELFLIB_GetSecData(d, rel_shdr);
        if (! rs) {
            continue;
        }
        int nrels = rel_shdr->sh_size / rel_shdr->sh_entsize;
        for (int i = 0; i < nrels; i++) {
            U32 id = ELF64_R_SYM(rs[i].r_info);
            if (id == sym_id) {
                return &rs[i];
            }
        }
    }

    return NULL;
}


Elf64_Shdr * ELFLIB_GetMdySecByRelSec(LLDATA_S *d, Elf64_Shdr *rel_shdr)
{
    return ELFLIB_GetSecByID(d, rel_shdr->sh_info);
}


Elf64_Shdr * ELFLIB_GetStrtabSec(LLDATA_S *d)
{
    return _elflib_get_strtab_sec(d);
}


Elf64_Shdr * ELFLIB_GetSymSec(LLDATA_S *d)
{
    return ELFLIB_GetTypeSec(d, SHT_SYMTAB, 0);
}


int ELFLIB_GetSymCount(Elf64_Shdr *sym_shdr)
{
    return sym_shdr->sh_size/sizeof(Elf64_Sym);
}


char * ELFLIB_GetSymName(LLDATA_S *d, Elf64_Sym *sym)
{
    
    if (ELF64_ST_TYPE(sym->st_info) == STT_SECTION) {
        return ELFLIB_GetSecName(d, ELFLIB_GetSecByID(d, sym->st_shndx));
    }
    Elf64_Shdr *str_shdr = ELFLIB_GetStrtabSec(d);
    return _elflib_get_strtab_str(d, str_shdr, sym->st_name);
}

int ELFLIB_GetSymID(LLDATA_S *d, Elf64_Sym *sym)
{
    Elf64_Shdr *sym_shdr = ELFLIB_GetSymSec(d);
    Elf64_Sym *syms = ELFLIB_GetSecData(d, sym_shdr);
    if (! syms) {
        return -1;
    }
    return ((U64)sym - (U64)syms)/sizeof(*sym);
}


Elf64_Sym * ELFLIB_GetSymByID(LLDATA_S *d, U32 id)
{
    Elf64_Shdr *sym_shdr = ELFLIB_GetSymSec(d);
    Elf64_Sym *syms = ELFLIB_GetSecData(d, sym_shdr);

    if (! syms) {
        return NULL;
    }

    if (id >= sym_shdr->sh_size/sizeof(Elf64_Sym)) {
        return NULL;
    }

    return &syms[id];
}


Elf64_Sym * ELFLIB_GetSymByName(LLDATA_S *d, char *sym_name)
{
    Elf64_Shdr *str_shdr = ELFLIB_GetStrtabSec(d);
    Elf64_Shdr *sym_shdr = ELFLIB_GetSymSec(d);
    Elf64_Sym *syms = ELFLIB_GetSecData(d, sym_shdr);

    if ((!str_shdr) || (! syms)) {
        return NULL;
    }

    for (int i=0; i<sym_shdr->sh_size/sizeof(Elf64_Sym); i++) {
       char *name =  _elflib_get_strtab_str(d, str_shdr, syms[i].st_name);
       if (! name) {
           continue;
       }
       if (strcmp(name, sym_name) == 0) {
           return &syms[i];
       }
    }
    
    return NULL;
}


Elf64_Sym * ELFLIB_GetLocalSymByName(LLDATA_S *d, char *sym_name)
{
    Elf64_Shdr *str_shdr = ELFLIB_GetStrtabSec(d);
    Elf64_Shdr *sym_shdr = ELFLIB_GetSymSec(d);
    Elf64_Sym *syms = ELFLIB_GetSecData(d, sym_shdr);

    if ((!str_shdr) || (! syms)) {
        return NULL;
    }

    for (int i=0; i<sym_shdr->sh_size/sizeof(Elf64_Sym); i++) {
       if (! syms[i].st_shndx) { 
           continue;
       }
       char *name =  _elflib_get_strtab_str(d, str_shdr, syms[i].st_name);
       if (! name) {
           continue;
       }
       if (strcmp(name, sym_name) == 0) {
           return &syms[i];
       }
    }
    
    return NULL;
}


Elf64_Shdr * ELFLIB_GetSecBySym(LLDATA_S *d, Elf64_Sym *sym)
{
    if (! sym) {
        return NULL;
    }
    return ELFLIB_GetSecByID(d, sym->st_shndx);
}


char * ELFLIB_GetStr(LLDATA_S *d, U64 off)
{
    return _elflib_get_str(d, off);
}


char * ELFLIB_GetSecName(LLDATA_S *d, Elf64_Shdr *shdr)
{
    if (! shdr) {
        return NULL;
    }
    return _elflib_get_str(d, shdr->sh_name);
}


Elf64_Shdr * ELFLIB_GetSecByName(LLDATA_S *d, char *sec_name)
{
    if (! sec_name) {
        return NULL;
    }

    int sec_count = ELFLIB_CountSection(d);
    
    for (int i=0; i<sec_count; i++) {
        Elf64_Shdr *shdr = ELFLIB_GetSecByID(d, i);
        if (! shdr) {
            continue;
        }

        char *name = ELFLIB_GetSecName(d, shdr);
        if (! name) {
            continue;
        }

        if (strcmp(name, sec_name) == 0) {
            return shdr;
        }
    }

    return NULL;
}


BOOL_T ELFLIB_IsProgSection(Elf64_Shdr *shdr)
{
    if ((shdr->sh_type != SHT_PROGBITS) || !(shdr->sh_flags & SHF_EXECINSTR)) {
        return FALSE;
    }

    return TRUE;
}

BOOL_T ELFLIB_IsTextSection(LLDATA_S *elf, Elf64_Shdr *shdr)
{
    char *name = ELFLIB_GetSecName(elf, shdr);

    if (! name) {
        return FALSE;
    }

    if (TXT_CONST_STRCMP(name, ".text") != 0) {
        return FALSE;
    }

    return TRUE;
}

BOOL_T ELFLIB_IsTextSecID(LLDATA_S *elf, U32 sec_id)
{
    Elf64_Shdr *shdr = ELFLIB_GetSecByID(elf, sec_id);
    return ELFLIB_IsTextSection(elf, shdr);
}

BOOL_T ELFLIB_IsBssSection(LLDATA_S *d, Elf64_Shdr *shdr)
{
    if (shdr->sh_type != SHT_NOBITS) {
        return FALSE;
    }

    char *name = ELFLIB_GetSecName(d, shdr);
    if (! name) {
        return FALSE;
    }

    if (TXT_CONST_STRCMP(name, ".bss") != 0) {
        return FALSE;
    }

    return TRUE;
}

BOOL_T ELFLIB_IsDataSection(LLDATA_S *d, Elf64_Shdr *shdr)
{
    if (shdr->sh_type != SHT_PROGBITS) {
        return FALSE;
    }

    if (shdr->sh_flags & SHF_EXECINSTR) {
        return FALSE;
    }

    char *name = ELFLIB_GetSecName(d, shdr);
    if (! name) {
        return FALSE;
    }

    if (TXT_CONST_STRCMP(name, ".data") != 0) {
        return FALSE;
    }

    return TRUE;
}

BOOL_T ELFLIB_IsRoDataSection(LLDATA_S *d, Elf64_Shdr *shdr)
{
    if (shdr->sh_type != SHT_PROGBITS) {
        return FALSE;
    }

    if (shdr->sh_flags & SHF_EXECINSTR) {
        return FALSE;
    }

    char *name = ELFLIB_GetSecName(d, shdr);
    if (! name) {
        return FALSE;
    }

    char TXT_STRING_ARRAY(rodata_name, ".rodata");

    if (TXT_STRNCMP(name, rodata_name) != 0) {
        return FALSE;
    }

    return TRUE;
}


BOOL_T ELFLIB_IsGlobalDataSection(LLDATA_S *d, Elf64_Shdr *shdr)
{
    if (ELFLIB_IsBssSection(d, shdr) || ELFLIB_IsDataSection(d, shdr) || ELFLIB_IsRoDataSection(d, shdr)) {
        return TRUE;
    }
    return FALSE;
}


int ELFLIB_GetRoDataCount(LLDATA_S *d)
{
    Elf64_Shdr *the_shdr = NULL;
    int count = 0;

    while ((the_shdr = ELFLIB_GetNextSec(d, the_shdr))) {
        if (! ELFLIB_IsRoDataSection(d, the_shdr)) {
            continue;
        }
        count ++;
    }

    return count;
}


int ELFLIB_GetRoDataID(LLDATA_S *d, Elf64_Shdr *shdr)
{
    Elf64_Shdr *the_shdr = NULL;
    int index = 0;

    while ((the_shdr = ELFLIB_GetNextSec(d, the_shdr))) {
        if (! ELFLIB_IsRoDataSection(d, the_shdr)) {
            continue;
        }
        if (shdr == the_shdr) {
            return index;
        }
        index ++;
    }

    return -1;
}


Elf64_Shdr * ELFLIB_GetNextRoDataSec(LLDATA_S *d, Elf64_Shdr *shdr)
{
    Elf64_Shdr *the_shdr = shdr;

    while ((the_shdr = ELFLIB_GetNextSec(d, the_shdr))) {
        if (ELFLIB_IsRoDataSection(d, the_shdr)) {
            return the_shdr;
        }
    }

    return NULL;
}


U64 ELFLIB_GetBssSize(LLDATA_S *d)
{
    Elf64_Shdr * shdr = ELFLIB_GetSecByName(d, ".bss");
    if (! shdr) {
        return 0;
    }
    return shdr->sh_size;
}


int ELFLIB_SecSymbolCount(LLDATA_S *d, int sec_id, int type)
{
	int i, count = 0;
    Elf64_Shdr *sym_shdr = ELFLIB_GetSymSec(d);
    Elf64_Sym *symbols = ELFLIB_GetSecData(d, sym_shdr);

    if (! symbols) {
        return 0;
    }

	int nr_syms = sym_shdr->sh_size / sizeof(Elf64_Sym);

	for (i = 0; i < nr_syms; i++) {
		Elf64_Sym *sym = &symbols[i];

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


Elf64_Sym * ELFLIB_GetSecSymByOffset(LLDATA_S *d, int sec_id, U64 offset)
{
	int i;
    Elf64_Shdr *sym_shdr = ELFLIB_GetSymSec(d);
    Elf64_Sym *symbols = ELFLIB_GetSecData(d, sym_shdr);

    if (! symbols) {
        return NULL;
    }

	int nr_syms = sym_shdr->sh_size / sizeof(Elf64_Sym);

	for (i = 0; i < nr_syms; i++) {
		Elf64_Sym *sym = &symbols[i];
		if (sym->st_shndx != sec_id) {
			continue;
        }
		if (ELF64_ST_TYPE(sym->st_info) == STT_SECTION) {
			continue;
        }
        if (sym->st_value == offset) {
            return sym;
        }
	}

    return NULL;
}


char * ELFLIB_GetSecSymNameByOffset(LLDATA_S *d, int sec_id, U64 offset)
{
    Elf64_Sym *sym = ELFLIB_GetSecSymByOffset(d, sec_id, offset);
    if (! sym) {
        return NULL;
    }

    return ELFLIB_GetSymName(d, sym);
}


Elf64_Sym * ELFLIB_GetSecSymByID(LLDATA_S *d, int sec_id, int type, int index)
{
	int i, count = 0;
    Elf64_Shdr *sym_shdr = ELFLIB_GetSymSec(d);
    Elf64_Sym *symbols = ELFLIB_GetSecData(d, sym_shdr);

    if (! symbols) {
        return NULL;
    }

	int nr_syms = sym_shdr->sh_size / sizeof(Elf64_Sym);

	for (i = 0; i < nr_syms; i++) {
		Elf64_Sym *sym = &symbols[i];
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


char * ELFLIB_GetSecSymNameByID(LLDATA_S *d, int sec_id, int type, int index)
{
    Elf64_Sym *sym = ELFLIB_GetSecSymByID(d, sec_id, type, index);
    if (! sym) {
        return NULL;
    }

    return ELFLIB_GetSymName(d, sym);
}


void * ELFLIB_GetTargetBySym(LLDATA_S *d, Elf64_Sym *sym)
{
    Elf64_Shdr *shdr = ELFLIB_GetSecBySym(d, sym);
    void *data = ELFLIB_GetSecData(d, shdr);
    if (! data) {
        return NULL;
    }

    return (void*)((char*)data + sym->st_value);
}


U64 ELFLIB_GetProgSecOffset(LLDATA_S *d, Elf64_Shdr *shdr)
{
    Elf64_Shdr *the_shdr = NULL;
    U64 offset = 0;

    while ((the_shdr = ELFLIB_GetNextSec(d, the_shdr))) {
        if (! ELFLIB_IsProgSection(the_shdr)) {
            continue;
        }

        if (shdr == the_shdr) {
            return offset;
        }

        offset += the_shdr->sh_size;
    }

    return 0;
}


U64 ELFLIB_GetProgsSize(LLDATA_S *d)
{
    U64 size = 0;
    Elf64_Shdr *shdr = NULL;

    while ((shdr = ELFLIB_GetNextSec(d, shdr))) {
        if (! ELFLIB_IsProgSection(shdr)) {
            continue;
        }
        size += shdr->sh_size;
    }

    return size;
}


int ELFLIB_GetFuncCount(LLDATA_S *d)
{
    Elf64_Shdr *shdr = NULL;
    int count = 0;

    while ((shdr = ELFLIB_GetNextSec(d, shdr))) {
        if (! ELFLIB_IsProgSection(shdr)) {
            continue;
        }
        count += ELFLIB_SecSymbolCount(d, ELFLIB_GetSecID(d, shdr), STT_FUNC);
    }

    return count;
}


int ELFLIB_GetFuncInfo(LLDATA_S *d, OUT ELF_PROG_INFO_S *funcs, int max_prog_count)
{
    int count = _elflib_get_func_info(d, funcs, max_prog_count);
    _elflib_sort_funcs_info(funcs, count);
    return count;
}


void ELFLIB_FixSecID(ELF_SEC_CHANGE_INFO_S *change, int old_sec_id, int new_sec_id, U64 offset)
{
    change[old_sec_id].new_sec_id = new_sec_id;
    change[old_sec_id].offset = offset;
}


void ELFLIB_SetRelLink(LLDATA_S *d, int new_symtab_id)
{
    int sec_count = ELFLIB_CountSection(d);

    for (int i=0; i<sec_count; i++) {
        Elf64_Shdr *shdr = ELFLIB_GetSecByID(d, i);
        if ((! shdr) || (shdr->sh_type != SHT_REL)){
            continue;
        }

        shdr->sh_link = new_symtab_id;
    }
}


void ELFLIB_SetEhdrStrtabID(void *data, U32 sec_id)
{
    Elf64_Ehdr *ehdr = data;
    ehdr->e_shstrndx = sec_id;
}

