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
    /* Match FD relocation against recorded map_data[] offset */
    for (int i= 0; i< maps_count; i++) {
        if (relo_maps[i].sec_id != sym->st_shndx) {
            continue;
        }

        if (_mybpf_relo_is_globle_data(&relo_maps[i])) {
            /* is global data */
            int old_imm = insn->imm;
            insn->imm = relo_maps[i].value;
            insn[1].imm = old_imm + sym->st_value;
            insn->src_reg = BPF_PSEUDO_MAP_VALUE;

            MYBPF_DBG_OUTPUT(MYBPF_DBG_ID_RELO, MYBPF_DBG_FLAG_RELO_PROCESS, 
                    "Relo insn %d to global map %d from offset %d to %d\n",
                    cur_pc, insn->imm, old_imm, insn[1].imm);

            return 0;
        }

        if (relo_maps[i].offset == sym->st_value) {
            insn->imm = relo_maps[i].value;
            insn->src_reg = BPF_PSEUDO_MAP_FD;

            MYBPF_DBG_OUTPUT(MYBPF_DBG_ID_RELO, MYBPF_DBG_FLAG_RELO_PROCESS, 
                    "Relo insn %d to map %d \n", cur_pc, insn->imm);

            return 0;
        }
    }

    return BS_ERR;
}

static int _mybpf_relo_sub_prog(MYBPF_INSN_S *insn, int cur_pc, GElf_Sym *sym)
{
    if (insn->src_reg != BPF_PSEUDO_CALL) {
        return BS_ERR;
    }

    /* insn->imm + sym->st_value/8 + 1: 目标绝对位置 */
    /* 目标绝对位置 - 当前位置: 相对于cur_pc的的相对位置 */
    /* 先对位置 - 1: 相对于pc的的相对位置. pc = cur_pc + 1 */

    int new_imm = (insn->imm + sym->st_value/8) - cur_pc;

    MYBPF_DBG_OUTPUT(MYBPF_DBG_ID_RELO, MYBPF_DBG_FLAG_RELO_PROCESS, 
            "Relo insn %d from %d to %d \n", cur_pc, insn->imm, new_imm);

    insn->imm = new_imm;

    return 0;
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
            ret = _mybpf_relo_map(&insn[cur_pc], cur_pc, sym, relo_maps, maps_count);
        } else if (insn[cur_pc].opcode == (BPF_JMP | BPF_CALL)) {
            ret = _mybpf_relo_sub_prog(&insn[cur_pc], cur_pc, sym);
        }

        if (ret < 0) {
            RETURNI(BS_ERR, "Can't process relo: insn:%d, sec_id=%d, value=%ld",
                    cur_pc, sym->st_shndx, sym->st_value);
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

/* 重定位Prog Map 和 sub prog */
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

        /* 根据sec id找到对应的prog */
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

