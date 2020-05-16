/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2012-11-19
* Description: ID-Resource映射表
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/idres_utl.h"

typedef struct
{
    UINT uiRef; /* 引用计数 */
    DLL_HEAD_S stIdResList;
    PF_IDRES_FREE pfFreeFunc;
}IDRES_CTRL_S;

typedef struct
{
    DLL_NODE_S stLinkNode;
    UINT uiId;
    HANDLE hRes;
}IDRES_NODE_S;

IDRES_HANDLE IDRES_Create(IN PF_IDRES_FREE pfFreeFunc)
{
    IDRES_CTRL_S *pstCtrl;

    pstCtrl = MEM_ZMalloc(sizeof(IDRES_CTRL_S));
    if (NULL == pstCtrl)
    {
        return NULL;
    }

    pstCtrl->uiRef = 1;
    pstCtrl->pfFreeFunc = pfFreeFunc;
    DLL_INIT(&pstCtrl->stIdResList);

    return pstCtrl;
}

VOID IDRES_Destory(IN IDRES_HANDLE hIdRes)
{
    IDRES_CTRL_S *pstCtrl = hIdRes;
    IDRES_NODE_S *pstNode, *pstNodeNext;

    if (NULL == pstCtrl)
    {
        return;
    }

    pstCtrl->uiRef --;

    if (pstCtrl->uiRef > 0)
    {
        return;
    }

    DLL_SAFE_SCAN(&pstCtrl->stIdResList, pstNode, pstNodeNext)
    {
        pstCtrl->pfFreeFunc(pstNode->uiId, pstNode->hRes);
        DLL_DEL(&pstCtrl->stIdResList, pstNode);
        MEM_Free(pstNode);
    }

    MEM_Free(pstCtrl);
}

VOID IDRES_Ref(IN IDRES_HANDLE hIdRes)
{
    IDRES_CTRL_S *pstCtrl = hIdRes;

    BS_DBGASSERT(NULL != pstCtrl);

    pstCtrl->uiRef ++;
}

static IDRES_NODE_S * idres_Find(IN IDRES_CTRL_S *pstCtrl, IN UINT uiId)
{
    IDRES_NODE_S *pstNode;

    DLL_SCAN(&pstCtrl->stIdResList, pstNode)
    {
        if (pstNode->uiId == uiId)
        {
            return pstNode;
        }
    }

    return NULL;
}

BS_STATUS IDRES_Set(IN IDRES_HANDLE hIdRes, IN UINT uiId, IN HANDLE hRes)
{
    IDRES_CTRL_S *pstCtrl = hIdRes;
    IDRES_NODE_S *pstNode;

    pstNode = idres_Find(pstCtrl, uiId);
    if (NULL == pstNode)
    {
        pstNode = MEM_ZMalloc(sizeof(IDRES_NODE_S));
        if (NULL == pstNode)
        {
            return BS_NO_MEMORY;
        }
        pstNode->uiId = uiId;
        DLL_ADD_TO_HEAD(&pstCtrl->stIdResList, pstNode);
    }

    pstNode->hRes = hRes;

    return BS_OK;
}

HANDLE IDRES_Get(IN IDRES_HANDLE hIdRes, IN UINT uiId)
{
    IDRES_CTRL_S *pstCtrl = hIdRes;
    IDRES_NODE_S *pstNode;

    pstNode = idres_Find(pstCtrl, uiId);
    if (NULL == pstNode)
    {
        return NULL;
    }

    return pstNode->hRes;
}


