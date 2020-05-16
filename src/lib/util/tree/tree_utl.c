/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2010-1-13
* Description: 多叉树
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/tree_utl.h"

/* 
双向父优先深度遍历
就像走路一样,经过每个节点，有去有回。
 不像是深度遍历只取不回的跳跃遍历.
 root节点也遍历.每个节点都有来回两次.
 */
static BS_WALK_RET_E _TREE_DepthBackWalk
(
    IN TREE_NODE_S *pstRoot,
    IN UINT ulDeepth,
    IN PF_TREE_DepthWalkNode pfFunc,
    IN VOID *pUserHandle
)
{
    TREE_NODE_S *pstNode;
    BS_WALK_RET_E eRet;
    
    eRet = pfFunc(pstRoot, ulDeepth, FALSE, pUserHandle);
    if (eRet == BS_WALK_STOP)
    {
        return BS_WALK_STOP;
    }

    DLL_SCAN(&pstRoot->stChildNode, pstNode)
    {
        if (BS_WALK_STOP == _TREE_DepthBackWalk(pstNode, ulDeepth + 1, pfFunc, pUserHandle))
        {
            return BS_WALK_STOP;
        }
    }

    return pfFunc(pstRoot, ulDeepth, TRUE, pUserHandle);
}

BS_WALK_RET_E TREE_DepthBackWalk
(
    IN TREE_NODE_S *pstRoot,
    IN PF_TREE_DepthWalkNode pfFunc,
    IN VOID *pUserHandle
)
{
    return _TREE_DepthBackWalk(pstRoot, 1, pfFunc, pUserHandle);
}

static BS_WALK_RET_E _TREE_DepthParentFirstWalk
(
    IN TREE_NODE_S *pstRoot,
    IN PF_TREE_DepthScanNode pfFunc,
    IN UINT ulDeepth,
    IN VOID *pUserHandle)
{
    TREE_NODE_S *pstNode;
    BS_WALK_RET_E eRet;
    
    eRet = pfFunc(pstRoot, ulDeepth, pUserHandle);
    if (eRet == BS_WALK_STOP)
    {
        return BS_WALK_STOP;
    }

    ulDeepth ++;

    DLL_SCAN(&pstRoot->stChildNode, pstNode)
    {
        if (BS_WALK_STOP == _TREE_DepthParentFirstWalk(pstNode, pfFunc, ulDeepth, pUserHandle))
        {
            return BS_WALK_STOP;
        }
    }

    return BS_WALK_CONTINUE;
}

/* 深度优先遍历. 父优先. root节点也遍历 */
BS_WALK_RET_E TREE_DepthParentFirstWalk
(
    IN TREE_NODE_S *pstRoot,
    IN PF_TREE_DepthScanNode pfFunc,
    IN VOID *pUserHandle
)
{
    return _TREE_DepthParentFirstWalk(pstRoot, pfFunc, 1, pUserHandle);
}


static BS_WALK_RET_E _TREE_DepthChildFirstWalk
(
    IN TREE_NODE_S *pstRoot,
    IN PF_TREE_DepthScanNode pfFunc,
    IN UINT ulDeepth,
    IN VOID *pUserHandle)
{
    TREE_NODE_S *pstNode;
    
    ulDeepth ++;

    DLL_SCAN(&pstRoot->stChildNode, pstNode)
    {
        if (BS_WALK_STOP == _TREE_DepthChildFirstWalk(pstNode, pfFunc, ulDeepth, pUserHandle))
        {
            return BS_WALK_STOP;
        }
    }

    return pfFunc(pstRoot, ulDeepth - 1, pUserHandle);
}

/* 深度优先遍历. 子优先.root 节点也遍历 */
BS_WALK_RET_E TREE_DepthChildFirstWalk
(
    IN TREE_NODE_S *pstRoot,
    IN PF_TREE_DepthScanNode pfFunc,
    IN VOID *pUserHandle
)
{
    return _TREE_DepthChildFirstWalk(pstRoot, pfFunc, 1, pUserHandle);
}

BS_STATUS TREE_AddNode(IN TREE_NODE_S *pstParent, IN TREE_NODE_S *pstChildNode)
{
    BS_DBGASSERT(NULL != pstParent);
    BS_DBGASSERT(NULL != pstChildNode);

    DLL_ADD(&pstParent->stChildNode, pstChildNode);

    return BS_OK;
}

BS_STATUS TREE_RemoveNode(IN TREE_NODE_S *pstNode)
{
    DLL_HEAD_S *pstHead;
    
    BS_DBGASSERT(NULL != pstNode);

    pstHead = DLL_GET_HEAD(pstNode);

    if (NULL == pstHead)
    {
        return BS_OK;
    }
    
    DLL_DEL(pstHead, pstNode);

    return BS_OK;
}

TREE_NODE_S * TREE_GetParent(IN TREE_NODE_S *pstNode)
{
    DLL_HEAD_S *pstHead;
    
    pstHead = DLL_GET_HEAD(pstNode);

    if (NULL == pstHead)
    {
        return NULL;
    }

    return container_of(pstHead, TREE_NODE_S, stChildNode);
}


VOID TREE_NodeInit(IN TREE_NODE_S *pstNode)
{
    Mem_Zero(pstNode, sizeof(TREE_NODE_S));
    DLL_INIT(&pstNode->stChildNode);
}



