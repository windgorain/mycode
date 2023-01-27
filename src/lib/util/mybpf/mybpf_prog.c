/*================================================================
*   Created by LiXingang, Copyright LiXingang
*   Date: 2017.10.1
*   Description: ebpf prog管理
*
================================================================*/
#include "bs.h"
#include "utl/exec_utl.h"
#include "utl/ufd_utl.h"
#include "utl/umap_utl.h"
#include "utl/mybpf_prog.h"
#include "utl/mybpf_loader.h"
#include "utl/mybpf_vm.h"

static void _mybpf_prog_free_prog(void *ufd_ctx, void *f)
{
    int i;
    MYBPF_PROG_NODE_S *node = f;

    for (i=0; i<node->used_map_cnt; i++) {
        UMAP_Close(ufd_ctx, node->used_maps[i]);
    }

    RcuEngine_Free(node);
}

MYBPF_PROG_NODE_S * MYBPF_PROG_Alloc(struct bpf_insn *insn, int len /* insn的字节数 */, char *sec_name, char *prog_name)
{
    MYBPF_PROG_NODE_S *node;
    int size = sizeof(MYBPF_PROG_NODE_S) + len;

    node = RcuEngine_Malloc(size);
    if (! node) {
        return NULL;
    }

    memset(node, 0, sizeof(MYBPF_PROG_NODE_S));

    if (sec_name) {
        strlcpy(node->sec_name, sec_name, sizeof(node->sec_name));
    }

    if (prog_name) {
        strlcpy(node->prog_name, prog_name, sizeof(node->prog_name));
    }

    node->insn_len = len;
    memcpy(node->insn, insn, len);

    return node;
}

void MYBPF_PROG_Free(MYBPF_RUNTIME_S *runtime, MYBPF_PROG_NODE_S *prog)
{
    _mybpf_prog_free_prog(runtime->ufd_ctx, prog);
}

int MYBPF_PROG_Add(MYBPF_RUNTIME_S *runtime, MYBPF_PROG_NODE_S *prog)
{
    return UFD_Open(runtime->ufd_ctx, UFD_FD_TYPE_PROG, prog, _mybpf_prog_free_prog);
}

MYBPF_PROG_NODE_S * MYBPF_PROG_GetByFD(MYBPF_RUNTIME_S *runtime, int fd)
{
    if (UFD_FD_TYPE_PROG != UFD_GetFileType(runtime->ufd_ctx, fd)) {
        return NULL;
    }
    return UFD_GetFileData(runtime->ufd_ctx, fd);
}

/* 获取prog并增加引用计数 */
MYBPF_PROG_NODE_S * MYBPF_PROG_RefByFD(MYBPF_RUNTIME_S *runtime, int fd)
{
    if (UFD_FD_TYPE_PROG != UFD_GetFileType(runtime->ufd_ctx, fd)) {
        return NULL;
    }
    return UFD_RefFileData(runtime->ufd_ctx, fd);
}

void MYBPF_PROG_Disable(MYBPF_RUNTIME_S *runtime, int fd)
{
    MYBPF_PROG_NODE_S *n = MYBPF_PROG_GetByFD(runtime, fd);
    if (! n) {
        return;
    }
    n->disabled = 1;
}

void MYBPF_PROG_Enable(MYBPF_RUNTIME_S *runtime, int fd)
{
    MYBPF_PROG_NODE_S *n = MYBPF_PROG_GetByFD(runtime, fd);
    if (! n) {
        return;
    }
    n->disabled = 0;
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

    for (i=0; i<loader->prog_count; i++) {
        prog = MYBPF_PROG_GetByFD(runtime, loader->prog_fd[i]);
        BS_DBGASSERT(prog);
        if (strcmp(prog->prog_name, func_name) == 0) {
            return loader->prog_fd[i];
        }
    }

    RETURNI(BS_NO_SUCH, "Can't get function %s", func_name);
}

/* 根据instance:sec_name 获取prog fd. 找不到则返回<0 */
int MYBPF_PROG_GetBySecName(MYBPF_RUNTIME_S *runtime, char *instance, char *sec_name)
{
    int i;
    MYBPF_PROG_NODE_S *prog;

    MYBPF_LOADER_NODE_S *loader = MYBPF_LoaderGet(runtime, instance);
    if (! loader) {
        return -1;
    }

    for (i=0; i<loader->prog_count; i++) {
        prog = MYBPF_PROG_GetByFD(runtime, loader->prog_fd[i]);
        BS_DBGASSERT(prog);
        if (strcmp(prog->sec_name, sec_name) == 0) {
            return loader->prog_fd[i];
        }
    }

    return -1;
}

int MYBPF_PROG_Run(MYBPF_RUNTIME_S *runtime, int fd, OUT UINT64 *bpf_ret,
        UINT64 p1, UINT64 p2, UINT64 p3, UINT64 p4, UINT64 p5)
{
    MYBPF_PROG_NODE_S *prog;

    prog = MYBPF_PROG_GetByFD(runtime, fd);
    if (! prog) {
        RETURN(BS_NOT_FOUND);
    }

    if (prog->disabled) {
        return BS_NOT_READY;
    }

    MYBPF_CTX_S ctx = {0};
    ctx.insts = prog->insn;

    int ret = MYBPF_DefultRun(&ctx, p1, p2, p3, p4, p5);
    if (ret < 0) {
        return ret;
    }

    *bpf_ret = ctx.bpf_ret;

    return 0;
}


