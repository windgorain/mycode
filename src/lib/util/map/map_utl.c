/******************************************************************************
* Copyright (C), 2000-2006,  Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2017-6-3
* Description: 集合
* History: rename from aggregate to map
******************************************************************************/
#include "bs.h"
#include "utl/jhash_utl.h"
#include "utl/hash_utl.h"
#include "utl/mem_utl.h"
#include "utl/map_utl.h"

#define _MAP_HASH_BUCKET_NUM 1024

typedef struct
{
    UINT uiFlag;
    HASH_HANDLE hHash;
}_MAP_CTRL_S;

typedef struct
{
    HASH_NODE_S stHashNode;
    MAP_ELE_S stEle;
}_MAP_NODE_S;

static UINT _map_Index(IN VOID *pstHashNode)
{
    _MAP_NODE_S *pstNode = pstHashNode;

    return JHASH_GeneralBuffer(pstNode->stEle.pKey, pstNode->stEle.uiKeyLen, 0);
}

static INT  _map_Cmp(IN VOID * pstHashNode1, IN VOID * pstHashNode2)
{
    _MAP_NODE_S *pstNode1 = pstHashNode1;
    _MAP_NODE_S *pstNode2 = pstHashNode2;

    return MEM_Cmp(pstNode1->stEle.pKey, pstNode1->stEle.uiKeyLen, pstNode2->stEle.pKey, pstNode2->stEle.uiKeyLen);
}

static _MAP_NODE_S * _map_Find(IN _MAP_CTRL_S *pstCtrl, IN VOID *pKey, IN UINT uiKeyLen)
{
    _MAP_NODE_S stNode;
    
    stNode.stEle.pKey = pKey;
    stNode.stEle.uiKeyLen = uiKeyLen;

    return HASH_Find(pstCtrl->hHash, _map_Cmp, &stNode);
}

static BS_WALK_RET_E _map_Walk(IN HASH_HANDLE hHashId, IN VOID *pNode, IN VOID * pUserHandle)
{
    USER_HANDLE_S *pstUserHandle = pUserHandle;
    PF_MAP_WALK_FUNC pfWalkFunc = pstUserHandle->ahUserHandle[0];
    VOID *pUserHandleTmp = pstUserHandle->ahUserHandle[1];
    _MAP_NODE_S *pstNode = pNode;

    return pfWalkFunc(&pstNode->stEle, pUserHandleTmp);
}

static void _map_FreeAll(IN HASH_HANDLE hHashId, IN VOID *pNode, IN VOID * pUserHandle)
{
    USER_HANDLE_S *pstUserHandle = pUserHandle;
    PF_MAP_FREE_FUNC func = pstUserHandle->ahUserHandle[0];
    VOID *pUserHandleTmp = pstUserHandle->ahUserHandle[1];
    _MAP_NODE_S *pstNode = pNode;

    func(pstNode->stEle.pData, pUserHandleTmp);
    MEM_Free(pstNode->stEle.pKey);
    MEM_Free(pstNode);
}

MAP_HANDLE MAP_Create()
{
    _MAP_CTRL_S *pstCtrl;

    pstCtrl = MEM_ZMalloc(sizeof(_MAP_CTRL_S));
    if (NULL == pstCtrl)
    {
        return NULL;
    }

    pstCtrl->hHash = HASH_CreateInstance(_MAP_HASH_BUCKET_NUM, _map_Index);
    if (NULL == pstCtrl->hHash)
    {
        MEM_Free(pstCtrl);
        return NULL;
    }

    return pstCtrl;
}

void MAP_Destroy(MAP_HANDLE hMap, PF_MAP_FREE_FUNC free_func, void *user_data)
{
    _MAP_CTRL_S *pstCtrl = hMap;

    if (NULL == pstCtrl) {
        return;
    }

    MAP_DelAll(hMap, free_func, user_data);

    HASH_DestoryInstance(pstCtrl->hHash);
    MEM_Free(pstCtrl);
}

BS_STATUS MAP_Add(MAP_HANDLE hMap, VOID *pKey, UINT uiKeyLen, VOID *pData)
{
    _MAP_NODE_S *pstNode;
    _MAP_CTRL_S *pstCtrl = hMap;

    if (NULL != _map_Find(hMap, pKey, uiKeyLen)) {
        return BS_ALREADY_EXIST;
    }

    pstNode = MEM_ZMalloc(sizeof(_MAP_NODE_S));
    if (NULL == pstNode) {
        return BS_NO_MEMORY;
    }

    pstNode->stEle.pKey = MEM_Dup(pKey, uiKeyLen);
    if (NULL == pstNode->stEle.pKey) {
        MEM_Free(pstNode);
        return BS_NO_MEMORY;
    }

    pstNode->stEle.uiKeyLen = uiKeyLen;
    pstNode->stEle.pData = pData;

    HASH_Add(pstCtrl->hHash, pstNode);

    return BS_OK;
}

VOID * MAP_Get(IN MAP_HANDLE hMap, IN VOID *pKey, IN UINT uiKeyLen)
{
    _MAP_CTRL_S *pstCtrl = hMap;
    _MAP_NODE_S *pstFind;

    pstFind = _map_Find(pstCtrl, pKey, uiKeyLen);
    if (NULL == pstFind)
    {
        return NULL;
    }

    return pstFind->stEle.pData;
}

/* 从集合中删除并返回pData */
VOID * MAP_Del(IN MAP_HANDLE hMap, IN VOID *pKey, IN UINT uiKeyLen)
{
    _MAP_CTRL_S *pstCtrl = hMap;
    _MAP_NODE_S *pstNode;
    VOID *pData;

    pstNode = _map_Find(hMap, pKey, uiKeyLen);
    if (NULL == pstNode)
    {
        return NULL;
    }

    HASH_Del(pstCtrl->hHash, pstNode);

    pData = pstNode->stEle.pData;

    MEM_Free(pstNode->stEle.pKey);
    MEM_Free(pstNode);

    return pData;
}

void MAP_DelAll(MAP_HANDLE hMap, PF_MAP_FREE_FUNC func, void * pUserData)
{
    _MAP_CTRL_S *pstCtrl = hMap;
    USER_HANDLE_S stUserHandle;

    stUserHandle.ahUserHandle[0] = func;
    stUserHandle.ahUserHandle[1] = pUserData;

    HASH_DelAll(pstCtrl->hHash, _map_FreeAll, &stUserHandle);
}

VOID MAP_Walk(IN MAP_HANDLE hMap, IN PF_MAP_WALK_FUNC pfWalkFunc, IN VOID *pUserData)
{
    _MAP_CTRL_S *pstCtrl = hMap;
    USER_HANDLE_S stUserHandle;

    stUserHandle.ahUserHandle[0] = pfWalkFunc;
    stUserHandle.ahUserHandle[1] = pUserData;

    HASH_Walk(pstCtrl->hHash, _map_Walk, &stUserHandle);
}

static INT _map_GetNextCmp(IN MAP_ELE_S *pstNode1, IN MAP_ELE_S *pstNode2)
{
    INT iCmpRet;

    if ((pstNode1 == NULL) && (pstNode2 == NULL))
    {
        return 0;
    }

    if (pstNode1 == NULL)
    {
        return -1;
    }

    if (pstNode2 == NULL)
    {
        return 1;
    }

    iCmpRet = MEM_Cmp(pstNode1->pKey, pstNode1->uiKeyLen, pstNode2->pKey, pstNode2->uiKeyLen);

    return iCmpRet;
}

static BS_WALK_RET_E _map_GetNextEach(IN MAP_ELE_S *pstEle, IN HANDLE hUserHandle)
{
    USER_HANDLE_S *pstUserHandle = hUserHandle;
    MAP_ELE_S *pstEleCurrent = pstUserHandle->ahUserHandle[0];
    MAP_ELE_S *pstEleNext = pstUserHandle->ahUserHandle[1];

    if (_map_GetNextCmp(pstEle, pstEleCurrent) <= 0)
    {
        return BS_WALK_CONTINUE;
    }

    if ((pstEleNext != NULL) && (_map_GetNextCmp(pstEle, pstEleNext) >= 0))
    {
        return BS_WALK_CONTINUE;
    }

    pstUserHandle->ahUserHandle[1] = pstEle;

    return BS_WALK_CONTINUE;
}

MAP_ELE_S * MAP_GetNext
(
    IN MAP_HANDLE hMap,
    IN MAP_ELE_S *pstCurrent /* 如果为NULL表示获取第一个. 只关心其中的pKye和uiKeyLen字段 */
)
{
    USER_HANDLE_S stUserHandle;

    stUserHandle.ahUserHandle[0] = pstCurrent;
    stUserHandle.ahUserHandle[1] = NULL;
  
    MAP_Walk(hMap, _map_GetNextEach, &stUserHandle);

    return stUserHandle.ahUserHandle[1];
}

