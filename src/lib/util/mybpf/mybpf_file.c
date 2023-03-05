/*********************************************************
*   Copyright (C) LiXingang
*   Description: 
*
********************************************************/
#include "bs.h"
#include "utl/mybpf_vm.h"
#include "utl/bpf_helper_utl.h"
#include "utl/mybpf_runtime.h"
#include "utl/mybpf_loader.h"
#include "utl/mybpf_prog.h"
#include "utl/mybpf_hookpoint.h"
#include "utl/mybpf_elf.h"
#include "utl/mybpf_file.h"

int MYBPF_WalkProg(char *file, PF_MYBPF_ELF_WALK_PROG walk_func, void *ud)
{
    ELF_S elf;

    int ret = ELF_Open(file, &elf);
    if (ret < 0) {
        RETURNI(BS_CAN_NOT_OPEN, "Can't open file %s \r\n", file);
    }

    ret = MYBPF_ELF_WalkProg(&elf, walk_func, ud);

    ELF_Close(&elf);

    return ret;
}

int MYBPF_WalkMap(char *file, PF_MYBPF_ELF_WALK_MAP walk_func, void *ud)
{
    ELF_S elf;

    int ret = ELF_Open(file, &elf);
    if (ret < 0) {
        RETURNI(BS_CAN_NOT_OPEN, "Can't open file %s \r\n", file);
    }

    ret = MYBPF_ELF_WalkMap(&elf, walk_func, ud);

    ELF_Close(&elf);

    return ret;
}

static inline int _mybpf_run_file(MYBPF_RUNTIME_S *runtime, MYBPF_FILE_CTX_S *ctx,
        UINT64 p1, UINT64 p2, UINT64 p3, UINT64 p4, UINT64 p5)
{
    int ret;
    MYBPF_LOADER_PARAM_S p = {0};

    p.instance = "runfile";
    p.filename = ctx->file;

    if (ctx->jit) {
        p.flag |= MYBPF_LOADER_FLAG_JIT;
    }

    if ((ret = MYBPF_LoaderLoad(runtime, &p)) < 0) {
        return ret;
    }

    MYBPF_PROG_NODE_S *prog = NULL;
    while ((prog = MYBPF_PROG_GetNext(runtime, "runfile", ctx->sec_prefix, prog))) {
        ret = MYBPF_PROG_Run(prog, &ctx->bpf_ret, p1, p2, p3, p4, p5);
        if (ret == MYBPF_HP_RET_STOP) {
            break;
        }
    }

    return ret;
}

int MYBPF_RunFile(MYBPF_FILE_CTX_S *ctx, UINT64 p1, UINT64 p2, UINT64 p3, UINT64 p4, UINT64 p5)
{
    MYBPF_RUNTIME_S runtime;
    int ret;

    if ((ret = MYBPF_RuntimeInit(&runtime, 128)) < 0) {
        return ret;
    }

    ret = _mybpf_run_file(&runtime, ctx, p1, p2, p3, p4, p5);

    MYBPF_RuntimeFini(&runtime);

    return ret;
}

