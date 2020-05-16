/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2014-11-16
* Description: 
* History:     
******************************************************************************/
#include "bs.h"
        
#include "utl/nap_utl.h"
#include "utl/rcu_utl.h"

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
        MEM_Free(pstNAPHead->pucMem);
    }
    
    MEM_Free(pstNAPHead);

    return;
}

static VOID * nap_array_Alloc(IN HANDLE hNapHandle, IN UINT uiIndex)
{
    _NAP_ARRAY_HEAD_S *pstHead = hNapHandle;
    UINT uiPos = uiIndex;

    if (pstHead->stCommonHead.uiFlag & NAP_FLAG_RCU)
    {
        BS_DBGASSERT(0);
    }

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

_NAP_HEAD_COMMON_S * _NAP_ArrayCreate(IN UINT uiMaxNum, IN UINT uiNapNodeSize)
{
    UINT uiNAPSize = uiNapNodeSize * uiMaxNum;
    _NAP_ARRAY_HEAD_S *pstNAPHead = NULL;

    if (uiMaxNum == 0) {
        return NULL;
    }

    pstNAPHead = MEM_ZMalloc(sizeof(_NAP_ARRAY_HEAD_S));
    if (pstNAPHead == NULL) {
        return NULL;
    }

    pstNAPHead->stCommonHead.pstFuncTbl = &g_stNapStaticFuncTbl;

    pstNAPHead->pucMem = MEM_Malloc(uiNAPSize);
    if (NULL == pstNAPHead->pucMem) {
        MEM_Free(pstNAPHead);
        return NULL;
    }

    pstNAPHead->uiNapNodeSize = uiNapNodeSize;

    return (_NAP_HEAD_COMMON_S*) pstNAPHead;
}

