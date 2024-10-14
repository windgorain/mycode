/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0
* Description: 
******************************************************************************/
#include "bs.h"
#include "utl/elf_utl.h"
#include "utl/elf_lib.h"
#include "utl/vbuf_utl.h"
#include "utl/mybpf_utl.h"
#include "utl/mybpf_spf_sec.h"
#include "utl/mybpf_asmdef.h"


static int _elflib_data_init_write_data_init(LLDATA_S *d, OUT VBUF_S *vbuf, OUT ELF_MDY_SEC_INFO_S *info, OUT Elf64_Sym *data_init_sym)
{
    Elf64_Shdr *data_shdr, *rel_shdr = NULL;
    Elf64_Rel *rs;
    MYBPF_INSN_S insn[5] = {0};
    Elf64_Shdr *strtab_shdr = ELFLIB_GetStrtabSec(d);
    int ret = 0;

    if (! strtab_shdr) {
        RETURN(BS_ERR);
    }

    if (info->sec_id >= ARRAY_SIZE(info->shdrs)) {
        RETURNI(BS_REACH_MAX, "sec num exceed");
    }

    Elf64_Shdr *shdr = &info->shdrs[info->sec_id];
    info->sec_id ++;
    shdr->sh_offset = VBUF_GetDataLength(vbuf);
    shdr->sh_name = strtab_shdr->sh_size; 
    shdr->sh_type = SHT_PROGBITS;
    shdr->sh_flags = SHF_EXECINSTR;
    shdr->sh_size = 0;

    while ((rel_shdr = ELFLIB_GetNextTypeSec(d, SHT_REL, rel_shdr))) {
        data_shdr = ELFLIB_GetMdySecByRelSec(d, rel_shdr);
        if (! ELFLIB_IsGlobalDataSection(d, data_shdr)) {
            continue;
        }

        void *mdy_data = ELFLIB_GetSecData(d, data_shdr);

        rel_shdr->sh_type = SHT_NULL; 

        rs = ELFLIB_GetSecData(d, rel_shdr);
        if (! rs) {
            return 0;
        }

        int rel_count = rel_shdr->sh_size/sizeof(*rs);
        shdr->sh_size += (rel_count * sizeof(insn));

        for (int i=0; i<rel_count; i++) {
            Elf64_Rel *rel = &rs[i];
            Elf64_Sym *sym = ELFLIB_GetSymByRel(d, rel);
            if (! sym) {
                RETURNI(BS_ERR, "Can't get rel's sym");
            }

            if (! sym->st_shndx) {
                RETURNI(BS_ERR, "Can't get symbol %s", ELFLIB_GetSymName(d, sym));
            }

            
            insn[0].opcode = 0x18;
            insn[0].dst_reg = 1;
            insn[0].src_reg = BPF_PSEUDO_MAP_VALUE;
            if (ELFLIB_IsDataSection(d, data_shdr)) {
                insn[0].imm = 1;
            } else if (ELFLIB_IsRoDataSection(d, data_shdr)) {
                int id = ELFLIB_GetRoDataID(d, data_shdr);
                insn[0].imm = id + 2;
            } else {
                RETURNI(BS_ERR, "Not support");
            }
            insn[1].imm = rel->r_offset;

            
            insn[2].opcode = 0x18;
            insn[2].dst_reg = 2;

            Elf64_Shdr *target_shdr = ELFLIB_GetSecBySym(d, sym);
            if (! target_shdr) {
                RETURNI(BS_ERR, "Can't get sym %s's sec", ELFLIB_GetSymName(d, sym));
            }

            long old_value = *(long*)((char*)mdy_data + rel->r_offset);
            int new_imm = sym->st_value + old_value;

            if (ELFLIB_IsBssSection(d, target_shdr)) {
                insn[2].imm = 0; 
                insn[2].src_reg = BPF_PSEUDO_MAP_VALUE;
                insn[3].imm = new_imm;
            } else if (ELFLIB_IsDataSection(d, target_shdr)) {
                insn[2].imm = 1; 
                insn[2].src_reg = BPF_PSEUDO_MAP_VALUE;
                insn[3].imm = new_imm;
            } else if (ELFLIB_IsRoDataSection(d, target_shdr)) {
                int id = ELFLIB_GetRoDataID(d, target_shdr);
                if (id < 0) {
                    RETURN(BS_ERR);
                }
                insn[2].imm = 2 + id; 
                insn[2].src_reg = BPF_PSEUDO_MAP_VALUE;
                insn[3].imm = new_imm;
            } else if (ELFLIB_IsTextSection(d, target_shdr)) {
                U64 sec_offset = ELFLIB_GetProgSecOffset(d, target_shdr);
                insn[2].src_reg = BPF_PSEUDO_FUNC_PTR;
                insn[2].imm = new_imm + sec_offset;
                insn[3].imm = 0;
            }

            
            insn[4].opcode = 0x7b;
            insn[4].dst_reg = 1;
            insn[4].src_reg = 2;

            ret |= VBUF_CatBuf(vbuf, insn, sizeof(insn));
        }
    }

    MYBPF_INSN_S exit_insn = {0};
    exit_insn.opcode = 0x95; 
    shdr->sh_size += sizeof(exit_insn);
    ret |= VBUF_CatBuf(vbuf, &exit_insn, sizeof(exit_insn));

    data_init_sym->st_name = shdr->sh_name;
    data_init_sym->st_size = shdr->sh_size;
    data_init_sym->st_info = STT_FUNC;
    data_init_sym->st_shndx = info->sec_id - 1;

    return ret;
}

static int _elflib_aggr_write_secs_permit(LLDATA_S *d, Elf64_Shdr *shdr, void *ud)
{
    if ((shdr->sh_type == SHT_NULL)
            || (shdr->sh_type == SHT_SYMTAB)
            || (shdr->sh_type == SHT_REL)) {
        return 0;
    }

    if (ELFLIB_IsProgSection(shdr)) {
        return 0;
    }

    return 1;
}

static int _elflib_all_progs_permit(LLDATA_S *d, Elf64_Shdr *shdr, void *ud)
{
    if (! ELFLIB_IsProgSection(shdr)) {
        return 0;
    }

    return 1;
}

static int _elflib_progs_non_text_permit(LLDATA_S *d, Elf64_Shdr *shdr, void *ud)
{
    int flag = *(U32*)ud;

    if (! ELFLIB_IsProgSection(shdr)) {
        return 0;
    }

    if ((! (flag & ELFLIB_AGGR_FLAG_KEEP_PROG_ORDER)) && ELFLIB_IsTextSection(d, shdr)) {
        return 0;
    }

    return 1;
}

static int _elflib_text_permit(LLDATA_S *d, Elf64_Shdr *shdr, void *ud)
{
    if (! ELFLIB_IsTextSection(d, shdr)) {
        return 0;
    }

    return 1;
}


static int _elflib_aggr_write_secs(LLDATA_S *d, OUT VBUF_S *vbuf, U32 flag, OUT ELF_MDY_SEC_INFO_S *info)
{
    int ret = 0;
    ELF_SEC_CHANGE_INFO_S change[ELF_MAX_SECTIONS];
    ELF_SEC_RECORD_S sec_record = {0};

    memset(change, 0, sizeof(change));

    sec_record.sec_info = info;
    sec_record.change1 = change;

    
    ret |= ELFLIB_WriteSecs(d, vbuf, &sec_record, _elflib_aggr_write_secs_permit, NULL);
    if (flag & ELFLIB_AGGR_FLAG_KEEP_PROG_ORDER) {
        
        ret |= ELFLIB_WriteSecs(d, vbuf, &sec_record, _elflib_all_progs_permit, NULL);
    } else {
        
        ret |= ELFLIB_WriteSecs(d, vbuf, &sec_record, _elflib_progs_non_text_permit, &flag);
        
        ret |= ELFLIB_WriteSecs(d, vbuf, &sec_record, _elflib_text_permit, NULL);
    }
    ret |= ELFBUF_WriteRelSecs(d, change, vbuf, info);
    ret |= ELFBUF_WriteSymtab(d, change, vbuf, info);

    return ret;
}

static int _elflib_datainit_write_secs_permit(LLDATA_S *d, Elf64_Shdr *shdr, void *ud)
{
    if ((shdr->sh_type == SHT_NULL)
            || (shdr->sh_type == SHT_SYMTAB)
            || (shdr->sh_type == SHT_STRTAB)
            || (shdr->sh_type == SHT_REL)) {
        return 0;
    }

    if (ELFLIB_IsProgSection(shdr)) {
        return 0;
    }

    return 1;
}


static int _elflib_datainit_write_secs(LLDATA_S *d, OUT VBUF_S *vbuf, OUT ELF_MDY_SEC_INFO_S *info)
{
    int ret = 0;
    ELF_SEC_CHANGE_INFO_S change[ELF_MAX_SECTIONS];
    ELF_SEC_RECORD_S sec_record = {0};
    Elf64_Sym data_init_sym = {0};

    memset(change, 0, sizeof(change));

    sec_record.sec_info = info;
    sec_record.change1 = change;

    
    char name[] = SPF_SEC_DATA_INIT;
    ret |= ELFBUF_WriteStrtab(d, name, vbuf, &sec_record);
    
    ret |= ELFLIB_WriteSecs(d, vbuf, &sec_record, _elflib_datainit_write_secs_permit, NULL);
    
    ret |= ELFLIB_WriteSecs(d, vbuf, &sec_record, _elflib_all_progs_permit, NULL);
    
    ret |= _elflib_data_init_write_data_init(d, vbuf, info, &data_init_sym);
    ret |= ELFBUF_WriteRelSecs(d, change, vbuf, info);
    ret |= ELFBUF_WriteSymtab(d, change, vbuf, info);
    
    ret |= ELFBUF_AddSym(d, &data_init_sym, vbuf, info);

    return ret;
}


int ELFLIB_AggrProgs(LLDATA_S *d, OUT VBUF_S *vbuf, U32 flag)
{
    ELF_MDY_SEC_INFO_S *sec_info;
    int ret;

    ret = ELFBUF_WriteEhdr(d, vbuf);
    if (ret < 0) {
        return ret;
    }

    sec_info = MEM_ZMalloc(sizeof(*sec_info));
    if (! sec_info) {
        RETURN(BS_NO_MEMORY);
    }

    sec_info->sec_id = 1; 

    ret |= _elflib_aggr_write_secs(d, vbuf, flag, sec_info);
    ret |= ELFBUF_WriteEnd(vbuf, sec_info);

    MEM_Free(sec_info);

    return ret;
}


int ELFLIB_AddDataInitSec(LLDATA_S *d, OUT VBUF_S *vbuf)
{
    ELF_MDY_SEC_INFO_S *sec_info;
    int ret;

    ret = ELFBUF_WriteEhdr(d, vbuf);
    if (ret < 0) {
        return ret;
    }

    sec_info = MEM_ZMalloc(sizeof(*sec_info));
    if (! sec_info) {
        RETURN(BS_NO_MEMORY);
    }

    sec_info->sec_id = 1; 

    ret |= _elflib_datainit_write_secs(d, vbuf, sec_info);
    ret |= ELFBUF_WriteEnd(vbuf, sec_info);

    MEM_Free(sec_info);

    return ret;
}

