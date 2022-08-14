/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-5-14
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
        MemCap_Free(pstNAPHead->stCommonHead.memcap, pstNAPHead->ppPrtArray);
    }
    
    MemCap_Free(pstNAPHead->stCommonHead.memcap, pstNAPHead);

    return;
}

static inline void * nap_ptrarray_InnerAlloc(IN _NAP_PTR_ARRAY_HEAD_S *pstHead)
{
    return MemCap_Malloc(pstHead->stCommonHead.memcap, pstHead->uiNapNodeSize);
}

static VOID * nap_ptrarray_Alloc(IN HANDLE hNapHandle, IN UINT uiIndex)
{
    _NAP_PTR_ARRAY_HEAD_S *pstHead = hNapHandle;
    UINT uiPos = uiIndex;

    BS_DBGASSERT(pstHead->ppPrtArray[uiPos] == NULL);

    pstHead->ppPrtArray[uiPos] = nap_ptrarray_InnerAlloc(pstHead);

    return pstHead->ppPrtArray[uiPos];
}

static VOID nap_ptrarray_InnerFree(IN _NAP_PTR_ARRAY_HEAD_S *pstNAPTbl, IN VOID *pNode)
{
    MemCap_Free(pstNAPTbl->stCommonHead.memcap, pNode);
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

_NAP_HEAD_COMMON_S * _NAP_PtrArrayCreate(NAP_PARAM_S *p)
{
    _NAP_PTR_ARRAY_HEAD_S *pstNAPHead = NULL;

    if (p->uiMaxNum == 0)
    {
        return NULL;
    }

    pstNAPHead = MemCap_ZMalloc(p->memcap, sizeof(_NAP_PTR_ARRAY_HEAD_S));
    if (pstNAPHead == NULL)
    {
        return NULL;
    }

    pstNAPHead->stCommonHead.memcap = p->memcap;
    pstNAPHead->stCommonHead.pstFuncTbl = &g_stNapPtrArrayFuncTbl;

    pstNAPHead->ppPrtArray = MemCap_Malloc(p->memcap, sizeof(VOID*) * p->uiMaxNum);
    if (NULL == pstNAPHead->ppPrtArray)
    {
        MemCap_Free(p->memcap, pstNAPHead);
        return NULL;
    }

    pstNAPHead->uiNapNodeSize = p->uiNodeSize;

    return (_NAP_HEAD_COMMON_S*) pstNAPHead;
}
