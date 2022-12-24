/*================================================================
*   Created by LiXingang, Copyright LiXingang
*   Description: 
*
================================================================*/
#ifndef _MYBPF_VM_H
#define _MYBPF_VM_H

#include "utl/bpf_helper_utl.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct {
    uint32_t base_func_max;
    uint32_t user_func_min;
    uint32_t user_func_max;
    int tail_call_index;
    PF_BPF_HELPER_FUNC *base_helpers;
    PF_BPF_HELPER_FUNC *user_helpers;
    PF_PRINT_FUNC print_func;
}MYBPF_VM_S;

typedef struct {
    uint64_t bpf_ret;
    void *insts;
    void *mem;        /* 可以访问的内存起始地址 */
    uint32_t mem_len; /* 可以访问的内存长度 */
    uint32_t mem_check: 1; /* 检查内存是否访问越界 */
}MYBPF_CTX_S;

int MYBPF_SetFunc(MYBPF_VM_S *vm, int idx, PF_BPF_HELPER_FUNC func);
int MYBPF_SetTailCallIndex(MYBPF_VM_S *vm, unsigned int idx);
bool MYBPF_Validate(MYBPF_VM_S *vm, void *insn, uint32_t num_insts);
int MYBPF_Run(MYBPF_VM_S *vm, MYBPF_CTX_S *ctx, UINT64 p1, UINT64 p2, UINT64 p3, UINT64 p4, UINT64 p5);
int MYBPF_RunP3(MYBPF_VM_S *vm, MYBPF_CTX_S *ctx, UINT64 p1, UINT64 p2, UINT64 p3);

/* 使用默认环境调用 */
int MYBPF_DefultRun(MYBPF_CTX_S *ctx, U64 r1, U64 r2, U64 r3, U64 r4, U64 r5);

#ifdef __cplusplus
}
#endif
#endif 
