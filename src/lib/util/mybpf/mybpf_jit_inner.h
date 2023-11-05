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
    void *jitted_buf;

    UINT *locs;  

    ELF_PROG_INFO_S *progs;
}MYBPF_JIT_RES_S;

typedef struct {
    MYBPF_JIT_CFG_S *jit_cfg;

    UINT is_main_prog: 1; 

    IN int stack_size;
    IN UINT prog_offset; 
    IN UINT max_jitted_size;
    OUT UINT jitted_size;
    INOUT void *jitted_buf;
    UINT * locs; 
}MYBPF_JIT_CTX_S;

int MYBPF_JitArm64_Jit(MYBPF_JIT_VM_S *vm, OUT MYBPF_JIT_CTX_S *jit_ctx);
int MYBPF_JitArm64_FixBpfCall(void *to_fix, void *func);
int MYBPF_JitArm64_FixLoadFuncPtr(void *to_fix, UINT64 func);

int MYBPF_JitX64_Jit(MYBPF_JIT_VM_S *vm, OUT MYBPF_JIT_CTX_S *jit_ctx);
int MYBPF_JitX64_FixBpfCall(void *to_fix, void *func);
int MYBPF_JitX64_FixLoadFuncPtr(void *to_fix, UINT64 func);

typedef int (*PF_MYBPF_ARCH_Jit)(MYBPF_JIT_VM_S *vm, OUT MYBPF_JIT_CTX_S *jit_ctx);

typedef int (*PF_MYBPF_ARCH_FixBpfCall)(void *to_fix, void *func);

typedef int (*PF_MYBPF_ARCH_FixLoadFuncPtr)(void *to_fix, UINT64 func);

typedef struct {
    PF_MYBPF_ARCH_Jit jit_func;
    PF_MYBPF_ARCH_FixBpfCall fix_bpf_call;
    PF_MYBPF_ARCH_FixLoadFuncPtr fix_load_func_ptr;
}MYBPF_JIT_ARCH_S;

#ifdef __cplusplus
}
#endif
#endif 
