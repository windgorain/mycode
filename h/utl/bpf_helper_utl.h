/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2017-10-3
* Description: 
******************************************************************************/
#ifndef _BPF_HELPER_UTL_H
#define _BPF_HELPER_UTL_H
#ifdef __cplusplus
extern "C"
{
#endif

typedef UINT64 (*PF_BPF_HELPER_FUNC)(UINT64 p1, UINT64 p2, UINT64 p3, UINT64 p4, UINT64 p5);

UINT BpfHelper_BaseSize(void);

PF_BPF_HELPER_FUNC BpfHelper_GetFunc(UINT id);
int BpfHelper_SetUserFunc(UINT id, void *func);
UINT64 BpfHelper_BaseHelper(UINT64 p1, UINT64 p2, UINT64 p3, UINT64 p4, UINT64 p5);


#ifdef __cplusplus
}
#endif
#endif //BPF_HELPER_UTL_H_
