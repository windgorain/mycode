/*********************************************************
*   Copyright (C) LiXingang
*   Description: bpf func; renamed frome idfunc_utl.c
*
********************************************************/
#include "bs.h"
#include "utl/bfunc_utl.h"
#include "utl/mybpf_vm.h"

static inline int _bfunc_call_raw(BFUNC_S *ctrl, BFUNC_NODE_S *node, UINT64 *bpf_ret,
        UINT64 p1, UINT64 p2, UINT64 p3, UINT64 p4, UINT64 p5)
{
    PF_BFUNC_FUNC func = node->func;

    UINT64 ret = func(p1, p2, p3, p4, p5);
    if (bpf_ret) {
        *bpf_ret = ret;
    }

    return 0;
}

static inline int _bfunc_call_bpf(BFUNC_S *ctrl, BFUNC_NODE_S *node, UINT64 *bpf_ret,
        UINT64 p1, UINT64 p2, UINT64 p3, UINT64 p4, UINT64 p5)
{
    MYBPF_CTX_S ctx;

    if (node->mem_check) {
        
        ctx.mem = (void*)(long)p1; 
        ctx.mem_len = p2; 
        ctx.mem_check = 1;
    }

    ctx.insts = node->func;
    ctx.begin_addr = node->begin_addr;
    ctx.end_addr = node->end_addr;

    int ret = MYBPF_DefultRun(&ctx, p1, p2, p3, p4, p5);
    if (ret < 0) {
        return ret;
    }

    if (bpf_ret) {
        *bpf_ret = ctx.bpf_ret;
    }

    return 0;
}

int BFUNC_Init(INOUT BFUNC_S *ctrl, MYBPF_RUNTIME_S *rt, UINT capacity)
{
    ctrl->capacity = capacity;
    ctrl->runtime = rt;
    return 0;
}

BFUNC_S * BFUNC_Create(MYBPF_RUNTIME_S *rt, UINT capacity)
{
    BFUNC_S *ctrl = MEM_ZMalloc(sizeof(BFUNC_S) + capacity * sizeof(BFUNC_NODE_S)); 
    if (! ctrl) {
        return NULL;
    }

    BFUNC_Init(ctrl, rt, capacity);
    return ctrl;
}

BFUNC_NODE_S * BFUNC_Get(BFUNC_S *ctrl, UINT id)
{
    if (id >= ctrl->capacity) {
        return NULL;
    }
    return &ctrl->nodes[id];
}

int BFUNC_Set(BFUNC_S *ctrl, UINT id, UINT jited, void *prog, void *func, void *begin_addr, void *end_addr)
{
    if (id >= ctrl->capacity) {
        RETURN(BS_OUT_OF_RANGE);
    }

    ctrl->nodes[id].jited = jited;
    ctrl->nodes[id].begin_addr = begin_addr;
    ctrl->nodes[id].end_addr = end_addr;
    ctrl->nodes[id].prog = prog;
    ctrl->nodes[id].func = func;

    return 0;
}


int BFUNC_Call(BFUNC_S *ctrl, UINT id, UINT64 *bpf_ret, UINT64 p1, UINT64 p2, UINT64 p3, UINT64 p4, UINT64 p5)
{
    BFUNC_NODE_S *node;
    int ret;

    if (id >= ctrl->capacity) {
        RETURN(BS_OUT_OF_RANGE);
    }

    node = &ctrl->nodes[id];
    if (! node->func) {
        RETURN(BS_NO_SUCH);
    }

    if (node->jited) {
        ret = _bfunc_call_raw(ctrl, node, bpf_ret, p1, p2, p3, p4, p5);
    } else {
        ret = _bfunc_call_bpf(ctrl, node, bpf_ret, p1, p2, p3, p4, p5);
    }

    return ret;
}


