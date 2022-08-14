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
#include "utl/mem_cap.h"

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

static VOID nap_avl_InnerFree(IN _NAP_AVL_TBL_S *pstNAPTbl, IN VOID *pNode)
{
    MemCap_Free(pstNAPTbl->stCommonHead.memcap, pNode);
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
    
    MemCap_Free(pstNAPTbl->stCommonHead.memcap, pstNAPTbl);

    return;
}

static int nap_avl_Cmp(void *key, void *pAvlNode)
{
    _NAP_AVL_NODE_S *pstNode = pAvlNode;
    UINT num = HANDLE_UINT(key);

    return (int)(num - pstNode->uiIndex);
}

static _NAP_AVL_NODE_S * nap_avl_Find(IN _NAP_AVL_TBL_S *pstNAPTbl, IN UINT uiIndex)
{
    return avlSearch(pstNAPTbl->avlRoot, UINT_HANDLE(uiIndex), nap_avl_Cmp);
}

static VOID * nap_avl_InnerAlloc(IN _NAP_AVL_TBL_S *pstNAPTbl)
{
    UINT uiSize;
    uiSize = (sizeof(_NAP_AVL_NODE_S) + pstNAPTbl->uiNapNodeSize);
    return MemCap_Malloc(pstNAPTbl->stCommonHead.memcap, uiSize);
}

static VOID * nap_avl_Alloc(IN HANDLE hNapHandle, IN UINT uiIndex)
{
    _NAP_AVL_TBL_S *pstNAPTbl = hNapHandle;
    _NAP_AVL_NODE_S *pstNode;

    pstNode = nap_avl_InnerAlloc(pstNAPTbl);
    if (NULL == pstNode)
    {
        return NULL;
    }

    pstNode->uiIndex = uiIndex;

    avlInsert(&pstNAPTbl->avlRoot, pstNode, UINT_HANDLE(uiIndex), nap_avl_Cmp);

    return (VOID*)(pstNode + 1);
}

static VOID nap_avl_Free(IN HANDLE hNapHandle, IN VOID *pstNapNode, IN UINT uiIndex)
{
    _NAP_AVL_TBL_S *pstNAPTbl = hNapHandle;
    _NAP_AVL_NODE_S *pstNode;

    pstNode = avlDelete(&pstNAPTbl->avlRoot, UINT_HANDLE(uiIndex), nap_avl_Cmp);

    if (NULL != pstNode) {
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

_NAP_HEAD_COMMON_S * _NAP_AvlCreate(NAP_PARAM_S *p)
{
    _NAP_AVL_TBL_S *pstNAPTbl = NULL;

    pstNAPTbl = MemCap_ZMalloc(p->memcap, sizeof(_NAP_AVL_TBL_S));
    if (pstNAPTbl == NULL)
    {
        return NULL;
    }

    pstNAPTbl->stCommonHead.memcap = p->memcap;
    pstNAPTbl->stCommonHead.pstFuncTbl = &g_stNapAvlFuncTbl;

    pstNAPTbl->uiNapNodeSize = p->uiNodeSize;

    return (_NAP_HEAD_COMMON_S*) pstNAPTbl;
}



