/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2014-11-16
* Description: 
* History:     
******************************************************************************/
#include "bs.h"
        
#include "utl/nap_utl.h"
#include "utl/rcu_utl.h"
#include "utl/mem_cap.h"

#include "nap_inner.h"

typedef struct
{
    _NAP_HEAD_COMMON_S stCommonHead;

    UINT uiNapNodeSize;
    UCHAR *pucMem;
}_NAP_ARRAY_HEAD_S;

static VOID nap_array_Destory(IN HANDLE hNAPHandle)
{
    _NAP_ARRAY_HEAD_S *pstNAPHead = hNAPHandle;

    if (NULL == pstNAPHead) {
        BS_WARNNING(("Null ptr!"));
        return;
    }

    if (pstNAPHead->pucMem) {
        MemCap_Free(pstNAPHead->stCommonHead.memcap, pstNAPHead->pucMem);
    }
    
    MemCap_Free(pstNAPHead->stCommonHead.memcap, pstNAPHead);

    return;
}

static VOID * nap_array_Alloc(IN HANDLE hNapHandle, IN UINT uiIndex)
{
    _NAP_ARRAY_HEAD_S *pstHead = hNapHandle;
    UINT uiPos = uiIndex;

    return (VOID*)(pstHead->pucMem + uiPos * pstHead->uiNapNodeSize);
}

static VOID nap_array_Free(IN HANDLE hNapHandle, IN VOID *pstNapNode, IN UINT uiIndex)
{
    return;
}

static VOID * nap_array_GetNodeByIndex(IN HANDLE hNAPHandle, IN UINT uiIndex)
{
    _NAP_ARRAY_HEAD_S *pstHead = hNAPHandle;
    UINT uiPos = uiIndex;

    return (VOID*)(pstHead->pucMem + pstHead->uiNapNodeSize * uiPos);
}

static _NAP_FUNC_TBL_S g_stNapStaticFuncTbl = 
{
    nap_array_Destory,
    nap_array_Alloc,
    nap_array_Free,
    nap_array_GetNodeByIndex
};

_NAP_HEAD_COMMON_S * _NAP_ArrayCreate(NAP_PARAM_S *p)
{
    UINT uiNAPSize = p->uiNodeSize * p->uiMaxNum;
    _NAP_ARRAY_HEAD_S *pstNAPHead = NULL;

    if (p->uiMaxNum == 0) {
        return NULL;
    }

    pstNAPHead = MemCap_ZMalloc(p->memcap, sizeof(_NAP_ARRAY_HEAD_S));
    if (pstNAPHead == NULL) {
        return NULL;
    }

    pstNAPHead->stCommonHead.memcap = p->memcap;
    pstNAPHead->stCommonHead.pstFuncTbl = &g_stNapStaticFuncTbl;

    pstNAPHead->pucMem = MemCap_Malloc(p->memcap, uiNAPSize);
    if (NULL == pstNAPHead->pucMem) {
        MemCap_Free(p->memcap, pstNAPHead);
        return NULL;
    }

    pstNAPHead->uiNapNodeSize = p->uiNodeSize;

    return (_NAP_HEAD_COMMON_S*) pstNAPHead;
}

