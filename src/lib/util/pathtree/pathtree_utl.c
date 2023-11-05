/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2010-1-14
* Description: 
* History:     
******************************************************************************/

#define RETCODE_FILE_NUM RETCODE_FILE_NUM_TXT

#include "bs.h"

#include "utl/pathtree_utl.h"
#include "utl/path_utl.h"
#include "utl/txt_utl.h"

PATHTREE_NODE_S * PathTree_Create()
{
    PATHTREE_NODE_S * pstNode;

    pstNode = MEM_ZMalloc(sizeof(PATHTREE_NODE_S));
    if (NULL == pstNode)
    {
        return NULL;
    }

    TREE_NodeInit(&pstNode->stTreeNode);

    pstNode->pszDirName = "/";

    return pstNode;
}

static int _PATHTREE_FreeNode(TREE_NODE_S *pstNode, UINT ulDeepth, void *pUserHandle)
{
    USER_HANDLE_S *pstUserHandle = (USER_HANDLE_S*)pUserHandle;
    PATHTREE_NODE_S *pstPathTreeNode = (PATHTREE_NODE_S*)pstNode;
    PF_PathTree_DestoryNode pfFunc = pstUserHandle->ahUserHandle[0];

    pfFunc(pstPathTreeNode, pstUserHandle->ahUserHandle[1]);    

    if (!PATHTREE_IS_ROOT(pstPathTreeNode))
    {
        MEM_Free(pstPathTreeNode->pszDirName);
    }

    MEM_Free(pstPathTreeNode);

    return 0;
}

VOID PathTree_Destory
(
    IN PATHTREE_NODE_S *pstRoot,
    IN PF_PathTree_DestoryNode pfFunc,
    IN VOID *pUserHandle
)
{
    USER_HANDLE_S stUserHandle;

    stUserHandle.ahUserHandle[0] = pfFunc;
    stUserHandle.ahUserHandle[1] = pUserHandle;
    
    TREE_DepthChildFirstWalk(&(pstRoot->stTreeNode), _PATHTREE_FreeNode, &stUserHandle);
}

BS_STATUS PathTree_AddPath
(
    IN PATHTREE_NODE_S *pstRoot,
    IN CHAR *pszPath,
    IN VOID * pUserHandle
)
{
    CHAR *pszDir;
    UINT ulLen;
    PATHTREE_NODE_S *pstNode = pstRoot;
    PATHTREE_NODE_S *pstTemp = NULL;
    BOOL_T bIsFound;
    
    PATH_WALK_DIRS_START(pszPath, pszDir, ulLen)
    {
        BS_DBGASSERT(ulLen > 0);
        bIsFound = FALSE;
        TREE_SCAN_SUBNODE(&pstNode->stTreeNode, pstTemp)
        {
            if ((strlen(pstTemp->pszDirName) == ulLen)
                && (0 == strncmp(pszDir, pstTemp->pszDirName, ulLen)))
            {
                bIsFound = TRUE;
                break;
            }
        }

        if (bIsFound == FALSE)
        {
            pstTemp = MEM_ZMalloc(sizeof(PATHTREE_NODE_S));
            if (NULL == pstTemp)
            {
                RETURN(BS_NO_MEMORY);
            }
            TREE_NodeInit(&pstTemp->stTreeNode);
            pstTemp->pszDirName = MEM_Malloc(ulLen + 1);
            if (NULL == pstTemp->pszDirName)
            {
                MEM_Free(pstTemp);
                RETURN(BS_NO_MEMORY);
            }
            TXT_Strlcpy(pstTemp->pszDirName, pszDir, ulLen + 1);
            TREE_AddNode(&pstNode->stTreeNode, &pstTemp->stTreeNode);
        }

        pstNode = pstTemp;
    }PATH_WALK_END();

    if (pstTemp != NULL)
    {
        pstTemp->pUserHandle = pUserHandle;
    }

    return BS_OK;
}

static int _PathTree_DepthBackWalkTreeNode(TREE_NODE_S *pstNode, UINT ulDeepth, BOOL_T bIsBack, VOID *pUserHandle)
{
    USER_HANDLE_S *pstUserHandle = pUserHandle;
    PF_PathTree_DepthWalkNode pfFunc = pstUserHandle->ahUserHandle[0];

    return pfFunc((PATHTREE_NODE_S*)pstNode, ulDeepth, bIsBack, pstUserHandle->ahUserHandle[1]);    
}

int PathTree_DepthBackWalk(PATHTREE_NODE_S *pstRoot, PF_PathTree_DepthWalkNode pfFunc, VOID *pUserHandle)
{
    USER_HANDLE_S stHandle;
    
    stHandle.ahUserHandle[0] = pfFunc;
    stHandle.ahUserHandle[1] = pUserHandle;
    
    return TREE_DepthBackWalk(&pstRoot->stTreeNode, _PathTree_DepthBackWalkTreeNode, &stHandle);
}


