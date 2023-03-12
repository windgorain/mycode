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
#include "utl/mybpf_verifier.h"
#include "utl/bpf_helper_utl.h"
#include "mybpf_osbase.h"
#include "mybpf_def_inner.h"

static UINT _mybpf_prog_xdp_convert_ctx_access(BOOL_T is_write,
				  IN MYBPF_INSN_S *si,
				  OUT MYBPF_INSN_S *insn_buf,
				  OUT UINT *target_size)
{
	MYBPF_INSN_S *insn = insn_buf;

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

static void _mybpf_prog_init_reg_state(struct reg_state *regs)
{
    int i;

    for (i = 0; i < MAX_BPF_REG; i++) {
        regs[i].type = NOT_INIT;
        regs[i].imm = 0;
        regs[i].map_ptr = NULL;
    }

    regs[BPF_REG_FP].type = PTR_TO_STACK;
    regs[BPF_REG_1].type = PTR_TO_CTX;
}

static void _mybpf_prog_mark_regs_alu(struct reg_state *regs, MYBPF_INSN_S *insn)
{
    UCHAR op = BPF_OP(insn->opcode);
    UCHAR src = BPF_SRC(insn->opcode);

    if ((op == BPF_MOV) && (src == BPF_X)) {
        regs[insn->dst_reg].type = regs[insn->src_reg].type;
    } else {
        regs[insn->dst_reg].type = SCALAR_VALUE;
    }
}

/* 标记寄存器type */
static void _mybpf_prog_mark_regs(struct reg_state *regs, MYBPF_INSN_S *insn)
{
    UCHAR class = BPF_CLASS(insn->opcode);

    switch (class) {
        case BPF_ALU:
        case BPF_ALU64:
            _mybpf_prog_mark_regs_alu(regs, insn);
            break;
        case BPF_LD:
        case BPF_LDX:
            regs[insn->dst_reg].type = SCALAR_VALUE;
            break;
    }
}

/* 是否dword指令 */
static BOOL_T _mybpf_prog_is_dword_insn(MYBPF_INSN_S *insn)
{
    UCHAR class = BPF_CLASS(insn->opcode);

    if ((class != BPF_LD) && (class != BPF_ST)) {
        return FALSE;
    }

    UCHAR size = BPF_SIZE(insn->opcode);
    if (size != BPF_DW) {
        return FALSE;
    }

    return TRUE;
}

static void _mybpf_prog_convert_ctx_access(MYBPF_INSN_S *insn, struct reg_state *regs)
{
    BOOL_T is_write;
    UINT target_size;

    if (regs[insn->src_reg].type != PTR_TO_CTX) {
        return;
    }

    if (insn->opcode == (BPF_LDX | BPF_MEM | BPF_B) ||
            insn->opcode == (BPF_LDX | BPF_MEM | BPF_H) ||
            insn->opcode == (BPF_LDX | BPF_MEM | BPF_W) ||
            insn->opcode == (BPF_LDX | BPF_MEM | BPF_DW)) {
        is_write = 0;
    } else if (insn->opcode == (BPF_STX | BPF_MEM | BPF_B) ||
            insn->opcode == (BPF_STX | BPF_MEM | BPF_H) ||
            insn->opcode == (BPF_STX | BPF_MEM | BPF_W) ||
            insn->opcode == (BPF_STX | BPF_MEM | BPF_DW)) {
        is_write = 1;
    } else {
        return;
    }

    _mybpf_prog_xdp_convert_ctx_access(is_write, insn, insn, &target_size);
}

int MYBPF_PROG_ConvertCtxAccess(MYBPF_PROG_NODE_S *prog)
{
    MYBPF_INSN_S *insn = prog->insn;
    int insn_cnt = prog->insn_len / sizeof(*insn);
    int i;
    struct reg_state regs[MAX_BPF_REG];

    _mybpf_prog_init_reg_state(regs);

	for (i = 0; i < insn_cnt; i++, insn++) {
        _mybpf_prog_mark_regs(regs, insn);

        _mybpf_prog_convert_ctx_access(insn, regs);

        if (_mybpf_prog_is_dword_insn(insn)) {
            i++; insn++;
            continue;
        }
    }

    return 0;
}

