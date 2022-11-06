/*================================================================
*   Created by LiXingang, Copyright LiXingang
*   Description: ebpf prog管理
*
================================================================*/
#include "bs.h"
#include "utl/exec_utl.h"
#include "utl/ufd_utl.h"
#include "utl/umap_utl.h"
#include "utl/mybpf_prog.h"

static void _mybpf_prog_free_prog(void *f)
{
    int i;
    MYBPF_PROG_NODE_S *node = f;

    for (i=0; i<node->used_map_cnt; i++) {
        UMAP_Close(node->used_maps[i]);
    }

    RcuEngine_Free(node);
}

MYBPF_PROG_NODE_S * MYBPF_PROG_Alloc(struct bpf_insn *insn, int len /* insn的字节数 */, char *prog_name)
{
    MYBPF_PROG_NODE_S *node;
    int size = sizeof(MYBPF_PROG_NODE_S) + len;

    node = RcuEngine_Malloc(size);
    if (! node) {
        return NULL;
    }

    memset(node, 0, sizeof(MYBPF_PROG_NODE_S));

    if (prog_name) {
        strlcpy(node->prog_name, prog_name, sizeof(node->prog_name));
    }
    node->insn_len = len;
    memcpy(node->insn, insn, len);

    return node;
}

void MYBPF_PROG_Free(MYBPF_PROG_NODE_S *prog)
{
    _mybpf_prog_free_prog(prog);
}

int MYBPF_PROG_Add(MYBPF_PROG_NODE_S *prog)
{
    return UFD_Open(UFD_FD_TYPE_PROG, prog, _mybpf_prog_free_prog);
}

MYBPF_PROG_NODE_S * MYBPF_PROG_GetByFD(int fd)
{
    if (UFD_FD_TYPE_PROG != UFD_GetFileType(fd)) {
        return NULL;
    }
    return UFD_GetFileData(fd);
}

/* 获取prog并增加引用计数 */
MYBPF_PROG_NODE_S * MYBPF_PROG_RefByFD(int fd)
{
    if (UFD_FD_TYPE_PROG != UFD_GetFileType(fd)) {
        return NULL;
    }
    return UFD_RefFileData(fd);
}

void MYBPF_PROG_Close(int fd)
{
    UFD_Close(fd);
}

void MYBPF_PROG_ShowProg(PF_PRINT_FUNC print_func)
{
    int fd = -1;
    int state;
    MYBPF_PROG_NODE_S *prog;

    state = RcuEngine_Lock();

    while ((fd = UFD_GetNextOfType(UFD_FD_TYPE_PROG, fd)) >= 0) {
        prog = UFD_GetFileData(fd);
        if (! prog) {
            continue;
        }
        print_func("fd:%d,xlated:%u,name:%s \r\n", fd, prog->insn_len, prog->prog_name);
    }

    RcuEngine_UnLock(state);
}

