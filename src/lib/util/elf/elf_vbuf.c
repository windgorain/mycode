/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0
* Description:
******************************************************************************/
#include "bs.h"
#include "utl/elf_utl.h"

int ELFBUF_Write(OUT VBUF_S *vbuf, void *buf, U32 buf_len)
{
    if ((! buf_len) || (! buf)) {
        return 0;
    }

    return VBUF_CatBuf(vbuf, buf, buf_len);
}


int ELFBUF_WriteEhdr(LLDATA_S *d, OUT VBUF_S *vbuf)
{
    Elf64_Ehdr *ehdr = ELFLIB_GetHeader(d);
    if (! ehdr) {
        RETURNI(BS_ERR, "Can't get elf header");
    }

    
    return ELFBUF_Write(vbuf, ehdr, sizeof(*ehdr));
}


static void _elfbuf_fix_rel_offset(LLDATA_S *d, Elf64_Shdr *rel_shdr, ELF_SEC_CHANGE_INFO_S *c)
{
    if (! c->offset) {
        return;
    }

    Elf64_Rel *rs = ELFLIB_GetSecData(d, rel_shdr);
    if (! rs) {
        return;
    }

    int nrels = rel_shdr->sh_size / rel_shdr->sh_entsize;
    for (int i = 0; i < nrels; i++) {
        if (rs[i].r_info == 0) {
            continue;
        }
        
        rs[i].r_offset += c->offset;
    }
}


int ELFBUF_WriteRelSecs(LLDATA_S *d, ELF_SEC_CHANGE_INFO_S *change, OUT VBUF_S *vbuf, OUT ELF_MDY_SEC_INFO_S *info)
{
    Elf64_Shdr *rel_shdr = NULL;
    int ret = 0;

    while ((rel_shdr = ELFLIB_GetNextTypeSec(d, SHT_REL, rel_shdr))) {
        if (info->sec_id >= ARRAY_SIZE(info->shdrs)) {
            RETURNI(BS_REACH_MAX, "sec num exceed");
        }

        Elf64_Shdr *shdr = &info->shdrs[info->sec_id];
        info->sec_id ++;

        *shdr = *rel_shdr;
        shdr->sh_offset = VBUF_GetDataLength(vbuf);

        if (shdr->sh_info < ELF_MAX_SECTIONS) {
            ELF_SEC_CHANGE_INFO_S *c = &change[shdr->sh_info];
            if (c->new_sec_id) {
                shdr->sh_info = c->new_sec_id;
                _elfbuf_fix_rel_offset(d, rel_shdr, c);
            }
        }

        ret |= ELFBUF_Write(vbuf, ELFLIB_GetSecData(d, rel_shdr), rel_shdr->sh_size);
    }

    return ret;
}


static int _elflib_merge_d1_rel_secs(LLDATA_S *d1, LLDATA_S *d2, ELF_SEC_CHANGE_INFO2_S *change,
        OUT VBUF_S *vbuf, OUT ELF_MDY_SEC_INFO_S *info)
{
    Elf64_Shdr *shdr1, *shdr2;
    int ret = 0;
    int sec_count = ELFLIB_CountSection(d1);

    for (int i=0; i<sec_count; i++) {

        shdr1 = ELFLIB_GetSecByID(d1, i);
        if ((! shdr1) || (shdr1->sh_type != SHT_REL)) {
            continue;
        }

        if (info->sec_id >= ARRAY_SIZE(info->shdrs)) {
            RETURNI(BS_REACH_MAX, "sec num exceed");
        }

        Elf64_Shdr *shdr = &info->shdrs[info->sec_id];
        info->sec_id ++;
        *shdr = *shdr1;
        shdr->sh_offset = VBUF_GetDataLength(vbuf);

        if (shdr1->sh_info < ELF_MAX_SECTIONS) {
            ELF_SEC_CHANGE_INFO_S *c = &change->c1[shdr1->sh_info];
            if (c->new_sec_id) {
                shdr->sh_info = c->new_sec_id;
                _elfbuf_fix_rel_offset(d1, shdr1, c);
            }
        }

        ret |= ELFBUF_Write(vbuf, ELFLIB_GetSecData(d1, shdr1), shdr1->sh_size);

        char *name = ELFLIB_GetSecName(d1, shdr1);
        
        shdr2 = ELFLIB_GetSecByName(d2, name);
        if (shdr2) {
            if (shdr->sh_info < ELF_MAX_SECTIONS) {
                ELF_SEC_CHANGE_INFO_S *c = &change->c2[shdr2->sh_info];
                _elfbuf_fix_rel_offset(d2, shdr2, c);
            }
            ret |= ELFBUF_Write(vbuf, ELFLIB_GetSecData(d2, shdr2), shdr2->sh_size);
            shdr->sh_size += shdr2->sh_size;
        }
    }

    return ret;
}


static int _elflib_merge_d2_rel_secs(LLDATA_S *d1, LLDATA_S *d2, ELF_SEC_CHANGE_INFO2_S *change,
        OUT VBUF_S *vbuf, OUT ELF_MDY_SEC_INFO_S *info)
{
    Elf64_Shdr *shdr1, *shdr2;
    int ret = 0;
    int sec_count = ELFLIB_CountSection(d2);
    Elf64_Shdr *d1_strtab = ELFLIB_GetStrtabSec(d1);

    for (int i=0; i<sec_count; i++) {
        shdr2 = ELFLIB_GetSecByID(d2, i);
        if ((! shdr2) || (shdr2->sh_type != SHT_REL)) {
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

        Elf64_Shdr *shdr = &info->shdrs[info->sec_id];
        info->sec_id ++;

        *shdr = *shdr2;
        shdr->sh_offset = VBUF_GetDataLength(vbuf);
        if (d1_strtab && shdr->sh_name) {
            shdr->sh_name += d1_strtab->sh_size;
        }

        if (shdr2->sh_info < ELF_MAX_SECTIONS) {
            ELF_SEC_CHANGE_INFO_S *c = &change->c2[shdr2->sh_info];
            if (c->new_sec_id) {
                shdr->sh_info = c->new_sec_id;
                _elfbuf_fix_rel_offset(d2, shdr2, c);
            }
        }

        ret |= ELFBUF_Write(vbuf, ELFLIB_GetSecData(d2, shdr2), shdr2->sh_size);
    }

    return ret;
}


int ELFBUF_MergeRelSecs(LLDATA_S *d1, LLDATA_S *d2, ELF_SEC_CHANGE_INFO2_S *change,
        OUT VBUF_S *vbuf, OUT ELF_MDY_SEC_INFO_S *info)
{
    int ret = _elflib_merge_d1_rel_secs(d1, d2, change, vbuf, info);
    ret |= _elflib_merge_d2_rel_secs(d1, d2, change, vbuf, info);
    return ret;
}

static void _elflib_fix_symtab(LLDATA_S *d, ELF_SEC_CHANGE_INFO_S *change)
{
    int strtab_id = ELFLIB_GetSecID(d, ELFLIB_GetStrtabSec(d));
    Elf64_Shdr *sym_shdr = ELFLIB_GetSymSec(d);
    Elf64_Sym * sym = ELFLIB_GetSecData(d, sym_shdr);

    if (! sym)  {
        return;
    }

    ELF_SEC_CHANGE_INFO_S *strtab_c = &change[strtab_id];

    for (int i=0; i<sym_shdr->sh_size/sizeof(*sym); i++) {
        if (sym[i].st_name) { 
            sym[i].st_name += strtab_c->offset;
        }
        if (sym[i].st_shndx < ELF_MAX_SECTIONS) {
            ELF_SEC_CHANGE_INFO_S *c = &change[sym[i].st_shndx];
            if (c->new_sec_id) {
                sym[i].st_shndx = c->new_sec_id;
                sym[i].st_value += c->offset;
            }
        }
    }
}


int ELFBUF_WriteStrtab(LLDATA_S *d, char *add_string, OUT VBUF_S *vbuf, OUT ELF_SEC_RECORD_S *sec_record)
{
    ELF_MDY_SEC_INFO_S *info = sec_record->sec_info;
    ELF_SEC_CHANGE_INFO_S *change = sec_record->change1;
    Elf64_Shdr *str_shdr;

    str_shdr = ELFLIB_GetStrtabSec(d);
    if (!str_shdr) {
        return 0;
    }

    if (info->sec_id >= ARRAY_SIZE(info->shdrs)) {
        RETURNI(BS_REACH_MAX, "sec num exceed");
    }

    ELFLIB_SetEhdrStrtabID(VBUF_GetData(vbuf), info->sec_id);

    Elf64_Shdr *shdr = &info->shdrs[info->sec_id];
    info->sec_id ++;
    *shdr = *str_shdr;
    shdr->sh_offset = VBUF_GetDataLength(vbuf);

    _elflib_fix_symtab(d, change);

    int ret = ELFBUF_Write(vbuf, ELFLIB_GetSecData(d, str_shdr), str_shdr->sh_size);

    if (add_string) {
        int add_len = strlen(add_string) + 1;
        ret |= ELFBUF_Write(vbuf, add_string, add_len); 
        shdr->sh_size += add_len;
    }

    return ret;
}


int ELFBUF_WriteSymtab(LLDATA_S *d, ELF_SEC_CHANGE_INFO_S *change, OUT VBUF_S *vbuf, OUT ELF_MDY_SEC_INFO_S *info)
{
    Elf64_Shdr *sym_shdr;

    sym_shdr = ELFLIB_GetSymSec(d);
    if (!sym_shdr) {
        return 0;
    }

    if (info->sec_id >= ARRAY_SIZE(info->shdrs)) {
        RETURNI(BS_REACH_MAX, "sec num exceed");
    }

    Elf64_Shdr *shdr = &info->shdrs[info->sec_id];
    info->sec_id ++;
    *shdr = *sym_shdr;
    shdr->sh_offset = VBUF_GetDataLength(vbuf);

    _elflib_fix_symtab(d, change);

    return ELFBUF_Write(vbuf, ELFLIB_GetSecData(d, sym_shdr), sym_shdr->sh_size);
}


int ELFBUF_AddSym(LLDATA_S *d, Elf64_Sym *sym, OUT VBUF_S *vbuf, OUT ELF_MDY_SEC_INFO_S *info)
{
    Elf64_Shdr *shdr = &info->shdrs[info->sec_id - 1];
    shdr->sh_size += sizeof(*sym);
    return ELFBUF_Write(vbuf, sym, sizeof(*sym));
}

int ELFBUF_MergeSymtab(LLDATA_S *d1, LLDATA_S *d2, ELF_SEC_CHANGE_INFO2_S *change,
        OUT VBUF_S *vbuf, OUT ELF_MDY_SEC_INFO_S *info)
{
    Elf64_Shdr *shdr1, *shdr2;
    int ret = 0;

    shdr1 = ELFLIB_GetSymSec(d1);
    shdr2 = ELFLIB_GetSymSec(d2);

    if ((!shdr1) || (!shdr2)) {
        return -1;
    }

    if (info->sec_id >= ARRAY_SIZE(info->shdrs)) {
        RETURNI(BS_REACH_MAX, "sec num exceed");
    }

    Elf64_Shdr *shdr = &info->shdrs[info->sec_id];
    info->sec_id ++;

    *shdr = *shdr1;
    shdr->sh_offset = VBUF_GetDataLength(vbuf);
    shdr->sh_size = (shdr1->sh_size + shdr2->sh_size);
    
    shdr->sh_info = (shdr2->sh_info + ELFLIB_GetSymCount(shdr1));

    _elflib_fix_symtab(d1, change->c1);
    ret |= ELFBUF_Write(vbuf, ELFLIB_GetSecData(d1, shdr1), shdr1->sh_size);

    Elf64_Sym *sym = ELFLIB_GetSecData(d2, shdr2);
    if (sym) {
        _elflib_fix_symtab(d2, change->c2);
        ret |= ELFBUF_Write(vbuf, sym, shdr2->sh_size);
    }

    return ret;
}


int ELFBUF_WriteEnd(OUT VBUF_S *vbuf, ELF_MDY_SEC_INFO_S *info)
{
    Elf64_Ehdr *ehdr;
    int ret;

    
    ehdr = VBUF_GetData(vbuf);
    ehdr->e_shnum = info->sec_id;
    ehdr->e_shoff = VBUF_GetDataLength(vbuf);

    ret = ELFBUF_Write(vbuf, info->shdrs, info->sec_id * sizeof(Elf64_Shdr));
    if (ret < 0) {
        return ret;
    }

    LLDATA_S d;
    d.data = VBUF_GetData(vbuf);
    d.len = VBUF_GetDataLength(vbuf);

    
    ELFLIB_SetRelLink(&d, info->sec_id - 1);

    return 0;
}


int ELFLIB_WriteSecs(LLDATA_S *d, OUT VBUF_S *vbuf, OUT ELF_SEC_RECORD_S *sec_record,
        PF_ELFLIB_WALK_SEC permit_func, void *ud)
{
    ELF_MDY_SEC_INFO_S *info = sec_record->sec_info;
    ELF_SEC_CHANGE_INFO_S *c = sec_record->change1;
    Elf64_Shdr *shdr = NULL;
    int ret = 0;

    while ((shdr = ELFLIB_GetNextSec(d, shdr))) {
        if (! permit_func(d, shdr, ud)) {
            continue;
        }

        if (info->sec_id >= ARRAY_SIZE(info->shdrs)) {
            RETURNI(BS_REACH_MAX, "sec num exceed");
        }

        info->shdrs[info->sec_id] = *shdr;
        info->shdrs[info->sec_id].sh_offset = VBUF_GetDataLength(vbuf);

        int old_sec_id = ELFLIB_GetSecID(d, shdr);
        ELFLIB_FixSecID(c, old_sec_id, info->sec_id, 0);

        ret |= ELFBUF_Write(vbuf, ELFLIB_GetSecData(d, shdr), shdr->sh_size);

        if (shdr->sh_type == SHT_STRTAB) {
            
            ELFLIB_SetEhdrStrtabID(VBUF_GetData(vbuf), info->sec_id);
        }

        info->sec_id ++;
    }

    return ret;
}
