/*================================================================
*   Created by LiXingang, Copyright LiXingang
*   Date: 2017.10.2
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/ufd_utl.h"
#include "utl/umap_utl.h"
#include "utl/mybpf_insn.h"
#include "utl/mybpf_prog.h"
#include "utl/bpf_helper_utl.h"
#include "mybpf_osbase.h"
#include "mybpf_def_inner.h"

int MYBPF_PROG_ReplaceMapFdWithMapPtr(void *insts, int insts_len,
        PF_MYBPF_INSN_MAP_FD_2_PTR map_2_ptr_fn, void *ud)
{
    MYBPF_INSN_S *insn = insts;
    int insn_cnt = insts_len / sizeof(*insn);
    int i;

	for (i = 0; i < insn_cnt; i++, insn++) {
		if (BPF_CLASS(insn->opcode) == BPF_LDX &&
		    (BPF_MODE(insn->opcode) != BPF_MEM || insn->imm != 0)) {
            RETURNI(BS_ERR, "pc=%d, opcode=0x%x, dst=%u, src=%u, off=%u, imm=%u",
                    i, insn->opcode, insn->dst_reg, insn->src_reg, insn->off, insn->imm);
        }
        if (BPF_CLASS(insn->opcode) == BPF_STX &&
                (((BPF_MODE(insn->opcode) != BPF_MEM) || (insn->imm != 0)) &&
                  BPF_MODE(insn->opcode) != BPF_XADD)) {
            RETURNI(BS_ERR, "pc=%d, opcode=0x%x, dst=%u, src=%u, off=%u, imm=%u",
                    i, insn->opcode, insn->dst_reg, insn->src_reg, insn->off, insn->imm);
        }
        if (insn[0].opcode == (BPF_LD | BPF_IMM | BPF_DW)) {
            U64 addr;

			if (i == insn_cnt - 1 || insn[1].opcode != 0 ||
			    insn[1].dst_reg != 0 || insn[1].src_reg != 0 ||
			    insn[1].off != 0) {
                RETURNI(BS_ERR, "pc=%d, opcode=0x%x, dst=%u, src=%u, off=%u, imm=%u",
                        i, insn->opcode, insn->dst_reg, insn->src_reg, insn->off, insn->imm);
            }

            if (insn[0].src_reg == 0)
                goto next_insn;

            
            if ((insn[0].src_reg != BPF_PSEUDO_MAP_FD) && (insn[0].src_reg != BPF_PSEUDO_MAP_VALUE)) {
                goto next_insn;
            }

            if ((insn[0].src_reg == BPF_PSEUDO_MAP_FD) && (insn[1].imm != 0)) {
                RETURNI(BS_ERR, "pc=%d, opcode=0x%x, dst=%u, src=%u, off=%u, imm=%u",
                        i, insn->opcode, insn->dst_reg, insn->src_reg, insn->off, insn->imm);
			}

			if (insn->src_reg == BPF_PSEUDO_MAP_FD) {
                addr = (unsigned long) map_2_ptr_fn(0, insn->imm, 0, ud);
                if (addr == 0) {
                    RETURNI(BS_ERR, "pc=%d, opcode=0x%x, dst=%u, src=%u, off=%u, imm=%u",
                            i, insn->opcode, insn->dst_reg, insn->src_reg, insn->off, insn->imm);
                }
            } else {
				U32 off = insn[1].imm;

				if (off >= BPF_MAX_VAR_OFF) {
                    RETURNI(BS_ERR, "pc=%d, opcode=0x%x, dst=%u, src=%u, off=%u, imm=%u",
                            i, insn->opcode, insn->dst_reg, insn->src_reg, insn->off, insn->imm);
                }

                addr = (unsigned long) map_2_ptr_fn(1, insn->imm, off, ud);
                if (addr == 0) {
                    RETURNI(BS_ERR, "pc=%d, opcode=0x%x, dst=%u, src=%u, off=%u, imm=%u",
                            i, insn->opcode, insn->dst_reg, insn->src_reg, insn->off, insn->imm);
                }

                addr += off;
			}

			insn[0].imm = (U32)addr;
			insn[1].imm = addr >> 32;

next_insn:
            insn++;
            i++;
            continue;
        }
    }

    return 0;
}

