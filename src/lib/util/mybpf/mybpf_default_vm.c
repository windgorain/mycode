/*================================================================
*   Created by LiXingang
*   Description: 大部分情况下, 采用默认内置的vm环境即可
*                省去了MYBPF_Run中需要自己构建vm环境的过程
*
================================================================*/
#include "bs.h"
#include "utl/mybpf_vm.h"
#include "utl/bpf_helper_utl.h"

static int _mybpf_default_vm_print(char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    return 0;
}

static MYBPF_VM_S g_mybpf_default_vm = {
    .ext_func_max = BPF_HELPER_MAX,
    .ext_funcs = (void*)g_bpf_helper_tbl,
    .print_func = _mybpf_default_vm_print
};

/* 使用默认环境调用 */
int MYBPF_DefultRun(MYBPF_CTX_S *ctx, U64 r1, U64 r2, U64 r3, U64 r4, U64 r5)
{
    return MYBPF_Run(&g_mybpf_default_vm, ctx, r1, r2, r3, r4, r5);
}

