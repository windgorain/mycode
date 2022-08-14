/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/exec_utl.h"
#include "../h/ulc_def.h"
#include "../h/ulc_osbase.h"
#include "../h/ulc_fd.h"
#include "../h/ulc_map.h"
#include "../h/ulc_prog.h"

static void _ulc_prog_free_prog(void *f)
{
    ULC_PROG_NODE_S *node = f;
    RcuEngine_Free(node);
}

ULC_PROG_NODE_S * ULC_PROG_Alloc(struct bpf_insn *insn, int len /* insn的字节数 */, char *prog_name)
{
    ULC_PROG_NODE_S *node;
    int size = sizeof(ULC_PROG_NODE_S) + len;

    node = RcuEngine_Malloc(size);
    if (! node) {
        return NULL;
    }

    memset(node, 0, sizeof(ULC_PROG_NODE_S));

    if (prog_name) {
        strlcpy(node->prog_name, prog_name, sizeof(node->prog_name));
    }
    node->insn_len = len;
    memcpy(node->insn, insn, len);

    return node;
}

void ULC_PROG_Free(ULC_PROG_NODE_S *prog)
{
    _ulc_prog_free_prog(prog);
}

int ULC_PROG_Add(ULC_PROG_NODE_S *prog)
{
    return ULC_FD_Open(ULC_FD_TYPE_PROG, prog, _ulc_prog_free_prog);
}

ULC_PROG_NODE_S * ULC_PROG_GetByFD(int fd)
{
    return ULC_FD_GetFileData(fd);
}

/* 获取prog并增加引用计数 */
ULC_PROG_NODE_S * ULC_PROG_RefByFD(int fd)
{
    return ULC_FD_RefFileData(fd);
}

void ULC_PROG_Del(int fd)
{
    ULC_FD_Close(fd);
}

void ULC_PROG_ShowProg()
{
    int fd = -1;
    int state;
    ULC_PROG_NODE_S *prog;

    state = RcuEngine_Lock();

    while ((fd = ULC_FD_GetNextOfType(ULC_FD_TYPE_PROG, fd)) >= 0) {
        prog = ULC_FD_GetFileData(fd);
        if (! prog) {
            continue;
        }
        EXEC_OutInfo("fd:%d,xlated:%u,name:%s \r\n", fd, prog->insn_len, prog->prog_name);
    }

    RcuEngine_UnLock(state);
}

