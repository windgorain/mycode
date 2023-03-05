/*********************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2017-10-1
* Description: 
*
********************************************************/
#include "bs.h"
#include "utl/mybpf_loader.h"
#include "utl/mybpf_prog.h"
#include "utl/mybpf_hookpoint.h"
#include "utl/ufd_utl.h"
#include "mybpf_def.h"
#include "mybpf_osbase.h"

static inline int _mybpf_hookpoint_process(void *prog, UINT64 p1, UINT64 p2, UINT64 p3, UINT64 p4, UINT64 p5)
{
    UINT64 bpf_ret;

    int ret = MYBPF_PROG_Run(prog, &bpf_ret, p1, p2, p3, p4, p5);
    if (ret < 0) {
        return -1;
    }

    return bpf_ret;
}

static MYBPF_HOOKPOINT_NODE_S * _mybpf_hookpoint_detach(DLL_HEAD_S *list, void *prog)
{
    MYBPF_HOOKPOINT_NODE_S *node;

    DLL_SCAN(list, node) {
        if (node->prog == prog) {
            DLL_DEL(list, &node->link_node);
            return node;
        }
    }

    return NULL;
}

int MYBPF_HookPointAttach(MYBPF_RUNTIME_S *runtime, DLL_HEAD_S *list, int fd)
{
    MYBPF_HOOKPOINT_NODE_S *node;

    UFD_IncRef(runtime->ufd_ctx, fd);

    node = MEM_RcuZMalloc(sizeof(MYBPF_HOOKPOINT_NODE_S));
    if (! node) {
        UFD_DecRef(runtime->ufd_ctx, fd);
        RETURN(BS_NO_MEMORY);
    }

    node->prog = MYBPF_PROG_GetByFD(runtime, fd);

    DLL_ADD_RCU(list, &node->link_node);

    return 0;
}

void MYBPF_HookPointDetach(MYBPF_RUNTIME_S *runtime, DLL_HEAD_S *list, int fd)
{
    void *prog = MYBPF_PROG_GetByFD(runtime, fd);
    MYBPF_HOOKPOINT_NODE_S * node = _mybpf_hookpoint_detach(list, prog);

    if (node) {
        MEM_RcuFree(node);
        UFD_DecRef(runtime->ufd_ctx, fd);
    }
}

int MYBPF_HookPointCall(MYBPF_RUNTIME_S *runtime, int type, UINT64 p1, UINT64 p2, UINT64 p3, UINT64 p4, UINT64 p5)
{
    MYBPF_HOOKPOINT_NODE_S *node;
    DLL_HEAD_S *list = &runtime->hp_list[type];
    int ret = 0;

    int state = RcuEngine_Lock();

    DLL_SCAN(list, node) {
        ret = _mybpf_hookpoint_process(node->prog, p1, p2, p3, p4, p5);
        if (ret == MYBPF_HP_RET_STOP) {
            break;
        }
    }

    RcuEngine_UnLock(state);

    return ret;
}

