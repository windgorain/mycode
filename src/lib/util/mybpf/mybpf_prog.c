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
#include "utl/mybpf_prog.h"
#include "utl/mybpf_vm.h"
#include "utl/mybpf_jit.h"
#include "mybpf_prog_inner.h"

static void _mybpf_prog_free_prog_rcu(void *pstRcuNode)
{
    MYBPF_PROG_NODE_S *node = container_of(pstRcuNode, MYBPF_PROG_NODE_S, rcu_node);
    MEM_Free(node);
}

static void _mybpf_prog_free_prog(void *ufd_ctx, void *f)
{
    MYBPF_PROG_NODE_S *node = f;
    RcuEngine_Call(&node->rcu_node, _mybpf_prog_free_prog_rcu);
}

static inline int _mybpf_prog_run_bpf_code(MYBPF_PROG_NODE_S *prog, OUT UINT64 *bpf_ret,
        UINT64 p1, UINT64 p2, UINT64 p3, UINT64 p4, UINT64 p5)
{
#if 1
    MYBPF_CTX_S ctx = {0};
    MYBPF_LOADER_NODE_S *n = prog->loader_node;
    ctx.start_addr = n->insts;
    ctx.mybpf_prog = prog;
    ctx.insts = prog->insn;
    ctx.auto_stack = 1;
    ctx.helper_fixed = 1;

    int ret = MYBPF_DefultRun(&ctx, p1, p2, p3, p4, p5);
    if (ret < 0) {
        return ret;
    }

    *bpf_ret = ctx.bpf_ret;
#else
    *bpf_ret = MYBPF_RunBpfCode(prog->insn, p1, p2, p3, p4, p5);
#endif

    return 0;
}

static inline UINT64 _mybpf_prog_agent_fix_p1(MYBPF_PREJIT_PROG_CTX_S *ctx, UINT64 p1)
{
    MYBPF_LOADER_NODE_S *n = ctx->loader_node;
    MYBPF_RUNTIME_S *d = n->runtime;
    return (long)UMAP_GetByFd(d->ufd_ctx, n->map_fds[p1]);
}

static inline UINT64 _mybpf_prog_agent_call_helper_func(UINT64 p1, UINT64 p2, UINT64 p3,
        UINT64 p4, UINT64 p5, UINT64 fid, void *ud)
{
    PF_BPF_HELPER_FUNC func = BpfHelper_GetFunc(fid);
    if (! func) {
        return -1;
    }

    if (fid <= 3) {
        p1 = _mybpf_prog_agent_fix_p1(ud, p1);
        if (! p1) {
            return (fid == 1) ? 0 : -1;
        }
    }

    return func(p1, p2, p3, p4, p5);
}

static UINT64 _mybpf_prog_agent_func(UINT64 p1, UINT64 p2, UINT64 p3, UINT64 p4, UINT64 p5, UINT64 fid, void *ud)
{
    return _mybpf_prog_agent_call_helper_func(p1, p2, p3, p4, p5, fid, ud);
}

typedef UINT64 (*PF_BPF_JITTED_FUNC)(UINT64 p1, UINT64 p2, UINT64 p3, UINT64 p4, UINT64 p5, void *ctx);

static inline int _mybpf_prog_run_jitted_code(MYBPF_PROG_NODE_S *prog, OUT UINT64 *bpf_ret,
        UINT64 p1, UINT64 p2, UINT64 p3, UINT64 p4, UINT64 p5)
{
    PF_BPF_JITTED_FUNC func = prog->insn;
    MYBPF_LOADER_NODE_S *n = prog->loader_node;
    MYBPF_PREJIT_PROG_CTX_S ctx = {0};

    ctx.agent_func = _mybpf_prog_agent_func;
    ctx.base_helpers = BpfHelper_BaseHelper();
    ctx.sys_helpers = BpfHelper_SysHelper();
    ctx.user_helpers = BpfHelper_UserHelper();
    ctx.maps = n->maps;
    ctx.global_map_data = n->global_data;
    ctx.loader_node = n;

    *bpf_ret = func(p1, p2, p3, p4, p5, &ctx);

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
    _mybpf_prog_free_prog(runtime->ufd_ctx, prog);
}

int MYBPF_PROG_Add(MYBPF_RUNTIME_S *runtime, MYBPF_PROG_NODE_S *prog)
{
    prog->fd = UFD_Open(runtime->ufd_ctx, UFD_FD_TYPE_PROG, prog, _mybpf_prog_free_prog);
    return prog->fd;
}

MYBPF_PROG_NODE_S * MYBPF_PROG_GetByFD(MYBPF_RUNTIME_S *runtime, int fd)
{
    if (UFD_FD_TYPE_PROG != UFD_GetFileType(runtime->ufd_ctx, fd)) {
        return NULL;
    }
    return UFD_GetFileData(runtime->ufd_ctx, fd);
}

void MYBPF_PROG_Close(MYBPF_RUNTIME_S *runtime, int fd)
{
    UFD_Close(runtime->ufd_ctx, fd);
}

void MYBPF_PROG_ShowProg(MYBPF_RUNTIME_S *runtime, PF_PRINT_FUNC print_func)
{
    int fd = -1;
    int state;
    MYBPF_PROG_NODE_S *prog;
    MYBPF_LOADER_NODE_S *loader;

    state = RcuEngine_Lock();

    while ((fd = UFD_GetNextOfType(runtime->ufd_ctx, UFD_FD_TYPE_PROG, fd)) >= 0) {
        prog = UFD_GetFileData(runtime->ufd_ctx, fd);
        if (! prog) {
            continue;
        }
        loader = prog->loader_node;
        print_func("fd:%d, xlated:%u, sec:%s, name:%s, instance:%s, file:%s \r\n",
                fd, prog->insn_len, prog->sec_name, prog->prog_name,
                loader->param.instance, loader->param.filename);
    }

    RcuEngine_UnLock(state);
}

/* 根据instance:func_name 获取prog fd. 找不到则返回<0 */
int MYBPF_PROG_GetByFuncName(MYBPF_RUNTIME_S *runtime, char *instance, char *func_name)
{
    int i;
    MYBPF_PROG_NODE_S *prog;

    MYBPF_LOADER_NODE_S *loader = MYBPF_LoaderGet(runtime, instance);
    if (! loader) {
        RETURNI(BS_NO_SUCH, "Can't get instance %s", instance);
    }

    for (i=0; i<loader->main_prog_count; i++) {
        prog = MYBPF_PROG_GetByFD(runtime, loader->main_prog_fd[i]);
        BS_DBGASSERT(prog);
        if (strcmp(prog->prog_name, func_name) == 0) {
            return loader->main_prog_fd[i];
        }
    }

    RETURNI(BS_NO_SUCH, "Can't get function %s", func_name);
}

int MYBPF_PROG_GetBySecName(MYBPF_RUNTIME_S *runtime, char *instance, char *sec_name)
{
    int i;
    MYBPF_PROG_NODE_S *prog;

    MYBPF_LOADER_NODE_S *loader = MYBPF_LoaderGet(runtime, instance);
    if (! loader) {
        return -1;
    }

    for (i=0; i<loader->main_prog_count; i++) {
        prog = MYBPF_PROG_GetByFD(runtime, loader->main_prog_fd[i]);
        BS_DBGASSERT(prog);
        if (strcmp(prog->sec_name, sec_name) == 0) {
            return loader->main_prog_fd[i];
        }
    }

    return -1;
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
        MYBPF_PROG_NODE_S * prog = MYBPF_PROG_GetByFD(runtime, loader->main_prog_fd[i]);
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
            MYBPF_PROG_NODE_S * prog = MYBPF_PROG_GetByFD(runtime, loader->main_prog_fd[i]);
            if (!sec_name) {
                return prog;
            }
            if ((wildcard) && (0 == strncmp(sec_name, prog->sec_name, sec_name_len))) {
                return prog;
            }
            if ((! wildcard) && (0 == strcmp(sec_name, prog->sec_name))) {
                return prog;
            }
        } else if (loader->main_prog_fd[i] == current->fd) {
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

int MYBPF_PROG_Run(MYBPF_PROG_NODE_S *prog, OUT UINT64 *bpf_ret, UINT64 p1, UINT64 p2, UINT64 p3, UINT64 p4, UINT64 p5)
{
    return _mybpf_prog_run(prog, bpf_ret, p1, p2, p3, p4, p5);
}

int MYBPF_PROG_RunByFd(MYBPF_RUNTIME_S *runtime, int fd, OUT UINT64 *bpf_ret,
        UINT64 p1, UINT64 p2, UINT64 p3, UINT64 p4, UINT64 p5)
{
    MYBPF_PROG_NODE_S *prog;

    prog = MYBPF_PROG_GetByFD(runtime, fd);
    if (! prog) {
        RETURN(BS_NOT_FOUND);
    }

    return _mybpf_prog_run(prog, bpf_ret, p1, p2, p3, p4, p5);
}

