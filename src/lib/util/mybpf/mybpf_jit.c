/*********************************************************
*   Copyright (C) LiXingang
*   Date: 2022.7.11
*   Description: 
*
********************************************************/
#include <sys/mman.h>
#include "bs.h"
#include "utl/ufd_utl.h"
#include "utl/umap_utl.h"
#include "utl/bpf_helper_utl.h"
#include "utl/mybpf_loader.h"
#include "utl/mybpf_prog.h"
#include "utl/mybpf_vm.h"
#include "utl/mybpf_jit.h"
#include "utl/mybpf_insn.h"
#include "mybpf_jit_inner.h"

static char * g_mybpf_jit_arch_name[] = {
    [MYBPF_JIT_ARCH_NONE] = "none",
    [MYBPF_JIT_ARCH_ARM64] = "arm64",
};

static MYBPF_JIT_ARCH_S g_mybpf_jit_arch[] = {
    [MYBPF_JIT_ARCH_ARM64] = {
        .jit_func = MYBPF_JitArm64_Jit,
        .fix_bpf_calls = MYBPF_JitArm64_FixBpfCalls,
    },
};

/* 获取jit runtime */
static MYBPF_JIT_ARCH_S * _mybpf_jit_get_arch(int arch_type)
{
    if ((arch_type <= 0) || (arch_type >= MYBPF_JIT_ARCH_MAX)) {
        return NULL;
    }

    return &g_mybpf_jit_arch[arch_type];
}

static void _mybpf_jit_fini_res(MYBPF_JIT_RES_S *res)
{
    MEM_FREE_NULL(res->locs);
    MEM_FREE_NULL(res->jitted_buf);
    MEM_FREE_NULL(res->calls);
    MEM_FREE_NULL(res->progs);
}

static int _mybpf_jit_init_res(MYBPF_JIT_INSN_S *jit_insn, OUT MYBPF_JIT_RES_S *res)
{
    int num_insts = jit_insn->insts_len / sizeof(MYBPF_INSN_S);
    res->max_jitted_size = jit_insn->insts_len * 8;

    res->locs = MEM_Malloc(sizeof(UINT) * num_insts);
    res->jitted_buf = MEM_Malloc(res->max_jitted_size);
    res->progs = MEM_ZMalloc(sizeof(ELF_PROG_INFO_S) * jit_insn->progs_count);

    if ((! res->locs) || (! res->jitted_buf) || (! res->progs)) {
        _mybpf_jit_fini_res(res);
        RETURNI(BS_NO_MEMORY, "Alloc memory error");
    }

    memcpy(res->progs, jit_insn->progs, sizeof(jit_insn->progs[0]) * jit_insn->progs_count);

    res->calls_count = MYBPF_INSN_GetCallsCount(jit_insn->insts, jit_insn->insts_len);
    if (res->calls_count) {
        res->calls = MEM_ZMalloc(sizeof(MYBPF_INSN_CALLS_S) * res->calls_count);
        if (! res->calls) {
            _mybpf_jit_fini_res(res);
            RETURNI(BS_NO_MEMORY, "Can't alloc memory");
        }
        MYBPF_INSN_GetCallsInfo(jit_insn->insts, jit_insn->insts_len, res->calls, res->calls_count);
    }

    return 0;
}

/* 校正locs */
static void _mybpf_recacl_locs(OUT UINT *locs, int num, int offset)
{
    int i;
    for (i=0; i<num; i++) {
        locs[i] += offset;
    }
}

/* 返回jitted_size. 失败返回<0 */
static int _mybpf_prog_jit_progs(MYBPF_JIT_ARCH_S *arch, MYBPF_JIT_INSN_S *jit_insn, MYBPF_JIT_RES_S *res, UINT flag)
{
    MYBPF_JIT_CTX_S ctx = {0};
    MYBPF_JIT_VM_S vm = {0};
    int i;
    int ret;
    int totle_jitted_size = 0;

    /* 不使用 imm = imm + base方式, 将base addr设置为0 */
    if (flag & MYBPF_JIT_FLAG_BPF_BASE) {
        vm.base_func_addr = (uintptr_t)BpfHelper_BaseHelper;
        vm.tail_call_func = BpfHelper_GetFunc(12) - BpfHelper_BaseHelper;
    }

    ctx.max_jitted_size = res->max_jitted_size;
    ctx.locs = res->locs;

    if (flag & MYBPF_JIT_FLAG_USE_AGENT)
        ctx.call_ext_agent = 1;

    for (i=0; i<jit_insn->progs_count; i++) {
        vm.insts = (void*)((char*)jit_insn->insts + res->progs[i].offset);
        vm.num_insts = res->progs[i].size / sizeof(MYBPF_INSN_S);
        ctx.jitted_buf = (char*)res->jitted_buf + totle_jitted_size;
        ctx.max_jitted_size = res->max_jitted_size - totle_jitted_size;
        ctx.stack_size = MYBPF_INSN_GetStackSize(vm.insts, res->progs[i].size);
        ctx.jitted_size = 0;

        if (flag & MYBPF_JIT_FLAG_USE_AGENT) {
            ctx.save_ext_agent = 1;
            if (strcmp(res->progs[i].sec_name, ".text") == 0) {
                ctx.save_ext_agent = 0;
            }
        }

        ret = arch->jit_func(&vm, &ctx);
        if (ret < 0) {
            return ret;
        }

        _mybpf_recacl_locs(ctx.locs, vm.num_insts, totle_jitted_size);
        res->progs[i].offset = totle_jitted_size;
        res->progs[i].size = ctx.jitted_size;
        totle_jitted_size += ctx.jitted_size;
        ctx.locs += vm.num_insts;
    }

    return totle_jitted_size;
}

static int _mybpf_jit_do(MYBPF_JIT_ARCH_S *arch, MYBPF_JIT_INSN_S *jit_insn, MYBPF_JIT_RES_S *res, UINT flag)
{
    int ret;
    int jitted_size;

    ret = jitted_size = _mybpf_prog_jit_progs(arch, jit_insn, res, flag);
    if (ret < 0) {
        return ret;
    }

    arch->fix_bpf_calls(res);

    void *jitted_code = MYBPF_JIT_Mmap(res->jitted_buf, jitted_size);
    if (! jitted_code) {
        RETURNI(BS_ERR, "mmap error");
    }

    jit_insn->insts = jitted_code;
    jit_insn->insts_len = jitted_size;

    /* 更新progs info */
    memcpy(jit_insn->progs, res->progs, sizeof(jit_insn->progs[0]) * jit_insn->progs_count);

    return 0;
}

static int _mybpf_prog_jit(MYBPF_JIT_ARCH_S *arch, MYBPF_JIT_INSN_S *jit_insn, UINT flag)
{
    MYBPF_JIT_RES_S res = {0};
    int ret;

    ret = _mybpf_jit_init_res(jit_insn, &res);
    if (ret < 0) {
        return ret;
    }

    ret = _mybpf_jit_do(arch, jit_insn, &res, flag);

    _mybpf_jit_fini_res(&res);

    return ret;
}

void * MYBPF_JIT_Mmap(void *jitted_buf, int jitted_size)
{
    void *jitted_code = NULL;

    jitted_code = mmap(0, jitted_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (jitted_code == MAP_FAILED) {
        return NULL;
    }

    memcpy(jitted_code, jitted_buf, jitted_size);

    if (mprotect(jitted_code, jitted_size, PROT_READ | PROT_EXEC) < 0) {
        munmap(jitted_code, jitted_size);
        return NULL;
    }

    return jitted_code;
}

/* 根据jit arch name获取jit arch type */
int MYBPF_JIT_GetJitTypeByName(char *jit_arch_name)
{
    if (! jit_arch_name) {
        return MYBPF_JIT_ARCH_NONE;
    }

    if (strcmp("arm64", jit_arch_name) == 0) {
        return MYBPF_JIT_ARCH_ARM64;
    }

    return MYBPF_JIT_ARCH_NONE;
}

/* 获取本地架构的jit arch type */
int MYBPF_JIT_LocalArch(void)
{
#ifdef __ARM64__
    return MYBPF_JIT_ARCH_ARM64;
#else
    return MYBPF_JIT_ARCH_NONE;
#endif
}

char * MYBPF_JIT_GetArchName(int arch_type)
{
    if ((arch_type <= 0) || (arch_type >= MYBPF_JIT_ARCH_MAX)) {
        return g_mybpf_jit_arch_name[MYBPF_JIT_ARCH_NONE];
    }
    return g_mybpf_jit_arch_name[arch_type];
}

int MYBPF_JitArch(int arch_type, MYBPF_JIT_INSN_S *jit_insn, UINT flag)
{
    if ((arch_type <= 0) || (arch_type >= MYBPF_JIT_ARCH_MAX)) {
        RETURN(BS_BAD_PARA);
    }

    MYBPF_JIT_ARCH_S *arch = _mybpf_jit_get_arch(arch_type);
    if (! arch) {
        RETURN(BS_ERR);
    }

    return _mybpf_prog_jit(arch, jit_insn, flag);
}

int MYBPF_JitLocal(MYBPF_JIT_INSN_S *jit_insn, UINT flag)
{
    return MYBPF_JitArch(MYBPF_JIT_LocalArch(), jit_insn, flag);
}

