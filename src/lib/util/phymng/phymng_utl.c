/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2008-9-21
* Description: 
* History:     
******************************************************************************/
/* retcode所需要的宏 */
#define RETCODE_FILE_NUM RETCODE_FILE_NUM_PHYMNGUTL

#include "bs.h"

#include "utl/hash_utl.h"
#include "utl/phymng_utl.h"

#define _PHYMNG_HASH_BUCKET_NUM (16*1024)
    
typedef struct
{
    HANDLE hHashId;
}_PHYMNG_CTRL_S;

typedef struct
{
    HASH_NODE_S stHashNode;
    PHYMNG_NODE_S stPhyNode;
}_PHYMNGTBL_NODE_S;

typedef struct
{
    PF_PHYMNG_WALK_FUNC pfWalkFunc;
    HANDLE hPhyMngId;
    HANDLE hUserHandle;
}_PHYMNG_WALK_USERHANDLE_S;

UINT _PHYMNG_GetHashIndex(IN _PHYMNGTBL_NODE_S *pstNode)
{
    return pstNode->stPhyNode.ulIfIndex + pstNode->stPhyNode.ulPhyId;
}

INT _PHYMNG_CmpNode(IN _PHYMNGTBL_NODE_S *pstNode1, IN _PHYMNGTBL_NODE_S *pstNode2)
{
    if (pstNode1->stPhyNode.ulIfIndex == pstNode2->stPhyNode.ulIfIndex)
    {
        if (pstNode1->stPhyNode.ulPhyId == pstNode2->stPhyNode.ulPhyId)
        {
            return 0;
        }

        return pstNode1->stPhyNode.ulPhyId - pstNode2->stPhyNode.ulPhyId;
    }

    return pstNode1->stPhyNode.ulIfIndex - pstNode2->stPhyNode.ulIfIndex;
}

BS_WALK_RET_E _PHYMNG_WalkHash(IN UINT ulHashId, IN _PHYMNGTBL_NODE_S *pstNode, IN _PHYMNG_WALK_USERHANDLE_S *pstUserHandle)
{
    pstUserHandle->pfWalkFunc(pstUserHandle->hPhyMngId, &(pstNode->stPhyNode), pstUserHandle->hUserHandle);
    return BS_WALK_CONTINUE;
}

HANDLE PHYMNG_Create()
{
    _PHYMNG_CTRL_S *pstPhyMngHead;

    pstPhyMngHead = MEM_ZMalloc(sizeof(_PHYMNG_CTRL_S));
    if (NULL == pstPhyMngHead)
    {
        return 0;
    }

    pstPhyMngHead->hHashId = HASH_CreateInstance(NULL, _PHYMNG_HASH_BUCKET_NUM, (PF_HASH_INDEX_FUNC)_PHYMNG_GetHashIndex);
    if (0 == pstPhyMngHead->hHashId)
    {
        MEM_Free(pstPhyMngHead);
        return 0;
    }

    return pstPhyMngHead;
}

BS_STATUS PHYMNG_Add(IN HANDLE hPhyMngId, IN PHYMNG_NODE_S *pstNode)
{
    _PHYMNGTBL_NODE_S *pstTblNode;
    _PHYMNG_CTRL_S *pstTbl;

    BS_DBGASSERT(0 != hPhyMngId);

    pstTbl = (_PHYMNG_CTRL_S *)hPhyMngId;

    pstTblNode = MEM_ZMalloc(sizeof(_PHYMNGTBL_NODE_S));
    pstTblNode->stPhyNode = *pstNode;
    HASH_Add(pstTbl->hHashId, (HASH_NODE_S*)pstTblNode);

    return BS_OK;
}

BS_STATUS PHYMNG_Del(IN HANDLE hPhyMngId, IN PHYMNG_NODE_S *pstNode)
{
    HASH_NODE_S *pstHashNode;
    _PHYMNG_CTRL_S *pstTbl;
    _PHYMNGTBL_NODE_S stTblNode;

    BS_DBGASSERT(0 != hPhyMngId);

    pstTbl = (_PHYMNG_CTRL_S *)hPhyMngId;

    stTblNode.stPhyNode = *pstNode;

    pstHashNode = HASH_Find(pstTbl->hHashId, (PF_HASH_CMP_FUNC)_PHYMNG_CmpNode, (HASH_NODE_S*)&stTblNode);
    if (NULL != pstHashNode)
    {
        HASH_Del(pstTbl->hHashId, pstHashNode);
    }

    return BS_OK;
}

BS_STATUS PHYMNG_Find(IN HANDLE hPhyMngId, INOUT PHYMNG_NODE_S *pstNode)
{
    _PHYMNGTBL_NODE_S *pstHashNode;
    _PHYMNG_CTRL_S *pstTbl;
    _PHYMNGTBL_NODE_S stTblNode;

    BS_DBGASSERT(0 != hPhyMngId);

    pstTbl = (_PHYMNG_CTRL_S *)hPhyMngId;

    stTblNode.stPhyNode = *pstNode;

    pstHashNode = (_PHYMNGTBL_NODE_S *)HASH_Find(pstTbl->hHashId,
        (PF_HASH_CMP_FUNC)_PHYMNG_CmpNode, (HASH_NODE_S*)&stTblNode);

    if (NULL == pstHashNode)
    {
        RETURN(BS_NO_SUCH);
    }

    *pstNode = pstHashNode->stPhyNode;

    return BS_OK;
}


VOID PHYMNG_Walk(IN HANDLE hPhyMngId, IN PF_PHYMNG_WALK_FUNC pfWalkFunc, IN HANDLE hUserHandle)
{
    _PHYMNG_WALK_USERHANDLE_S stUserHandle;
    _PHYMNG_CTRL_S *pstTbl;

    BS_DBGASSERT(0 != hPhyMngId);

    pstTbl = (_PHYMNG_CTRL_S *)hPhyMngId;

    stUserHandle.pfWalkFunc = pfWalkFunc;
    stUserHandle.hPhyMngId = hPhyMngId;
    stUserHandle.hUserHandle = hUserHandle;

    HASH_Walk(pstTbl->hHashId, (PF_HASH_WALK_FUNC)_PHYMNG_WalkHash, &stUserHandle);
}

