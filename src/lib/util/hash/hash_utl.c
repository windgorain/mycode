/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2008-9-20
* Description: 
* History:     
******************************************************************************/

#define RETCODE_FILE_NUM RETCODE_FILE_NUM_HASHUTL

#include "bs.h"

#include "utl/hash_utl.h"
#include "utl/num_utl.h"
#include "utl/mem_cap.h"


typedef struct
{
    UINT uiInitHashBucketNum;
    UINT uiCurrHashBucketNum;
    USHORT high_watter_percent; 
    USHORT low_watter_percent;
    UINT high_watter_count; 
    UINT low_watter_count;
    UINT uiMask;
    UINT uiNodeCount;   
    PF_HASH_INDEX_FUNC pfHashIndexFunc;
    void *memcap;
    DLL_HEAD_S *pstBuckets;
}_HASH_CTRL_S;

static inline void hash_resize_buckets(_HASH_CTRL_S *ctrl, UINT bucket_num)
{
    UINT hash_index;
    UINT mask = bucket_num - 1;
    DLL_HEAD_S *buckets;
    HASH_NODE_S *pstNodeFind, *pstNodeTmp;
    UINT i;

    buckets = MemCap_Malloc(ctrl->memcap, bucket_num * sizeof(DLL_HEAD_S));
    if (! buckets) {
        return;
    }

    for (i=0; i<bucket_num; i++) {
        DLL_INIT(&buckets[i]);
    }

    for (i=0; i<ctrl->uiCurrHashBucketNum; i++) {
        DLL_SAFE_SCAN(&ctrl->pstBuckets[i], pstNodeFind, pstNodeTmp) {
            DLL_DEL(&ctrl->pstBuckets[i], pstNodeFind);
            hash_index = pstNodeFind->hash_factor & mask;
            DLL_ADD(&buckets[hash_index], pstNodeFind);
        }
    }

    MEM_Free(ctrl->pstBuckets);
    ctrl->pstBuckets = buckets;
    ctrl->uiCurrHashBucketNum = bucket_num;
    ctrl->uiMask = mask;
}


static inline void hash_resize_up(_HASH_CTRL_S *ctrl)
{
    if (ctrl->uiNodeCount < ctrl->high_watter_count) {
        return;
    }

    hash_resize_buckets(ctrl, ctrl->uiCurrHashBucketNum * 2);
}

static inline void hash_resize_down(_HASH_CTRL_S *ctrl)
{
    UINT bucket_num;

    if (ctrl->uiNodeCount >= ctrl->low_watter_count) {
        return;
    }

    bucket_num = ctrl->uiCurrHashBucketNum >> 1;
    if (bucket_num < ctrl->uiInitHashBucketNum) {
        return;
    }

    hash_resize_buckets(ctrl, bucket_num);
}

static int _hash_cmp_node(PF_HASH_CMP_FUNC pfCmpFunc, HASH_NODE_S *node1, HASH_NODE_S *node2)
{
    if (! node1) {
        return -1;
    }

    if (! node2) {
        return 1;
    }

    return pfCmpFunc(node1, node2);
}

HASH_HANDLE HASH_CreateInstance(void *memcap, IN UINT ulHashBucketNum, IN PF_HASH_INDEX_FUNC pfFunc)
{
    _HASH_CTRL_S *pstHashHead;
    UINT i;

    BS_DBGASSERT(NUM_IS2N(ulHashBucketNum));

    pstHashHead = MemCap_ZMalloc(memcap, sizeof(_HASH_CTRL_S));
    if (NULL == pstHashHead) {
        return NULL;
    }
    pstHashHead->memcap = memcap;

    pstHashHead->pstBuckets = MemCap_Malloc(memcap, sizeof(DLL_HEAD_S) * ulHashBucketNum);
    if (! pstHashHead->pstBuckets) {
        MemCap_Free(memcap, pstHashHead);
        return NULL;
    }

    pstHashHead->uiInitHashBucketNum = ulHashBucketNum;
    pstHashHead->uiCurrHashBucketNum = ulHashBucketNum;
    pstHashHead->uiMask = ulHashBucketNum - 1;
    pstHashHead->pfHashIndexFunc = pfFunc;

    for (i=0; i<ulHashBucketNum; i++)
    {
        DLL_INIT(&pstHashHead->pstBuckets[i]);
    }

    return pstHashHead;
}

VOID HASH_DestoryInstance(IN HASH_HANDLE hHashId)
{
    _HASH_CTRL_S *pstHashCtrl = (_HASH_CTRL_S*)hHashId;

    BS_DBGASSERT(0 != hHashId);

    if (pstHashCtrl->pstBuckets) {
        MemCap_Free(pstHashCtrl->memcap, pstHashCtrl->pstBuckets);
    }

    MemCap_Free(pstHashCtrl->memcap, pstHashCtrl);
}

void HASH_SetResizeWatter(HASH_HANDLE hHashId, UINT high_watter_percent, UINT low_watter_percent)
{
    _HASH_CTRL_S *pstHashCtrl = (_HASH_CTRL_S*)hHashId;

    pstHashCtrl->high_watter_percent = high_watter_percent;
    pstHashCtrl->low_watter_percent = low_watter_percent;
    pstHashCtrl->high_watter_count = (high_watter_percent * pstHashCtrl->uiCurrHashBucketNum) / 100;
    pstHashCtrl->low_watter_count = (low_watter_percent * pstHashCtrl->uiCurrHashBucketNum) / 100;
}

void HASH_AddWithFactor(IN HASH_HANDLE hHashId, IN VOID *pstNode , UINT hash_factor)
{
    _HASH_CTRL_S *pstHashCtrl = (_HASH_CTRL_S*)hHashId;
    HASH_NODE_S *node = pstNode;
    UINT ulHashIndex = hash_factor & pstHashCtrl->uiMask;

    node->hash_factor = hash_factor;

    DLL_ADD(&pstHashCtrl->pstBuckets[ulHashIndex], pstNode);

    pstHashCtrl->uiNodeCount ++;

    if (pstHashCtrl->high_watter_count) {
        hash_resize_up(pstHashCtrl);
    }
}

VOID HASH_Add(IN HASH_HANDLE hHashId, IN VOID *pstNode )
{
    _HASH_CTRL_S *pstHashCtrl = (_HASH_CTRL_S*)hHashId;

    BS_DBGASSERT(0 != hHashId);

    HASH_AddWithFactor(hHashId, pstNode, pstHashCtrl->pfHashIndexFunc(pstNode));
}

VOID HASH_Del(IN HASH_HANDLE hHashId, IN VOID *pstNode)
{
    UINT ulHashIndex;
    _HASH_CTRL_S *pstHashCtrl;

    BS_DBGASSERT(0 != hHashId);

    pstHashCtrl = (_HASH_CTRL_S*)hHashId;

    ulHashIndex = pstHashCtrl->pfHashIndexFunc(pstNode);
    ulHashIndex &= pstHashCtrl->uiMask;

    DLL_DEL(&pstHashCtrl->pstBuckets[ulHashIndex], pstNode);

    pstHashCtrl->uiNodeCount --;

    if (pstHashCtrl->low_watter_count) {
        hash_resize_down(pstHashCtrl);
    }
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

    for (i=0; i<pstHashCtrl->uiCurrHashBucketNum; i++)
    {
        if (NULL != pfFreeFunc)
        {
            DLL_SAFE_SCAN(&pstHashCtrl->pstBuckets[i], pstNodeFind, pstNodeTmp)
            {
                DLL_DEL(&pstHashCtrl->pstBuckets[i], pstNodeFind);
                pfFreeFunc(hHashId, pstNodeFind, pUserHandle);
            }
        }
    }

    pstHashCtrl->uiNodeCount = 0;

    if (pstHashCtrl->low_watter_count) {
        hash_resize_down(pstHashCtrl);
    }
}

VOID * HASH_FindWithFactor(IN HASH_HANDLE hHashId, UINT hash_factor, IN PF_HASH_CMP_FUNC pfCmpFunc, IN VOID *pstNodeToFind)
{
    _HASH_CTRL_S *pstHashCtrl = (_HASH_CTRL_S*)hHashId;
    UINT ulHashIndex = hash_factor & pstHashCtrl->uiMask;
    HASH_NODE_S *pstNodeFind;

    DLL_SCAN(&pstHashCtrl->pstBuckets[ulHashIndex], pstNodeFind) {
        if ((hash_factor == pstNodeFind->hash_factor)
                && (pfCmpFunc(pstNodeFind, pstNodeToFind) == 0)) {
            return pstNodeFind;
        }
    }

    return NULL;
}

VOID * HASH_Find(IN HASH_HANDLE hHashId, IN PF_HASH_CMP_FUNC pfCmpFunc, IN VOID *pstNodeToFind)
{
    UINT hash_factor;
    _HASH_CTRL_S *pstHashCtrl;

    BS_DBGASSERT(0 != hHashId);

    pstHashCtrl = (_HASH_CTRL_S*)hHashId;

    hash_factor = pstHashCtrl->pfHashIndexFunc(pstNodeToFind);

    return HASH_FindWithFactor(hHashId, hash_factor, pfCmpFunc, pstNodeToFind);
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

    for (i=0; i<pstHashCtrl->uiCurrHashBucketNum; i++) {
        DLL_SAFE_SCAN(&pstHashCtrl->pstBuckets[i], pstNodeFind, pstNodeTmp) {
            if (pfWalkFunc(hHashId, pstNodeFind, pUserHandle) < 0) {
                return;
            }
        }
    }

    return;
}


HASH_NODE_S * HASH_GetNext(HASH_HANDLE hHash, HASH_NODE_S *curr_node )
{
    _HASH_CTRL_S *pstHashCtrl;
    UINT i;
    HASH_NODE_S *node = NULL;
    UINT hash_factor;
    UINT index = 0;

    BS_DBGASSERT(0 != hHash);

    pstHashCtrl = (_HASH_CTRL_S*)hHash;

    
    if (curr_node) {
        hash_factor = pstHashCtrl->pfHashIndexFunc(curr_node);
        index = hash_factor & pstHashCtrl->uiMask;
        node = DLL_NEXT(&pstHashCtrl->pstBuckets[index], curr_node);
        if (node) {
            return node;
        }
        index ++;
    }

    for (i=index; i<pstHashCtrl->uiCurrHashBucketNum; i++) {
        node = DLL_FIRST(&pstHashCtrl->pstBuckets[i]);
        if (node) {
            break;
        }
    }

    return node;
}


HASH_NODE_S * HASH_GetNextDict(HASH_HANDLE hHash, PF_HASH_CMP_FUNC pfCmpFunc, HASH_NODE_S *curr_node )
{
    _HASH_CTRL_S *pstHashCtrl;
    HASH_NODE_S *pstNodeFind;
    HASH_NODE_S *pstNext = NULL;
    UINT i;

    BS_DBGASSERT(0 != hHash);

    pstHashCtrl = (_HASH_CTRL_S*)hHash;

    for (i=0; i<pstHashCtrl->uiCurrHashBucketNum; i++) {
        DLL_SCAN(&pstHashCtrl->pstBuckets[i], pstNodeFind) {
            if (_hash_cmp_node(pfCmpFunc, pstNodeFind, curr_node) > 0) {
                if (! pstNext) {
                    pstNext = pstNodeFind;
                } else if (_hash_cmp_node(pfCmpFunc, pstNodeFind, pstNext) < 0) {
                    pstNext = pstNodeFind;
                }
            }
        }
    }

    return pstNext;
}


