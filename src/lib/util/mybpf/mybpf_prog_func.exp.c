#include "bs.h" 
#include "utl/mybpf_regs.h" 
#include "utl/mybpf_asmdef.h" 
#include "utl/bpfasm_utl.h" 

static MYBPF_INSN_S g_bpfasm_insts[] = { 
    /* _MYBPF_PROG_GetFirst */
    BPF_MOV64_REG(BPF_R9, BPF_R3), 
    BPF_MOV64_IMM(BPF_R7, 0), 
    BPF_STX_MEM(BPF_DW, BPF_R10, BPF_R2, -8), 
    BPF_JMP_IMM(BPF_JEQ, BPF_R2, 0, 51), 
    BPF_MOV64_IMM(BPF_R1, 1), 
    BPF_STX_MEM(BPF_DW, BPF_R10, BPF_R1, -16), 
    BPF_MOV64_IMM(BPF_R7, 0), 
    BPF_MOV64_IMM(BPF_R8, 0), 
    BPF_MOV64_IMM(BPF_R3, 0), 
    BPF_JMP_IMM(BPF_JEQ, BPF_R9, 0, 23), 
    BPF_MOV64_REG(BPF_R1, BPF_R9), 
    BPF_RAW_INSN(BPF_JMP | BPF_CALL, 0, 0, 0, 1000009), 
    BPF_MOV64_IMM(BPF_R8, 0), 
    BPF_MOV64_REG(BPF_R1, BPF_R0), 
    BPF_ALU64_IMM(BPF_LSH, BPF_R1, 32), 
    BPF_MOV64_REG(BPF_R2, BPF_R1), 
    BPF_ALU64_IMM(BPF_RSH, BPF_R2, 32), 
    BPF_MOV64_IMM(BPF_R3, 0), 
    BPF_JMP_IMM(BPF_JEQ, BPF_R2, 0, 14), 
    BPF_LD_IMM64_RAW(BPF_R2, BPF_R0, 0xffffffff00000000LL), 
    BPF_ALU64_REG(BPF_ADD, BPF_R1, BPF_R2), 
    BPF_ALU64_IMM(BPF_ARSH, BPF_R1, 32), 
    BPF_MOV64_REG(BPF_R2, BPF_R9), 
    BPF_ALU64_REG(BPF_ADD, BPF_R2, BPF_R1), 
    BPF_LDX_MEM(BPF_B, BPF_R1, BPF_R2, 0), 
    BPF_MOV64_REG(BPF_R8, BPF_R9), 
    BPF_MOV64_REG(BPF_R3, BPF_R0), 
    BPF_JMP_IMM(BPF_JNE, BPF_R1, 47, 4), 
    BPF_MOV64_IMM(BPF_R1, 0), 
    BPF_STX_MEM(BPF_DW, BPF_R10, BPF_R1, -16), 
    BPF_MOV64_REG(BPF_R8, BPF_R9), 
    BPF_MOV64_REG(BPF_R3, BPF_R0), 
    BPF_STX_MEM(BPF_DW, BPF_R10, BPF_R3, -24), 
    BPF_LDX_MEM(BPF_DW, BPF_R1, BPF_R10, -8), 
    BPF_LDX_MEM(BPF_W, BPF_R1, BPF_R1, 112), 
    BPF_ALU64_IMM(BPF_AND, BPF_R1, 65280), 
    BPF_JMP_IMM(BPF_JEQ, BPF_R1, 0, 17), 
    BPF_MOV64_IMM(BPF_R9, 0), 
    BPF_LDX_MEM(BPF_DW, BPF_R6, BPF_R10, -8), 
    BPF_ALU64_IMM(BPF_ADD, BPF_R6, 640), 
    BPF_JMP_A(15), 
    BPF_MOV64_REG(BPF_R1, BPF_R8), 
    BPF_RAW_INSN(BPF_JMP | BPF_CALL, 0, 0, 0, 1000011), 
    BPF_ALU64_IMM(BPF_LSH, BPF_R0, 32), 
    BPF_ALU64_IMM(BPF_RSH, BPF_R0, 32), 
    BPF_JMP_IMM(BPF_JEQ, BPF_R0, 0, 8), 
    BPF_MOV64_IMM(BPF_R7, 0), 
    BPF_ALU64_IMM(BPF_ADD, BPF_R6, 8), 
    BPF_ALU64_IMM(BPF_ADD, BPF_R9, 1), 
    BPF_LDX_MEM(BPF_DW, BPF_R1, BPF_R10, -8), 
    BPF_LDX_MEM(BPF_W, BPF_R1, BPF_R1, 112), 
    BPF_ALU64_IMM(BPF_RSH, BPF_R1, 8), 
    BPF_ALU64_IMM(BPF_AND, BPF_R1, 255), 
    BPF_JMP_REG(BPF_JGT, BPF_R1, BPF_R9, 2), 
    BPF_MOV64_REG(BPF_R0, BPF_R7), 
    BPF_EXIT_INSN(), 
    BPF_LDX_MEM(BPF_DW, BPF_R7, BPF_R6, 0), 
    BPF_JMP_IMM(BPF_JEQ, BPF_R8, 0, -4), 
    BPF_MOV64_REG(BPF_R2, BPF_R7), 
    BPF_ALU64_IMM(BPF_ADD, BPF_R2, 16), 
    BPF_LDX_MEM(BPF_DW, BPF_R1, BPF_R10, -16), 
    BPF_JMP_IMM(BPF_JNE, BPF_R1, 0, -21), 
    BPF_MOV64_REG(BPF_R1, BPF_R8), 
    BPF_LDX_MEM(BPF_DW, BPF_R3, BPF_R10, -24), 
    BPF_RAW_INSN(BPF_JMP | BPF_CALL, 0, 0, 0, 1000008), 
    BPF_ALU64_IMM(BPF_LSH, BPF_R0, 32), 
    BPF_ALU64_IMM(BPF_RSH, BPF_R0, 32), 
    BPF_JMP_IMM(BPF_JEQ, BPF_R0, 0, -14), 
    BPF_JMP_A(-23), 
    /* _MYBPF_PROG_GetNext */
    BPF_STX_MEM(BPF_DW, BPF_R10, BPF_R4, -24), 
    BPF_MOV64_REG(BPF_R6, BPF_R3), 
    BPF_STX_MEM(BPF_DW, BPF_R10, BPF_R2, -8), 
    BPF_MOV64_IMM(BPF_R1, 1), 
    BPF_STX_MEM(BPF_DW, BPF_R10, BPF_R1, -16), 
    BPF_MOV64_IMM(BPF_R9, 0), 
    BPF_MOV64_IMM(BPF_R8, 0), 
    BPF_MOV64_IMM(BPF_R3, 0), 
    BPF_JMP_IMM(BPF_JEQ, BPF_R6, 0, 23), 
    BPF_MOV64_REG(BPF_R1, BPF_R6), 
    BPF_RAW_INSN(BPF_JMP | BPF_CALL, 0, 0, 0, 1000009), 
    BPF_MOV64_IMM(BPF_R8, 0), 
    BPF_MOV64_REG(BPF_R1, BPF_R0), 
    BPF_ALU64_IMM(BPF_LSH, BPF_R1, 32), 
    BPF_MOV64_REG(BPF_R2, BPF_R1), 
    BPF_ALU64_IMM(BPF_RSH, BPF_R2, 32), 
    BPF_MOV64_IMM(BPF_R3, 0), 
    BPF_JMP_IMM(BPF_JEQ, BPF_R2, 0, 14), 
    BPF_LD_IMM64_RAW(BPF_R2, BPF_R0, 0xffffffff00000000LL), 
    BPF_ALU64_REG(BPF_ADD, BPF_R1, BPF_R2), 
    BPF_ALU64_IMM(BPF_ARSH, BPF_R1, 32), 
    BPF_MOV64_REG(BPF_R2, BPF_R6), 
    BPF_ALU64_REG(BPF_ADD, BPF_R2, BPF_R1), 
    BPF_LDX_MEM(BPF_B, BPF_R1, BPF_R2, 0), 
    BPF_MOV64_REG(BPF_R8, BPF_R6), 
    BPF_MOV64_REG(BPF_R3, BPF_R0), 
    BPF_JMP_IMM(BPF_JNE, BPF_R1, 47, 4), 
    BPF_MOV64_IMM(BPF_R1, 0), 
    BPF_STX_MEM(BPF_DW, BPF_R10, BPF_R1, -16), 
    BPF_MOV64_REG(BPF_R8, BPF_R6), 
    BPF_MOV64_REG(BPF_R3, BPF_R0), 
    BPF_STX_MEM(BPF_DW, BPF_R10, BPF_R3, -32), 
    BPF_LDX_MEM(BPF_DW, BPF_R1, BPF_R10, -8), 
    BPF_LDX_MEM(BPF_W, BPF_R1, BPF_R1, 112), 
    BPF_ALU64_IMM(BPF_AND, BPF_R1, 65280), 
    BPF_JMP_IMM(BPF_JEQ, BPF_R1, 0, 28), 
    BPF_MOV64_IMM(BPF_R6, 0), 
    BPF_LDX_MEM(BPF_DW, BPF_R7, BPF_R10, -8), 
    BPF_ALU64_IMM(BPF_ADD, BPF_R7, 640), 
    BPF_MOV64_IMM(BPF_R1, 0), 
    BPF_ALU64_IMM(BPF_LSH, BPF_R1, 32), 
    BPF_ALU64_IMM(BPF_RSH, BPF_R1, 32), 
    BPF_JMP_IMM(BPF_JEQ, BPF_R1, 0, 23), 
    BPF_LDX_MEM(BPF_DW, BPF_R9, BPF_R7, 0), 
    BPF_JMP_IMM(BPF_JEQ, BPF_R8, 0, 19), 
    BPF_MOV64_REG(BPF_R2, BPF_R9), 
    BPF_ALU64_IMM(BPF_ADD, BPF_R2, 16), 
    BPF_LDX_MEM(BPF_DW, BPF_R1, BPF_R10, -16), 
    BPF_JMP_IMM(BPF_JNE, BPF_R1, 0, 23), 
    BPF_MOV64_REG(BPF_R1, BPF_R8), 
    BPF_LDX_MEM(BPF_DW, BPF_R3, BPF_R10, -32), 
    BPF_RAW_INSN(BPF_JMP | BPF_CALL, 0, 0, 0, 1000008), 
    BPF_MOV64_IMM(BPF_R1, 1), 
    BPF_ALU64_IMM(BPF_LSH, BPF_R0, 32), 
    BPF_ALU64_IMM(BPF_RSH, BPF_R0, 32), 
    BPF_JMP_IMM(BPF_JEQ, BPF_R0, 0, 8), 
    BPF_MOV64_IMM(BPF_R9, 0), 
    BPF_ALU64_IMM(BPF_ADD, BPF_R7, 8), 
    BPF_ALU64_IMM(BPF_ADD, BPF_R6, 1), 
    BPF_LDX_MEM(BPF_DW, BPF_R2, BPF_R10, -8), 
    BPF_LDX_MEM(BPF_W, BPF_R2, BPF_R2, 112), 
    BPF_ALU64_IMM(BPF_RSH, BPF_R2, 8), 
    BPF_ALU64_IMM(BPF_AND, BPF_R2, 255), 
    BPF_JMP_REG(BPF_JGT, BPF_R2, BPF_R6, -24), 
    BPF_MOV64_REG(BPF_R0, BPF_R9), 
    BPF_EXIT_INSN(), 
    BPF_MOV64_IMM(BPF_R1, 0), 
    BPF_LDX_MEM(BPF_DW, BPF_R2, BPF_R7, 0), 
    BPF_LDX_MEM(BPF_DW, BPF_R3, BPF_R10, -24), 
    BPF_JMP_REG(BPF_JNE, BPF_R2, BPF_R3, -14), 
    BPF_MOV64_IMM(BPF_R1, 1), 
    BPF_JMP_A(-16), 
    BPF_MOV64_REG(BPF_R1, BPF_R8), 
    BPF_RAW_INSN(BPF_JMP | BPF_CALL, 0, 0, 0, 1000011), 
    BPF_MOV64_IMM(BPF_R1, 1), 
    BPF_ALU64_IMM(BPF_LSH, BPF_R0, 32), 
    BPF_ALU64_IMM(BPF_RSH, BPF_R0, 32), 
    BPF_JMP_IMM(BPF_JEQ, BPF_R0, 0, -14), 
    BPF_JMP_A(-23), 
}; 

U64 _MYBPF_PROG_GetFirst(U64 p1, U64 p2, U64 p3, U64 p4, U64 p5) 
{ 
    MYBPF_PARAM_S p; 
    MYBPF_CTX_S ctx = {0}; 

    ctx.begin_addr = g_bpfasm_insts; 
    ctx.end_addr = (char*)(void*)g_bpfasm_insts + sizeof(g_bpfasm_insts); 
    ctx.insts = (char*)g_bpfasm_insts; 
    p.p[0]=p1; p.p[1]=p2; p.p[2]=p3; p.p[3]=p4; p.p[4]=p5; 
    int ret = MYBPF_DefultRun(&ctx, &p); 
    if (ret < 0) return ret; 
    return ctx.bpf_ret; 
} 

U64 _MYBPF_PROG_GetNext(U64 p1, U64 p2, U64 p3, U64 p4, U64 p5) 
{ 
    MYBPF_PARAM_S p; 
    MYBPF_CTX_S ctx = {0}; 

    ctx.begin_addr = g_bpfasm_insts; 
    ctx.end_addr = (char*)(void*)g_bpfasm_insts + sizeof(g_bpfasm_insts); 
    ctx.insts = (char*)g_bpfasm_insts + 560; 
    p.p[0]=p1; p.p[1]=p2; p.p[2]=p3; p.p[3]=p4; p.p[4]=p5; 
    int ret = MYBPF_DefultRun(&ctx, &p); 
    if (ret < 0) return ret; 
    return ctx.bpf_ret; 
} 
