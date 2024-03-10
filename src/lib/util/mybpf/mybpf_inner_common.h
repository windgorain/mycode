/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0
* Description:
******************************************************************************/
#ifndef _MYBPF_INNER_COMMON_H
#define _MYBPF_INNER_COMMON_H
#ifdef __cplusplus
extern "C"
{
#endif

#define MYBPF_JIT_MAX_INSTS 65536

typedef struct jump {
    uint32_t offset_loc;
    uint32_t target_pc;
}MYBPF_JIT_JUMP_S;

typedef struct {
    uint32_t offset_loc;
}MYBPF_JIT_CALL_S;

typedef struct jit_state {
    uint8_t *buf;
    IN uint32_t prog_offset; 
    uint32_t offset;
    uint32_t size;
    uint32_t *pc_locs;
    uint32_t exit_loc;
    uint32_t unwind_loc;
    MYBPF_JIT_JUMP_S *jumps;
    MYBPF_JIT_CALL_S *calls;
    int num_jumps;
    int num_calls;
    int max_num_calls;
    uint32_t stack_size;
    uint32_t is_main_prog: 1; 
    MYBPF_JIT_CFG_S *jit_cfg;
}MYBPF_JIT_STATE_S;

static noinline int translate(MYBPF_JIT_VM_S *vm, MYBPF_JIT_CTX_S *ctx, MYBPF_JIT_STATE_S *state);
static noinline void resolve_jumps(struct jit_state* state);

static inline int _mybpf_jit_common_do(MYBPF_JIT_VM_S *vm, OUT MYBPF_JIT_CTX_S *jit_ctx)
{
    MYBPF_JIT_STATE_S state = {0};
    int result = -1;
    void * pc_locs = NULL;

    state.prog_offset = jit_ctx->prog_offset;
    state.size = jit_ctx->max_jitted_size;
    state.buf = jit_ctx->jitted_buf;
    state.is_main_prog = jit_ctx->is_main_prog;
    state.jit_cfg = jit_ctx->jit_cfg;

    state.pc_locs = jit_ctx->locs;
    if (! state.pc_locs) {
        state.pc_locs = pc_locs = calloc(vm->num_insts, sizeof(state.pc_locs[0]));
        if (! pc_locs) {
            goto out;
        }
    }

    state.jumps = calloc(MYBPF_JIT_MAX_INSTS, sizeof(state.jumps[0]));
    if (! state.jumps) {
        goto out;
    }

    state.num_jumps = 0;
    state.offset = 0;

    if (translate(vm, jit_ctx, &state) < 0) {
        goto out;
    }

    if (state.num_jumps == MYBPF_JIT_MAX_INSTS) {
        RETURNI(BS_ERR, "Excessive number of jump targets");
        goto out;
    }
    
    if (state.offset == state.size) {
        RETURNI(BS_ERR, "Target buffer too small");
        goto out;
    }

    resolve_jumps(&state);
    result = 0;

    jit_ctx->jitted_size = state.offset;

out:
    if (pc_locs) {
        free(pc_locs);
    }
    if (state.jumps) {
        free(state.jumps);
    }
    return result;
}

#ifdef __cplusplus
}
#endif
#endif 
