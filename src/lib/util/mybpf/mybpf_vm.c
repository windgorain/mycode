/*================================================================
*   Created by LiXingang, Copyright LiXingang
*   Date: 2017.10.1
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/mybpf_vm.h"
#include "utl/mybpf_insn.h"
#include "utl/mybpf_loader.h"
#include "utl/mybpf_prog.h"
#include "utl/mybpf_utl.h"
#include "utl/mybpf_dbg.h"
#include "utl/ubpf/ebpf.h"
#include "utl/endian_utl.h"
#include "mybpf_osbase.h"

#define MYBPF_STACK_SIZE 512
#define MYBPF_MAX_INSTS 1000000

static int _mybpf_run(MYBPF_VM_S *vm, MYBPF_CTX_S *ctx, U64 p1, U64 p2, U64 p3, U64 p4, U64 p5);

static inline uint32_t _mybpf_vm_to_u32(uint64_t x)
{
    return x;
}

static inline bool _mybpf_bounds_check(MYBPF_VM_S *vm, MYBPF_CTX_S *ctx, void *addr, int size,
        const char *type, uint16_t cur_pc, void *mem, size_t mem_len)
{
    if (! ctx->mem_check)
        return true;

    if (mem && (addr >= mem && ((char*)addr + size) <= ((char*)mem + mem_len))) {
        return true; /* Context access */
    } else if ((char*)addr >= ctx->stack && ((char*)addr + size) <= (ctx->stack + ctx->stack_size)) {
        return true; /* Stack access */
    } else {
        vm->print_func("uBPF error: out of bounds memory %s at PC %u, addr %p, size %d\nmem %p/%d stack %p/%d\n",
                type, cur_pc, addr, size, mem, mem_len, ctx->stack, ctx->stack_size);
        return false;
    }
}

int MYBPF_SetTailCallIndex(MYBPF_VM_S *vm, unsigned int id)
{
    if (vm->tail_call_index != 0) {
        return -1;
    }

    vm->tail_call_index = id;
    return 0;
}

static inline void * _mybpf_get_helper(MYBPF_VM_S *vm, int imm)
{
    if (imm == (UINT)(-1)) {
        return NULL;
    }

    return BpfHelper_BaseHelper + imm;
}

static inline UINT64 _mybpf_call(MYBPF_VM_S *vm, int imm, UINT64 p1, UINT64 p2, UINT64 p3, UINT64 p4, UINT64 p5)
{
    PF_BPF_HELPER_FUNC func = _mybpf_get_helper(vm, imm);

    if (! func) {
        return -1;
    }

    return func(p1, p2, p3, p4, p5);
}

static ELF_PROG_INFO_S * _mybpf_vm_get_sub_prog(MYBPF_CTX_S *ctx, void *insn)
{
    MYBPF_PROG_NODE_S *prog = ctx->prog;
    if (! prog) {
        PRINTFL();
        return NULL;
    }

    MYBPF_LOADER_NODE_S *n = prog->loader_node;
    if (! n) {
        PRINTFL();
        return NULL;
    }

    for (int i=0; i<n->progs_count; i++) {
        if (((char*)n->insts + n->progs[i].offset) == (char*)insn) {
            return &n->progs[i];
        }
    }

    return NULL;
}

static inline UINT64 _mybpf_call_sub_prog(MYBPF_VM_S *vm, MYBPF_CTX_S *ctx, 
        void *insn, UINT64 p1, UINT64 p2, UINT64 p3, UINT64 p4, UINT64 p5)
{
    ctx->insts = insn;
    _mybpf_run(vm, ctx, p1, p2, p3, p4, p5);

    return ctx->bpf_ret;
}

BOOL_T MYBPF_Validate(MYBPF_VM_S *vm, void *insn, UINT num_insts)
{
    struct ebpf_inst *insts = insn;

    if (num_insts > MYBPF_MAX_INSTS) {
        vm->print_func("too many instructions (max %u)", MYBPF_MAX_INSTS);
        return false;
    }

    int i;
    for (i = 0; i < num_insts; i++) {
        struct ebpf_inst inst = insts[i];
        bool store = false;

        switch (inst.opcode) {
        case EBPF_OP_ADD_IMM:
        case EBPF_OP_ADD_REG:
        case EBPF_OP_SUB_IMM:
        case EBPF_OP_SUB_REG:
        case EBPF_OP_MUL_IMM:
        case EBPF_OP_MUL_REG:
        case EBPF_OP_DIV_REG:
        case EBPF_OP_OR_IMM:
        case EBPF_OP_OR_REG:
        case EBPF_OP_AND_IMM:
        case EBPF_OP_AND_REG:
        case EBPF_OP_LSH_IMM:
        case EBPF_OP_LSH_REG:
        case EBPF_OP_RSH_IMM:
        case EBPF_OP_RSH_REG:
        case EBPF_OP_NEG:
        case EBPF_OP_MOD_REG:
        case EBPF_OP_XOR_IMM:
        case EBPF_OP_XOR_REG:
        case EBPF_OP_MOV_IMM:
        case EBPF_OP_MOV_REG:
        case EBPF_OP_ARSH_IMM:
        case EBPF_OP_ARSH_REG:
            break;

        case EBPF_OP_LE:
        case EBPF_OP_BE:
            if (inst.imm != 16 && inst.imm != 32 && inst.imm != 64) {
                vm->print_func("invalid endian immediate at PC %d", i);
                return false;
            }
            break;

        case EBPF_OP_ADD64_IMM:
        case EBPF_OP_ADD64_REG:
        case EBPF_OP_SUB64_IMM:
        case EBPF_OP_SUB64_REG:
        case EBPF_OP_MUL64_IMM:
        case EBPF_OP_MUL64_REG:
        case EBPF_OP_DIV64_REG:
        case EBPF_OP_OR64_IMM:
        case EBPF_OP_OR64_REG:
        case EBPF_OP_AND64_IMM:
        case EBPF_OP_AND64_REG:
        case EBPF_OP_LSH64_IMM:
        case EBPF_OP_LSH64_REG:
        case EBPF_OP_RSH64_IMM:
        case EBPF_OP_RSH64_REG:
        case EBPF_OP_NEG64:
        case EBPF_OP_MOD64_REG:
        case EBPF_OP_XOR64_IMM:
        case EBPF_OP_XOR64_REG:
        case EBPF_OP_MOV64_IMM:
        case EBPF_OP_MOV64_REG:
        case EBPF_OP_ARSH64_IMM:
        case EBPF_OP_ARSH64_REG:
            break;

        case EBPF_OP_LDXW:
        case EBPF_OP_LDXH:
        case EBPF_OP_LDXB:
        case EBPF_OP_LDXDW:
            break;

        case EBPF_OP_STW:
        case EBPF_OP_STH:
        case EBPF_OP_STB:
        case EBPF_OP_STDW:
        case EBPF_OP_STXW:
        case EBPF_OP_STXH:
        case EBPF_OP_STXB:
        case EBPF_OP_STXDW:
            store = true;
            break;

        case EBPF_OP_LDDW:
            if (i + 1 >= num_insts || insts[i+1].opcode != 0) {
                vm->print_func("incomplete lddw at PC %d", i);
                return false;
            }
            i++; /* Skip next instruction */
            break;

        case EBPF_OP_JA:
        case EBPF_OP_JEQ_REG:
        case EBPF_OP_JEQ_IMM:
        case EBPF_OP_JGT_REG:
        case EBPF_OP_JGT_IMM:
        case EBPF_OP_JGE_REG:
        case EBPF_OP_JGE_IMM:
        case EBPF_OP_JLT_REG:
        case EBPF_OP_JLT_IMM:
        case EBPF_OP_JLE_REG:
        case EBPF_OP_JLE_IMM:
        case EBPF_OP_JSET_REG:
        case EBPF_OP_JSET_IMM:
        case EBPF_OP_JNE_REG:
        case EBPF_OP_JNE_IMM:
        case EBPF_OP_JSGT_IMM:
        case EBPF_OP_JSGT_REG:
        case EBPF_OP_JSGE_IMM:
        case EBPF_OP_JSGE_REG:
        case EBPF_OP_JSLT_IMM:
        case EBPF_OP_JSLT_REG:
        case EBPF_OP_JSLE_IMM:
        case EBPF_OP_JSLE_REG:
            if (inst.off == -1) {
                vm->print_func("infinite loop at PC %d", i);
                return false;
            }
            int new_pc = i + 1 + inst.off;
            if (new_pc < 0 || new_pc >= num_insts) {
                vm->print_func("jump out of bounds at PC %d", i);
                return false;
            } else if (insts[new_pc].opcode == 0) {
                vm->print_func("jump to middle of lddw at PC %d", i);
                return false;
            }
            break;

        case EBPF_OP_CALL:
            if (NULL == _mybpf_get_helper(vm, inst.imm)) {
                vm->print_func("invalid call immediate at PC %d", i);
                return false;
            }
            break;

        case EBPF_OP_EXIT:
            break;

        case EBPF_OP_DIV_IMM:
        case EBPF_OP_MOD_IMM:
        case EBPF_OP_DIV64_IMM:
        case EBPF_OP_MOD64_IMM:
            if (inst.imm == 0) {
                vm->print_func("division by zero at PC %d", i);
                return false;
            }
            break;

        default:
            vm->print_func("unknown opcode 0x%02x at PC %d", inst.opcode, i);
            return false;
        }

        if (inst.src_reg > 10) {
            vm->print_func("invalid source register at PC %d", i);
            return false;
        }

        if (inst.dst_reg > 9 && !(store && inst.dst_reg == 10)) {
            vm->print_func("invalid destination register at PC %d", i);
            return false;
        }
    }

    return true;
}

#define MYBPF_BOUNDS_CHECK_LOAD(size) \
    do { \
        if (!_mybpf_bounds_check(vm, ctx, (char *)reg[inst.src_reg] + inst.off, size, \
                    "load", cur_pc, ctx->mem, ctx->mem_len)) { \
            return -1; \
        } \
    } while (0)

#define MYBPF_BOUNDS_CHECK_STORE(size) \
    do { \
        if (!_mybpf_bounds_check(vm, ctx, (char *)reg[inst.dst_reg] + inst.off, size,\
                    "store", cur_pc, ctx->mem, ctx->mem_len)) { \
            return -1; \
        } \
    } while (0)

static int _mybpf_run_bpf(MYBPF_VM_S *vm, MYBPF_CTX_S *ctx, U64 p1, U64 p2, U64 p3, U64 p4, U64 p5)
{
    uint16_t pc = 0;
    struct ebpf_inst *insts = ctx->insts;
    uint64_t reg[16];

    if (!insts) {
        /* Code must be loaded before we can execute */
        RETURN(BS_ERR);
    }

    reg[1] = p1;
    reg[2] = p2;
    reg[3] = p3;
    reg[4] = p4;
    reg[5] = p5;
    reg[10] = (unsigned long)(ctx->stack + ctx->stack_size);

    while (1) {
        const uint16_t cur_pc = pc;
        struct ebpf_inst inst = insts[pc++];

        switch (inst.opcode) {
        case EBPF_OP_ADD_IMM:
            reg[inst.dst_reg] += inst.imm;
            reg[inst.dst_reg] &= UINT32_MAX;
            break;
        case EBPF_OP_ADD_REG:
            reg[inst.dst_reg] += reg[inst.src_reg];
            reg[inst.dst_reg] &= UINT32_MAX;
            break;
        case EBPF_OP_SUB_IMM:
            reg[inst.dst_reg] -= inst.imm;
            reg[inst.dst_reg] &= UINT32_MAX;
            break;
        case EBPF_OP_SUB_REG:
            reg[inst.dst_reg] -= reg[inst.src_reg];
            reg[inst.dst_reg] &= UINT32_MAX;
            break;
        case EBPF_OP_MUL_IMM:
            reg[inst.dst_reg] *= inst.imm;
            reg[inst.dst_reg] &= UINT32_MAX;
            break;
        case EBPF_OP_MUL_REG:
            reg[inst.dst_reg] *= reg[inst.src_reg];
            reg[inst.dst_reg] &= UINT32_MAX;
            break;
        case EBPF_OP_DIV_IMM:
            reg[inst.dst_reg] = _mybpf_vm_to_u32(reg[inst.dst_reg]) / _mybpf_vm_to_u32(inst.imm);
            reg[inst.dst_reg] &= UINT32_MAX;
            break;
        case EBPF_OP_DIV_REG:
            if (reg[inst.src_reg] == 0) {
                vm->print_func("uBPF error: division by zero at PC %u\n", cur_pc);
                RETURN(BS_ERR);
            }
            reg[inst.dst_reg] = _mybpf_vm_to_u32(reg[inst.dst_reg]) / _mybpf_vm_to_u32(reg[inst.src_reg]);
            reg[inst.dst_reg] &= UINT32_MAX;
            break;
        case EBPF_OP_OR_IMM:
            reg[inst.dst_reg] |= inst.imm;
            reg[inst.dst_reg] &= UINT32_MAX;
            break;
        case EBPF_OP_OR_REG:
            reg[inst.dst_reg] |= reg[inst.src_reg];
            reg[inst.dst_reg] &= UINT32_MAX;
            break;
        case EBPF_OP_AND_IMM:
            reg[inst.dst_reg] &= inst.imm;
            reg[inst.dst_reg] &= UINT32_MAX;
            break;
        case EBPF_OP_AND_REG:
            reg[inst.dst_reg] &= reg[inst.src_reg];
            reg[inst.dst_reg] &= UINT32_MAX;
            break;
        case EBPF_OP_LSH_IMM:
            reg[inst.dst_reg] <<= inst.imm;
            reg[inst.dst_reg] &= UINT32_MAX;
            break;
        case EBPF_OP_LSH_REG:
            reg[inst.dst_reg] <<= reg[inst.src_reg];
            reg[inst.dst_reg] &= UINT32_MAX;
            break;
        case EBPF_OP_RSH_IMM:
            reg[inst.dst_reg] = _mybpf_vm_to_u32(reg[inst.dst_reg]) >> inst.imm;
            reg[inst.dst_reg] &= UINT32_MAX;
            break;
        case EBPF_OP_RSH_REG:
            reg[inst.dst_reg] = _mybpf_vm_to_u32(reg[inst.dst_reg]) >> reg[inst.src_reg];
            reg[inst.dst_reg] &= UINT32_MAX;
            break;
        case EBPF_OP_NEG:
            reg[inst.dst_reg] = -(int64_t)reg[inst.dst_reg];
            reg[inst.dst_reg] &= UINT32_MAX;
            break;
        case EBPF_OP_MOD_IMM:
            reg[inst.dst_reg] = _mybpf_vm_to_u32(reg[inst.dst_reg]) % _mybpf_vm_to_u32(inst.imm);
            reg[inst.dst_reg] &= UINT32_MAX;
            break;
        case EBPF_OP_MOD_REG:
            if (reg[inst.src_reg] == 0) {
                vm->print_func("uBPF error: division by zero at PC %u\n", cur_pc);
                RETURN(BS_ERR);
            }
            reg[inst.dst_reg] = _mybpf_vm_to_u32(reg[inst.dst_reg]) % _mybpf_vm_to_u32(reg[inst.src_reg]);
            break;
        case EBPF_OP_XOR_IMM:
            reg[inst.dst_reg] ^= inst.imm;
            reg[inst.dst_reg] &= UINT32_MAX;
            break;
        case EBPF_OP_XOR_REG:
            reg[inst.dst_reg] ^= reg[inst.src_reg];
            reg[inst.dst_reg] &= UINT32_MAX;
            break;
        case EBPF_OP_MOV_IMM:
            reg[inst.dst_reg] = inst.imm;
            reg[inst.dst_reg] &= UINT32_MAX;
            break;
        case EBPF_OP_MOV_REG:
            reg[inst.dst_reg] = reg[inst.src_reg];
            reg[inst.dst_reg] &= UINT32_MAX;
            break;
        case EBPF_OP_ARSH_IMM:
            reg[inst.dst_reg] = (int32_t)reg[inst.dst_reg] >> inst.imm;
            reg[inst.dst_reg] &= UINT32_MAX;
            break;
        case EBPF_OP_ARSH_REG:
            reg[inst.dst_reg] = (int32_t)reg[inst.dst_reg] >> _mybpf_vm_to_u32(reg[inst.src_reg]);
            reg[inst.dst_reg] &= UINT32_MAX;
            break;

        case EBPF_OP_LE:
            if (inst.imm == 16) {
                reg[inst.dst_reg] = htole16(reg[inst.dst_reg]);
            } else if (inst.imm == 32) {
                reg[inst.dst_reg] = htole32(reg[inst.dst_reg]);
            } else if (inst.imm == 64) {
                reg[inst.dst_reg] = htole64(reg[inst.dst_reg]);
            }
            break;
        case EBPF_OP_BE:
            if (inst.imm == 16) {
                reg[inst.dst_reg] = htobe16(reg[inst.dst_reg]);
            } else if (inst.imm == 32) {
                reg[inst.dst_reg] = htobe32(reg[inst.dst_reg]);
            } else if (inst.imm == 64) {
                reg[inst.dst_reg] = htobe64(reg[inst.dst_reg]);
            }
            break;
        case EBPF_OP_ADD64_IMM:
            reg[inst.dst_reg] += inst.imm;
            break;
        case EBPF_OP_ADD64_REG:
            reg[inst.dst_reg] += reg[inst.src_reg];
            break;
        case EBPF_OP_SUB64_IMM:
            reg[inst.dst_reg] -= inst.imm;
            break;
        case EBPF_OP_SUB64_REG:
            reg[inst.dst_reg] -= reg[inst.src_reg];
            break;
        case EBPF_OP_MUL64_IMM:
            reg[inst.dst_reg] *= inst.imm;
            break;
        case EBPF_OP_MUL64_REG:
            reg[inst.dst_reg] *= reg[inst.src_reg];
            break;
        case EBPF_OP_DIV64_IMM:
            reg[inst.dst_reg] /= inst.imm;
            break;
        case EBPF_OP_DIV64_REG:
            if (reg[inst.src_reg] == 0) {
                vm->print_func("uBPF error: division by zero at PC %u\n", cur_pc);
                RETURN(BS_ERR);
            }
            reg[inst.dst_reg] /= reg[inst.src_reg];
            break;
        case EBPF_OP_OR64_IMM:
            reg[inst.dst_reg] |= inst.imm;
            break;
        case EBPF_OP_OR64_REG:
            reg[inst.dst_reg] |= reg[inst.src_reg];
            break;
        case EBPF_OP_AND64_IMM:
            reg[inst.dst_reg] &= inst.imm;
            break;
        case EBPF_OP_AND64_REG:
            reg[inst.dst_reg] &= reg[inst.src_reg];
            break;
        case EBPF_OP_LSH64_IMM:
            reg[inst.dst_reg] <<= inst.imm;
            break;
        case EBPF_OP_LSH64_REG:
            reg[inst.dst_reg] <<= reg[inst.src_reg];
            break;
        case EBPF_OP_RSH64_IMM:
            reg[inst.dst_reg] >>= inst.imm;
            break;
        case EBPF_OP_RSH64_REG:
            reg[inst.dst_reg] >>= reg[inst.src_reg];
            break;
        case EBPF_OP_NEG64:
            reg[inst.dst_reg] = -reg[inst.dst_reg];
            break;
        case EBPF_OP_MOD64_IMM:
            reg[inst.dst_reg] %= inst.imm;
            break;
        case EBPF_OP_MOD64_REG:
            if (reg[inst.src_reg] == 0) {
                vm->print_func("uBPF error: division by zero at PC %u\n", cur_pc);
                RETURN(BS_ERR);
            }
            reg[inst.dst_reg] %= reg[inst.src_reg];
            break;
        case EBPF_OP_XOR64_IMM:
            reg[inst.dst_reg] ^= inst.imm;
            break;
        case EBPF_OP_XOR64_REG:
            reg[inst.dst_reg] ^= reg[inst.src_reg];
            break;
        case EBPF_OP_MOV64_IMM:
            reg[inst.dst_reg] = inst.imm;
            break;
        case EBPF_OP_MOV64_REG:
            reg[inst.dst_reg] = reg[inst.src_reg];
            break;
        case EBPF_OP_ARSH64_IMM:
            reg[inst.dst_reg] = (int64_t)reg[inst.dst_reg] >> inst.imm;
            break;
        case EBPF_OP_ARSH64_REG:
            reg[inst.dst_reg] = (int64_t)reg[inst.dst_reg] >> reg[inst.src_reg];
            break;
        case EBPF_OP_LDXW:
            MYBPF_BOUNDS_CHECK_LOAD(4);
            reg[inst.dst_reg] = *(uint32_t *)(uintptr_t)(reg[inst.src_reg] + inst.off);
            break;
        case EBPF_OP_LDXH:
            MYBPF_BOUNDS_CHECK_LOAD(2);
            reg[inst.dst_reg] = *(uint16_t *)(uintptr_t)(reg[inst.src_reg] + inst.off);
            break;
        case EBPF_OP_LDXB:
            MYBPF_BOUNDS_CHECK_LOAD(1);
            reg[inst.dst_reg] = *(uint8_t *)(uintptr_t)(reg[inst.src_reg] + inst.off);
            break;
        case EBPF_OP_LDXDW:
            MYBPF_BOUNDS_CHECK_LOAD(8);
            reg[inst.dst_reg] = *(uint64_t *)(uintptr_t)(reg[inst.src_reg] + inst.off);
            break;

        case EBPF_OP_STW:
            MYBPF_BOUNDS_CHECK_STORE(4);
            *(uint32_t *)(uintptr_t)(reg[inst.dst_reg] + inst.off) = inst.imm;
            break;
        case EBPF_OP_STH:
            MYBPF_BOUNDS_CHECK_STORE(2);
            *(uint16_t *)(uintptr_t)(reg[inst.dst_reg] + inst.off) = inst.imm;
            break;
        case EBPF_OP_STB:
            MYBPF_BOUNDS_CHECK_STORE(1);
            *(uint8_t *)(uintptr_t)(reg[inst.dst_reg] + inst.off) = inst.imm;
            break;
        case EBPF_OP_STDW:
            MYBPF_BOUNDS_CHECK_STORE(8);
            *(uint64_t *)(uintptr_t)(reg[inst.dst_reg] + inst.off) = inst.imm;
            break;

        case EBPF_OP_STXW:
            MYBPF_BOUNDS_CHECK_STORE(4);
            *(uint32_t *)(uintptr_t)(reg[inst.dst_reg] + inst.off) = reg[inst.src_reg];
            break;
        case EBPF_OP_STXH:
            MYBPF_BOUNDS_CHECK_STORE(2);
            *(uint16_t *)(uintptr_t)(reg[inst.dst_reg] + inst.off) = reg[inst.src_reg];
            break;
        case EBPF_OP_STXB:
            MYBPF_BOUNDS_CHECK_STORE(1);
            *(uint8_t *)(uintptr_t)(reg[inst.dst_reg] + inst.off) = reg[inst.src_reg];
            break;
        case EBPF_OP_STXDW:
            MYBPF_BOUNDS_CHECK_STORE(8);
            *(uint64_t *)(uintptr_t)(reg[inst.dst_reg] + inst.off) = reg[inst.src_reg];
            break;

        case EBPF_OP_LDDW:
            reg[inst.dst_reg] = (uint32_t)inst.imm | ((uint64_t)insts[pc++].imm << 32);
            break;

        case EBPF_OP_JA:
            pc += inst.off;
            break;
        case EBPF_OP_JEQ_IMM:
            if (reg[inst.dst_reg] == inst.imm) {
                pc += inst.off;
            }
            break;
        case EBPF_OP_JEQ_REG:
            if (reg[inst.dst_reg] == reg[inst.src_reg]) {
                pc += inst.off;
            }
            break;
        case EBPF_OP_JGT_IMM:
            if (reg[inst.dst_reg] > (uint32_t)inst.imm) {
                pc += inst.off;
            }
            break;
        case EBPF_OP_JGT_REG:
            if (reg[inst.dst_reg] > reg[inst.src_reg]) {
                pc += inst.off;
            }
            break;
        case EBPF_OP_JGE_IMM:
            if (reg[inst.dst_reg] >= (uint32_t)inst.imm) {
                pc += inst.off;
            }
            break;
        case EBPF_OP_JGE_REG:
            if (reg[inst.dst_reg] >= reg[inst.src_reg]) {
                pc += inst.off;
            }
            break;
        case EBPF_OP_JLT_IMM:
            if (reg[inst.dst_reg] < (uint32_t)inst.imm) {
                pc += inst.off;
            }
            break;
        case EBPF_OP_JLT_REG:
            if (reg[inst.dst_reg] < reg[inst.src_reg]) {
                pc += inst.off;
            }
            break;
        case EBPF_OP_JLE_IMM:
            if (reg[inst.dst_reg] <= (uint32_t)inst.imm) {
                pc += inst.off;
            }
            break;
        case EBPF_OP_JLE_REG:
            if (reg[inst.dst_reg] <= reg[inst.src_reg]) {
                pc += inst.off;
            }
            break;
        case EBPF_OP_JSET_IMM:
            if (reg[inst.dst_reg] & inst.imm) {
                pc += inst.off;
            }
            break;
        case EBPF_OP_JSET_REG:
            if (reg[inst.dst_reg] & reg[inst.src_reg]) {
                pc += inst.off;
            }
            break;
        case EBPF_OP_JNE_IMM:
            if (reg[inst.dst_reg] != inst.imm) {
                pc += inst.off;
            }
            break;
        case EBPF_OP_JNE_REG:
            if (reg[inst.dst_reg] != reg[inst.src_reg]) {
                pc += inst.off;
            }
            break;
        case EBPF_OP_JSGT_IMM:
            if ((int64_t)reg[inst.dst_reg] > inst.imm) {
                pc += inst.off;
            }
            break;
        case EBPF_OP_JSGT_REG:
            if ((int64_t)reg[inst.dst_reg] > (int64_t)reg[inst.src_reg]) {
                pc += inst.off;
            }
            break;
        case EBPF_OP_JSGE_IMM:
            if ((int64_t)reg[inst.dst_reg] >= inst.imm) {
                pc += inst.off;
            }
            break;
        case EBPF_OP_JSGE_REG:
            if ((int64_t)reg[inst.dst_reg] >= (int64_t)reg[inst.src_reg]) {
                pc += inst.off;
            }
            break;
        case EBPF_OP_JSLT_IMM:
            if ((int64_t)reg[inst.dst_reg] < inst.imm) {
                pc += inst.off;
            }
            break;
        case EBPF_OP_JSLT_REG:
            if ((int64_t)reg[inst.dst_reg] < (int64_t)reg[inst.src_reg]) {
                pc += inst.off;
            }
            break;
        case EBPF_OP_JSLE_IMM:
            if ((int64_t)reg[inst.dst_reg] <= inst.imm) {
                pc += inst.off;
            }
            break;
        case EBPF_OP_JSLE_REG:
            if ((int64_t)reg[inst.dst_reg] <= (int64_t)reg[inst.src_reg]) {
                pc += inst.off;
            }
            break;
        case EBPF_OP_EXIT:
            ctx->bpf_ret = reg[0];
            return 0;
        case EBPF_OP_CALL:
            if (inst.src_reg == BPF_PSEUDO_CALL) {
                struct ebpf_inst *n = &insts[pc];
                reg[0] = _mybpf_call_sub_prog(vm, ctx, n + inst.imm, reg[1], reg[2], reg[3], reg[4], reg[5]);
            } else {
                reg[0] = _mybpf_call(vm, inst.imm, reg[1], reg[2], reg[3], reg[4], reg[5]);
                if ((inst.imm == vm->tail_call_index) && (reg[0] == 0)) {
                    ctx->bpf_ret = reg[0];
                    return 0;
                }
            }
            break;
        }
    }
}

static int _mybpf_run_stack8(MYBPF_VM_S *vm, MYBPF_CTX_S *ctx, U64 p1, U64 p2, U64 p3, U64 p4, U64 p5)
{
    char stack[8];
    ctx->stack_size = 8;
    ctx->stack = stack;
    return _mybpf_run_bpf(vm, ctx, p1, p2, p3, p4, p5);
}

static int _mybpf_run_stack16(MYBPF_VM_S *vm, MYBPF_CTX_S *ctx, U64 p1, U64 p2, U64 p3, U64 p4, U64 p5)
{
    char stack[16];
    ctx->stack_size = 16;
    ctx->stack = stack;
    return _mybpf_run_bpf(vm, ctx, p1, p2, p3, p4, p5);
}

static int _mybpf_run_stack32(MYBPF_VM_S *vm, MYBPF_CTX_S *ctx, U64 p1, U64 p2, U64 p3, U64 p4, U64 p5)
{
    char stack[32];
    ctx->stack_size = 32;
    ctx->stack = stack;
    return _mybpf_run_bpf(vm, ctx, p1, p2, p3, p4, p5);
}

static int _mybpf_run_stack64(MYBPF_VM_S *vm, MYBPF_CTX_S *ctx, U64 p1, U64 p2, U64 p3, U64 p4, U64 p5)
{
    char stack[64];
    ctx->stack_size = 64;
    ctx->stack = stack;
    return _mybpf_run_bpf(vm, ctx, p1, p2, p3, p4, p5);
}

static int _mybpf_run_stack128(MYBPF_VM_S *vm, MYBPF_CTX_S *ctx, U64 p1, U64 p2, U64 p3, U64 p4, U64 p5)
{
    char stack[128];
    ctx->stack_size = 128;
    ctx->stack = stack;
    return _mybpf_run_bpf(vm, ctx, p1, p2, p3, p4, p5);
}

static noinline int _mybpf_run_stack256(MYBPF_VM_S *vm, MYBPF_CTX_S *ctx, U64 p1, U64 p2, U64 p3, U64 p4, U64 p5)
{
    char stack[256];
    ctx->stack_size = 256;
    ctx->stack = stack;
    return _mybpf_run_bpf(vm, ctx, p1, p2, p3, p4, p5);
}

static noinline int _mybpf_run_stack512(MYBPF_VM_S *vm, MYBPF_CTX_S *ctx, U64 p1, U64 p2, U64 p3, U64 p4, U64 p5)
{
    char stack[512];
    ctx->stack_size = 512;
    ctx->stack = stack;
    return _mybpf_run_bpf(vm, ctx, p1, p2, p3, p4, p5);
}

static noinline int _mybpf_run_stack1024(MYBPF_VM_S *vm, MYBPF_CTX_S *ctx, U64 p1, U64 p2, U64 p3, U64 p4, U64 p5)
{
    char stack[1024];
    ctx->stack_size = 1024;
    ctx->stack = stack;
    return _mybpf_run_bpf(vm, ctx, p1, p2, p3, p4, p5);
}

static noinline int _mybpf_run_stack2048(MYBPF_VM_S *vm, MYBPF_CTX_S *ctx, U64 p1, U64 p2, U64 p3, U64 p4, U64 p5)
{
    char stack[2048];
    ctx->stack_size = 2048;
    ctx->stack = stack;
    return _mybpf_run_bpf(vm, ctx, p1, p2, p3, p4, p5);
}

static noinline int _mybpf_run_stack4096(MYBPF_VM_S *vm, MYBPF_CTX_S *ctx, U64 p1, U64 p2, U64 p3, U64 p4, U64 p5)
{
    char stack[4096];
    ctx->stack_size = 4096;
    ctx->stack = stack;
    return _mybpf_run_bpf(vm, ctx, p1, p2, p3, p4, p5);
}

static int _mybpf_run(MYBPF_VM_S *vm, MYBPF_CTX_S *ctx, U64 p1, U64 p2, U64 p3, U64 p4, U64 p5)
{
    ELF_PROG_INFO_S *info = _mybpf_vm_get_sub_prog(ctx, ctx->insts);
    if (! info) {
        return -1;
    }

    MYBPF_DBG_OUTPUT(MYBPF_DBG_ID_VM, MYBPF_DBG_FLAG_VM_PROCESS,
            "call %s:%s, offset:%d, addr:0x%x \n",
            info->sec_name, info->func_name, info->offset, ctx->insts);

    if (! ctx->auto_stack) {
        return _mybpf_run_stack512(vm, ctx, p1, p2, p3, p4, p5);
    }

    int stack_size = MYBPF_INSN_GetStackSize(ctx->insts, info->size);

    if (stack_size <= 8) {
        return _mybpf_run_stack8(vm, ctx, p1, p2, p3, p4, p5);
    } else if (stack_size <= 16) {
        return _mybpf_run_stack16(vm, ctx, p1, p2, p3, p4, p5);
    } else if (stack_size <= 32) {
        return _mybpf_run_stack32(vm, ctx, p1, p2, p3, p4, p5);
    } else if (stack_size <= 64) {
        return _mybpf_run_stack64(vm, ctx, p1, p2, p3, p4, p5);
    } else if (stack_size <= 128) {
        return _mybpf_run_stack128(vm, ctx, p1, p2, p3, p4, p5);
    } else if (stack_size <= 256) {
        return _mybpf_run_stack256(vm, ctx, p1, p2, p3, p4, p5);
    } else if (stack_size <= 512) {
        return _mybpf_run_stack512(vm, ctx, p1, p2, p3, p4, p5);
    } else if (stack_size <= 1024) {
        return _mybpf_run_stack1024(vm, ctx, p1, p2, p3, p4, p5);
    } else if (stack_size <= 2048) {
        return _mybpf_run_stack2048(vm, ctx, p1, p2, p3, p4, p5);
    } else if (stack_size <= 4096) {
        return _mybpf_run_stack4096(vm, ctx, p1, p2, p3, p4, p5);
    } else {
        BS_DBGASSERT(0);
        RETURN(BS_OUT_OF_RANGE);
    }
}

int MYBPF_Run(MYBPF_VM_S *vm, MYBPF_CTX_S *ctx, UINT64 p1, UINT64 p2, UINT64 p3, UINT64 p4, UINT64 p5)
{
    /* 注意: 它需要已经调用过MYBPF_PROG_FixupBpfCalls */
    return _mybpf_run(vm, ctx, p1, p2, p3, p4, p5);
}

/* 以3个ebpf参数方式运行 */
int MYBPF_RunP3(MYBPF_VM_S *vm, MYBPF_CTX_S *ctx, UINT64 p1, UINT64 p2, UINT64 p3)
{
    return _mybpf_run(vm, ctx, p1, p2, p3, 0, 0);
}


