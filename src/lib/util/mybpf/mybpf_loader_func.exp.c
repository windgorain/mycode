#include "bs.h" 
#include "utl/mybpf_regs.h" 
#include "utl/mybpf_asmdef.h" 
#include "utl/bpfasm_utl.h" 

static MYBPF_INSN_S g_bpfasm_insts[] = { 

    BPF_MOV64_REG(BPF_R6, BPF_R1), 
    BPF_LDX_MEM(BPF_W, BPF_R1, BPF_R2, 128), 
    BPF_ALU64_IMM(BPF_RSH, BPF_R1, 16), 
    BPF_STX_MEM(BPF_B, BPF_R6, BPF_R1, 130), 
    BPF_LDX_MEM(BPF_W, BPF_R1, BPF_R2, 132), 
    BPF_STX_MEM(BPF_W, BPF_R6, BPF_R1, 132), 
    BPF_ALU64_IMM(BPF_ADD, BPF_R2, 136), 
    BPF_MOV64_REG(BPF_R7, BPF_R6), 
    BPF_ALU64_IMM(BPF_ADD, BPF_R7, 136), 
    BPF_MOV64_REG(BPF_R1, BPF_R7), 
    BPF_MOV64_IMM(BPF_R3, 256), 
    BPF_RAW_INSN(BPF_JMP | BPF_CALL, 0, 0, 0, 1000040), 
    BPF_LDX_MEM(BPF_W, BPF_R1, BPF_R6, 128), 
    BPF_ALU64_IMM(BPF_AND, BPF_R1, 16711680), 
    BPF_JMP_IMM(BPF_JEQ, BPF_R1, 0, 11), 
    BPF_MOV64_IMM(BPF_R1, 0), 
    BPF_LDX_MEM(BPF_DW, BPF_R2, BPF_R7, 0), 
    BPF_LDX_MEM(BPF_W, BPF_R3, BPF_R2, 128), 
    BPF_ALU64_IMM(BPF_ADD, BPF_R3, 1), 
    BPF_STX_MEM(BPF_W, BPF_R2, BPF_R3, 128), 
    BPF_ALU64_IMM(BPF_ADD, BPF_R7, 8), 
    BPF_ALU64_IMM(BPF_ADD, BPF_R1, 1), 
    BPF_LDX_MEM(BPF_W, BPF_R2, BPF_R6, 128), 
    BPF_ALU64_IMM(BPF_RSH, BPF_R2, 16), 
    BPF_ALU64_IMM(BPF_AND, BPF_R2, 255), 
    BPF_JMP_REG(BPF_JGT, BPF_R2, BPF_R1, -10), 
    BPF_EXIT_INSN(), 

    BPF_MOV64_REG(BPF_R8, BPF_R2), 
    BPF_MOV64_IMM(BPF_R1, 120), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -27), 
    BPF_MOV64_IMM(BPF_R1, 101), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -28), 
    BPF_MOV64_IMM(BPF_R1, 116), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -26), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -29), 
    BPF_MOV64_IMM(BPF_R1, 46), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -30), 
    BPF_MOV64_IMM(BPF_R0, 0), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R0, -25), 
    BPF_LDX_MEM(BPF_W, BPF_R1, BPF_R8, 912), 
    BPF_ALU64_IMM(BPF_LSH, BPF_R1, 32), 
    BPF_ALU64_IMM(BPF_ARSH, BPF_R1, 32), 
    BPF_MOV64_IMM(BPF_R2, 1), 
    BPF_JMP_REG(BPF_JSGT, BPF_R2, BPF_R1, 32), 
    BPF_MOV64_IMM(BPF_R9, 0), 
    BPF_MOV64_IMM(BPF_R7, 0), 
    BPF_STX_MEM(BPF_DW, BPF_R10, BPF_R8, -80), 
    BPF_JMP_A(29), 
    BPF_MOV64_REG(BPF_R1, BPF_R8), 
    BPF_ALU64_IMM(BPF_ADD, BPF_R1, 144), 
    BPF_LDX_MEM(BPF_DW, BPF_R2, BPF_R10, -48), 
    BPF_MOV64_IMM(BPF_R3, 64), 
    BPF_RAW_INSN(BPF_JMP | BPF_CALL, 0, 0, 0, 1000012), 
    BPF_STX_MEM(BPF_DW, BPF_R8, BPF_R6, 224), 
    BPF_LDX_MEM(BPF_DW, BPF_R1, BPF_R10, -64), 
    BPF_STX_MEM(BPF_W, BPF_R8, BPF_R1, 220), 
    BPF_LDX_MEM(BPF_DW, BPF_R2, BPF_R10, -80), 
    BPF_STX_MEM(BPF_DW, BPF_R8, BPF_R2, 208), 
    BPF_ALU64_IMM(BPF_LSH, BPF_R7, 3), 
    BPF_MOV64_REG(BPF_R1, BPF_R2), 
    BPF_ALU64_REG(BPF_ADD, BPF_R1, BPF_R7), 
    BPF_STX_MEM(BPF_DW, BPF_R1, BPF_R8, 656), 
    BPF_MOV64_REG(BPF_R8, BPF_R2), 
    BPF_LDX_MEM(BPF_W, BPF_R1, BPF_R8, 128), 
    BPF_ALU64_IMM(BPF_ADD, BPF_R1, 256), 
    BPF_ALU64_IMM(BPF_RSH, BPF_R1, 8), 
    BPF_STX_MEM(BPF_B, BPF_R8, BPF_R1, 129), 
    BPF_LDX_MEM(BPF_DW, BPF_R9, BPF_R10, -56), 
    BPF_LDX_MEM(BPF_DW, BPF_R7, BPF_R10, -40), 
    BPF_MOV64_IMM(BPF_R0, 0), 
    BPF_ALU64_IMM(BPF_ADD, BPF_R9, 32), 
    BPF_ALU64_IMM(BPF_ADD, BPF_R7, 1), 
    BPF_LDX_MEM(BPF_W, BPF_R1, BPF_R8, 912), 
    BPF_ALU64_IMM(BPF_LSH, BPF_R1, 32), 
    BPF_ALU64_IMM(BPF_ARSH, BPF_R1, 32), 
    BPF_JMP_REG(BPF_JSGT, BPF_R1, BPF_R7, 1), 
    BPF_EXIT_INSN(), 
    BPF_LDX_MEM(BPF_DW, BPF_R6, BPF_R8, 928), 
    BPF_ALU64_REG(BPF_ADD, BPF_R6, BPF_R9), 
    BPF_LDX_MEM(BPF_DW, BPF_R1, BPF_R6, 0), 
    BPF_MOV64_REG(BPF_R2, BPF_R10), 
    BPF_ALU64_IMM(BPF_ADD, BPF_R2, -30), 
    BPF_RAW_INSN(BPF_JMP | BPF_CALL, 0, 0, 0, 1000011), 
    BPF_ALU64_IMM(BPF_LSH, BPF_R0, 32), 
    BPF_ALU64_IMM(BPF_RSH, BPF_R0, 32), 
    BPF_JMP_IMM(BPF_JEQ, BPF_R0, 0, -17), 
    BPF_LDX_MEM(BPF_DW, BPF_R2, BPF_R6, 8), 
    BPF_JMP_IMM(BPF_JNE, BPF_R2, 0, 43), 
    BPF_MOV64_IMM(BPF_R1, 0), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -1), 
    BPF_MOV64_IMM(BPF_R1, 109), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -3), 
    BPF_MOV64_IMM(BPF_R1, 111), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -8), 
    BPF_MOV64_IMM(BPF_R1, 105), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -9), 
    BPF_MOV64_IMM(BPF_R1, 99), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -11), 
    BPF_MOV64_IMM(BPF_R1, 117), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -13), 
    BPF_MOV64_IMM(BPF_R1, 102), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -14), 
    BPF_MOV64_IMM(BPF_R1, 101), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -2), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -17), 
    BPF_MOV64_IMM(BPF_R1, 103), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -18), 
    BPF_MOV64_IMM(BPF_R1, 32), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -6), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -15), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -19), 
    BPF_MOV64_IMM(BPF_R1, 116), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -10), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -16), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -20), 
    BPF_MOV64_IMM(BPF_R1, 39), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -21), 
    BPF_MOV64_IMM(BPF_R1, 110), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -5), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -7), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -12), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -22), 
    BPF_MOV64_IMM(BPF_R1, 97), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -4), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -23), 
    BPF_MOV64_IMM(BPF_R1, 67), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -24), 
    BPF_MOV64_REG(BPF_R1, BPF_R10), 
    BPF_ALU64_IMM(BPF_ADD, BPF_R1, -24), 
    BPF_MOV64_IMM(BPF_R2, 24), 
    BPF_JMP_A(72), 
    BPF_STX_MEM(BPF_DW, BPF_R10, BPF_R7, -40), 
    BPF_LDX_MEM(BPF_W, BPF_R7, BPF_R8, 128), 
    BPF_ALU64_IMM(BPF_RSH, BPF_R7, 8), 
    BPF_ALU64_IMM(BPF_AND, BPF_R7, 255), 
    BPF_MOV64_IMM(BPF_R1, 32), 
    BPF_JMP_REG(BPF_JGT, BPF_R1, BPF_R7, 42), 
    BPF_MOV64_IMM(BPF_R1, 0), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -3), 
    BPF_MOV64_IMM(BPF_R1, 98), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -6), 
    BPF_MOV64_IMM(BPF_R1, 117), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -8), 
    BPF_MOV64_IMM(BPF_R1, 110), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -9), 
    BPF_MOV64_IMM(BPF_R1, 120), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -11), 
    BPF_MOV64_IMM(BPF_R1, 109), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -7), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -13), 
    BPF_MOV64_IMM(BPF_R1, 103), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -15), 
    BPF_MOV64_IMM(BPF_R1, 111), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -16), 
    BPF_MOV64_IMM(BPF_R1, 114), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -4), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -17), 
    BPF_MOV64_IMM(BPF_R1, 112), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -18), 
    BPF_MOV64_IMM(BPF_R1, 32), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -10), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -14), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -19), 
    BPF_MOV64_IMM(BPF_R1, 104), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -20), 
    BPF_MOV64_IMM(BPF_R1, 99), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -21), 
    BPF_MOV64_IMM(BPF_R1, 97), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -12), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -22), 
    BPF_MOV64_IMM(BPF_R1, 101), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -5), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -23), 
    BPF_MOV64_IMM(BPF_R1, 82), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -24), 
    BPF_MOV64_REG(BPF_R1, BPF_R10), 
    BPF_ALU64_IMM(BPF_ADD, BPF_R1, -24), 
    BPF_MOV64_IMM(BPF_R2, 22), 
    BPF_JMP_A(24), 
    BPF_STX_MEM(BPF_DW, BPF_R10, BPF_R2, -48), 
    BPF_STX_MEM(BPF_DW, BPF_R10, BPF_R9, -56), 
    BPF_LDX_MEM(BPF_DW, BPF_R1, BPF_R6, 0), 
    BPF_STX_MEM(BPF_DW, BPF_R10, BPF_R1, -72), 
    BPF_LDX_MEM(BPF_W, BPF_R1, BPF_R6, 24), 
    BPF_STX_MEM(BPF_DW, BPF_R10, BPF_R1, -64), 
    BPF_LDX_MEM(BPF_W, BPF_R9, BPF_R6, 20), 
    BPF_LDX_MEM(BPF_DW, BPF_R6, BPF_R8, 944), 
    BPF_MOV64_IMM(BPF_R1, 232), 
    BPF_RAW_INSN(BPF_JMP | BPF_CALL, 0, 0, 0, 1000005), 
    BPF_MOV64_REG(BPF_R8, BPF_R0), 
    BPF_JMP_IMM(BPF_JEQ, BPF_R8, 0, 16), 
    BPF_ALU64_REG(BPF_ADD, BPF_R6, BPF_R9), 
    BPF_MOV64_REG(BPF_R1, BPF_R8), 
    BPF_MOV64_IMM(BPF_R2, 0), 
    BPF_MOV64_IMM(BPF_R3, 232), 
    BPF_RAW_INSN(BPF_JMP | BPF_CALL, 0, 0, 0, 1000041), 
    BPF_LDX_MEM(BPF_DW, BPF_R2, BPF_R10, -72), 
    BPF_JMP_IMM(BPF_JEQ, BPF_R2, 0, -150), 
    BPF_MOV64_REG(BPF_R1, BPF_R8), 
    BPF_ALU64_IMM(BPF_ADD, BPF_R1, 16), 
    BPF_MOV64_IMM(BPF_R3, 128), 
    BPF_RAW_INSN(BPF_JMP | BPF_CALL, 0, 0, 0, 1000012), 
    BPF_JMP_A(-155), 
    BPF_RAW_INSN(BPF_JMP | BPF_CALL, 0, 0, 0, 6), 
    BPF_LD_IMM64_RAW(BPF_R0, BPF_R0, 0xffffffffLL), 
    BPF_JMP_A(-131), 
    BPF_MOV64_IMM(BPF_R1, 0), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -5), 
    BPF_MOV64_IMM(BPF_R1, 10), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -6), 
    BPF_MOV64_IMM(BPF_R1, 13), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -7), 
    BPF_MOV64_IMM(BPF_R1, 58), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -11), 
    BPF_MOV64_IMM(BPF_R1, 115), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -9), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -12), 
    BPF_MOV64_IMM(BPF_R1, 37), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -10), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -13), 
    BPF_MOV64_IMM(BPF_R1, 100), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -15), 
    BPF_MOV64_IMM(BPF_R1, 111), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -17), 
    BPF_MOV64_IMM(BPF_R1, 108), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -18), 
    BPF_MOV64_IMM(BPF_R1, 32), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -8), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -14), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -19), 
    BPF_MOV64_IMM(BPF_R1, 116), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -20), 
    BPF_MOV64_IMM(BPF_R1, 39), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -21), 
    BPF_MOV64_IMM(BPF_R1, 110), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -22), 
    BPF_MOV64_IMM(BPF_R1, 97), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -16), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -23), 
    BPF_MOV64_IMM(BPF_R1, 67), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -24), 
    BPF_LDX_MEM(BPF_DW, BPF_R1, BPF_R10, -80), 
    BPF_LDX_MEM(BPF_DW, BPF_R3, BPF_R1, 8), 
    BPF_MOV64_REG(BPF_R1, BPF_R10), 
    BPF_ALU64_IMM(BPF_ADD, BPF_R1, -24), 
    BPF_MOV64_IMM(BPF_R2, 20), 
    BPF_LDX_MEM(BPF_DW, BPF_R4, BPF_R10, -48), 
    BPF_RAW_INSN(BPF_JMP | BPF_CALL, 0, 0, 0, 6), 
    BPF_LD_IMM64_RAW(BPF_R0, BPF_R0, 0xffffffffLL), 
    BPF_JMP_A(-176), 

    BPF_MOV64_IMM(BPF_R0, 0), 
    BPF_LDX_MEM(BPF_W, BPF_R1, BPF_R2, 8), 
    BPF_LDX_MEM(BPF_W, BPF_R4, BPF_R3, 128), 
    BPF_ALU64_IMM(BPF_RSH, BPF_R4, 16), 
    BPF_ALU64_IMM(BPF_AND, BPF_R4, 255), 
    BPF_JMP_REG(BPF_JNE, BPF_R1, BPF_R4, 34), 
    BPF_LDX_MEM(BPF_W, BPF_R4, BPF_R2, 4), 
    BPF_LDX_MEM(BPF_W, BPF_R5, BPF_R3, 132), 
    BPF_JMP_REG(BPF_JNE, BPF_R4, BPF_R5, 31), 
    BPF_MOV64_IMM(BPF_R0, 1), 
    BPF_JMP_IMM(BPF_JEQ, BPF_R1, 0, 29), 
    BPF_MOV64_IMM(BPF_R5, 0), 
    BPF_ALU64_IMM(BPF_ADD, BPF_R3, 136), 
    BPF_LDX_MEM(BPF_DW, BPF_R2, BPF_R2, 16), 
    BPF_ALU64_IMM(BPF_ADD, BPF_R2, 8), 
    BPF_ALU64_IMM(BPF_LSH, BPF_R4, 32), 
    BPF_ALU64_IMM(BPF_ARSH, BPF_R4, 32), 
    BPF_LDX_MEM(BPF_DW, BPF_R6, BPF_R3, 0), 
    BPF_MOV64_IMM(BPF_R0, 0), 
    BPF_JMP_IMM(BPF_JEQ, BPF_R6, 0, 20), 
    BPF_LDX_MEM(BPF_W, BPF_R7, BPF_R6, 132), 
    BPF_LDX_MEM(BPF_W, BPF_R8, BPF_R2, -8), 
    BPF_JMP_REG(BPF_JNE, BPF_R8, BPF_R7, 17), 
    BPF_LDX_MEM(BPF_W, BPF_R7, BPF_R6, 144), 
    BPF_LDX_MEM(BPF_W, BPF_R8, BPF_R2, 4), 
    BPF_JMP_REG(BPF_JNE, BPF_R8, BPF_R7, 14), 
    BPF_LDX_MEM(BPF_W, BPF_R7, BPF_R6, 148), 
    BPF_LDX_MEM(BPF_W, BPF_R8, BPF_R2, 8), 
    BPF_JMP_REG(BPF_JNE, BPF_R8, BPF_R7, 11), 
    BPF_LDX_MEM(BPF_W, BPF_R7, BPF_R6, 136), 
    BPF_LDX_MEM(BPF_W, BPF_R8, BPF_R2, -4), 
    BPF_JMP_REG(BPF_JNE, BPF_R8, BPF_R7, 8), 
    BPF_LDX_MEM(BPF_W, BPF_R6, BPF_R6, 140), 
    BPF_LDX_MEM(BPF_W, BPF_R7, BPF_R2, 0), 
    BPF_JMP_REG(BPF_JNE, BPF_R7, BPF_R6, 5), 
    BPF_ALU64_REG(BPF_ADD, BPF_R2, BPF_R4), 
    BPF_ALU64_IMM(BPF_ADD, BPF_R3, 8), 
    BPF_ALU64_IMM(BPF_ADD, BPF_R5, 1), 
    BPF_MOV64_IMM(BPF_R0, 1), 
    BPF_JMP_REG(BPF_JGT, BPF_R1, BPF_R5, -23), 
    BPF_EXIT_INSN(), 

    BPF_MOV64_REG(BPF_R8, BPF_R3), 
    BPF_MOV64_REG(BPF_R9, BPF_R2), 
    BPF_MOV64_REG(BPF_R7, BPF_R1), 
    BPF_MOV64_REG(BPF_R2, BPF_R8), 
    BPF_ALU64_IMM(BPF_LSH, BPF_R2, 32), 
    BPF_ALU64_IMM(BPF_ARSH, BPF_R2, 32), 
    BPF_ALU64_IMM(BPF_ADD, BPF_R2, 8), 
    BPF_LD_IMM64_RAW(BPF_R6, BPF_R0, 0xffffffffLL), 
    BPF_MOV64_IMM(BPF_R1, 0), 
    BPF_LD_IMM64_RAW(BPF_R3, BPF_R0, 0x300000022LL), 
    BPF_LD_IMM64_RAW(BPF_R4, BPF_R0, 0xffffffffLL), 
    BPF_MOV64_IMM(BPF_R5, 0), 
    BPF_RAW_INSN(BPF_JMP | BPF_CALL, 0, 0, 0, 1000500), 
    BPF_STX_MEM(BPF_DW, BPF_R7, BPF_R0, 936), 
    BPF_JMP_IMM(BPF_JNE, BPF_R0, -1, 52), 
    BPF_MOV64_IMM(BPF_R1, 0), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -1), 
    BPF_MOV64_IMM(BPF_R1, 115), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -2), 
    BPF_MOV64_IMM(BPF_R1, 103), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -3), 
    BPF_MOV64_IMM(BPF_R1, 112), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -6), 
    BPF_MOV64_IMM(BPF_R1, 102), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -10), 
    BPF_MOV64_IMM(BPF_R1, 121), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -12), 
    BPF_MOV64_IMM(BPF_R1, 114), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -5), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -8), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -13), 
    BPF_MOV64_IMM(BPF_R1, 101), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -16), 
    BPF_MOV64_IMM(BPF_R1, 109), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -15), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -17), 
    BPF_MOV64_IMM(BPF_R1, 99), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -19), 
    BPF_MOV64_IMM(BPF_R1, 111), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -4), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -9), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -14), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -20), 
    BPF_MOV64_IMM(BPF_R1, 108), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -21), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -22), 
    BPF_MOV64_IMM(BPF_R1, 32), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -7), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -11), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -18), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -24), 
    BPF_MOV64_IMM(BPF_R1, 116), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -25), 
    BPF_MOV64_IMM(BPF_R1, 39), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -26), 
    BPF_MOV64_IMM(BPF_R1, 110), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -27), 
    BPF_MOV64_IMM(BPF_R1, 97), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -23), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -28), 
    BPF_MOV64_IMM(BPF_R1, 67), 
    BPF_STX_MEM(BPF_B, BPF_R10, BPF_R1, -29), 
    BPF_MOV64_REG(BPF_R1, BPF_R10), 
    BPF_ALU64_IMM(BPF_ADD, BPF_R1, -29), 
    BPF_MOV64_IMM(BPF_R2, 29), 
    BPF_RAW_INSN(BPF_JMP | BPF_CALL, 0, 0, 0, 6), 
    BPF_JMP_A(21), 
    BPF_ALU64_IMM(BPF_ADD, BPF_R0, 8), 
    BPF_MOV64_REG(BPF_R1, BPF_R0), 
    BPF_MOV64_REG(BPF_R2, BPF_R9), 
    BPF_MOV64_REG(BPF_R3, BPF_R8), 
    BPF_RAW_INSN(BPF_JMP | BPF_CALL, 0, 0, 0, 1000040), 
    BPF_MOV64_REG(BPF_R2, BPF_R7), 
    BPF_ALU64_IMM(BPF_ADD, BPF_R2, 952), 
    BPF_LDX_MEM(BPF_DW, BPF_R1, BPF_R7, 936), 
    BPF_STX_MEM(BPF_DW, BPF_R1, BPF_R2, 0), 
    BPF_MOV64_REG(BPF_R2, BPF_R1), 
    BPF_ALU64_IMM(BPF_ADD, BPF_R2, 8), 
    BPF_STX_MEM(BPF_DW, BPF_R7, BPF_R2, 944), 
    BPF_LDX_MEM(BPF_W, BPF_R2, BPF_R7, 128), 
    BPF_ALU64_IMM(BPF_OR, BPF_R2, 1), 
    BPF_STX_MEM(BPF_W, BPF_R7, BPF_R2, 128), 
    BPF_LDX_MEM(BPF_W, BPF_R2, BPF_R7, 920), 
    BPF_ALU64_IMM(BPF_ADD, BPF_R2, 8), 
    BPF_STX_MEM(BPF_W, BPF_R7, BPF_R2, 916), 
    BPF_MOV64_IMM(BPF_R3, 5), 
    BPF_RAW_INSN(BPF_JMP | BPF_CALL, 0, 0, 0, 1000502), 
    BPF_MOV64_IMM(BPF_R6, 0), 
    BPF_MOV64_REG(BPF_R0, BPF_R6), 
    BPF_EXIT_INSN(), 
}; 

U64 _MYBPF_LOADER_CopyMapsFd(U64 p1, U64 p2, U64 p3, U64 p4, U64 p5) 
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

U64 _MYBPF_LOADER_LoadProgs(U64 p1, U64 p2, U64 p3, U64 p4, U64 p5) 
{ 
    MYBPF_PARAM_S p; 
    MYBPF_CTX_S ctx = {0}; 

    ctx.begin_addr = g_bpfasm_insts; 
    ctx.end_addr = (char*)(void*)g_bpfasm_insts + sizeof(g_bpfasm_insts); 
    ctx.insts = (char*)g_bpfasm_insts + 216; 
    p.p[0]=p1; p.p[1]=p2; p.p[2]=p3; p.p[3]=p4; p.p[4]=p5; 
    int ret = MYBPF_DefultRun(&ctx, &p); 
    if (ret < 0) return ret; 
    return ctx.bpf_ret; 
} 

U64 _MYBPF_LOADER_CheckMayKeepMap(U64 p1, U64 p2, U64 p3, U64 p4, U64 p5) 
{ 
    MYBPF_PARAM_S p; 
    MYBPF_CTX_S ctx = {0}; 

    ctx.begin_addr = g_bpfasm_insts; 
    ctx.end_addr = (char*)(void*)g_bpfasm_insts + sizeof(g_bpfasm_insts); 
    ctx.insts = (char*)g_bpfasm_insts + 2016; 
    p.p[0]=p1; p.p[1]=p2; p.p[2]=p3; p.p[3]=p4; p.p[4]=p5; 
    int ret = MYBPF_DefultRun(&ctx, &p); 
    if (ret < 0) return ret; 
    return ctx.bpf_ret; 
} 

U64 _MYBPF_LOADER_MakeExe(U64 p1, U64 p2, U64 p3, U64 p4, U64 p5) 
{ 
    MYBPF_PARAM_S p; 
    MYBPF_CTX_S ctx = {0}; 

    ctx.begin_addr = g_bpfasm_insts; 
    ctx.end_addr = (char*)(void*)g_bpfasm_insts + sizeof(g_bpfasm_insts); 
    ctx.insts = (char*)g_bpfasm_insts + 2344; 
    p.p[0]=p1; p.p[1]=p2; p.p[2]=p3; p.p[3]=p4; p.p[4]=p5; 
    int ret = MYBPF_DefultRun(&ctx, &p); 
    if (ret < 0) return ret; 
    return ctx.bpf_ret; 
} 

