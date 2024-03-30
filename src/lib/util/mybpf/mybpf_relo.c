/*********************************************************
*   Copyright (C) LiXingang
*   Description: 
*
********************************************************/
#include "bs.h"
#include "utl/mybpf_utl.h"
#include "utl/mybpf_relo.h"
#include "utl/mybpf_dbg.h"
#include "mybpf_def_inner.h"
#include "mybpf_osbase.h"

static BOOL_T _mybpf_relo_is_globle_data(MYBPF_RELO_MAP_S *relo_map)
{
    if ((relo_map->type == MYBPF_RELO_MAP_BSS)
            || (relo_map->type == MYBPF_RELO_MAP_DATA)
            || (relo_map->type == MYBPF_RELO_MAP_RODATA)) {
        return TRUE;
    }
    return FALSE;
}

static int _mybpf_relo_map(MYBPF_INSN_S *insn, int cur_pc,
        GElf_Sym *sym, MYBPF_RELO_MAP_S *relo_maps, int maps_count)
{
    
    for (int i= 0; i< maps_count; i++) {
        if (relo_maps[i].sec_id != sym->st_shndx) {
            continue;
        }

        if (_mybpf_relo_is_globle_data(&relo_maps[i])) {
            
            int old_imm = insn->imm;
            insn->imm = relo_maps[i].value;
            insn[1].imm = old_imm + sym->st_value;
            insn->src_reg = BPF_PSEUDO_MAP_VALUE;

            MYBPF_DBG_OUTPUT(MYBPF_DBG_ID_RELO, DBG_UTL_FLAG_PROCESS, 
                    "Relo insn %d to global map %d from offset %d to %d\n",
                    cur_pc, insn->imm, old_imm, insn[1].imm);

            return 0;
        }

        if (relo_maps[i].offset == sym->st_value) {
            insn->imm = relo_maps[i].value;
            insn->src_reg = BPF_PSEUDO_MAP_FD;

            MYBPF_DBG_OUTPUT(MYBPF_DBG_ID_RELO, DBG_UTL_FLAG_PROCESS, 
                    "Relo insn %d to map %d \n", cur_pc, insn->imm);

            return 0;
        }
    }

    RETURN(BS_ERR);
}

static BOOL_T _mybpf_is_text_sec(ELF_S *elf, int sec_id)
{
    ELF_SECTION_S sec;

    if (ELF_GetSecByID(elf, sec_id, &sec) < 0) {
        return FALSE;
    }

    if (strcmp(sec.shname, ".text") != 0) {
        return FALSE;
    }

    return TRUE;
}


static int _mybpf_relo_sub_prog(ELF_S *elf, MYBPF_INSN_S *insn, int cur_pc, GElf_Sym *sym)
{
    if (insn->src_reg != BPF_PSEUDO_CALL) {
        MYBPF_DBG_OUTPUT(MYBPF_DBG_ID_RELO, DBG_UTL_FLAG_PROCESS, "Err: not call \n");
        RETURN(BS_ERR);
    }

    if (! _mybpf_is_text_sec(elf, sym->st_shndx)) {
        RETURN(BS_ERR);
    }

    
    
    
    int new_imm = (insn->imm + sym->st_value/8) - cur_pc;

    MYBPF_DBG_OUTPUT(MYBPF_DBG_ID_RELO, DBG_UTL_FLAG_PROCESS, 
            "Relo insn %d from %d to %d \n", cur_pc, insn->imm, new_imm);

    insn->imm = new_imm;

    return 0;
}

static int _mybpf_relo_ext_call(ELF_S *elf, MYBPF_INSN_S *insn, int cur_pc, GElf_Sym *sym)
{
    if (insn->src_reg != BPF_PSEUDO_CALL) {
        MYBPF_DBG_OUTPUT(MYBPF_DBG_ID_RELO, DBG_UTL_FLAG_PROCESS, "Err: not call \n");
        RETURN(BS_ERR);
    }

    char *name = ELF_GetSymbolName(elf, sym);
    if (! name) {
        MYBPF_DBG_OUTPUT(MYBPF_DBG_ID_RELO, DBG_UTL_FLAG_PROCESS, "Err: Can't get func name \n");
        RETURN(BS_ERR);
    }

#if 1
    RETURNI(BS_NOT_SUPPORT, "Not support extern call");
#else
    if (strcmp(name, "printf") == 0) {
        insn->imm = 1000070;
    } else if (strcmp(name, "puts") == 0) {
        insn->imm = 1000071;
    } else {
        BS_WARNNING(("Err: Can't find func %s \n", name));
        RETURN(BS_ERR);
    }

    insn->src_reg = 0;

    return 0;
#endif
}

static int _mybpf_relo_call(ELF_S *elf, MYBPF_INSN_S *insn, int cur_pc, GElf_Sym *sym)
{
    if (sym->st_shndx) {
        return _mybpf_relo_sub_prog(elf, insn, cur_pc, sym);
    } else {
        return _mybpf_relo_ext_call(elf, insn, cur_pc, sym);
    }
}

static int _mybpf_relo_func_ptr(ELF_S *elf, MYBPF_INSN_S *insn, int cur_pc, GElf_Sym *sym)
{
    if (! _mybpf_is_text_sec(elf, sym->st_shndx)) {
        RETURN(BS_ERR);
    }

    insn->imm += sym->st_value; 
    insn->src_reg = BPF_PSEUDO_FUNC_PTR;

    MYBPF_DBG_OUTPUT(MYBPF_DBG_ID_RELO, DBG_UTL_FLAG_PROCESS, 
            "Relo insn %d to func ptr %d \n", cur_pc, insn->imm/sizeof(MYBPF_INSN_S));

    return 0;
}

static int _mybpf_relo_load(ELF_S *elf, MYBPF_INSN_S *insn, int cur_pc,
        GElf_Sym *sym, MYBPF_RELO_MAP_S *relo_maps, int maps_count)
{
    int ret;

    ret = _mybpf_relo_map(insn, cur_pc, sym, relo_maps, maps_count);
    if (ret < 0) { 
        ret = _mybpf_relo_func_ptr(elf, insn, cur_pc, sym);
    }

    return ret;
}

static int _mybpf_relo_and_apply(ELF_S *elf, MYBPF_INSN_S *insn, MYBPF_RELO_MAP_S *relo_maps, int maps_count,
        ELF_SECTION_S *relo_sec, ELF_PROG_INFO_S *prog_info)
{
    Elf_Data *data = relo_sec->data;
    GElf_Shdr *shdr = &relo_sec->shdr;
    int nrels = shdr->sh_size / shdr->sh_entsize;
    int i;

	for (i = 0; i < nrels; i++) {
		GElf_Sym *sym;
		GElf_Rel rel;
		unsigned int insn_idx;

        ELF_GetRel(data, i, &rel);

		insn_idx = rel.r_offset / sizeof(MYBPF_INSN_S);

		sym = ELF_GetSymbol(elf, GELF_R_SYM(rel.r_info));

        int ret = -1;

        int cur_pc = (prog_info->sec_offset / sizeof(MYBPF_INSN_S)) + insn_idx;

        if (insn[cur_pc].opcode == (BPF_LD | BPF_IMM | BPF_DW)) {
            ret = _mybpf_relo_load(elf, &insn[cur_pc], cur_pc, sym, relo_maps, maps_count);
        } else if (insn[cur_pc].opcode == (BPF_JMP | BPF_CALL)) {
            ret = _mybpf_relo_call(elf, &insn[cur_pc], cur_pc, sym);
        }

        if (ret < 0) {
            BS_WARNNING(("Can't process relo: insn:%d, sec_id=%d, sym_name=%s",
                    cur_pc, sym->st_shndx, ELF_GetSymbolName(elf, sym)));
            RETURN(BS_ERR);
        }
	}

	return 0;
}

static ELF_PROG_INFO_S * _mybpf_relo_get_prog_by_sec_id(ELF_PROG_INFO_S *progs_info,
        int prog_count, int sec_id)
{
    for (int i=0; i<prog_count; i++) {
        if (progs_info[i].sec_id == sec_id) {
            return &progs_info[i];
        }
    }

    return NULL;
}


int MYBPF_RELO_ProgRelo(ELF_S *elf, void *insts, MYBPF_RELO_MAP_S *relo_maps, int maps_count,
        ELF_PROG_INFO_S *progs_info, int prog_count)
{
    int ret;
    void *iter = NULL;
    ELF_SECTION_S relo_sec;
    ELF_PROG_INFO_S *prog_info;

    while((iter = ELF_GetNextSection(elf, iter, &relo_sec))) {
        if (relo_sec.shdr.sh_type != SHT_REL) {
            continue;
        }

        
        prog_info = _mybpf_relo_get_prog_by_sec_id(progs_info, prog_count, relo_sec.shdr.sh_info);
        if (! prog_info) {
            continue;
        }

        if ((ret = _mybpf_relo_and_apply(elf, insts, relo_maps, maps_count, &relo_sec, prog_info)) < 0) {
            return ret;
        }
    }

    return 0;
}

static BOOL_T _mybpf_relo_is_matched(GElf_Sym *sym, int map_sec_id, int map_offset, BOOL_T is_global_data)
{
    if (map_sec_id != sym->st_shndx) {
        return FALSE;
    }

    if ((! is_global_data) && (map_offset != sym->st_value)) {
        return FALSE;
    }

    return TRUE;
}

static BOOL_T _mybpf_relo_is_map_used_by_prog(ELF_S *elf, ELF_SECTION_S *sec,
        int map_sec_id, int map_offset, BOOL_T is_global_data)
{
    Elf_Data *data = sec->data;
    GElf_Shdr *shdr = &sec->shdr;
	int nrels = shdr->sh_size / shdr->sh_entsize;
    int i;

	for (i = 0; i < nrels; i++) {
		GElf_Sym *sym;
		GElf_Rel rel;

        ELF_GetRel(data, i, &rel);

		sym = ELF_GetSymbol(elf, GELF_R_SYM(rel.r_info));

        if (_mybpf_relo_is_matched(sym, map_sec_id, map_offset, is_global_data)) {
            return TRUE;
        }

	}

	return FALSE;
}


static BOOL_T _mybpf_relo_is_map_used(ELF_S *elf, int map_sec_id, int map_offset, int is_global_data)
{
    void *iter = NULL;
    ELF_SECTION_S sec;

    while((iter = ELF_GetNextSection(elf, iter, &sec))) {
        if (sec.shdr.sh_type != SHT_REL) {
            continue;
        }

        if (_mybpf_relo_is_map_used_by_prog(elf, &sec, map_sec_id, map_offset, is_global_data)) {
            return TRUE;
        }
    }

    return FALSE;
}


BOOL_T MYBPF_RELO_IsMapUsed(ELF_S *elf, int map_sec_id, int map_offset, int is_global_data)
{
    return _mybpf_relo_is_map_used(elf, map_sec_id, map_offset, is_global_data);
}
