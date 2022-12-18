/*********************************************************
*   Copyright (C) LiXingang
*   Description: 
*
********************************************************/
#include "bs.h"
#include "utl/idfunc_utl.h"
#include "utl/mybpf_vm.h"

static inline int _idfunc_call_raw(IDFUNC_S *ctrl, IDFUNC_NODE_S *node, UINT64 *func_ret,
        UINT64 p1, UINT64 p2, UINT64 p3, UINT64 p4, UINT64 p5)
{
    PF_IDFUNC_FUNC func = node->func;

    UINT64 ret = func(p1, p2, p3, p4, p5);
    if (func_ret) {
        *func_ret = ret;
    }

    return 0;
}

static inline int _idfunc_call_bpf(IDFUNC_S *ctrl, IDFUNC_NODE_S *node, UINT64 *func_ret,
        UINT64 p1, UINT64 p2, UINT64 p3, UINT64 p4, UINT64 p5)
{
    MYBPF_CTX_S ctx;

    if (node->mem_check) {
        /* 如果开启了mem check, 则这个函数的入参必须是data + data_size */
        ctx.mem = (void*)(long)p1; /* data */
        ctx.mem_len = p2; /* data_size */
        ctx.mem_check = 1;
    }

    ctx.insts = node->func;

    int ret = MYBPF_DefultRun(&ctx, p1, p2, p3, p4, p5);
    if (ret < 0) {
        return ret;
    }

    if (func_ret) {
        *func_ret = ctx.bpf_ret;
    }

    return 0;
}

int IDFUNC_Init(INOUT IDFUNC_S *ctrl, UINT capacity)
{
    ctrl->capacity = capacity;
    return 0;
}

IDFUNC_S * IDFUNC_Create(UINT capacity)
{
    IDFUNC_S *ctrl = MEM_ZMalloc(sizeof(IDFUNC_S) + capacity * sizeof(IDFUNC_NODE_S)); 
    if (! ctrl) {
        return NULL;
    }

    IDFUNC_Init(ctrl, capacity);
    return ctrl;
}

IDFUNC_NODE_S * IDFUNC_GetNode(IDFUNC_S *ctrl, UINT id)
{
    if (id >= ctrl->capacity) {
        return NULL;
    }
    return &ctrl->nodes[id];
}

int IDFUNC_SetNode(IDFUNC_S *ctrl, UINT id, UCHAR type, void *func)
{
    if (id >= ctrl->capacity) {
        RETURN(BS_OUT_OF_RANGE);
    }

    ctrl->nodes[id].type = type;
    ctrl->nodes[id].func = func;

    return 0;
}

/* 
id: 被调用函数的ID
func_ret: 被调用函数的返回值. 可以为NULL不关心返回值
px: 传给被调用函数的参数
return: 调用成功失败
 */
int IDFUNC_Call(IDFUNC_S *ctrl, UINT id, UINT64 *func_ret, UINT64 p1, UINT64 p2, UINT64 p3, UINT64 p4, UINT64 p5)
{
    IDFUNC_NODE_S *node;
    int ret = 0;

    if (id >= ctrl->capacity) {
        RETURN(BS_OUT_OF_RANGE);
    }

    node = &ctrl->nodes[id];
    if (! node->func) {
        RETURN(BS_NO_SUCH);
    }

    switch (node->type) {
        case IDFUNC_TYPE_RAW:
            ret = _idfunc_call_raw(ctrl, node, func_ret, p1, p2, p3, p4, p5);
            break;
        case IDFUNC_TYPE_BPF:
            ret = _idfunc_call_bpf(ctrl, node, func_ret, p1, p2, p3, p4, p5);
            break;
        default:
            RETURN(BS_NOT_SUPPORT);
            break;
    }

    return ret;
}


