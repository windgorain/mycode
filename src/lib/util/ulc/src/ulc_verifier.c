/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/ulc_utl.h"
#include "../h/ulc_def.h"
#include "../h/ulc_map.h"
#include "../h/ulc_prog.h"
#include "../h/ulc_fd.h"
#include "../h/ulc_hookpoint.h"
#include "../h/ulc_osbase.h"
#include "../h/ulc_base_helpers.h"

static UINT _ulc_verifier_xdp_convert_ctx_access(BOOL_T is_write,
				  IN struct bpf_insn *si,
				  OUT struct bpf_insn *insn_buf,
				  OUT UINT *target_size)
{
	struct bpf_insn *insn = insn_buf;

	switch (si->off) {
	case offsetof(struct xdp_md, data):
		*insn++ = BPF_LDX_MEM(BPF_FIELD_SIZEOF(struct xdp_buff, data),
				      si->dst_reg, si->src_reg,
				      offsetof(struct xdp_buff, data));
		break;
	case offsetof(struct xdp_md, data_meta):
		*insn++ = BPF_LDX_MEM(BPF_FIELD_SIZEOF(struct xdp_buff, data_meta),
				      si->dst_reg, si->src_reg,
				      offsetof(struct xdp_buff, data_meta));
		break;
	case offsetof(struct xdp_md, data_end):
		*insn++ = BPF_LDX_MEM(BPF_FIELD_SIZEOF(struct xdp_buff, data_end),
				      si->dst_reg, si->src_reg,
				      offsetof(struct xdp_buff, data_end));
		break;
	case offsetof(struct xdp_md, ingress_ifindex):
		*insn++ = BPF_LDX_MEM(BPF_FIELD_SIZEOF(struct xdp_buff, ingress_ifindex),
				      si->dst_reg, si->src_reg,
				      offsetof(struct xdp_buff, ingress_ifindex));
		break;
	case offsetof(struct xdp_md, rx_queue_index):
		*insn++ = BPF_LDX_MEM(BPF_FIELD_SIZEOF(struct xdp_buff, rx_queue_index),
				      si->dst_reg, si->src_reg,
				      offsetof(struct xdp_buff, rx_queue_index));
		break;
	}

	return insn - insn_buf;
}


int ULC_VERIFIER_ReplaceMapFdWithMapPtr(ULC_PROG_NODE_S *prog)
{
    struct bpf_insn *insn = (void*)prog->insn;
    int insn_cnt = prog->insn_len / sizeof(*insn);
    int i, j;
    int fd;

	for (i = 0; i < insn_cnt; i++, insn++) {
		if (BPF_CLASS(insn->code) == BPF_LDX &&
		    (BPF_MODE(insn->code) != BPF_MEM || insn->imm != 0)) {
			return -EINVAL;
		}
		if (BPF_CLASS(insn->code) == BPF_STX &&
		    ((BPF_MODE(insn->code) != BPF_MEM &&
		      BPF_MODE(insn->code) != BPF_XADD) || insn->imm != 0)) {
			return -EINVAL;
		}
		if (insn[0].code == (BPF_LD | BPF_IMM | BPF_DW)) {
            ULC_MAP_HEADER_S *map;
            u64 addr;

			if (i == insn_cnt - 1 || insn[1].code != 0 ||
			    insn[1].dst_reg != 0 || insn[1].src_reg != 0 ||
			    insn[1].off != 0) {
				return -EINVAL;
			}

			if (insn[0].src_reg == 0)
				goto next_insn;

			/* In final convert_pseudo_ld_imm64() step, this is
			 * converted into regular 64-bit imm load insn.
			 */
			if ((insn[0].src_reg != BPF_PSEUDO_MAP_FD &&
			     insn[0].src_reg != BPF_PSEUDO_MAP_VALUE) ||
			    (insn[0].src_reg == BPF_PSEUDO_MAP_FD &&
			     insn[1].imm != 0)) {
				return -EINVAL;
			}

            fd = insn->imm;

            map = ULC_MAP_RefByFd(fd);
			if (! map) {
				return -EINVAL;
			}

			if (insn->src_reg == BPF_PSEUDO_MAP_FD) {
				addr = (unsigned long)map;
			} else {
				u32 off = insn[1].imm;

				if (off >= BPF_MAX_VAR_OFF) {
                    ULC_FD_DecRef(fd);
					return -EINVAL;
				}

				int err = ULC_MAP_DirectValue(map, &addr, off);
				if (err) {
                    ULC_FD_DecRef(fd);
					return err;
				}

				addr += off;
			}

			insn[0].imm = (u32)addr;
			insn[1].imm = addr >> 32;

			/* check whether we recorded this map already */
			for (j = 0; j < prog->used_map_cnt; j++) {
				if (prog->used_maps[j] == fd) {
                    ULC_FD_DecRef(fd);
					goto next_insn;
				}
			}

			if (prog->used_map_cnt >= MAX_USED_MAPS) {
                ULC_FD_DecRef(fd);
                return -E2BIG;
			}

			prog->used_maps[prog->used_map_cnt++] = fd;

next_insn:
			insn++;
			i++;
			continue;
		}
    }

    return 0;
}

static void _ulc_verifier_init_reg_state(struct reg_state *regs)
{
    int i;

    for (i = 0; i < MAX_BPF_REG; i++) {
        regs[i].type = NOT_INIT;
        regs[i].imm = 0;
        regs[i].map_ptr = NULL;
    }

    regs[BPF_REG_FP].type = FRAME_PTR;
    regs[BPF_REG_1].type = PTR_TO_CTX;
}

static void _ulc_verifier_mark_regs_alu(struct reg_state *regs, struct bpf_insn *insn)
{
    UCHAR op = BPF_OP(insn->code);
    UCHAR src = BPF_SRC(insn->code);

    if ((op == BPF_MOV) && (src == BPF_X)) {
        regs[insn->dst_reg].type = regs[insn->src_reg].type;
    } else {
        regs[insn->dst_reg].type = UNKNOWN_VALUE;
    }
}

/* 标记寄存器type */
static void _ulc_verifier_mark_regs(struct reg_state *regs, struct bpf_insn *insn)
{
    UCHAR class = BPF_CLASS(insn->code);

    switch (class) {
        case BPF_ALU:
        case BPF_ALU64:
            _ulc_verifier_mark_regs_alu(regs, insn);
            break;
        case BPF_LD:
        case BPF_LDX:
            regs[insn->dst_reg].type = UNKNOWN_VALUE;
            break;
    }
}

/* 是否dword指令 */
static BOOL_T _ulc_verifier_is_dword_insn(struct bpf_insn *insn)
{
    UCHAR class = BPF_CLASS(insn->code);

    if ((class != BPF_LD) && (class != BPF_ST)) {
        return FALSE;
    }

    UCHAR size = BPF_SIZE(insn->code);
    if (size != BPF_DW) {
        return FALSE;
    }

    return TRUE;
}

static void _ulc_verifier_convert_ctx_access(ULC_PROG_NODE_S *prog, struct bpf_insn *insn, struct reg_state *regs)
{
    BOOL_T is_write;
    UINT target_size;

    if (regs[insn->src_reg].type != PTR_TO_CTX) {
        return;
    }

    if (insn->code == (BPF_LDX | BPF_MEM | BPF_B) ||
            insn->code == (BPF_LDX | BPF_MEM | BPF_H) ||
            insn->code == (BPF_LDX | BPF_MEM | BPF_W) ||
            insn->code == (BPF_LDX | BPF_MEM | BPF_DW)) {
        is_write = 0;
    } else if (insn->code == (BPF_STX | BPF_MEM | BPF_B) ||
            insn->code == (BPF_STX | BPF_MEM | BPF_H) ||
            insn->code == (BPF_STX | BPF_MEM | BPF_W) ||
            insn->code == (BPF_STX | BPF_MEM | BPF_DW)) {
        is_write = 1;
    } else {
        return;
    }

    _ulc_verifier_xdp_convert_ctx_access(is_write, insn, insn, &target_size);
}

int ULC_VERIFIER_ConvertCtxAccess(ULC_PROG_NODE_S *prog)
{
    struct bpf_insn *insn = (void*)prog->insn;
    int insn_cnt = prog->insn_len / sizeof(*insn);
    int i;
    struct reg_state regs[MAX_BPF_REG];

    _ulc_verifier_init_reg_state(regs);

	for (i = 0; i < insn_cnt; i++, insn++) {
        _ulc_verifier_mark_regs(regs, insn);

        _ulc_verifier_convert_ctx_access(prog, insn, regs);

        if (_ulc_verifier_is_dword_insn(insn)) {
            i++; insn++;
            continue;
        }
    }

    return 0;
}

int ULC_VERIFIER_FixupBpfCalls(ULC_PROG_NODE_S *prog)
{
    struct bpf_insn *insn = (void*)prog->insn;
    int insn_cnt = prog->insn_len / sizeof(struct bpf_insn);
    int i;
    int new_imm;

	for (i = 0; i < insn_cnt; i++, insn++) {
		if (insn->code != (BPF_JMP | BPF_CALL))
			continue;
		if (insn->src_reg == BPF_PSEUDO_CALL)
			continue;

		new_imm = ULC_BaseHelp_GetFunc(insn->imm);
        if (! new_imm) {
            BS_WARNNING(("Not support"));
            RETURN(BS_NOT_SUPPORT); 
        }
		insn->imm = new_imm;
    }

    return 0;
}

