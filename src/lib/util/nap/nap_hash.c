/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2014-11-21
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/hash_utl.h"        
#include "utl/nap_utl.h"
#include "utl/rcu_utl.h"
#include "utl/mem_cap.h"

#include "nap_inner.h"


#define _NAP_HASH_MAX_BUCKET_NUM  (1024 * 16)


typedef struct
{
    HASH_NODE_S stLinkNode;
    UINT uiIndex;
}_NAP_HASH_NODE_S;

typedef struct
{
    _NAP_HEAD_COMMON_S stCommonHead;

    UINT uiNapNodeSize;
    HASH_S * hHashTbl;
}_NAP_HASH_TBL_S;


static UINT nap_hash_Index(IN VOID *pstHashNode)
{
    _NAP_HASH_NODE_S *pstNode = pstHashNode;

    return pstNode->uiIndex;
}

static VOID nap_hash_InnerFree(IN _NAP_HASH_TBL_S *pstNAPTbl, IN VOID *pNode)
{
    MemCap_Free(pstNAPTbl->stCommonHead.memcap, pNode);
}

static VOID nap_hash_FreeEach(IN void *hHashId, IN VOID *pstNode, IN VOID * pUserHandle)
{
    nap_hash_InnerFree(pUserHandle, pstNode);
}

static VOID nap_hash_Destory(IN HANDLE hNAPHandle)
{
    _NAP_HASH_TBL_S *pstNAPTbl = hNAPHandle;

    if (NULL == pstNAPTbl)
    {
        BS_WARNNING(("Null ptr!"));
        return;
    }

    if (pstNAPTbl->hHashTbl != NULL)
    {
        HASH_DelAll(pstNAPTbl->hHashTbl, nap_hash_FreeEach, pstNAPTbl);
        HASH_DestoryInstance(pstNAPTbl->hHashTbl);
    }
    
    MemCap_Free(pstNAPTbl->stCommonHead.memcap, pstNAPTbl);

    return;
}

static INT nap_hash_Cmp(IN VOID * pHashNode, IN VOID * pNodeToFind)
{
    _NAP_HASH_NODE_S *pstHashNode = pHashNode;
    _NAP_HASH_NODE_S *pstNodeToFind = pNodeToFind;

    return (INT)(pstHashNode->uiIndex - pstNodeToFind->uiIndex);
}

static _NAP_HASH_NODE_S * nap_hash_Find(IN _NAP_HASH_TBL_S *pstNAPTbl, IN UINT uiIndex)
{
    _NAP_HASH_NODE_S stNodeToFind;

    stNodeToFind.uiIndex = uiIndex;
    
    return HASH_Find(pstNAPTbl->hHashTbl, nap_hash_Cmp, &stNodeToFind);
}

static VOID * nap_hash_InnerAlloc(IN _NAP_HASH_TBL_S *pstNAPTbl)
{
    UINT uiSize = (sizeof(_NAP_HASH_NODE_S) + pstNAPTbl->uiNapNodeSize);

    return MemCap_Malloc(pstNAPTbl->stCommonHead.memcap, uiSize);
}

static VOID * nap_hash_Alloc(IN HANDLE hNapHandle, IN UINT uiIndex)
{
    _NAP_HASH_TBL_S *pstNAPTbl = hNapHandle;
    _NAP_HASH_NODE_S *pstNode;

    pstNode = nap_hash_InnerAlloc(pstNAPTbl);
    if (NULL == pstNode)
    {
        return NULL;
    }

    pstNode->uiIndex = uiIndex;

    HASH_Add(pstNAPTbl->hHashTbl, pstNode);

    return (VOID*)(pstNode + 1);
}

static VOID nap_hash_Free(IN HANDLE hNapHandle, IN VOID *pstNapNode, IN UINT uiIndex)
{
    _NAP_HASH_TBL_S *pstNAPTbl = hNapHandle;
    _NAP_HASH_NODE_S *pstNode;

    pstNode = (VOID*)((UCHAR*)pstNapNode - sizeof(_NAP_HASH_NODE_S));

    HASH_Del(pstNAPTbl->hHashTbl, pstNode);

    nap_hash_InnerFree(pstNAPTbl, pstNode);
}

static VOID * nap_hash_GetNodeByIndex(IN HANDLE hNAPHandle, IN UINT uiIndex)
{
    _NAP_HASH_NODE_S *pstNode;

    pstNode = nap_hash_Find(hNAPHandle, uiIndex);
    if (NULL == pstNode)
    {
        return NULL;
    }

    return (VOID*)(pstNode + 1);
}


static _NAP_FUNC_TBL_S g_stNapHashFuncTbl = 
{
    nap_hash_Destory,
    nap_hash_Alloc,
    nap_hash_Free,
    nap_hash_GetNodeByIndex
};

_NAP_HEAD_COMMON_S * _NAP_HashCreate(NAP_PARAM_S *p)
{
    _NAP_HASH_TBL_S *pstNAPTbl = NULL;

    pstNAPTbl = MemCap_ZMalloc(p->memcap, sizeof(_NAP_HASH_TBL_S));
    if (pstNAPTbl == NULL)
    {
        return NULL;
    }

    pstNAPTbl->stCommonHead.memcap = p->memcap;
    pstNAPTbl->stCommonHead.pstFuncTbl = &g_stNapHashFuncTbl;

    pstNAPTbl->hHashTbl = HASH_CreateInstance(p->memcap, _NAP_HASH_MAX_BUCKET_NUM, nap_hash_Index);
    if (NULL == pstNAPTbl->hHashTbl)
    {
        MemCap_Free(p->memcap, pstNAPTbl);
        return NULL;
    }

    pstNAPTbl->uiNapNodeSize = p->uiNodeSize;

    return (_NAP_HEAD_COMMON_S*) pstNAPTbl;
}


