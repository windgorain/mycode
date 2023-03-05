/*================================================================
*   Created by LiXingang, Copyright LiXingang
*   Date: 2017.10.2
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/ufd_utl.h"
#include "utl/umap_utl.h"
#include "utl/mybpf_loader.h"
#include "utl/mybpf_prog.h"
#include "utl/bpf_helper_utl.h"
#include "mybpf_osbase.h"
#include "mybpf_def.h"

int MYBPF_PROG_ReplaceMapFdWithMapPtr(MYBPF_RUNTIME_S *runtime, int *map_fds, MYBPF_LOADER_NODE_S *n)
{
    MYBPF_INSN_S *insn = n->insts;
    int insn_cnt = n->insts_len / sizeof(*insn);
    int i, j;
    int fd;
    int used_map_cnt = 0; 
    int used_maps[MYBPF_PROG_MAX_MAPS];

	for (i = 0; i < insn_cnt; i++, insn++) {
		if (BPF_CLASS(insn->opcode) == BPF_LDX &&
		    (BPF_MODE(insn->opcode) != BPF_MEM || insn->imm != 0)) {
            RETURNI(BS_ERR, "pc=%d, opcode=0x%x, dst=%u, src=%u, off=%u, imm=%u",
                    i, insn->opcode, insn->dst_reg, insn->src_reg, insn->off, insn->imm);
        }
        if (BPF_CLASS(insn->opcode) == BPF_STX &&
                ((BPF_MODE(insn->opcode) != BPF_MEM &&
                  BPF_MODE(insn->opcode) != BPF_XADD) || insn->imm != 0)) {
            RETURNI(BS_ERR, "pc=%d, opcode=0x%x, dst=%u, src=%u, off=%u, imm=%u",
                    i, insn->opcode, insn->dst_reg, insn->src_reg, insn->off, insn->imm);
        }
        if (insn[0].opcode == (BPF_LD | BPF_IMM | BPF_DW)) {
            UMAP_HEADER_S *map;
            U64 addr;

			if (i == insn_cnt - 1 || insn[1].opcode != 0 ||
			    insn[1].dst_reg != 0 || insn[1].src_reg != 0 ||
			    insn[1].off != 0) {
                RETURNI(BS_ERR, "pc=%d, opcode=0x%x, dst=%u, src=%u, off=%u, imm=%u",
                        i, insn->opcode, insn->dst_reg, insn->src_reg, insn->off, insn->imm);
            }

            if (insn[0].src_reg == 0)
                goto next_insn;

            /* In final convert_pseudo_ld_imm64() step, this is
             * converted into regular 64-bit imm load insn.
             */
            if ((insn[0].src_reg != BPF_PSEUDO_MAP_FD && insn[0].src_reg != BPF_PSEUDO_MAP_VALUE)
                    || (insn[0].src_reg == BPF_PSEUDO_MAP_FD && insn[1].imm != 0)) {
                RETURNI(BS_ERR, "pc=%d, opcode=0x%x, dst=%u, src=%u, off=%u, imm=%u",
                        i, insn->opcode, insn->dst_reg, insn->src_reg, insn->off, insn->imm);
			}

            fd = map_fds[insn->imm];

            map = UMAP_GetByFd(runtime->ufd_ctx, fd);
			if (! map) {
                RETURNI(BS_ERR, "pc=%d, opcode=0x%x, dst=%u, src=%u, off=%u, imm=%u",
                        i, insn->opcode, insn->dst_reg, insn->src_reg, insn->off, insn->imm);
			}

			if (insn->src_reg == BPF_PSEUDO_MAP_FD) {
				addr = (unsigned long)map;
			} else {
				U32 off = insn[1].imm;

				if (off >= BPF_MAX_VAR_OFF) {
                    RETURNI(BS_ERR, "pc=%d, opcode=0x%x, dst=%u, src=%u, off=%u, imm=%u",
                            i, insn->opcode, insn->dst_reg, insn->src_reg, insn->off, insn->imm);
                }

                int err = UMAP_DirectValue(map, &addr, off);
                if (err) {
                    return err;
                }

                addr += off;
			}

			insn[0].imm = (U32)addr;
			insn[1].imm = addr >> 32;

			/* check whether we recorded this map already */
			for (j = 0; j < used_map_cnt; j++) {
				if (used_maps[j] == fd) {
					goto next_insn;
				}
			}

			if (used_map_cnt >= MYBPF_PROG_MAX_MAPS) {
                RETURNI(BS_ERR, "pc=%d, opcode=0x%x, dst=%u, src=%u, off=%u, imm=%u",
                        i, insn->opcode, insn->dst_reg, insn->src_reg, insn->off, insn->imm);
            }

            used_maps[used_map_cnt++] = fd;

next_insn:
            insn++;
            i++;
            continue;
        }
    }

    return 0;
}

