/*********************************************************
*   Copyright (C) LiXingang
*   Description: 
*
********************************************************/
#ifndef _IDFUNC_UTL_H
#define _IDFUNC_UTL_H
#include "utl/mybpf_runtime.h"
#ifdef __cplusplus
extern "C"
{
#endif

typedef UINT64 (*PF_IDFUNC_FUNC)(UINT64 p1, UINT64 p2, UINT64 p3, UINT64 p4, UINT64 p5);

enum {
    IDFUNC_TYPE_RAW = 0, /* 原生函数指针 */
    IDFUNC_TYPE_BPF, /* bpf函数 */
};

typedef struct {
    UINT type:8;
    UINT mem_check:1;
    int fd;
    void *func;
}IDFUNC_NODE_S;

typedef struct {
    UINT capacity;
    IDFUNC_NODE_S nodes[0];
}IDFUNC_S;

int IDFUNC_Init(INOUT IDFUNC_S *ctrl, UINT capacity);
IDFUNC_S * IDFUNC_Create(UINT capacity);
IDFUNC_NODE_S * IDFUNC_Get(IDFUNC_S *ctrl, UINT id);
int IDFUNC_Set(IDFUNC_S *ctrl, UINT id, UCHAR type, int fd, void *func);
int IDFUNC_Call(IDFUNC_S *ctrl, UINT id, UINT64 *func_ret, UINT64 p1, UINT64 p2, UINT64 p3, UINT64 p4, UINT64 p5);

int IDFUNC_Load(MYBPF_RUNTIME_S *rt, IDFUNC_S *ctrl, char *conf_file);

#ifdef __cplusplus
}
#endif
#endif //IDFUNC_UTL_H_
