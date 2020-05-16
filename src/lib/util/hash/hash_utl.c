/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2008-9-20
* Description: 
* History:     
******************************************************************************/
/* retcode所需要的宏 */
#define RETCODE_FILE_NUM RETCODE_FILE_NUM_HASHUTL

#include "bs.h"

#include "utl/hash_utl.h"


typedef struct
{
    UINT ulHashBucketNum;
    UINT uiNodeCount;   /* Hash表中有多少个节点 */
    PF_HASH_INDEX_FUNC pfHashIndexFunc;
    DLL_HEAD_S astDllHead[1];
}_HASH_CTRL_S;


HASH_HANDLE HASH_CreateInstance(IN UINT ulHashBucketNum, IN PF_HASH_INDEX_FUNC pfFunc)
{
    _HASH_CTRL_S *pstHashHead;
    UINT i;

    if (ulHashBucketNum <= 0)
    {
        return NULL;
    }

    pstHashHead = MEM_ZMalloc(sizeof(_HASH_CTRL_S) + sizeof(DLL_HEAD_S) * (ulHashBucketNum - 1));
    if (NULL == pstHashHead)
    {
        return NULL;
    }

    pstHashHead->ulHashBucketNum = ulHashBucketNum;
    pstHashHead->pfHashIndexFunc = pfFunc;

    for (i=0; i<ulHashBucketNum; i++)
    {
        DLL_INIT(&pstHashHead->astDllHead[i]);
    }

    return pstHashHead;
}

VOID HASH_DestoryInstance(IN HASH_HANDLE hHashId)
{
    _HASH_CTRL_S *pstHashCtrl;

    BS_DBGASSERT(0 != hHashId);

    pstHashCtrl = (_HASH_CTRL_S*)hHashId;

    MEM_Free(pstHashCtrl);
}

VOID HASH_Add(IN HASH_HANDLE hHashId, IN VOID *pstNode)
{
    UINT ulHashIndex;
    _HASH_CTRL_S *pstHashCtrl;

    BS_DBGASSERT(0 != hHashId);

    pstHashCtrl = (_HASH_CTRL_S*)hHashId;

    ulHashIndex = pstHashCtrl->pfHashIndexFunc(pstNode);
    ulHashIndex %= pstHashCtrl->ulHashBucketNum;

    DLL_ADD(&pstHashCtrl->astDllHead[ulHashIndex], pstNode);

    pstHashCtrl->uiNodeCount ++;
}

/* 按照顺序添加 */
VOID HASH_SortAdd(IN HASH_HANDLE hHashId, IN VOID *pstNode, IN PF_HASH_CMP_FUNC pfCmpFunc)
{
    UINT ulHashIndex;
    _HASH_CTRL_S *pstHashCtrl;
    VOID *pstNodeTmp;

    BS_DBGASSERT(0 != hHashId);

    pstHashCtrl = (_HASH_CTRL_S*)hHashId;

    ulHashIndex = pstHashCtrl->pfHashIndexFunc(pstNode);
    ulHashIndex %= pstHashCtrl->ulHashBucketNum;

    DLL_SCAN(&pstHashCtrl->astDllHead[ulHashIndex], pstNodeTmp)
    {
        if (pfCmpFunc(pstNode, pstNodeTmp) >= 0)
        {
            DLL_INSERT_BEFORE(&pstHashCtrl->astDllHead[ulHashIndex], pstNode, pstNodeTmp);
            break;
        }
    }

    if (pstNodeTmp == NULL)
    {
        DLL_ADD(&pstHashCtrl->astDllHead[ulHashIndex], pstNode);
    }

    pstHashCtrl->uiNodeCount ++;
}

VOID HASH_Del(IN HASH_HANDLE hHashId, IN VOID *pstNode)
{
    UINT ulHashIndex;
    _HASH_CTRL_S *pstHashCtrl;

    BS_DBGASSERT(0 != hHashId);

    pstHashCtrl = (_HASH_CTRL_S*)hHashId;

    ulHashIndex = pstHashCtrl->pfHashIndexFunc(pstNode);
    ulHashIndex %= pstHashCtrl->ulHashBucketNum;

    DLL_DEL(&pstHashCtrl->astDllHead[ulHashIndex], pstNode);

    pstHashCtrl->uiNodeCount --;
}

VOID HASH_DelAll(IN HASH_HANDLE hHashId, IN PF_HASH_FREE_FUNC pfFreeFunc, IN VOID *pUserHandle)
{
    _HASH_CTRL_S *pstHashCtrl;
    HASH_NODE_S *pstNodeFind, *pstNodeTmp;
    UINT i;

    BS_DBGASSERT(0 != hHashId);

    pstHashCtrl = (_HASH_CTRL_S*)hHashId;

    if (pstHashCtrl->uiNodeCount == 0)
    {
        return;
    }

    for (i=0; i<pstHashCtrl->ulHashBucketNum; i++)
    {

        if (NULL != pfFreeFunc)
        {
            DLL_SAFE_SCAN(&pstHashCtrl->astDllHead[i], pstNodeFind, pstNodeTmp)
            {
                DLL_DEL(&pstHashCtrl->astDllHead[i], pstNodeFind);
                pfFreeFunc(hHashId, pstNodeFind, pUserHandle);
            }
        }
    }

    pstHashCtrl->uiNodeCount = 0;
}

VOID * HASH_Find(IN HASH_HANDLE hHashId, IN PF_HASH_CMP_FUNC pfCmpFunc, IN VOID *pstNodeToFind)
{
    UINT ulHashIndex;
    _HASH_CTRL_S *pstHashCtrl;
    HASH_NODE_S *pstNodeFind;

    BS_DBGASSERT(0 != hHashId);

    pstHashCtrl = (_HASH_CTRL_S*)hHashId;

    ulHashIndex = pstHashCtrl->pfHashIndexFunc(pstNodeToFind);
    ulHashIndex %= pstHashCtrl->ulHashBucketNum;

    DLL_SCAN(&pstHashCtrl->astDllHead[ulHashIndex], pstNodeFind)
    {
        if (pfCmpFunc(pstNodeFind, pstNodeToFind) == 0)
        {
            return pstNodeFind;
        }
    }

    return NULL;
}

UINT HASH_Count(IN HASH_HANDLE hHashId)
{
    _HASH_CTRL_S *pstHashCtrl;

    BS_DBGASSERT(0 != hHashId);

    pstHashCtrl = (_HASH_CTRL_S*)hHashId;

    return pstHashCtrl->uiNodeCount;
}

VOID HASH_Walk(IN HASH_HANDLE hHashId, IN PF_HASH_WALK_FUNC pfWalkFunc, IN VOID * pUserHandle)
{
    _HASH_CTRL_S *pstHashCtrl;
    HASH_NODE_S *pstNodeFind, *pstNodeTmp;
    UINT i;

    BS_DBGASSERT(0 != hHashId);

    pstHashCtrl = (_HASH_CTRL_S*)hHashId;

    for (i=0; i<pstHashCtrl->ulHashBucketNum; i++)
    {
        DLL_SAFE_SCAN(&pstHashCtrl->astDllHead[i], pstNodeFind, pstNodeTmp)
        {
            if (pfWalkFunc(hHashId, pstNodeFind, pUserHandle) != BS_WALK_CONTINUE)
            {
                return;
            }
        }
    }

    return;
}

