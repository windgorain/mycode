/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-5-14
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
    VOID **ppPrtArray;
}_NAP_PTR_ARRAY_HEAD_S;

static VOID nap_ptrarray_Destory(IN HANDLE hNAPHandle)
{
    _NAP_PTR_ARRAY_HEAD_S *pstNAPHead = hNAPHandle;

    if (NULL == pstNAPHead)
    {
        BS_WARNNING(("Null ptr!"));
        return;
    }

    if (pstNAPHead->ppPrtArray)
    {
        MEM_Free(pstNAPHead->ppPrtArray);
    }
    
    MEM_Free(pstNAPHead);

    return;
}

static VOID * nap_ptrarray_InnerAlloc(IN _NAP_PTR_ARRAY_HEAD_S *pstHead)
{
    UINT uiSize;
    VOID *pNode;

    uiSize = pstHead->uiNapNodeSize;
    if (pstHead->stCommonHead.uiFlag & NAP_FLAG_RCU)
    {
        uiSize += sizeof(RCU_NODE_S);
    }

    pNode = MEM_Malloc(uiSize);
    if (NULL == pNode)
    {
        return NULL;
    }

    if (pstHead->stCommonHead.uiFlag & NAP_FLAG_RCU)
    {
        pNode = (UCHAR*)pNode + sizeof(RCU_NODE_S);
    }

    return pNode;
}

static VOID * nap_ptrarray_Alloc(IN HANDLE hNapHandle, IN UINT uiIndex)
{
    _NAP_PTR_ARRAY_HEAD_S *pstHead = hNapHandle;
    UINT uiPos = uiIndex;

    BS_DBGASSERT(pstHead->ppPrtArray[uiPos] == NULL);

    pstHead->ppPrtArray[uiPos] = nap_ptrarray_InnerAlloc(pstHead);

    return pstHead->ppPrtArray[uiPos];
}

static VOID nap_ptrarray_RcuFree(IN VOID *pstRcuNode)
{
    MEM_Free(pstRcuNode);
}

static VOID nap_ptrarray_InnerFree(IN _NAP_PTR_ARRAY_HEAD_S *pstNAPTbl, IN VOID *pNode)
{
    RCU_NODE_S *pstRcu;

    if (pstNAPTbl->stCommonHead.uiFlag & NAP_FLAG_RCU)
    {
        pstRcu = (VOID*)((UCHAR*)pNode - sizeof(RCU_NODE_S));
        RcuBs_Free(pstRcu, nap_ptrarray_RcuFree);
    }
    else
    {
        MEM_Free(pNode);
    }
}

static VOID nap_ptrarray_Free(IN HANDLE hNapHandle, IN VOID *pstNapNode, IN UINT uiIndex)
{
    _NAP_PTR_ARRAY_HEAD_S *pstHead = hNapHandle;
    UINT uiPos = uiIndex;

    if (pstHead->ppPrtArray[uiPos] == NULL)
    {
        BS_DBGASSERT(0);
        return;
    }

    BS_DBGASSERT(pstHead->ppPrtArray[uiPos] == pstNapNode);

    pstHead->ppPrtArray[uiPos] = NULL;

    nap_ptrarray_InnerFree(pstHead, pstNapNode);
}

static VOID * nap_ptrarray_GetNodeByIndex(IN HANDLE hNAPHandle, IN UINT uiIndex)
{
    _NAP_PTR_ARRAY_HEAD_S *pstHead = hNAPHandle;
    UINT uiPos = uiIndex;

    return pstHead->ppPrtArray[uiPos];
}

static _NAP_FUNC_TBL_S g_stNapPtrArrayFuncTbl = 
{
    nap_ptrarray_Destory,
    nap_ptrarray_Alloc,
    nap_ptrarray_Free,
    nap_ptrarray_GetNodeByIndex
};

_NAP_HEAD_COMMON_S * _NAP_PtrArrayCreate(IN UINT uiMaxNum, IN UINT uiNapNodeSize)
{
    _NAP_PTR_ARRAY_HEAD_S *pstNAPHead = NULL;

    if (uiMaxNum == 0)
    {
        return NULL;
    }

    pstNAPHead = MEM_ZMalloc(sizeof(_NAP_PTR_ARRAY_HEAD_S));
    if (pstNAPHead == NULL)
    {
        return NULL;
    }

    pstNAPHead->stCommonHead.pstFuncTbl = &g_stNapPtrArrayFuncTbl;

    pstNAPHead->ppPrtArray = MEM_Malloc(sizeof(VOID*) * uiMaxNum);
    if (NULL == pstNAPHead->ppPrtArray)
    {
        MEM_Free(pstNAPHead);
        return NULL;
    }

    pstNAPHead->uiNapNodeSize = uiNapNodeSize;

    return (_NAP_HEAD_COMMON_S*) pstNAPHead;
}
