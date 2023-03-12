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

int MYBPF_INSN_GetCallsCount(void *insts, int insts_len /* 字节数 */)
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

/* 返回获取了多少个calls,失败返回<0 */
int MYBPF_INSN_GetCallsInfo(void *insts, int insts_len/* 字节数 */, MYBPF_INSN_CALLS_S *calls, int max_count)
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

/* fixup extern calls
 * len: 字节数 */
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

/* 获取使用了多少栈空间 */
int MYBPF_INSN_GetStackSize(void *insts, int insts_len /* 字节数 */)
{
    MYBPF_INSN_S *insn = insts;
    int insn_cnt = insts_len / sizeof(*insn);
    int i;
    int count = 0;
    int off;

	for (i = 0; i < insn_cnt; i++, insn++) {
        if ((insn->opcode & 0x7) >= 4) {
            /* 非访存指令 */
            continue;
        }
        if (insn->dst_reg != 10) {
            /* 非BP操作 */
            continue;
        }

        off = -insn->off;
        if (count < off) {
            count = off;
        }
    }

    /* 返回占用了多少字节的栈空间 */
    return count;
}

/* 获取使用了多少栈空间, 扫描到exit指令截止 */
int MYBPF_INSN_GetStackSizeUntilExit(void *insts)
{
    MYBPF_INSN_S *insn = insts;
    int count = 0;
    int off;

	for (; insn->opcode != 0x95; insn++) {
        if ((insn->opcode & 0x7) >= 4) {
            /* 非访存指令 */
            continue;
        }
        if (insn->dst_reg != 10) {
            /* 非BP操作 */
            continue;
        }

        off = -insn->off;
        if (count < off) {
            count = off;
        }
    }

    /* 返回占用了多少字节的栈空间 */
    return count;
}

