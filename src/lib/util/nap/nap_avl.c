/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-5-14
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/avllib_utl.h"        
#include "utl/nap_utl.h"
#include "utl/rcu_utl.h"

#include "nap_inner.h"


typedef struct
{
    AVL_NODE stLinkNode;
    UINT uiIndex;
}_NAP_AVL_NODE_S;

typedef struct
{
    _NAP_HEAD_COMMON_S stCommonHead;

    UINT uiNapNodeSize;
    AVL_TREE avlRoot;
}_NAP_AVL_TBL_S;

static VOID nap_avl_RcuFree(IN VOID *pstRcuNode)
{
    MEM_Free(pstRcuNode);
}

static VOID nap_avl_InnerFree(IN _NAP_AVL_TBL_S *pstNAPTbl, IN VOID *pNode)
{
    RCU_NODE_S *pstRcu;

    if (pstNAPTbl->stCommonHead.uiFlag & NAP_FLAG_RCU)
    {
        pstRcu = (VOID*)((UCHAR*)pNode - sizeof(RCU_NODE_S));
        RcuBs_Free(pstRcu, nap_avl_RcuFree);
    }
    else
    {
        MEM_Free(pNode);
    }
}

static VOID nap_avl_FreeEach(IN VOID *pNode, IN VOID *pUserHandle)
{
    nap_avl_InnerFree(pUserHandle, pNode);
}

static VOID nap_avl_Destory(IN HANDLE hNAPHandle)
{
    _NAP_AVL_TBL_S *pstNAPTbl = hNAPHandle;

    if (NULL == pstNAPTbl)
    {
        BS_WARNNING(("Null ptr!"));
        return;
    }

    avlTreeErase(&pstNAPTbl->avlRoot, nap_avl_FreeEach, pstNAPTbl);
    
    MEM_Free(pstNAPTbl);

    return;
}

static INT nap_avl_Cmp(IN VOID * pAvlNode, IN GENERIC_ARGUMENT stKey)
{
    _NAP_AVL_NODE_S *pstNode = pAvlNode;

    return (INT)(pstNode->uiIndex - stKey.u);
}

static _NAP_AVL_NODE_S * nap_avl_Find(IN _NAP_AVL_TBL_S *pstNAPTbl, IN UINT uiIndex)
{
    GENERIC_ARGUMENT stKey;

    stKey.u = uiIndex;

    return avlSearch(pstNAPTbl->avlRoot, stKey, nap_avl_Cmp);
}

static VOID * nap_avl_InnerAlloc(IN _NAP_AVL_TBL_S *pstNAPTbl)
{
    UINT uiSize;
    VOID *pNode;

    uiSize = (sizeof(_NAP_AVL_NODE_S) + pstNAPTbl->uiNapNodeSize);

    if (pstNAPTbl->stCommonHead.uiFlag & NAP_FLAG_RCU)
    {
        uiSize += sizeof(RCU_NODE_S);
    }

    pNode = MEM_Malloc(uiSize);
    if (NULL == pNode)
    {
        return NULL;
    }

    if (pstNAPTbl->stCommonHead.uiFlag & NAP_FLAG_RCU)
    {
        pNode = (UCHAR*)pNode + sizeof(RCU_NODE_S);
    }

    return pNode;
}

static VOID * nap_avl_Alloc(IN HANDLE hNapHandle, IN UINT uiIndex)
{
    _NAP_AVL_TBL_S *pstNAPTbl = hNapHandle;
    _NAP_AVL_NODE_S *pstNode;
    GENERIC_ARGUMENT stKey;

    pstNode = nap_avl_InnerAlloc(pstNAPTbl);
    if (NULL == pstNode)
    {
        return NULL;
    }

    pstNode->uiIndex = uiIndex;
    stKey.u = uiIndex;

    avlInsert(&pstNAPTbl->avlRoot, pstNode, stKey, nap_avl_Cmp);

    return (VOID*)(pstNode + 1);
}

static VOID nap_avl_Free(IN HANDLE hNapHandle, IN VOID *pstNapNode, IN UINT uiIndex)
{
    _NAP_AVL_TBL_S *pstNAPTbl = hNapHandle;
    _NAP_AVL_NODE_S *pstNode;
    GENERIC_ARGUMENT stKey;

    stKey.u = uiIndex;
    pstNode = avlDelete(&pstNAPTbl->avlRoot, stKey, nap_avl_Cmp);

    if (NULL != pstNode)
    {
        nap_avl_InnerFree(pstNAPTbl, pstNode);
    }
}

static VOID * nap_avl_GetNodeByIndex(IN HANDLE hNAPHandle, IN UINT uiIndex)
{
    _NAP_AVL_NODE_S *pstNode;

    pstNode = nap_avl_Find(hNAPHandle, uiIndex);
    if (NULL == pstNode)
    {
        return NULL;
    }

    return (VOID*)(pstNode + 1);
}


static _NAP_FUNC_TBL_S g_stNapAvlFuncTbl = 
{
    nap_avl_Destory,
    nap_avl_Alloc,
    nap_avl_Free,
    nap_avl_GetNodeByIndex
};

_NAP_HEAD_COMMON_S * _NAP_AvlCreate(IN UINT uiMaxNum, IN UINT uiNapNodeSize)
{
    _NAP_AVL_TBL_S *pstNAPTbl = NULL;

    pstNAPTbl = MEM_ZMalloc(sizeof(_NAP_AVL_TBL_S));
    if (pstNAPTbl == NULL)
    {
        return NULL;
    }

    pstNAPTbl->stCommonHead.pstFuncTbl = &g_stNapAvlFuncTbl;

    pstNAPTbl->uiNapNodeSize = uiNapNodeSize;

    return (_NAP_HEAD_COMMON_S*) pstNAPTbl;
}



