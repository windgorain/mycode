/*********************************************************
*   Copyright (C) LiXingang
*   Description: 
*
********************************************************/
#include "bs.h"
#include "utl/mybpf_utl.h"
#include "utl/mybpf_relo.h"
#include "mybpf_def.h"
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

static int _mybpf_relo_map(MYBPF_INSN_S *insn, GElf_Sym *sym, MYBPF_RELO_MAP_S *relo_maps, int maps_count)
{
    /* Match FD relocation against recorded map_data[] offset */
    for (int i= 0; i< maps_count; i++) {
        if (relo_maps[i].sec_id != sym->st_shndx) {
            continue;
        }

        if (_mybpf_relo_is_globle_data(&relo_maps[i])) {
            /* is global data */
            insn[1].imm = insn->imm + sym->st_value;
            insn->imm = relo_maps[i].value;
            insn->src_reg = BPF_PSEUDO_MAP_VALUE;
            return 0;
        }

        if (relo_maps[i].offset == sym->st_value) {
            insn->imm = relo_maps[i].value;
            insn->src_reg = BPF_PSEUDO_MAP_FD;
            return 0;
        }
    }

    return BS_ERR;
}

static int _mybpf_relo_sub_prog(MYBPF_INSN_S *insn, GElf_Sym *sym)
{
    if (insn->src_reg != BPF_PSEUDO_CALL) {
        return BS_ERR;
    }

    insn->imm = sym->st_value;

    return 0;
}

static int _mybpf_relo_and_apply(ELF_S *elf, MYBPF_RELO_MAP_S *relo_maps, int maps_count,
        ELF_SECTION_S *relo_sec, ELF_SECTION_S *prog_sec)
{
    Elf_Data *data = relo_sec->data;
    GElf_Shdr *shdr = &relo_sec->shdr;
    MYBPF_INSN_S *insn = (MYBPF_INSN_S *) prog_sec->data->d_buf;
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

        if (insn[insn_idx].opcode == (BPF_LD | BPF_IMM | BPF_DW)) {
            ret = _mybpf_relo_map(&insn[insn_idx], sym, relo_maps, maps_count);
        } else if (insn[insn_idx].opcode == (BPF_JMP | BPF_CALL)) {
            ret = _mybpf_relo_sub_prog(&insn[insn_idx], sym);
        }

        if (ret < 0) {
            RETURNI(BS_ERR, "Can't process relo: insn:%d, sec_id=%d, value=%ld",
                    insn_idx, sym->st_shndx, sym->st_value);
        }
	}

	return 0;
}

/* 重定位Prog Map */
int MYBPF_RELO_ProgRelo(ELF_S *elf, MYBPF_RELO_MAP_S *relo_maps, int maps_count)
{
    int ret;
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

        if ((ret = _mybpf_relo_and_apply(elf, relo_maps, maps_count, &relo_sec, &prog_sec)) < 0) {
            return ret;
        }
    }

    return 0;
}

