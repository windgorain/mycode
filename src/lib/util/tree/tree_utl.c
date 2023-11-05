/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2010-1-13
* Description: 多叉树
* History:     
******************************************************************************/
#include "bs.h"
#include "utl/tree_utl.h"


static int _tree_depth_back_walk
(
    IN TREE_NODE_S *pstRoot,
    IN UINT ulDeepth,
    IN PF_TREE_DepthWalkNode pfFunc,
    IN VOID *pUserHandle
)
{
    TREE_NODE_S *pstNode;
    int ret;
    
    ret = pfFunc(pstRoot, ulDeepth, FALSE, pUserHandle);
    if (ret < 0) {
        return ret;
    }

    DLL_SCAN(&pstRoot->stChildNode, pstNode) {
        ret = _tree_depth_back_walk(pstNode, ulDeepth + 1, pfFunc, pUserHandle);
        if (ret < 0) {
            return ret;
        }
    }

    return pfFunc(pstRoot, ulDeepth, TRUE, pUserHandle);
}

int TREE_DepthBackWalk
(
    IN TREE_NODE_S *pstRoot,
    IN PF_TREE_DepthWalkNode pfFunc,
    IN VOID *pUserHandle
)
{
    return _tree_depth_back_walk(pstRoot, 1, pfFunc, pUserHandle);
}

static int _tree_depth_parent_first_walk
(
    IN TREE_NODE_S *pstRoot,
    IN PF_TREE_DepthScanNode pfFunc,
    IN UINT ulDeepth,
    IN VOID *pUserHandle)
{
    TREE_NODE_S *pstNode;
    int ret;
    
    ret= pfFunc(pstRoot, ulDeepth, pUserHandle);
    if (ret < 0) {
        return ret;
    }

    ulDeepth ++;

    DLL_SCAN(&pstRoot->stChildNode, pstNode) {
        ret = _tree_depth_parent_first_walk(pstNode, pfFunc, ulDeepth, pUserHandle);
        if (ret < 0) {
            return ret;
        }
    }

    return 0;
}


int TREE_DepthParentFirstWalk
(
    IN TREE_NODE_S *pstRoot,
    IN PF_TREE_DepthScanNode pfFunc,
    IN VOID *pUserHandle
)
{
    return _tree_depth_parent_first_walk(pstRoot, pfFunc, 1, pUserHandle);
}


static int _tree_depth_child_first_walk(TREE_NODE_S *pstRoot,
        PF_TREE_DepthScanNode pfFunc, UINT ulDeepth, void *pUserHandle)
{
    TREE_NODE_S *pstNode;
    int ret;
    
    ulDeepth ++;

    DLL_SCAN(&pstRoot->stChildNode, pstNode) {
        ret = _tree_depth_child_first_walk(pstNode, pfFunc, ulDeepth, pUserHandle);
        if (ret < 0) {
            return ret;
        }
    }

    return pfFunc(pstRoot, ulDeepth - 1, pUserHandle);
}


int TREE_DepthChildFirstWalk(TREE_NODE_S *pstRoot, PF_TREE_DepthScanNode pfFunc, VOID *pUserHandle)
{
    return _tree_depth_child_first_walk(pstRoot, pfFunc, 1, pUserHandle);
}

int TREE_AddNode(IN TREE_NODE_S *pstParent, IN TREE_NODE_S *pstChildNode)
{
    BS_DBGASSERT(NULL != pstParent);
    BS_DBGASSERT(NULL != pstChildNode);

    DLL_ADD(&pstParent->stChildNode, pstChildNode);

    return BS_OK;
}

int TREE_RemoveNode(IN TREE_NODE_S *pstNode)
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

TREE_HANDLE Tree_Create()
{
    TREE_CTRL_S *ctrl;

    ctrl = MEM_ZMalloc(sizeof(TREE_CTRL_S));
    if (! ctrl) {
        return NULL;
    }

    TREE_NodeInit(&ctrl->root);
    return ctrl;
}

