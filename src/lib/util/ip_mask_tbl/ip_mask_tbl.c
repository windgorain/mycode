/*================================================================
*   Created by LiXingang
*   Description: 根据IP和Mask的表
*
================================================================*/
#include "bs.h"
#include "utl/hash_utl.h"
#include "utl/net.h"
#include "utl/num_utl.h"
#include "utl/ip_mask_tbl.h"

#define IPMASKTBL_HASH_BUCKET_NUM 1024


static UINT ipmasktbl_HashIndex(IN VOID *pstHashNode)
{
    IPMASKTBL_NODE_S *pstIpMaskTblNode = pstHashNode;
    UINT uiDstIp;

    uiDstIp = pstIpMaskTblNode->ip;

    return ntohl(uiDstIp);
}

static INT ipmasktbl_HashCmp(IN VOID *pstHashNode, IN VOID *pstNodeToFind)
{
    IPMASKTBL_NODE_S *pstIpMaskTblNode = pstHashNode;
    IPMASKTBL_NODE_S *pstToFind = pstNodeToFind;
    INT iCmpRet;

    iCmpRet = pstIpMaskTblNode->ip - pstToFind->ip;
    if (0 != iCmpRet) {
        return iCmpRet;
    }

    iCmpRet = pstIpMaskTblNode->depth - pstToFind->depth;
    if (0 != iCmpRet) {
        return iCmpRet;
    }

    return 0;
}

static BS_STATUS ipmasktbl_Add(IN IPMASKTBL_S *ipmasktbl, IN IPMASKTBL_NODE_S *pstIpMaskTblNode)
{
    IPMASKTBL_NODE_S *pstNode;

    pstNode = (void*) HASH_Find(ipmasktbl->hHash, ipmasktbl_HashCmp, pstIpMaskTblNode);
    if (NULL != pstNode) {
        RETURN(BS_ALREADY_EXIST);
    }

    HASH_Add(ipmasktbl->hHash, (HASH_NODE_S*)pstIpMaskTblNode);
    return BS_OK;
}

static int ipmasktbl_WalkEach(IN HASH_HANDLE hHashId, IN HASH_NODE_S *pstNode, IN VOID * pUserHandle)
{
    IPMASKTBL_NODE_S *pstIpMaskTblNode = (void*)pstNode;
    USER_HANDLE_S *pstUserHandle = pUserHandle;
    PF_IPMASKTBL_WALK_FUNC pfWalkFunc = pstUserHandle->ahUserHandle[0];

    return pfWalkFunc(pstIpMaskTblNode, pstUserHandle->ahUserHandle[1]);
}

static int ipmasktbl_MergeEach(IN HASH_HANDLE hHashId, IN HASH_NODE_S *pstNode, IN VOID * pUserHandle)
{
    PF_IPMASKTBL_MERGE_USER_DATA pfFunc = pUserHandle;
    IPMASKTBL_NODE_S *pstIpMaskTblNode = (void*)pstNode;
    UINT ip = pstIpMaskTblNode->ip;
    UCHAR depth = pstIpMaskTblNode->depth;
    IPMASKTBL_NODE_S *pstFound = NULL;
    IPMASKTBL_NODE_S stToFind;
    UINT i;
    UINT uiMask;

    for (i=0; i<depth; i++) {
        uiMask = PREFIX_2_MASK(i);
        uiMask = htonl(uiMask);
        stToFind.ip = (ip & uiMask);
        stToFind.depth = i;
        pstFound = (void*) HASH_Find(hHashId, ipmasktbl_HashCmp, (HASH_NODE_S*)&stToFind);
        if (pstFound) {
            pfFunc((void*)pstNode, (void*)pstFound);
        }
    }

    return 0;
}

static INT ipmasktbl_GetNextCmp(IN IPMASKTBL_NODE_S *pstNode1, IN IPMASKTBL_NODE_S *pstNode2)
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

static int ipmasktbl_GetNextEach(IN IPMASKTBL_NODE_S *pstIpMaskNode, IN HANDLE hUserHandle)
{
    USER_HANDLE_S *pstUserHandle = hUserHandle;
    IPMASKTBL_NODE_S *pstCurrent = pstUserHandle->ahUserHandle[0];
    IPMASKTBL_NODE_S *pstNext = pstUserHandle->ahUserHandle[1];

    if (pstCurrent != NULL) {
        if (ipmasktbl_GetNextCmp(pstIpMaskNode, pstCurrent) >= 0) {
            return 0;
        }
    }

    if (pstNext != NULL) {
        if (ipmasktbl_GetNextCmp(pstIpMaskNode, pstNext) <= 0) {
            return 0;
        }
    }

    pstUserHandle->ahUserHandle[1] = pstIpMaskNode;

    return 0;
}

int IPMASKTBL_Init(IN IPMASKTBL_S *ipmasktbl)
{
    HASH_HANDLE hHash;

    hHash = HASH_CreateInstance(NULL, IPMASKTBL_HASH_BUCKET_NUM, ipmasktbl_HashIndex);
    if (NULL == hHash) {
        RETURN(BS_NO_MEMORY);
    }

    ipmasktbl->hHash = hHash;

    return 0;
}

VOID IPMASKTBL_Fini(IN IPMASKTBL_S *ipmasktbl, PF_HASH_FREE_FUNC pfFree, void *user_handle)
{
    if (NULL != ipmasktbl) {
        HASH_DelAll(ipmasktbl->hHash, pfFree, user_handle);
        HASH_DestoryInstance(ipmasktbl->hHash);
    }
}

BS_STATUS IPMASKTBL_Add(IN IPMASKTBL_S *ipmasktbl, IN IPMASKTBL_NODE_S *pstIpMaskTblNode)
{
    UINT mask;

    mask = PREFIX_2_MASK(pstIpMaskTblNode->depth);
    mask = htonl(mask);

    pstIpMaskTblNode->ip &= mask;

    return ipmasktbl_Add(ipmasktbl, pstIpMaskTblNode);
}

IPMASKTBL_NODE_S * IPMASKTBL_Get(IN IPMASKTBL_S *ipmasktbl, UINT ip, UCHAR depth)
{
    IPMASKTBL_NODE_S stToFind;

    stToFind.ip = ip;
    stToFind.depth = depth;

    return (void*) HASH_Find(ipmasktbl->hHash, ipmasktbl_HashCmp, &stToFind);
}

VOID IPMASKTBL_Del(IN IPMASKTBL_S *ipmasktbl, IN IPMASKTBL_NODE_S *pstNode)
{
    HASH_Del(ipmasktbl->hHash, (HASH_NODE_S*)pstNode);

    return;
}

VOID IPMASKTBL_DelAll(IN IPMASKTBL_S *ipmasktbl, PF_HASH_FREE_FUNC pfFree, void *user_handle)
{
    HASH_DelAll(ipmasktbl->hHash, pfFree, user_handle);
}

IPMASKTBL_NODE_S * IPMASKTBL_Match(IN IPMASKTBL_S *ipmasktbl, IN UINT uiDstIp )
{
    IPMASKTBL_NODE_S *pstFound = NULL;
    IPMASKTBL_NODE_S stToFind;
    UINT i;
    UINT uiMask;

    for (i=0; i<=32; i++) {
        uiMask = PREFIX_2_MASK(32 - i);
        uiMask = htonl(uiMask);
        stToFind.ip = (uiDstIp & uiMask);
        stToFind.depth = 32-i;
        pstFound = (void*) HASH_Find(ipmasktbl->hHash, ipmasktbl_HashCmp, (HASH_NODE_S*)&stToFind);
        if (NULL != pstFound) {
            break;
        }
    }

    return pstFound;
}

IPMASKTBL_NODE_S * IPMASKTBL_BfMatch(IN IPMASKTBL_S *ipmasktbl, IPMASKTBL_BF_S *ipmasktbl_bf, IN UINT uiDstIp )
{
    IPMASKTBL_NODE_S *pstFound = NULL;
    IPMASKTBL_NODE_S stToFind;
    UINT i;
    UINT uiMask;

    for (i=0; i<=32; i++) {
        uiMask = PREFIX_2_MASK(32 - i);
        uiMask = htonl(uiMask);
        stToFind.ip = (uiDstIp & uiMask);
        stToFind.depth = 32-i;

        if (! IPMASKTBL_BfTest(ipmasktbl_bf, stToFind.ip, stToFind.depth)) {
            continue;
        }

        pstFound = (void*) HASH_Find(ipmasktbl->hHash, ipmasktbl_HashCmp, (HASH_NODE_S*)&stToFind);
        if (NULL != pstFound) {
            break;
        }
    }

    return pstFound;
}

VOID IPMASKTBL_Walk(IN IPMASKTBL_S *ipmasktbl, IN PF_IPMASKTBL_WALK_FUNC pfWalkFunc, IN HANDLE hUserHandle)
{
    USER_HANDLE_S stUserHandle;

    stUserHandle.ahUserHandle[0] = pfWalkFunc;
    stUserHandle.ahUserHandle[1] = hUserHandle;

    HASH_Walk(ipmasktbl->hHash, (PF_HASH_WALK_FUNC)ipmasktbl_WalkEach, &stUserHandle);
}


IPMASKTBL_NODE_S * IPMASKTBL_GetNext( IN IPMASKTBL_S *ipmasktbl, IN IPMASKTBL_NODE_S *pstFibCurrent)
{
    USER_HANDLE_S stUserHandle;

    stUserHandle.ahUserHandle[0] = pstFibCurrent;
  
    IPMASKTBL_Walk(ipmasktbl, ipmasktbl_GetNextEach, &stUserHandle);

    return stUserHandle.ahUserHandle[1];
}


void IPMASKTBL_MergeUserData(IPMASKTBL_S *ipmasktbl, PF_IPMASKTBL_MERGE_USER_DATA pfFunc)
{
    HASH_Walk(ipmasktbl->hHash, (PF_HASH_WALK_FUNC)ipmasktbl_MergeEach, pfFunc);
}


