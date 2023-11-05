/*********************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2017-10-4
* Description: 
*
********************************************************/
#include "bs.h"
#include "utl/mybpf_utl.h"
#include "utl/mybpf_asm.h"
#include "mybpf_def_inner.h"

static int _mybpf_asm_insn_to_exp_load_store(MYBPF_INSN_S *insn, int count, int idx, OUT char *buf, int size)
{
    UCHAR op = insn->opcode;
    int imm = insn->imm;
    INT64 imm64 = (UINT)imm;
    UCHAR dst = insn->dst_reg;
    UCHAR src = insn->src_reg;
    short off = insn->off;
    int consume_count = 1;

    switch (op) {
        case 0x18:
            if (idx + 1 < count) {
                INT64 imm2 = (insn + 1)->imm;
                imm64 = (UINT)imm | ((UINT64)imm2 << 32);
            }
            snprintf(buf, size, "BPF_LD_IMM64_RAW(BPF_R%d, BPF_R%d, 0x%llxLL)", dst, src, imm64);
            consume_count ++;
            break;
        case 0x20:
            snprintf(buf, size, "BPF_LD_ABS(BPF_W, %d)", imm);
            break;
        case 0x28:
            snprintf(buf, size, "BPF_LD_ABS(BPF_H, %d)", imm);
            break;
        case 0x30:
            snprintf(buf, size, "BPF_LD_ABS(BPF_B, %d)", imm);
            break;
        case 0x38:
            snprintf(buf, size, "BPF_LD_ABS(BPF_DW, %d)", imm);
            break;
        case 0x40:
            snprintf(buf, size, "BPF_LD_IND(BPF_W, BPF_R%d, %d)", src, imm);
            break;
        case 0x48:
            snprintf(buf, size, "BPF_LD_IND(BPF_H, BPF_R%d, %d)", src, imm);
            break;
        case 0x50:
            snprintf(buf, size, "BPF_LD_IND(BPF_B, BPF_R%d, %d)", src, imm);
            break;
        case 0x58:
            snprintf(buf, size, "BPF_LD_IND(BPF_DW, BPF_R%d, %d)", src, imm);
            break;
        case 0x61:
            snprintf(buf, size, "BPF_LDX_MEM(BPF_W, BPF_R%d, BPF_R%d, %d)", dst, src, off);
            break;
        case 0x69:
            snprintf(buf, size, "BPF_LDX_MEM(BPF_H, BPF_R%d, BPF_R%d, %d)", dst, src, off);
            break;
        case 0x71:
            snprintf(buf, size, "BPF_LDX_MEM(BPF_B, BPF_R%d, BPF_R%d, %d)", dst, src, off);
            break;
        case 0x79:
            snprintf(buf, size, "BPF_LDX_MEM(BPF_DW, BPF_R%d, BPF_R%d, %d)", dst, src, off);
            break;
        case 0x62:
            snprintf(buf, size, "BPF_ST_MEM(BPF_W, BPF_R%d, %d, %d)", dst, off, imm);
            break;
        case 0x6a:
            snprintf(buf, size, "BPF_ST_MEM(BPF_H, BPF_R%d, %d, %d)", dst, off, imm);
            break;
        case 0x72:
            snprintf(buf, size, "BPF_ST_MEM(BPF_B, BPF_R%d, %d, %d)", dst, off, imm);
            break;
        case 0x7a:
            snprintf(buf, size, "BPF_ST_MEM(BPF_DW, BPF_R%d, %d, %d)", dst, off, imm);
            break;
        case 0x63:
            snprintf(buf, size, "BPF_STX_MEM(BPF_W, BPF_R%d, BPF_R%d, %d)", dst, src, off);
            break;
        case 0x6b:
            snprintf(buf, size, "BPF_STX_MEM(BPF_H, BPF_R%d, BPF_R%d, %d)", dst, src, off);
            break;
        case 0x73:
            snprintf(buf, size, "BPF_STX_MEM(BPF_B, BPF_R%d, BPF_R%d, %d)", dst, src, off);
            break;
        case 0x7b:
            snprintf(buf, size, "BPF_STX_MEM(BPF_DW, BPF_R%d, BPF_R%d, %d)", dst, src, off);
            break;
    }

    return consume_count;
}

static int _mybpf_asm_insn_to_asm_load_store(MYBPF_INSN_S *insn, int count, int idx, OUT char *buf, int size)
{
    UCHAR opcode = insn->opcode;
    int imm = insn->imm;
    INT64 imm64 = imm;
    UCHAR dst = insn->dst_reg;
    UCHAR src = insn->src_reg;
    short off = insn->off;
    int consume_count = 1;

    char *op = "+";
    if (off < 0) {
        op = "-";
        off = -off;
    }

    switch (opcode) {
        case 0x18:
            if (idx + 1 < count) {
                INT64 imm2 = (insn + 1)->imm;
                imm64 = (UINT)imm | ((UINT64)imm2 << 32);
            }
            snprintf(buf, size, "r%d = 0x%llx(%lld) ll", dst, imm64, imm64);
            consume_count ++;
            break;
        case 0x20:
            snprintf(buf, size, "ldabsw r%d, r%d, %d", src, dst, imm);
            break;
        case 0x28:
            snprintf(buf, size, "ldabsh r%d, r%d, %d", src, dst, imm);
            break;
        case 0x30:
            snprintf(buf, size, "ldabsb r%d, r%d, %d", src, dst, imm);
            break;
        case 0x38:
            snprintf(buf, size, "ldabsdw r%d, r%d, %d", src, dst, imm);
            break;
        case 0x40:
            snprintf(buf, size, "ldindw r%d, r%d, %d", src, dst, imm);
            break;
        case 0x48:
            snprintf(buf, size, "ldindh r%d, r%d, %d", src, dst, imm);
            break;
        case 0x50:
            snprintf(buf, size, "ldindb r%d, r%d, %d", src, dst, imm);
            break;
        case 0x58:
            snprintf(buf, size, "ldinddw r%d, r%d, %d", src, dst, imm);
            break;
        case 0x61:
            snprintf(buf, size, "r%d = *(u32*) (r%d %s %d)", dst, src, op, off);
            break;
        case 0x69:
            snprintf(buf, size, "r%d = *(u16*) (r%d %s %d)", dst, src, op, off);
            break;
        case 0x71:
            snprintf(buf, size, "r%d = *(u8*) (r%d %s %d)", dst, src, op, off);
            break;
        case 0x79:
            snprintf(buf, size, "r%d = *(u64*) (r%d %s %d)", dst, src, op, off);
            break;
        case 0x62:
            snprintf(buf, size, "*(u32*) (r%d %s %d) = %d", dst, op, off, imm);
            break;
        case 0x6a:
            snprintf(buf, size, "*(u16*) (r%d %s %d) = %d", dst, op, off, imm);
            break;
        case 0x72:
            snprintf(buf, size, "*(u8*) (r%d %s %d) = %d", dst, op, off, imm);
            break;
        case 0x7a:
            snprintf(buf, size, "*(u64*) (r%d %s %d) = %d", dst, op, off, imm);
            break;
        case 0x63:
            snprintf(buf, size, "*(u32*) (r%d %s %d) = r%d", dst, op, off, src);
            break;
        case 0x6b:
            snprintf(buf, size, "*(u16*) (r%d %s %d) = r%d", dst, op, off, src);
            break;
        case 0x73:
            snprintf(buf, size, "*(u8*) (r%d %s %d) = r%d", dst, op, off, src);
            break;
        case 0x7b:
            snprintf(buf, size, "*(u64*) (r%d %s %d) = r%d", dst, op, off, src);
            break;
    }

    return consume_count;
}

static int _mybpf_asm_insn_to_exp_alu32(MYBPF_INSN_S *insn, OUT char *buf, int size)
{
    UCHAR op = insn->opcode;
    int imm = insn->imm;
    UCHAR dst = insn->dst_reg;
    UCHAR src = insn->src_reg;
    int consume_count = 1;

    switch (op) {
        case 0x04:
            snprintf(buf, size, "BPF_ALU32_IMM(BPF_ADD, BPF_R%d, %d)", dst, imm);
            break;
        case 0x0c:
            snprintf(buf, size, "BPF_ALU32_REG(BPF_ADD, BPF_R%d, BPF_R%d)", dst, src);
            break;
        case 0x14:
            snprintf(buf, size, "BPF_ALU32_IMM(BPF_SUB, BPF_R%d, %d)", dst, imm);
            break;
        case 0x1c:
            snprintf(buf, size, "BPF_ALU32_REG(BPF_SUB, BPF_R%d, BPF_R%d)", dst, src);
            break;
        case 0x24:
            snprintf(buf, size, "BPF_ALU32_IMM(BPF_MUL, BPF_R%d, %d)", dst, imm);
            break;
        case 0x2c:
            snprintf(buf, size, "BPF_ALU32_REG(BPF_MUL, BPF_R%d, BPF_R%d)", dst, src);
            break;
        case 0x34:
            snprintf(buf, size, "BPF_ALU32_IMM(BPF_DIV, BPF_R%d, %d)", dst, imm);
            break;
        case 0x3c:
            snprintf(buf, size, "BPF_ALU32_REG(BPF_DIV, BPF_R%d, BPF_R%d)", dst, src);
            break;
        case 0x44:
            snprintf(buf, size, "BPF_ALU32_IMM(BPF_OR, BPF_R%d, %d)", dst, imm);
            break;
        case 0x4c:
            snprintf(buf, size, "BPF_ALU32_REG(BPF_OR, BPF_R%d, BPF_R%d)", dst, src);
            break;
        case 0x54:
            snprintf(buf, size, "BPF_ALU32_IMM(BPF_AND, BPF_R%d, %d)", dst, imm);
            break;
        case 0x5c:
            snprintf(buf, size, "BPF_ALU32_REG(BPF_AND, BPF_R%d, BPF_R%d)", dst, src);
            break;
        case 0x64:
            snprintf(buf, size, "BPF_ALU32_IMM(BPF_LSH, BPF_R%d, %d)", dst, imm);
            break;
        case 0x6c:
            snprintf(buf, size, "BPF_ALU32_REG(BPF_LSH, BPF_R%d, BPF_R%d)", dst, src);
            break;
        case 0x74:
            snprintf(buf, size, "BPF_ALU32_IMM(BPF_RSH, BPF_R%d, %d)", dst, imm);
            break;
        case 0x7c:
            snprintf(buf, size, "BPF_ALU32_REG(BPF_RSH, BPF_R%d, BPF_R%d)", dst, src);
            break;
        case 0x84:
            snprintf(buf, size, "BPF_ALU32_IMM(BPF_NEG, BPF_R%d, 0)", dst);
            break;
        case 0x94:
            snprintf(buf, size, "BPF_ALU32_IMM(BPF_MOD, BPF_R%d, %d)", dst, imm);
            break;
        case 0x9c:
            snprintf(buf, size, "BPF_ALU32_REG(BPF_MOD, BPF_R%d, BPF_R%d)", dst, src);
            break;
        case 0xa4:
            snprintf(buf, size, "BPF_ALU32_IMM(BPF_XOR, BPF_R%d, %d)", dst, imm);
            break;
        case 0xac:
            snprintf(buf, size, "BPF_ALU32_REG(BPF_XOR, BPF_R%d, BPF_R%d)", dst, src);
            break;
        case 0xb4:
            snprintf(buf, size, "BPF_MOV32_IMM(BPF_R%d, %d)", dst, imm);
            break;
        case 0xbc:
            snprintf(buf, size, "BPF_MOV32_REG(BPF_R%d, BPF_R%d)", dst, src);
            break;
        case 0xc4:
            snprintf(buf, size, "BPF_ALU32_IMM(BPF_ARSH, BPF_R%d, %d)", dst, imm);
            break;
        case 0xcc:
            snprintf(buf, size, "BPF_ALU32_REG(BPF_ARSH, BPF_R%d, BPF_R%d)", dst, src);
            break;
        case 0xd4:
            snprintf(buf, size, "BPF_ENDIAN(BPF_TO_LE, BPF_R%d, %d)", dst, imm);
            break;
        case 0xdc:
            snprintf(buf, size, "BPF_ENDIAN(BPF_TO_BE, BPF_R%d, %d)", dst, imm);
            break;
    }

    return consume_count;
}

static int _mybpf_asm_insn_to_asm_alu32(MYBPF_INSN_S *insn, OUT char *buf, int size)
{
    UCHAR op = insn->opcode;
    int imm = insn->imm;
    UCHAR dst = insn->dst_reg;
    UCHAR src = insn->src_reg;
    int consume_count = 1;

    switch (op) {
        case 0x04:
            snprintf(buf, size, "w%d += %d", dst, imm);
            break;
        case 0x0c:
            snprintf(buf, size, "w%d += w%d", dst, src);
            break;
        case 0x14:
            snprintf(buf, size, "w%d -= %d", dst, imm);
            break;
        case 0x1c:
            snprintf(buf, size, "w%d -= w%d", dst, src);
            break;
        case 0x24:
            snprintf(buf, size, "w%d *= %d", dst, imm);
            break;
        case 0x2c:
            snprintf(buf, size, "w%d *= w%d", dst, src);
            break;
        case 0x34:
            snprintf(buf, size, "w%d /= %d", dst, imm);
            break;
        case 0x3c:
            snprintf(buf, size, "w%d /= w%d", dst, src);
            break;
        case 0x44:
            snprintf(buf, size, "w%d |= %d", dst, imm);
            break;
        case 0x4c:
            snprintf(buf, size, "w%d |= w%d", dst, src);
            break;
        case 0x54:
            snprintf(buf, size, "w%d &= %d", dst, imm);
            break;
        case 0x5c:
            snprintf(buf, size, "w%d &= w%d", dst, src);
            break;
        case 0x64:
            snprintf(buf, size, "w%d <<= %d", dst, imm);
            break;
        case 0x6c:
            snprintf(buf, size, "w%d <<= w%d", dst, src);
            break;
        case 0x74:
            snprintf(buf, size, "w%d >>= %d", dst, imm);
            break;
        case 0x7c:
            snprintf(buf, size, "w%d >>= w%d", dst, src);
            break;
        case 0x84:
            snprintf(buf, size, "w%d = -w%d", dst, dst);
            break;
        case 0x94:
            snprintf(buf, size, "w%d %%= %d", dst, imm);
            break;
        case 0x9c:
            snprintf(buf, size, "w%d %%= w%d", dst, src);
            break;
        case 0xa4:
            snprintf(buf, size, "w%d ^= %d", dst, imm);
            break;
        case 0xac:
            snprintf(buf, size, "w%d ^= w%d", dst, src);
            break;
        case 0xb4:
            snprintf(buf, size, "w%d = %d", dst, imm);
            break;
        case 0xbc:
            snprintf(buf, size, "w%d = w%d", dst, src);
            break;
        case 0xc4:
            snprintf(buf, size, "w%d s >>= %d", dst, imm);
            break;
        case 0xcc:
            snprintf(buf, size, "w%d s >>= w%d", dst, src);
            break;
        case 0xd4:
            if (imm == 16) {
                snprintf(buf, size, "h%d = htole16(h%d)", dst, dst);
            } else if (imm == 32) {
                snprintf(buf, size, "w%d = htole32(w%d)", dst, dst);
            } else if (imm == 64) {
                snprintf(buf, size, "r%d = htole64(r%d)", dst, dst);
            }
            break;
        case 0xdc:
            if (imm == 16) {
                snprintf(buf, size, "h%d = htobe16(h%d)", dst, dst);
            } else if (imm == 32) {
                snprintf(buf, size, "w%d = htobe32(w%d)", dst, dst);
            } else if (imm == 64) {
                snprintf(buf, size, "r%d = htobe64(r%d)", dst, dst);
            }
            break;
    }

    return consume_count;
}

static int _mybpf_asm_insn_to_exp_alu64(MYBPF_INSN_S *insn, OUT char *buf, int size)
{
    UCHAR op = insn->opcode;
    int imm = insn->imm;
    UCHAR dst = insn->dst_reg;
    UCHAR src = insn->src_reg;
    int consume_count = 1;

    switch (op) {
        case 0x07:
            snprintf(buf, size, "BPF_ALU64_IMM(BPF_ADD, BPF_R%d, %d)", dst, imm);
            break;
        case 0x0f:
            snprintf(buf, size, "BPF_ALU64_REG(BPF_ADD, BPF_R%d, BPF_R%d)", dst, src);
            break;
        case 0x17:
            snprintf(buf, size, "BPF_ALU64_IMM(BPF_SUB, BPF_R%d, %d)", dst, imm);
            break;
        case 0x1f:
            snprintf(buf, size, "BPF_ALU64_REG(BPF_SUB, BPF_R%d, BPF_R%d)", dst, src);
            break;
        case 0x27:
            snprintf(buf, size, "BPF_ALU64_IMM(BPF_MUL, BPF_R%d, %d)", dst, imm);
            break;
        case 0x2f:
            snprintf(buf, size, "BPF_ALU64_REG(BPF_MUL, BPF_R%d, BPF_R%d)", dst, src);
            break;
        case 0x37:
            snprintf(buf, size, "BPF_ALU64_IMM(BPF_DIV, BPF_R%d, %d)", dst, imm);
            break;
        case 0x3f:
            snprintf(buf, size, "BPF_ALU64_REG(BPF_DIV, BPF_R%d, BPF_R%d)", dst, src);
            break;
        case 0x47:
            snprintf(buf, size, "BPF_ALU64_IMM(BPF_OR, BPF_R%d, %d)", dst, imm);
            break;
        case 0x4f:
            snprintf(buf, size, "BPF_ALU64_REG(BPF_OR, BPF_R%d, BPF_R%d)", dst, src);
            break;
        case 0x57:
            snprintf(buf, size, "BPF_ALU64_IMM(BPF_AND, BPF_R%d, %d)", dst, imm);
            break;
        case 0x5f:
            snprintf(buf, size, "BPF_ALU64_REG(BPF_AND, BPF_R%d, BPF_R%d)", dst, src);
            break;
        case 0x67:
            snprintf(buf, size, "BPF_ALU64_IMM(BPF_LSH, BPF_R%d, %d)", dst, imm);
            break;
        case 0x6f:
            snprintf(buf, size, "BPF_ALU64_REG(BPF_LSH, BPF_R%d, BPF_R%d)", dst, src);
            break;
        case 0x77:
            snprintf(buf, size, "BPF_ALU64_IMM(BPF_RSH, BPF_R%d, %d)", dst, imm);
            break;
        case 0x7f:
            snprintf(buf, size, "BPF_ALU64_REG(BPF_RSH, BPF_R%d, BPF_R%d)", dst, src);
            break;
        case 0x87:
            snprintf(buf, size, "BPF_ALU64_IMM(BPF_NEG, BPF_R%d, 0)", dst);
            break;
        case 0x97:
            snprintf(buf, size, "BPF_ALU64_IMM(BPF_MOD, BPF_R%d, %d)", dst, imm);
            break;
        case 0x9f:
            snprintf(buf, size, "BPF_ALU64_REG(BPF_MOD, BPF_R%d, BPF_R%d)", dst, src);
            break;
        case 0xa7:
            snprintf(buf, size, "BPF_ALU64_IMM(BPF_XOR, BPF_R%d, %d)", dst, imm);
            break;
        case 0xaf:
            snprintf(buf, size, "BPF_ALU64_REG(BPF_XOR, BPF_R%d, BPF_R%d)", dst, src);
            break;
        case 0xb7:
            snprintf(buf, size, "BPF_MOV64_IMM(BPF_R%d, %d)", dst, imm);
            break;
        case 0xbf:
            snprintf(buf, size, "BPF_MOV64_REG(BPF_R%d, BPF_R%d)", dst, src);
            break;
        case 0xc7:
            snprintf(buf, size, "BPF_ALU64_IMM(BPF_ARSH, BPF_R%d, %d)", dst, imm);
            break;
        case 0xcf:
            snprintf(buf, size, "BPF_ALU64_REG(BPF_ARSH, BPF_R%d, BPF_R%d)", dst, src);
            break;
    }

    return consume_count;
}

static int _mybpf_asm_insn_to_asm_alu64(MYBPF_INSN_S *insn, OUT char *buf, int size)
{
    UCHAR op = insn->opcode;
    int imm = insn->imm;
    UCHAR dst = insn->dst_reg;
    UCHAR src = insn->src_reg;
    int consume_count = 1;

    switch (op) {
        case 0x07:
            snprintf(buf, size, "r%d += %d", dst, imm);
            break;
        case 0x0f:
            snprintf(buf, size, "r%d += r%d", dst, src);
            break;
        case 0x17:
            snprintf(buf, size, "r%d -= %d", dst, imm);
            break;
        case 0x1f:
            snprintf(buf, size, "r%d -= r%d", dst, src);
            break;
        case 0x27:
            snprintf(buf, size, "r%d *= %d", dst, imm);
            break;
        case 0x2f:
            snprintf(buf, size, "r%d *= r%d", dst, src);
            break;
        case 0x37:
            snprintf(buf, size, "r%d /= %d", dst, imm);
            break;
        case 0x3f:
            snprintf(buf, size, "r%d /= r%d", dst, src);
            break;
        case 0x47:
            snprintf(buf, size, "r%d |= %d", dst, imm);
            break;
        case 0x4f:
            snprintf(buf, size, "r%d |= r%d", dst, src);
            break;
        case 0x57:
            snprintf(buf, size, "r%d &= %d", dst, imm);
            break;
        case 0x5f:
            snprintf(buf, size, "r%d &= r%d", dst, src);
            break;
        case 0x67:
            snprintf(buf, size, "r%d <<= %d", dst, imm);
            break;
        case 0x6f:
            snprintf(buf, size, "r%d <<= r%d", dst, src);
            break;
        case 0x77:
            snprintf(buf, size, "r%d >>= %d", dst, imm);
            break;
        case 0x7f:
            snprintf(buf, size, "r%d >>= r%d", dst, src);
            break;
        case 0x87:
            snprintf(buf, size, "r%d = -r%d", dst, dst);
            break;
        case 0x97:
            snprintf(buf, size, "r%d %%= %d", dst, imm);
            break;
        case 0x9f:
            snprintf(buf, size, "r%d %%= r%d", dst, src);
            break;
        case 0xa7:
            snprintf(buf, size, "r%d ^= %d", dst, imm);
            break;
        case 0xaf:
            snprintf(buf, size, "r%d ^= r%d", dst, src);
            break;
        case 0xb7:
            snprintf(buf, size, "r%d = %d", dst, imm);
            break;
        case 0xbf:
            snprintf(buf, size, "r%d = r%d", dst, src);
            break;
        case 0xc7:
            snprintf(buf, size, "r%d s >>= %d", dst, imm);
            break;
        case 0xcf:
            snprintf(buf, size, "r%d s >>= r%d", dst, src);
            break;
    }

    return consume_count;
}

static int _mybpf_asm_insn_to_exp_jmp32(MYBPF_INSN_S *insn, int idx, OUT char *buf, int size)
{
    UCHAR op = insn->opcode;
    int imm = insn->imm;
    UCHAR dst = insn->dst_reg;
    UCHAR src = insn->src_reg;
    int consume_count = 1;
    int off = insn->off;

    switch (op) {
        case 0x06:
            snprintf(buf, size, "BPF_JMP32_IMM(BPF_JA, 0, 0, %d)", off);
            break;
        case 0x16:
            snprintf(buf, size, "BPF_JMP32_IMM(BPF_JEQ, BPF_R%d, %d, %d)", dst, imm, off);
            break;
        case 0x1e:
            snprintf(buf, size, "BPF_JMP32_REG(BPF_JEQ, BPF_R%d, BPF_R%d, %d)", dst, src, off);
            break;
        case 0x26:
            snprintf(buf, size, "BPF_JMP32_IMM(BPF_JGT, BPF_R%d, %d, %d)", dst, imm, off);
            break;
        case 0x2e:
            snprintf(buf, size, "BPF_JMP32_REG(BPF_JGT, BPF_R%d, BPF_R%d, %d)", dst, src, off);
            break;
        case 0x36:
            snprintf(buf, size, "BPF_JMP32_IMM(BPF_JGE, BPF_R%d, %d, %d)", dst, imm, off);
            break;
        case 0x3e:
            snprintf(buf, size, "BPF_JMP32_REG(BPF_JGE, BPF_R%d, BPF_R%d, %d)", dst, src, off);
            break;
        case 0xa6:
            snprintf(buf, size, "BPF_JMP32_IMM(BPF_JLT, BPF_R%d, %d, %d)", dst, imm, off);
            break;
        case 0xae:
            snprintf(buf, size, "BPF_JMP32_REG(BPF_JLT, BPF_R%d, BPF_R%d, %d)", dst, src, off);
            break;
        case 0xb6:
            snprintf(buf, size, "BPF_JMP32_IMM(BPF_JLE, BPF_R%d, %d, %d)", dst, imm, off);
            break;
        case 0xbe:
            snprintf(buf, size, "BPF_JMP32_REG(BPF_JLE, BPF_R%d, BPF_R%d, %d)", dst, src, off);
            break;
        case 0x46:
            snprintf(buf, size, "BPF_JMP32_IMM(BPF_JSET, BPF_R%d, %d, %d)", dst, imm, off);
            break;
        case 0x4e:
            snprintf(buf, size, "BPF_JMP32_REG(BPF_JSET, BPF_R%d, BPF_R%d, %d)", dst, src, off);
            break;
        case 0x56:
            snprintf(buf, size, "BPF_JMP32_IMM(BPF_JNE, BPF_R%d, %d, %d)", dst, imm, off);
            break;
        case 0x5e:
            snprintf(buf, size, "BPF_JMP32_REG(BPF_JNE, BPF_R%d, BPF_R%d, %d)", dst, src, off);
            break;
        case 0x66:
            snprintf(buf, size, "BPF_JMP32_IMM(BPF_JSGT, BPF_R%d, %d, %d)", dst, imm, off);
            break;
        case 0x6e:
            snprintf(buf, size, "BPF_JMP32_REG(BPF_JSGT, BPF_R%d, BPF_R%d, %d)", dst, src, off);
            break;
        case 0x76:
            snprintf(buf, size, "BPF_JMP32_IMM(BPF_JSGE, BPF_R%d, %d, %d)", dst, imm, off);
            break;
        case 0x7e:
            snprintf(buf, size, "BPF_JMP32_REG(BPF_JSGE, BPF_R%d, BPF_R%d, %d)", dst, src, off);
            break;
        case 0xc6:
            snprintf(buf, size, "BPF_JMP32_IMM(BPF_JSLT, BPF_R%d, %d, %d)", dst, imm, off);
            break;
        case 0xce:
            snprintf(buf, size, "BPF_JMP32_REG(BPF_JSLT, BPF_R%d, BPF_R%d, %d)", dst, src, off);
            break;
        case 0xd6:
            snprintf(buf, size, "BPF_JMP32_IMM(BPF_JSLE, BPF_R%d, %d, %d)", dst, imm, off);
            break;
        case 0xde:
            snprintf(buf, size, "BPF_JMP32_REG(BPF_JSLE, BPF_R%d, BPF_R%d, %d)", dst, src, off);
            break;
        case 0x86:
            snprintf(buf, size, "BPF_RAW_INSN(BPF_JMP32 | BPF_CALL, %d, %d, %d, %d)", dst, src, off, imm);
            break;
    }

    return consume_count;
}

static int _mybpf_asm_insn_to_asm_jmp32(MYBPF_INSN_S *insn, int idx, OUT char *buf, int size)
{
    UCHAR op = insn->opcode;
    int imm = insn->imm;
    UCHAR dst = insn->dst_reg;
    UCHAR src = insn->src_reg;
    int consume_count = 1;
    int off = insn->off;
    int to = idx + off + 1;
    char *add = "";

    if (off >= 0) {
        add = "+";
    }

    switch (op) {
        case 0x06:
            snprintf(buf, size, "goto %s%d (%d)", add, off, to);
            break;
        case 0x16:
            snprintf(buf, size, "if w%d == %d goto %s%d (%d)", dst, imm, add, off, to);
            break;
        case 0x1e:
            snprintf(buf, size, "if w%d == w%d goto %s%d (%d)", dst, src, add, off, to);
            break;
        case 0x26:
            snprintf(buf, size, "if w%d > %d goto %s%d (%d)", dst, imm, add, off, to);
            break;
        case 0x2e:
            snprintf(buf, size, "if w%d > w%d goto %s%d (%d)", dst, src, add, off, to);
            break;
        case 0x36:
            snprintf(buf, size, "if w%d >= %d goto %s%d (%d)", dst, imm, add, off, to);
            break;
        case 0x3e:
            snprintf(buf, size, "if w%d >= w%d goto %s%d (%d)", dst, src, add, off, to);
            break;
        case 0xa6:
            snprintf(buf, size, "if w%d < %d goto %s%d (%d)", dst, imm, add, off, to);
            break;
        case 0xae:
            snprintf(buf, size, "if w%d < w%d goto %s%d (%d)", dst, src, add, off, to);
            break;
        case 0xb6:
            snprintf(buf, size, "if w%d <= %d goto %s%d (%d)", dst, imm, add, off, to);
            break;
        case 0xbe:
            snprintf(buf, size, "if w%d <= w%d goto %s%d (%d)", dst, src, add, off, to);
            break;
        case 0x46:
            snprintf(buf, size, "if w%d & %d goto %s%d (%d)", dst, imm, add, off, to);
            break;
        case 0x4e:
            snprintf(buf, size, "if w%d & w%d goto %s%d (%d)", dst, src, add, off, to);
            break;
        case 0x56:
            snprintf(buf, size, "if w%d != %d goto %s%d (%d)", dst, imm, add, off, to);
            break;
        case 0x5e:
            snprintf(buf, size, "if w%d != w%d goto %s%d (%d)", dst, src, add, off, to);
            break;
        case 0x66:
            snprintf(buf, size, "if w%d > %d goto %s%d (%d)", dst, imm, add, off, to);
            break;
        case 0x6e:
            snprintf(buf, size, "if w%d > w%d goto %s%d (%d)", dst, src, add, off, to);
            break;
        case 0x76:
            snprintf(buf, size, "if w%d >= %d goto %s%d (%d)", dst, imm, add, off, to);
            break;
        case 0x7e:
            snprintf(buf, size, "if w%d >= w%d goto %s%d (%d)", dst, src, add, off, to);
            break;
        case 0xc6:
            snprintf(buf, size, "if w%d < %d goto %s%d (%d)", dst, imm, add, off, to);
            break;
        case 0xce:
            snprintf(buf, size, "if w%d < w%d goto %s%d (%d)", dst, src, add, off, to);
            break;
        case 0xd6:
            snprintf(buf, size, "if w%d <= %d goto %s%d (%d)", dst, imm, add, off, to);
            break;
        case 0xde:
            snprintf(buf, size, "if w%d <= w%d goto %s%d (%d)", dst, src, add, off, to);
            break;
        case 0x86:
            snprintf(buf, size, "call %d", imm);
            break;
    }

    return consume_count;
}

static int _mybpf_asm_insn_to_exp_jmp(MYBPF_INSN_S *insn, int idx, OUT char *buf, int size)
{
    UCHAR op = insn->opcode;
    int imm = insn->imm;
    UCHAR dst = insn->dst_reg;
    UCHAR src = insn->src_reg;
    int consume_count = 1;
    int off = insn->off;

    switch (op) {
        case 0x05:
            snprintf(buf, size, "BPF_JMP_A(%d)", off);
            break;
        case 0x15:
            snprintf(buf, size, "BPF_JMP_IMM(BPF_JEQ, BPF_R%d, %d, %d)", dst, imm, off);
            break;
        case 0x1d:
            snprintf(buf, size, "BPF_JMP_REG(BPF_JEQ, BPF_R%d, BPF_R%d, %d)", dst, src, off);
            break;
        case 0x25:
            snprintf(buf, size, "BPF_JMP_IMM(BPF_JGT, BPF_R%d, %d, %d)", dst, imm, off);
            break;
        case 0x2d:
            snprintf(buf, size, "BPF_JMP_REG(BPF_JGT, BPF_R%d, BPF_R%d, %d)", dst, src, off);
            break;
        case 0x35:
            snprintf(buf, size, "BPF_JMP_IMM(BPF_JGE, BPF_R%d, %d, %d)", dst, imm, off);
            break;
        case 0x3d:
            snprintf(buf, size, "BPF_JMP_REG(BPF_JGE, BPF_R%d, BPF_R%d, %d)", dst, src, off);
            break;
        case 0xa5:
            snprintf(buf, size, "BPF_JMP_IMM(BPF_JLT, BPF_R%d, %d, %d)", dst, imm, off);
            break;
        case 0xad:
            snprintf(buf, size, "BPF_JMP_REG(BPF_JLT, BPF_R%d, BPF_R%d, %d)", dst, src, off);
            break;
        case 0xb5:
            snprintf(buf, size, "BPF_JMP_IMM(BPF_JLE, BPF_R%d, %d, %d)", dst, imm, off);
            break;
        case 0xbd:
            snprintf(buf, size, "BPF_JMP_REG(BPF_JLE, BPF_R%d, BPF_R%d, %d)", dst, src, off);
            break;
        case 0x45:
            snprintf(buf, size, "BPF_JMP_IMM(BPF_JSET, BPF_R%d, %d, %d)", dst, imm, off);
            break;
        case 0x4d:
            snprintf(buf, size, "BPF_JMP_REG(BPF_JSET, BPF_R%d, BPF_R%d, %d)", dst, src, off);
            break;
        case 0x55:
            snprintf(buf, size, "BPF_JMP_IMM(BPF_JNE, BPF_R%d, %d, %d)", dst, imm, off);
            break;
        case 0x5d:
            snprintf(buf, size, "BPF_JMP_REG(BPF_JNE, BPF_R%d, BPF_R%d, %d)", dst, src, off);
            break;
        case 0x65:
            snprintf(buf, size, "BPF_JMP_IMM(BPF_JSGT, BPF_R%d, %d, %d)", dst, imm, off);
            break;
        case 0x6d:
            snprintf(buf, size, "BPF_JMP_REG(BPF_JSGT, BPF_R%d, BPF_R%d, %d)", dst, src, off);
            break;
        case 0x75:
            snprintf(buf, size, "BPF_JMP_IMM(BPF_JSGE, BPF_R%d, %d, %d)", dst, imm, off);
            break;
        case 0x7d:
            snprintf(buf, size, "BPF_JMP_REG(BPF_JSGE, BPF_R%d, BPF_R%d, %d)", dst, src, off);
            break;
        case 0xc5:
            snprintf(buf, size, "BPF_JMP_IMM(BPF_JSLT, BPF_R%d, %d, %d)", dst, imm, off);
            break;
        case 0xcd:
            snprintf(buf, size, "BPF_JMP_REG(BPF_JSLT, BPF_R%d, BPF_R%d, %d)", dst, src, off);
            break;
        case 0xd5:
            snprintf(buf, size, "BPF_JMP_IMM(BPF_JSLE, BPF_R%d, %d, %d)", dst, imm, off);
            break;
        case 0xdd:
            snprintf(buf, size, "BPF_JMP_REG(BPF_JSLE, BPF_R%d, BPF_R%d, %d)", dst, src, off);
            break;
        case 0x85:
            snprintf(buf, size, "BPF_RAW_INSN(BPF_JMP | BPF_CALL, %d, %d, %d, %d)", dst, src, off, imm);
            break;
        case 0x95:
            snprintf(buf, size, "BPF_EXIT_INSN()");
            break;
    }

    return consume_count;
}

static int _mybpf_asm_insn_to_asm_jmp(MYBPF_INSN_S *insn, int idx, OUT char *buf, int size)
{
    UCHAR op = insn->opcode;
    int imm = insn->imm;
    UCHAR dst = insn->dst_reg;
    UCHAR src = insn->src_reg;
    int consume_count = 1;
    int off = insn->off;
    int to = idx + off + 1;
    char *add = "";

    if (off >= 0) {
        add = "+";
    }

    switch (op) {
        case 0x05:
            snprintf(buf, size, "goto %s%d (%d)", add, off, to);
            break;
        case 0x15:
            snprintf(buf, size, "if r%d == %d goto %s%d (%d)", dst, imm, add, off, to);
            break;
        case 0x1d:
            snprintf(buf, size, "if r%d == r%d goto %s%d (%d)", dst, src, add, off, to);
            break;
        case 0x25:
            snprintf(buf, size, "if r%d > %d goto %s%d (%d)", dst, imm, add, off, to);
            break;
        case 0x2d:
            snprintf(buf, size, "if r%d > r%d goto %s%d (%d)", dst, src, add, off, to);
            break;
        case 0x35:
            snprintf(buf, size, "if r%d >= %d goto %s%d (%d)", dst, imm, add, off, to);
            break;
        case 0x3d:
            snprintf(buf, size, "if r%d >= r%d goto %s%d (%d)", dst, src, add, off, to);
            break;
        case 0xa5:
            snprintf(buf, size, "if r%d < %d goto %s%d (%d)", dst, imm, add, off, to);
            break;
        case 0xad:
            snprintf(buf, size, "if r%d < r%d goto %s%d (%d)", dst, src, add, off, to);
            break;
        case 0xb5:
            snprintf(buf, size, "if r%d <= %d goto %s%d (%d)", dst, imm, add, off, to);
            break;
        case 0xbd:
            snprintf(buf, size, "if r%d <= r%d goto %s%d (%d)", dst, src, add, off, to);
            break;
        case 0x45:
            snprintf(buf, size, "if r%d & %d goto %s%d (%d)", dst, imm, add, off, to);
            break;
        case 0x4d:
            snprintf(buf, size, "if r%d & r%d goto %s%d (%d)", dst, src, add, off, to);
            break;
        case 0x55:
            snprintf(buf, size, "if r%d != %d goto %s%d (%d)", dst, imm, add, off, to);
            break;
        case 0x5d:
            snprintf(buf, size, "if r%d != r%d goto %s%d (%d)", dst, src, add, off, to);
            break;
        case 0x65:
            snprintf(buf, size, "if r%d > %d goto %s%d (%d)", dst, imm, add, off, to);
            break;
        case 0x6d:
            snprintf(buf, size, "if r%d > r%d goto %s%d (%d)", dst, src, add, off, to);
            break;
        case 0x75:
            snprintf(buf, size, "if r%d >= %d goto %s%d (%d)", dst, imm, add, off, to);
            break;
        case 0x7d:
            snprintf(buf, size, "if r%d >= r%d goto %s%d (%d)", dst, src, add, off, to);
            break;
        case 0xc5:
            snprintf(buf, size, "if r%d < %d goto %s%d (%d)", dst, imm, add, off, to);
            break;
        case 0xcd:
            snprintf(buf, size, "if r%d < r%d goto %s%d (%d)", dst, src, add, off, to);
            break;
        case 0xd5:
            snprintf(buf, size, "if r%d <= %d goto %s%d (%d)", dst, imm, add, off, to);
            break;
        case 0xdd:
            snprintf(buf, size, "if r%d <= r%d goto %s%d (%d)", dst, src, add, off, to);
            break;
        case 0x85:
            snprintf(buf, size, "call %d", imm);
            break;
        case 0x95:
            snprintf(buf, size, "exit");
            break;
    }

    return consume_count;
}


int MYBPF_ASM_Insn2Asm(MYBPF_INSN_S *insn, int insn_count, int insn_idx, OUT char *buf, int size)
{
    MYBPF_INSN_S *cur = &insn[insn_idx];
    UCHAR op = cur->opcode;
    UCHAR class = op & 0x7;
    int consume_count = 1;

    buf[0] = 0;

    switch (class) {
        case BPF_LD:
        case BPF_LDX:
        case BPF_ST:
        case BPF_STX:
            consume_count = _mybpf_asm_insn_to_asm_load_store(cur, insn_count, insn_idx, buf, size);
            break;
        case BPF_ALU:
            consume_count = _mybpf_asm_insn_to_asm_alu32(cur, buf, size);
            break;
        case BPF_ALU64:
            consume_count = _mybpf_asm_insn_to_asm_alu64(cur, buf, size);
            break;
        case BPF_JMP:
            consume_count = _mybpf_asm_insn_to_asm_jmp(cur, insn_idx, buf, size);
            break;
        case BPF_JMP32:
            consume_count = _mybpf_asm_insn_to_asm_jmp32(cur, insn_idx, buf, size);
            break;
    }

    return consume_count;
}


int MYBPF_ASM_Insn2Exp(MYBPF_INSN_S *insn, int insn_count, int insn_idx, OUT char *buf, int size)
{
    MYBPF_INSN_S *cur = &insn[insn_idx];
    UCHAR op = cur->opcode;
    UCHAR class = op & 0x7;
    int consume_count = 1;

    buf[0] = 0;

    switch (class) {
        case BPF_LD:
        case BPF_LDX:
        case BPF_ST:
        case BPF_STX:
            consume_count = _mybpf_asm_insn_to_exp_load_store(cur, insn_count, insn_idx, buf, size);
            break;
        case BPF_ALU:
            consume_count = _mybpf_asm_insn_to_exp_alu32(cur, buf, size);
            break;
        case BPF_ALU64:
            consume_count = _mybpf_asm_insn_to_exp_alu64(cur, buf, size);
            break;
        case BPF_JMP:
            consume_count = _mybpf_asm_insn_to_exp_jmp(cur, insn_idx, buf, size);
            break;
        case BPF_JMP32:
            consume_count = _mybpf_asm_insn_to_exp_jmp32(cur, insn_idx, buf, size);
            break;
    }

    return consume_count;
}

static void _mybpf_asm_print(MYBPF_INSN_S *insn, int insn_idx, int len, char *asm_string, char *exp_string, UINT flag)
{
    if (flag & MYBPF_DUMP_FLAG_LINE) {
        printf("%4u%-4c", insn_idx, ':');
    }

    UCHAR *d = (void*)&insn[insn_idx];

    if (flag & MYBPF_DUMP_FLAG_RAW) {
        for (int i=0; i<8; i++) {
            printf("%02x ", d[i]);
        }
        if (len > 8) {
            if (flag & MYBPF_DUMP_FLAG_LINE) {
                printf("\n%4u%-4c", insn_idx + 1, ':');
            }
            for (int i=8; i<len; i++) {
                printf("%02x ", d[i]);
            }
        }
    }

    if (flag & MYBPF_DUMP_FLAG_ASM) {
        printf("%-40s ", asm_string);
    }

    if (flag & MYBPF_DUMP_FLAG_EXP) {
        printf("%s ", exp_string);
    }

    printf("\n");
}

void MYBPF_ASM_DumpAsm(void *data, int len, UINT flag)
{
    char asm_string[128];
    char exp_string[128];
    MYBPF_INSN_S *insn = data;
    int insn_idx = 0;
    int insn_count = len / sizeof(MYBPF_INSN_S);
    int insn_consume;

    while (insn_idx < insn_count) {
        insn_consume = MYBPF_ASM_Insn2Asm(insn, insn_count, insn_idx, asm_string, sizeof(asm_string));
        MYBPF_ASM_Insn2Exp(insn, insn_count, insn_idx, exp_string, sizeof(exp_string));
        _mybpf_asm_print(insn, insn_idx, insn_consume * sizeof(MYBPF_INSN_S), asm_string, exp_string, flag);
        insn_idx += insn_consume;
    }
}


