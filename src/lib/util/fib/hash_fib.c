/*================================================================
*   Created：2018.12.07 LiXingang All rights reserved.
*   Description：hash fib
*
================================================================*/
#include "bs.h"
#include "utl/hash_utl.h"
#include "utl/fib_utl.h"

#define HASHFIB_HASH_BUCKET_NUM 1024

typedef struct {
    UINT ip; 
    UINT nexthop:24;
    UINT depth:8;
}FIB_ENTRY_S;

typedef struct
{
    HASH_NODE_S stHashNode;
    FIB_ENTRY_S fib_entry;
}HASH_FIB_NODE_S;

typedef struct
{
    HASH_HANDLE hHash;
}HASH_FIB_S;

typedef int (*PF_HASH_FIB_WALK_FUNC)(IN FIB_ENTRY_S *pstFibEntry, IN HANDLE hUserHandle);

static UINT hashfib_HashIndex(IN VOID *pstHashNode)
{
    HASH_FIB_NODE_S *pstFibHashNode = pstHashNode;
    UINT uiDstIp;

    uiDstIp = pstFibHashNode->fib_entry.ip;

    return ntohl(uiDstIp);
}

static INT hashfib_HashCmp(IN VOID *pstHashNode, IN VOID *pstNodeToFind)
{
    HASH_FIB_NODE_S *pstFibHashNode = pstHashNode;
    HASH_FIB_NODE_S *pstToFind = pstNodeToFind;
    INT iCmpRet;

    iCmpRet = pstFibHashNode->fib_entry.ip - pstToFind->fib_entry.ip;
    if (0 != iCmpRet) {
        return iCmpRet;
    }

    iCmpRet = pstFibHashNode->fib_entry.depth - pstToFind->fib_entry.depth;
    if (0 != iCmpRet) {
        return iCmpRet;
    }

    return 0;
}

static FIB_ENTRY_S * hashfib_PrefixMatch(IN HASH_FIB_S *hashfib, IN UINT uiDstIp )
{
    HASH_FIB_NODE_S *pstFound = NULL;
    HASH_FIB_NODE_S stFibToFind;
    UINT i;
    UINT uiMask;

    for (i=0; i<=32; i++) {
        uiMask = PREFIX_2_MASK(32 - i);
        uiMask = htonl(uiMask);
        stFibToFind.fib_entry.ip = (uiDstIp & uiMask);
        stFibToFind.fib_entry.depth = 32-i;
        pstFound = (void*) HASH_Find(hashfib->hHash, hashfib_HashCmp, (HASH_NODE_S*)&stFibToFind);
        if (NULL != pstFound) {
            break;
        }
    }

    return &pstFound->fib_entry;
}

static BS_STATUS hashfib_Add(IN HASH_FIB_S *hashfib, IN FIB_ENTRY_S *pstFibEntry)
{
    HASH_FIB_NODE_S *pstNode;
    HASH_FIB_NODE_S stToFind;

    stToFind.fib_entry = *pstFibEntry;

    pstNode = (void*) HASH_Find(hashfib->hHash, hashfib_HashCmp, &stToFind);
    if (NULL == pstNode) {
        pstNode = MEM_ZMalloc(sizeof(HASH_FIB_NODE_S));
        if (NULL == pstNode) {
            RETURN(BS_NO_MEMORY);
        }
        pstNode->fib_entry = *pstFibEntry;
        HASH_Add(hashfib->hHash, (HASH_NODE_S*)pstNode);
        return BS_OK;
    }

    pstNode->fib_entry.nexthop = pstFibEntry->nexthop;

    return BS_OK;
}

static VOID hashfib_Del(IN HASH_FIB_S *hashfib, IN FIB_ENTRY_S *pstFibEntry)
{
    HASH_FIB_NODE_S *pstFibHashNode;
    HASH_FIB_NODE_S stToFind;

    stToFind.fib_entry = *pstFibEntry;

    pstFibHashNode = (void*) HASH_Find(hashfib->hHash, hashfib_HashCmp, &stToFind);
    if (NULL == pstFibHashNode) {
        return;
    }

    HASH_Del(hashfib->hHash, (HASH_NODE_S*)pstFibHashNode);
    MEM_Free(pstFibHashNode);

    return;
}

static VOID  hashfib_FreeHashNode(IN HASH_HANDLE hHashId, IN VOID *pstNode, IN VOID * pUserHandle)
{
    HASH_FIB_NODE_S *pstFibHashNode = (void*)pstNode;

    MEM_Free(pstFibHashNode);
}

static int hashfib_WalkEach(IN HASH_HANDLE hHashId, IN HASH_NODE_S *pstNode, IN VOID * pUserHandle)
{
    HASH_FIB_NODE_S *pstFibHashNode = (void*)pstNode;
    USER_HANDLE_S *pstUserHandle = pUserHandle;
    PF_HASH_FIB_WALK_FUNC pfWalkFunc = pstUserHandle->ahUserHandle[0];

    return pfWalkFunc(&pstFibHashNode->fib_entry, pstUserHandle->ahUserHandle[1]);
}

int HashFib_Init(IN HASH_FIB_S *hashfib)
{
    HASH_HANDLE hHash;

    hHash = HASH_CreateInstance(NULL, HASHFIB_HASH_BUCKET_NUM, hashfib_HashIndex);
    if (NULL == hHash) {
        RETURN(BS_NO_MEMORY);
    }

    hashfib->hHash = hHash;

    return 0;
}

VOID HashFib_Fini(IN HASH_FIB_S *hashfib)
{
    if (NULL != hashfib) {
        HASH_DelAll(hashfib->hHash, hashfib_FreeHashNode, NULL);
        HASH_DestoryInstance(hashfib->hHash);
    }
}

BS_STATUS HashFib_Add(IN HASH_FIB_S *hashfib, IN FIB_ENTRY_S *pstFibEntry)
{
    UINT mask;

    mask = PREFIX_2_MASK(pstFibEntry->depth);
    mask = htonl(mask);

    pstFibEntry->ip &= mask;

    return hashfib_Add(hashfib, pstFibEntry);
}

VOID HashFib_Del(IN HASH_FIB_S *hashfib, IN FIB_ENTRY_S *pstFibEntry)
{
    UINT mask;

    mask = PREFIX_2_MASK(pstFibEntry->depth);
    mask = htonl(mask);

    pstFibEntry->ip &= mask;

    hashfib_Del(hashfib, pstFibEntry);

    return;
}

VOID HashFib_DelAll(IN HASH_FIB_S *hashfib)
{
    HASH_DelAll(hashfib->hHash, hashfib_FreeHashNode, NULL);
}

FIB_ENTRY_S * HashFib_Match(IN HASH_FIB_S *hashfib, IN UINT uiDstIp )
{
    return hashfib_PrefixMatch(hashfib, uiDstIp);
}

VOID HashFib_Walk(IN HASH_FIB_S *hashfib, IN PF_HASH_FIB_WALK_FUNC pfWalkFunc, IN HANDLE hUserHandle)
{
    USER_HANDLE_S stUserHandle;

    stUserHandle.ahUserHandle[0] = pfWalkFunc;
    stUserHandle.ahUserHandle[1] = hUserHandle;

    HASH_Walk(hashfib->hHash, (PF_HASH_WALK_FUNC)hashfib_WalkEach, &stUserHandle);
}

static INT hashfib_GetNextCmp(IN FIB_ENTRY_S *pstNode1, IN FIB_ENTRY_S *pstNode2)
{
    INT iCmpRet;

    if ((pstNode1 == NULL) && (pstNode2 == NULL)) {
        return 0;
    }

    if (pstNode1 == NULL) {
        return -1;
    }

    if (pstNode2 == NULL) {
        return 1;
    }

    iCmpRet = NUM_Cmp(pstNode1->ip, pstNode2->ip);
    if (0 != iCmpRet) {
        return iCmpRet;
    }

    iCmpRet = NUM_Cmp(pstNode1->depth, pstNode2->depth);
    if (0 != iCmpRet) {
        return iCmpRet;
    }

    return iCmpRet;
}

static int hashfib_GetNextEach(IN FIB_ENTRY_S *pstFibNode, IN HANDLE hUserHandle)
{
    USER_HANDLE_S *pstUserHandle = hUserHandle;
    FIB_ENTRY_S *pstFibCurrent = pstUserHandle->ahUserHandle[0];
    FIB_ENTRY_S *pstFibNext = pstUserHandle->ahUserHandle[1];
    BOOL_T *pbFound = pstUserHandle->ahUserHandle[2];

    if (pstFibCurrent != NULL) {
        if (hashfib_GetNextCmp(pstFibNode, pstFibCurrent) >= 0) {
            return 0;
        }
    }

    if (hashfib_GetNextCmp(pstFibNode, pstFibNext) <= 0) {
        return 0;
    }

    *pstFibNext = *pstFibNode;
    *pbFound = TRUE;

    return 0;
}


BS_STATUS HashFib_GetNext
(
    IN HASH_FIB_S *hashfib,
    IN FIB_ENTRY_S *pstFibCurrent,
    OUT FIB_ENTRY_S *pstFibNext
)
{
    USER_HANDLE_S stUserHandle;
    FIB_ENTRY_S stFibNext = {0};
    BOOL_T bFound = FALSE;

    stUserHandle.ahUserHandle[0] = pstFibCurrent;
    stUserHandle.ahUserHandle[1] = &stFibNext;
    stUserHandle.ahUserHandle[2] = &bFound;
  
    HashFib_Walk(hashfib, hashfib_GetNextEach, &stUserHandle);

    if (!bFound) {
        return BS_NOT_FOUND;
    }

    *pstFibNext = stFibNext;

    return BS_OK;
}
