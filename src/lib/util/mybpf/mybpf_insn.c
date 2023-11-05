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
#include "utl/mybpf_insn.h"
#include "utl/bpf_helper_utl.h"
#include "mybpf_osbase.h"
#include "mybpf_def_inner.h"

int MYBPF_INSN_GetCallsCount(void *insts, int insts_len )
{
    MYBPF_INSN_S *insn = insts;
    int insn_cnt = insts_len / sizeof(*insn);
    int i;
    int count = 0;

	for (i = 0; i < insn_cnt; i++, insn++) {
		if (insn->opcode != (BPF_JMP | BPF_CALL))
			continue;
        count ++;
    }

    return count;
}


int MYBPF_INSN_GetCallsInfo(void *insts, int insts_len, MYBPF_INSN_CALLS_S *calls, int max_count)
{
    MYBPF_INSN_S *insn = insts;
    int insn_cnt = insts_len / sizeof(*insn);
    int count = 0;
    int i;

	for (i = 0; i < insn_cnt; i++, insn++) {
		if (insn->opcode != (BPF_JMP | BPF_CALL))
			continue;
        if (count >= max_count) {
            RETURN(BS_OUT_OF_RANGE);
        }
		if (insn->src_reg == BPF_PSEUDO_CALL)
            calls[count].is_bpf_2_bpf = 1;
        calls[count].insn_index = i;
        calls[count].imm = insn->imm;
        count ++;
    }

    return count;
}

int MYBPF_INSN_WalkCalls(void *insts, int insts_len, PF_MYBPF_INSN_WALK walk_func, void *ud)
{
    MYBPF_INSN_S *insn = insts;
    int insn_cnt = insts_len / sizeof(*insn);
    int i;
    int ret;

	for (i = 0; i < insn_cnt; i++, insn++) {
		if (insn->opcode != (BPF_JMP | BPF_CALL))
			continue;
        ret = walk_func(insts, i, ud);
        if (ret < 0) {
            return ret;
        }
    }

    return 0;
}

int MYBPF_INSN_WalkExternCalls(void *insts, int insts_len, PF_MYBPF_INSN_WALK walk_func, void *ud)
{
    MYBPF_INSN_S *insn = insts;
    int insn_cnt = insts_len / sizeof(*insn);
    int i;
    int ret;

	for (i = 0; i < insn_cnt; i++, insn++) {
		if (insn->opcode != (BPF_JMP | BPF_CALL))
			continue;
		if (insn->src_reg == BPF_PSEUDO_CALL)
			continue;
        ret = walk_func(insts, i, ud);
        if (ret < 0) {
            return ret;
        }
    }

    return 0;
}


int MYBPF_INSN_WalkLddw(void *insts, int insts_len, PF_MYBPF_INSN_WALK walk_func, void *ud)
{
    MYBPF_INSN_S *insn = insts;
    int insn_cnt = insts_len / sizeof(*insn);
    int i;
    int ret;

	for (i = 0; i < insn_cnt; i++, insn++) {
		if (insn->opcode != (BPF_LD | BPF_IMM | BPF_DW))
			continue;
        ret = walk_func(insts, i, ud);
        if (ret < 0) {
            return ret;
        }
        i++;
        insn++;
    }

    return 0;
}


int MYBPF_INSN_FixupExtCalls(void *insts, int len, PF_MYBPF_INSN_EXTCALL_CONVERT convert_imm, void *ud)
{
    MYBPF_INSN_S *insn = insts;
    int insn_cnt = len / sizeof(*insn);
    int i;
    int new_imm;

	for (i = 0; i < insn_cnt; i++, insn++) {
		if (insn->opcode != (BPF_JMP | BPF_CALL))
			continue;
		if (insn->src_reg == BPF_PSEUDO_CALL)
			continue;

		new_imm = convert_imm(insn->imm, ud);
        if (! new_imm) {
            BS_WARNNING(("Not support func id %d", insn->imm));
            RETURN(BS_NOT_SUPPORT); 
        }
		insn->imm = new_imm;
    }

    return 0;
}


int MYBPF_INSN_GetStackSize(void *insts, int insts_len )
{
    int insn_cnt = insts_len / sizeof(MYBPF_INSN_S);
    MYBPF_INSN_S *insn = insts;
    MYBPF_INSN_S *end = insn + insn_cnt;
    int count = 0;
    int off;
    U8 class;

    for (; insn < end; insn++) {
        off = 0;
        class = BPF_CLASS(insn->opcode);

        if (((insn->opcode == (BPF_ALU64 | BPF_MOV | BPF_X)) && (insn->src_reg == 10)) && ((insn + 1) < end)) {
            if ((insn[1].opcode == (BPF_ALU64 | BPF_ADD | BPF_K)) && (insn[1].dst_reg == insn->dst_reg)) {
                off = -insn[1].imm;
            } else if ((insn[1].opcode == (BPF_ALU64 | BPF_SUB | BPF_K)) && (insn[1].dst_reg == insn->dst_reg)) {
                off = insn[1].imm;
            } else {
                continue;
            }
        } else if (((class == BPF_STX) || (class == BPF_ST)) && (insn->dst_reg == 10)) {
            off = -insn->off;
        } else if (((class == BPF_LDX) || (class == BPF_LD)) && (insn->src_reg == 10)) {
            off = -insn->off;
        } else {
            continue;
        }

        if (count < off) {
            count = off;
        }
    }

    
    return count;
}


int MYBPF_INSN_GetStackSizeUntilExit(void *insts)
{
    MYBPF_INSN_S *insn = insts;
    int count = 0;
    int off;

	for (; insn->opcode != 0x95; insn++) {
        if ((insn->opcode & 0x7) >= 4) {
            
            continue;
        }
        if (insn->dst_reg != 10) {
            
            continue;
        }

        off = -insn->off;
        if (count < off) {
            count = off;
        }
    }

    
    return count;
}

