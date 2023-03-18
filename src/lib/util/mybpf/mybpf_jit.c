/*********************************************************
*   Copyright (C) LiXingang
*   Date: 2022.7.11
*   Description: 
*
********************************************************/
#include "bs.h"
#include "utl/ufd_utl.h"
#include "utl/umap_utl.h"
#include "utl/bpf_helper_utl.h"
#include "utl/mybpf_loader.h"
#include "utl/mybpf_prog.h"
#include "utl/mybpf_vm.h"
#include "utl/mybpf_jit.h"
#include "utl/mybpf_insn.h"
#include "utl/mybpf_file.h"
#include "mybpf_jit_inner.h"

static char * g_mybpf_jit_arch_name[] = {
    [MYBPF_JIT_ARCH_NONE] = "none",
    [MYBPF_JIT_ARCH_ARM64] = "arm64",
};

static MYBPF_JIT_ARCH_S g_mybpf_jit_arch[] = {
    [MYBPF_JIT_ARCH_ARM64] = {
        .filename = "mybpf_jit_arm64.o"
    },
};

static MYBPF_JIT_ARCH_S * _mybpf_jit_get_arch(int arch_type)
{
    if ((arch_type <= 0) || (arch_type >= MYBPF_JIT_ARCH_MAX)) {
        return NULL;
    }

    return &g_mybpf_jit_arch[arch_type];
}

static void _mybpf_jit_fini_res(MYBPF_JIT_RES_S *res)
{
    MEM_ExistFree(res->locs);
    MEM_ExistFree(res->jitted_buf);
    MEM_ExistFree(res->calls);
    MEM_ExistFree(res->progs);
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

static void _mybpf_recacl_locs(OUT UINT *locs, int num, int offset)
{
    int i;
    for (i=0; i<num; i++) {
        locs[i] += offset;
    }
}

static int _mybpf_prog_jit_progs(MYBPF_JIT_ARCH_S *arch, MYBPF_JIT_INSN_S *jit_insn,
        MYBPF_JIT_RES_S *res, MYBPF_JIT_CFG_S *cfg)
{
    MYBPF_JIT_CTX_S jit_ctx = {0};
    MYBPF_JIT_VM_S vm = {0};
    int i;
    int ret;
    int totle_jitted_size = 0;

    if (cfg->helper_mode == MYBPF_JIT_HELPER_MODE_BASE) {
        vm.base_func_addr = (uintptr_t)BpfHelper_BaseHelper;
        vm.tail_call_func = BpfHelper_GetFunc(12) - (PF_BPF_HELPER_FUNC) BpfHelper_BaseHelper;
    }

    jit_ctx.max_jitted_size = res->max_jitted_size;
    jit_ctx.locs = res->locs;

    jit_ctx.jit_cfg = cfg;

    for (i=0; i<jit_insn->progs_count; i++) {
        vm.insts = (void*)((char*)jit_insn->insts + res->progs[i].prog_offset);
        vm.num_insts = res->progs[i].size / sizeof(MYBPF_INSN_S);
        jit_ctx.jitted_buf = (char*)res->jitted_buf + totle_jitted_size;
        jit_ctx.max_jitted_size = res->max_jitted_size - totle_jitted_size;
        jit_ctx.stack_size = MYBPF_INSN_GetStackSize(vm.insts, res->progs[i].size);
        jit_ctx.jitted_size = 0;

        if (strcmp(res->progs[i].sec_name, ".text") == 0) {
            jit_ctx.is_main_prog = 0;
        } else {
            jit_ctx.is_main_prog = 1;
        }

        //ret = arch->jit_func(&vm, &jit_ctx);
        ret = MYBPF_RunFile(arch->filename, "api/MYBPF_Jit_Do", (long)&vm, (long)&jit_ctx, 0, 0, 0);
        if (ret < 0) {
            return ret;
        }

        _mybpf_recacl_locs(jit_ctx.locs, vm.num_insts, totle_jitted_size);
        res->progs[i].prog_offset = totle_jitted_size;
        res->progs[i].size = jit_ctx.jitted_size;
        totle_jitted_size += jit_ctx.jitted_size;
        jit_ctx.locs += vm.num_insts;
    }

    return totle_jitted_size;
}

static int _mybpf_jit_do(MYBPF_JIT_ARCH_S *arch, MYBPF_JIT_INSN_S *jit_insn,
        MYBPF_JIT_RES_S *res, MYBPF_JIT_CFG_S *cfg)
{
    int ret;
    int jitted_size;

    ret = jitted_size = _mybpf_prog_jit_progs(arch, jit_insn, res, cfg);
    if (ret < 0) {
        return ret;
    }

    MYBPF_RunFile(arch->filename, "api/MYBPF_Jit_FixBpfCalls", (long)res, 0, 0, 0, 0);

    void *jitted_code = MEM_Dup(res->jitted_buf, jitted_size);
    if (! jitted_code) {
        RETURNI(BS_ERR, "mmap error");
    }

    jit_insn->insts = jitted_code;
    jit_insn->insts_len = jitted_size;

    memcpy(jit_insn->progs, res->progs, sizeof(jit_insn->progs[0]) * jit_insn->progs_count);

    return 0;
}

static int _mybpf_prog_jit(MYBPF_JIT_ARCH_S *arch, MYBPF_JIT_INSN_S *jit_insn, MYBPF_JIT_CFG_S *cfg)
{
    MYBPF_JIT_RES_S res = {0};
    int ret;

    ret = _mybpf_jit_init_res(jit_insn, &res);
    if (ret < 0) {
        return ret;
    }

    ret = _mybpf_jit_do(arch, jit_insn, &res, cfg);

    _mybpf_jit_fini_res(&res);

    return ret;
}

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

int MYBPF_Jit(MYBPF_JIT_INSN_S *jit_insn, MYBPF_JIT_CFG_S *cfg)
{
    int arch_type = cfg->jit_arch;

    if (arch_type == MYBPF_JIT_ARCH_NONE) {
        arch_type = MYBPF_JIT_LocalArch();
    }

    if ((arch_type <= 0) || (arch_type >= MYBPF_JIT_ARCH_MAX)) {
        RETURN(BS_BAD_PARA);
    }

    MYBPF_JIT_ARCH_S *arch = _mybpf_jit_get_arch(arch_type);
    if (! arch) {
        RETURN(BS_ERR);
    }

    return _mybpf_prog_jit(arch, jit_insn, cfg);
}

