/*********************************************************
*   Copyright (C) LiXingang
*   Description: 
*
********************************************************/
#include "bs.h"
#include "utl/idfunc_utl.h"
#include "utl/mybpf_vm.h"

static inline int _idfunc_call_raw(IDFUNC_CTRL_S *ctrl, IDFUNC_NODE_S *node, UINT64 *func_ret, UINT64 p1, UINT64 p2)
{
    PF_IDFUNC_FUNC func = node->func;

    UINT64 ret = func(p1, p2);
    if (func_ret) {
        *func_ret = ret;
    }

    return 0;
}

static inline int _idfunc_call_bpf(IDFUNC_CTRL_S *ctrl, IDFUNC_NODE_S *node, UINT64 *func_ret, UINT64 p1, UINT64 p2)
{
    MYBPF_CTX_S ctx;

    if (node->mem_check) {
        /* 如果开启了mem check, 则这个函数的入参必须是data + data_size */
        ctx.mem = (void*)(long)p1; /* data */
        ctx.mem_len = p2; /* data_size */
        ctx.mem_check = 1;
    }

    ctx.insts = node->func;

    int ret = MYBPF_DefultRun(&ctx, p1, p2, 0, 0, 0);
    if (ret < 0) {
        return ret;
    }

    if (func_ret) {
        *func_ret = ctx.bpf_ret;
    }

    return 0;
}

IDFUNC_NODE_S * IDFUNC_GetNode(IDFUNC_CTRL_S *ctrl, UINT id)
{
    if (id >= IDFUNC_MAX) {
        return NULL;
    }
    return ctrl->nodes[id];
}

int IDFUNC_SetNode(IDFUNC_CTRL_S *ctrl, UINT id, IDFUNC_NODE_S *node)
{
    if (id >= IDFUNC_MAX) {
        RETURN(BS_OUT_OF_RANGE);
    }

    ctrl->nodes[id] = node;

    return 0;
}

/* 
id: 被调用函数的ID
func_ret: 被调用函数的返回值. 可以为NULL不关心返回值
p1, p2: 传给被调用函数的参数
return: 调用成功失败
 */
int IDFUNC_Call(IDFUNC_CTRL_S *ctrl, UINT id, UINT64 *func_ret, UINT64 p1, UINT64 p2)
{
    IDFUNC_NODE_S *node;
    int ret = 0;

    if (id >= IDFUNC_MAX) {
        RETURN(BS_OUT_OF_RANGE);
    }

    node = ctrl->nodes[id];
    if (! node) {
        RETURN(BS_NO_SUCH);
    }

    BS_DBGASSERT(node->func);

    switch (node->type) {
        case IDFUNC_TYPE_RAW:
            ret = _idfunc_call_raw(ctrl, node, func_ret, p1, p2);
            break;
        case IDFUNC_TYPE_BPF:
            ret = _idfunc_call_bpf(ctrl, node, func_ret, p1, p2);
            break;
        default:
            RETURN(BS_NOT_SUPPORT);
            break;
    }

    return ret;
}

