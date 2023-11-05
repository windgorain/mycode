/*================================================================
*   Created by LiXingang, Copyright LiXingang
*   Date: 2017.10.1
*   Description: ebpf prog管理
*
================================================================*/
#include "bs.h"
#include "utl/ufd_utl.h"
#include "utl/umap_utl.h"
#include "utl/bpf_helper_utl.h"
#include "utl/mybpf_loader.h"
#include "utl/mybpf_prog_def.h"
#include "utl/mybpf_prog.h"
#include "utl/mybpf_vm.h"
#include "utl/mybpf_jit.h"

static void _mybpf_prog_free_prog_rcu(void *pstRcuNode)
{
    MYBPF_PROG_NODE_S *node = container_of(pstRcuNode, MYBPF_PROG_NODE_S, rcu_node);
    MEM_Free(node);
}

static inline void _mybpf_prog_free_prog(void *f)
{
    MYBPF_PROG_NODE_S *node = f;
    RcuEngine_Call(&node->rcu_node, _mybpf_prog_free_prog_rcu);
}

static inline int _mybpf_prog_run_bpf_code(MYBPF_PROG_NODE_S *prog, OUT UINT64 *bpf_ret,
        UINT64 p1, UINT64 p2, UINT64 p3, UINT64 p4, UINT64 p5)
{
    MYBPF_CTX_S ctx = {0};
    MYBPF_LOADER_NODE_S *n = prog->loader_node;
    ctx.start_addr = n->insts;
    ctx.mybpf_prog = prog;
    ctx.insts = prog->insn;
    ctx.auto_stack = 1;
    ctx.helper_fixed = 0;

    int ret = MYBPF_DefultRun(&ctx, p1, p2, p3, p4, p5);
    if (ret < 0) {
        return ret;
    }

    *bpf_ret = ctx.bpf_ret;

    return 0;
}

typedef UINT64 (*PF_BPF_JITTED_FUNC)(UINT64 p1, UINT64 p2, UINT64 p3, UINT64 p4, UINT64 p5);

static inline int _mybpf_prog_run_jitted_code(MYBPF_PROG_NODE_S *prog, OUT UINT64 *bpf_ret,
        UINT64 p1, UINT64 p2, UINT64 p3, UINT64 p4, UINT64 p5)
{
    PF_BPF_JITTED_FUNC func = prog->insn;
    *bpf_ret = func(p1, p2, p3, p4, p5);
    return 0;
}

static inline int _mybpf_prog_run(MYBPF_PROG_NODE_S *prog, OUT UINT64 *bpf_ret,
        UINT64 p1, UINT64 p2, UINT64 p3, UINT64 p4, UINT64 p5)
{
    MYBPF_LOADER_NODE_S *n = prog->loader_node;

    if (n->jitted) {
        return _mybpf_prog_run_jitted_code(prog, bpf_ret, p1, p2, p3, p4, p5);
    }

    return _mybpf_prog_run_bpf_code(prog, bpf_ret, p1, p2, p3, p4, p5);
}


MYBPF_PROG_NODE_S * MYBPF_PROG_Alloc(void *insn, int len, char *sec_name, char *prog_name)
{
    MYBPF_PROG_NODE_S *node;

    node = MEM_ZMalloc(sizeof(MYBPF_PROG_NODE_S));
    if (! node) {
        return NULL;
    }
    
    if (sec_name) {
        strlcpy(node->sec_name, sec_name, sizeof(node->sec_name));
    }

    if (prog_name) {
        strlcpy(node->prog_name, prog_name, sizeof(node->prog_name));
    }

    node->insn_len = len;
    node->insn = insn;

    return node;
}

void MYBPF_PROG_Free(MYBPF_RUNTIME_S *runtime, MYBPF_PROG_NODE_S *prog)
{
    _mybpf_prog_free_prog(prog);
}

void MYBPF_PROG_ShowProg(MYBPF_RUNTIME_S *runtime, PF_PRINT_FUNC print_func)
{
    MYBPF_PROG_NODE_S *prog;
    void *iter = NULL;
    MYBPF_LOADER_NODE_S *n;
    int i;

    while ((n = MYBPF_LoaderGetNext(runtime, &iter))) {
        for (i=0; i<n->main_prog_count; i++) {
            prog = n->main_progs[i];
            print_func("xlated:%u, sec:%s, name:%s, instance:%s, file:%s \r\n",
                    prog->insn_len, prog->sec_name, prog->prog_name,
                    n->param.instance, n->param.filename);
        }
    }
}


MYBPF_PROG_NODE_S * MYBPF_PROG_GetByFuncName(MYBPF_RUNTIME_S *runtime, char *instance, char *func_name)
{
    int i;
    MYBPF_PROG_NODE_S *prog;

    MYBPF_LOADER_NODE_S *loader = MYBPF_LoaderGet(runtime, instance);
    if (! loader) {
        return NULL;
    }

    for (i=0; i<loader->main_prog_count; i++) {
        prog = loader->main_progs[i];
        BS_DBGASSERT(prog);
        if (strcmp(prog->prog_name, func_name) == 0) {
            return prog;
        }
    }

    return NULL;
}


MYBPF_PROG_NODE_S * MYBPF_PROG_GetBySecName(MYBPF_RUNTIME_S *runtime, char *instance, char *sec_name)
{
    int i;
    MYBPF_PROG_NODE_S *prog;

    MYBPF_LOADER_NODE_S *loader = MYBPF_LoaderGet(runtime, instance);
    if (! loader) {
        return NULL;
    }

    for (i=0; i<loader->main_prog_count; i++) {
        prog = loader->main_progs[i];
        BS_DBGASSERT(prog);
        if (strcmp(prog->sec_name, sec_name) == 0) {
            return prog;
        }
    }

    return NULL;
}

static MYBPF_PROG_NODE_S * _mybpf_prog_get_first(MYBPF_RUNTIME_S *runtime,
        MYBPF_LOADER_NODE_S *loader, char *sec_name)
{
    int sec_name_len = 0;
    int wildcard = 0;

    if (! loader) {
        return NULL;
    }

    if (sec_name) {
        sec_name_len = strlen(sec_name);
        if (sec_name_len == 0) {
            sec_name = NULL;
        } else if (sec_name[sec_name_len - 1] == '/') {
            wildcard = 1;
        }
    }

    for (int i=0; i<loader->main_prog_count; i++) {
        MYBPF_PROG_NODE_S * prog = loader->main_progs[i];
        if (! sec_name) {
            return prog;
        }
        if ((wildcard) && (0 == strncmp(sec_name, prog->sec_name, sec_name_len))) {
            return prog;
        }
        if ((! wildcard) && (0 == strcmp(sec_name, prog->sec_name))) {
            return prog;
        }
    }

    return NULL;
}

static MYBPF_PROG_NODE_S * _mybpf_prog_get_next(MYBPF_RUNTIME_S *runtime,
        MYBPF_LOADER_NODE_S *loader, char *sec_name, MYBPF_PROG_NODE_S *current)
{
    int sec_name_len = 0;
    int wildcard = 0;

    if (sec_name) {
        sec_name_len = strlen(sec_name);
        if (sec_name_len == 0) {
            sec_name = NULL;
        } else if (sec_name[sec_name_len - 1] == '/') {
            wildcard = 1;
        }
    }

    int reach_cur = 0;

    for (int i=0; i<loader->main_prog_count; i++) {
        if (reach_cur) {
            MYBPF_PROG_NODE_S * prog = loader->main_progs[i];
            if (!sec_name) {
                return prog;
            }
            if ((wildcard) && (0 == strncmp(sec_name, prog->sec_name, sec_name_len))) {
                return prog;
            }
            if ((! wildcard) && (0 == strcmp(sec_name, prog->sec_name))) {
                return prog;
            }
        } else if (loader->main_progs[i] == current) {
            reach_cur = 1;
        }
    }

    return NULL;
}


MYBPF_PROG_NODE_S * MYBPF_PROG_GetNext(MYBPF_RUNTIME_S *runtime, char *instance, char *sec_name,
        MYBPF_PROG_NODE_S *current)
{
    if (! current) {
        return _mybpf_prog_get_first(runtime, MYBPF_LoaderGet(runtime, instance), sec_name);
    }

    return _mybpf_prog_get_next(runtime, current->loader_node, sec_name, current);
}

int MYBPF_PROG_Run(MYBPF_PROG_NODE_S *prog, OUT UINT64 *bpf_ret,
        UINT64 p1, UINT64 p2, UINT64 p3, UINT64 p4, UINT64 p5)
{
    return _mybpf_prog_run(prog, bpf_ret, p1, p2, p3, p4, p5);
}

