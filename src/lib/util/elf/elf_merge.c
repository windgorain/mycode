/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0
* Description:
******************************************************************************/
#include "bs.h"
#include "utl/elf_utl.h"
#include "utl/elf_lib.h"
#include "utl/vbuf_utl.h"
#include "utl/ulc_user.h"


static void _elflib_fix_d2_rel_info(LLDATA_S *d1, LLDATA_S *d2)
{
    Elf64_Shdr *shdr1 = ELFLIB_GetSymSec(d1);
    U32 offset = shdr1->sh_size;

    Elf64_Shdr *shdr = NULL;

    while ((shdr = ELFLIB_GetNextTypeSec(d2, SHT_REL, shdr))) {
        Elf64_Rel *rs = ELFLIB_GetSecData(d2, shdr);
        if (! rs) {
            continue;
        }

        int nrels = shdr->sh_size / shdr->sh_entsize;
        for (int i = 0; i < nrels; i++) {
            U32 sym = ELF64_R_SYM(rs[i].r_info);
            U32 type = ELF64_R_TYPE(rs[i].r_info);
            
            rs[i].r_info = ELF64_R_INFO(sym + offset / sizeof(Elf64_Sym), type);
        }
    }
}


static int _elflib_merge_d1_sec(LLDATA_S *d1, LLDATA_S *d2, OUT VBUF_S *vbuf,
        OUT ELF_MDY_SEC_INFO_S *info, OUT ELF_SEC_CHANGE_INFO2_S *changes)
{
    Elf64_Shdr *shdr1, *shdr2;
    int ret = 0;
    int sec_count = ELFLIB_CountSection(d1);

    for (int i=0; i<sec_count; i++) {

        shdr1 = ELFLIB_GetSecByID(d1, i);
        if ((! shdr1)
                || (shdr1->sh_type == SHT_NULL)
                || (shdr1->sh_type == SHT_SYMTAB)
                || (shdr1->sh_type == SHT_REL)) {
            continue;
        }

        if (info->sec_id >= ARRAY_SIZE(info->shdrs)) {
            RETURNI(BS_REACH_MAX, "sec num exceed");
        }

        
        if (shdr1->sh_type == SHT_STRTAB) {
            ELFLIB_SetEhdrStrtabID(VBUF_GetData(vbuf), info->sec_id);
        }

        info->shdrs[info->sec_id] = *shdr1;
        info->shdrs[info->sec_id].sh_offset = VBUF_GetDataLength(vbuf);

        ELFLIB_FixSecID(changes->c1, i, info->sec_id, 0);
        ret |= ELFBUF_Write(vbuf, ELFLIB_GetSecData(d1, shdr1), shdr1->sh_size);

        char *name = ELFLIB_GetSecName(d1, shdr1);
        
        shdr2 = ELFLIB_GetSecByName(d2, name);
        if (shdr2) {
            int old_sec_id = ELFLIB_GetSecID(d2, shdr2);
            ELFLIB_FixSecID(changes->c2, old_sec_id, info->sec_id, shdr1->sh_size);
            ret |= ELFBUF_Write(vbuf, ELFLIB_GetSecData(d2, shdr2), shdr2->sh_size);
            info->shdrs[info->sec_id].sh_size += shdr2->sh_size;
        }

        info->sec_id ++;
    }

    return ret;
}


static int _elflib_merge_d2_sec(LLDATA_S *d1, LLDATA_S *d2, OUT VBUF_S *vbuf,
        OUT ELF_MDY_SEC_INFO_S *info, OUT ELF_SEC_CHANGE_INFO2_S *changes)
{
    Elf64_Shdr *shdr1, *shdr2;
    int ret = 0;
    int sec_count = ELFLIB_CountSection(d2);
    Elf64_Shdr *d1_strtab = ELFLIB_GetStrtabSec(d1);

    for (int i=0; i<sec_count; i++) {
        shdr2 = ELFLIB_GetSecByID(d2, i);
        if ((! shdr2)
                || (shdr2->sh_type == 0)
                || (shdr2->sh_type == SHT_SYMTAB)
                || (shdr2->sh_type == SHT_REL)) {
            continue;
        }

        char *name = ELFLIB_GetSecName(d2, shdr2);
        shdr1 = ELFLIB_GetSecByName(d1, name);
        if (shdr1) { 
            continue;
        }

        if (info->sec_id >= ARRAY_SIZE(info->shdrs)) {
            RETURNI(BS_REACH_MAX, "sec num exceed");
        }

        info->shdrs[info->sec_id] = *shdr2;
        info->shdrs[info->sec_id].sh_offset = VBUF_GetDataLength(vbuf);
        if (d1_strtab) {
            info->shdrs[info->sec_id].sh_name += d1_strtab->sh_size;
        }
        ELFLIB_FixSecID(changes->c2, i, info->sec_id, 0);
        ret |= ELFBUF_Write(vbuf, ELFLIB_GetSecData(d2, shdr2), shdr2->sh_size);
        info->sec_id ++;
    }

    return ret;
}

static int _elflib_merge_sec(LLDATA_S *d1, LLDATA_S *d2, OUT VBUF_S *vbuf, OUT ELF_MDY_SEC_INFO_S *info)
{
    int ret;
    ELF_SEC_CHANGE_INFO2_S changes;

    memset(&changes, 0, sizeof(changes));

    ret = _elflib_merge_d1_sec(d1, d2, vbuf, info, &changes);
    ret |= _elflib_merge_d2_sec(d1, d2, vbuf, info, &changes);
    _elflib_fix_d2_rel_info(d1, d2);
    ret |= ELFBUF_MergeRelSecs(d1, d2, &changes, vbuf, info);
    ret |= ELFBUF_MergeSymtab(d1, d2, &changes, vbuf, info);

    return ret;
}


static void _elflib_fix_rel_sym_id(LLDATA_S *d, int old_sym_id, int new_sym_id)
{
    Elf64_Shdr *shdr = NULL;

    while ((shdr = ELFLIB_GetNextTypeSec(d, SHT_REL, shdr))) {
        Elf64_Rel *rs = ELFLIB_GetSecData(d, shdr);
        if (! rs) {
            continue;
        }

        int nrels = shdr->sh_size / shdr->sh_entsize;
        for (int i = 0; i < nrels; i++) {
            if (ELF64_R_SYM(rs[i].r_info) == old_sym_id) {
                U32 type = ELF64_R_TYPE(rs[i].r_info);
                rs[i].r_info = ELF64_R_INFO(new_sym_id, type);
            }
        }
    }
}


static void _elflib_link_ext_sym(VBUF_S *vbuf)
{
    LLDATA_S d;
    Elf64_Shdr *shdr;

    d.data = VBUF_GetData(vbuf);
    d.len = VBUF_GetDataLength(vbuf);

    shdr = ELFLIB_GetSymSec(&d);

    Elf64_Sym * syms = ELFLIB_GetSecData(&d, shdr);

    int count = shdr->sh_size/sizeof(*syms);

    
    for (int i=0; i<count; i++) {
        if (syms[i].st_shndx) {
            continue; 
        }
        char *sym_name = ELFLIB_GetStr(&d, syms[i].st_name);
        if ((! sym_name) || (sym_name[0] == '\0')) {
            continue;
        }

        Elf64_Sym *target_sym = ELFLIB_GetLocalSymByName(&d, sym_name);
        if ((! target_sym) || (! target_sym->st_shndx)) {
            continue;
        }

        _elflib_fix_rel_sym_id(&d, i, ((U64)target_sym - (U64)syms)/sizeof(*syms));
        memset(&syms[i], 0, sizeof(syms[0])); 
    }
}

int ELFLIB_Merge(LLDATA_S *d1, LLDATA_S *d2, OUT VBUF_S *vbuf)
{
    int ret;
    ELF_MDY_SEC_INFO_S *sec_info;

    ret = ELFBUF_WriteEhdr(d1, vbuf);
    if (ret < 0) {
        return ret;
    }

    sec_info = MEM_ZMalloc(sizeof(*sec_info));
    if (! sec_info) {
        RETURN(BS_NO_MEMORY);
    }

    sec_info->sec_id = 1; 

    ret |= _elflib_merge_sec(d1, d2, vbuf, sec_info);
    ret |= ELFBUF_WriteEnd(vbuf, sec_info);
    if (ret == 0) {
        _elflib_link_ext_sym(vbuf);
    }

    MEM_Free(sec_info);

    return ret;
}

