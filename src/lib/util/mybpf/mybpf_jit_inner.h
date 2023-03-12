/*********************************************************
*   Copyright (C) LiXingang
*   Description: 
*
********************************************************/
#ifndef _MYBPF_JIT_INNER_H
#define _MYBPF_JIT_INNER_H
#ifdef __cplusplus
extern "C"
{
#endif

typedef struct MYBPF_JIT_RES_S {
    int max_jitted_size;
    int jitted_size;
    int calls_count;
    UINT *locs; /* insn index to jitted offset */ 
    void *jitted_buf;
    MYBPF_INSN_CALLS_S *calls;
    ELF_PROG_INFO_S *progs;
}MYBPF_JIT_RES_S;

typedef struct {
    MYBPF_JIT_CFG_S *jit_cfg;

    UINT is_main_prog: 1; /* 是否main prog */

    IN int stack_size;
    IN UINT max_jitted_size;
    OUT UINT jitted_size;
    INOUT void *jitted_buf;
    UINT * locs; /* insn index to jitted offset */
}MYBPF_JIT_CTX_S;

int MYBPF_JitArm64_Jit(MYBPF_JIT_VM_S *vm, OUT MYBPF_JIT_CTX_S *jit_ctx);
int MYBPF_JitArm64_FixBpfCalls(MYBPF_JIT_RES_S *res);

typedef int (*PF_MYBPF_ARCH_Jit)(MYBPF_JIT_VM_S *vm, OUT MYBPF_JIT_CTX_S *jit_ctx);
typedef int (*PF_MYBPF_ARCH_FixBpfCalls)(MYBPF_JIT_RES_S *res);

typedef struct {
    PF_MYBPF_ARCH_Jit jit_func;
    PF_MYBPF_ARCH_FixBpfCalls fix_bpf_calls;
}MYBPF_JIT_ARCH_S;

#ifdef __cplusplus
}
#endif
#endif //MYBPF_JIT_INNER_H_
