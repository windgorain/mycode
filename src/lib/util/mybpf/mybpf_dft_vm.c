/*================================================================
*   Created by LiXingang
*   Description: 大部分情况下, 采用默认内置的vm环境即可
*                这样就省去了自己构建vm环境
*                default vm是禁止动态修改的, 它是const的
*
================================================================*/
#include "bs.h"
#include "utl/mybpf_vm.h"
#include "utl/bpf_helper_utl.h"

static int _mybpf_default_vm_print(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    return 0;
}

static const MYBPF_VM_S g_mybpf_default_vm = {
    .print_func = _mybpf_default_vm_print
};

/* 使用默认环境调用 */
int MYBPF_DefultRun(MYBPF_CTX_S *ctx, U64 r1, U64 r2, U64 r3, U64 r4, U64 r5)
{
    return MYBPF_Run((void*)&g_mybpf_default_vm, ctx, r1, r2, r3, r4, r5);
}

/* 使用默认环境调用 */
int MYBPF_DefultRunCode(void *code, OUT UINT64 *bpf_ret, U64 r1, U64 r2, U64 r3, U64 r4, U64 r5)
{
    MYBPF_CTX_S ctx = {0};
    ctx.insts = code;

    int ret = MYBPF_Run((void*)&g_mybpf_default_vm, &ctx, r1, r2, r3, r4, r5);
    if (ret < 0) {
        return ret;
    }

    if (bpf_ret) {
        *bpf_ret = ctx.bpf_ret;
    }

    return 0;
}

