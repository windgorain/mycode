/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2014-11-16
* Description: node id pool
*              带有id/index的node管理, key为id/index. 适用于根据id/index快速查找node
*              但是不能根据node的信息快速查找node, 只能遍历比较查找.
*              node有一个隐藏header,里面包含id的信息, 所以可以根据node直接获取id/index
* History:     
******************************************************************************/
#include "bs.h"
        
#include "utl/net.h"
#include "utl/num_utl.h"
#include "utl/nap_utl.h"
#include "utl/mem_cap.h"

#include "nap_inner.h"


typedef struct
{
    UINT ulID;
}_NAP_NODE_COMMON_S;

static BS_STATUS nap_CommonInit
(
    INOUT _NAP_HEAD_COMMON_S *pstCommonHead,
    NAP_PARAM_S *p
)
{
    UCHAR ucIndexBits;
    UCHAR ucSpaceBits;
    UINT uiSpaceMask;

    pstCommonHead->uiMaxNum = p->uiMaxNum;
    if (p->uiMaxNum == 0) {
        pstCommonHead->uiMaxNum = 0x7fffffff;
    }

    pstCommonHead->uiNodeSize = p->uiNodeSize;

    ucIndexBits = NUM_NeedBits(pstCommonHead->uiMaxNum);

    ucSpaceBits = 32 - ucIndexBits;

    uiSpaceMask = PREFIX_2_MASK(ucSpaceBits);
    
    pstCommonHead->ulIndexMask = ~uiSpaceMask;

    LBITMAP_PARAM_S lbitmap_param = {0};
    lbitmap_param.memcap = p->memcap;

    pstCommonHead->hLBitmap = LBitMap_Create(&lbitmap_param);
    if (NULL == pstCommonHead->hLBitmap) {
        return BS_NO_MEMORY;
    }

    return BS_OK;
}

static VOID nap_CommonFini(INOUT _NAP_HEAD_COMMON_S *pstCommonHead)
{
    if (NULL != pstCommonHead->hLBitmap)
    {
        LBitMap_Destory(pstCommonHead->hLBitmap);
        pstCommonHead->hLBitmap = NULL;
    }
    
    if (NULL != pstCommonHead->stSeqOpt.seq_array)
    {
        MemCap_Free(pstCommonHead->memcap, pstCommonHead->stSeqOpt.seq_array);
        pstCommonHead->stSeqOpt.seq_array = NULL;
    }
}

static inline UINT nap_AllocIndex(IN _NAP_HEAD_COMMON_S *pstCommonHead)
{
    UINT uiIndex;

    if (BS_OK != LBitMap_AllocByRange(pstCommonHead->hLBitmap,
                0, 0x7fffffff, &uiIndex)) {
        return NAP_INVALID_INDEX;
    }

    return uiIndex;
}

static inline UINT nap_AllocSpecIndex(IN _NAP_HEAD_COMMON_S *pstCommonHead, IN UINT uiSpecIndex)
{
    if (LBitMap_IsBitSetted(pstCommonHead->hLBitmap, uiSpecIndex)) {
        return NAP_INVALID_INDEX;
    }

    if (BS_OK != LBitMap_SetBit(pstCommonHead->hLBitmap, uiSpecIndex)) {
        return NAP_INVALID_INDEX;
    }

    return uiSpecIndex;
}

static inline VOID nap_FreeIndex(IN _NAP_HEAD_COMMON_S *pstCommonHead, IN UINT uiIndex)
{
    LBitMap_ClrBit(pstCommonHead->hLBitmap, uiIndex);
}

static UINT nap_CacleIDByIndex(IN _NAP_HEAD_COMMON_S *pstCommonHead, IN UINT uiIndex)
{
    UINT uiPos;
    UINT ulTmp;
    UINT ulID = uiIndex;

    if (TRUE == pstCommonHead->stSeqOpt.bEnable) {
        uiPos = uiIndex;
        uiPos = uiPos % pstCommonHead->stSeqOpt.uiSeqArrayCount;
        ulTmp = pstCommonHead->stSeqOpt.seq_array[uiPos] ++;
        ulTmp = ulTmp << pstCommonHead->stSeqOpt.ucSeqStartIndex;
        ulTmp &= pstCommonHead->stSeqOpt.ulSeqMask;
        ulID &= (~pstCommonHead->stSeqOpt.ulSeqMask);
        ulID |= ulTmp;
    } 

    return ulID | 0x80000000;
}

static inline UINT nap_CacleIndexByID(IN _NAP_HEAD_COMMON_S *pstCommonHead, IN UINT ulID)
{
    return (UINT)(ulID & pstCommonHead->ulIndexMask);
}

static inline BOOL_T nap_IsIndexSetted(_NAP_HEAD_COMMON_S *pstCommonHead, UINT index)
{
    return LBitMap_IsBitSetted(pstCommonHead->hLBitmap, index);
}

static BS_STATUS nap_SetSeqMask(IN HANDLE hNAPHandle, IN UINT ulSeqMask)
{
    UINT i;
    UCHAR ucSeqStartIndex = 0;
    _NAP_HEAD_COMMON_S *pstCommonHead = hNAPHandle;

    for (i=0; i<32; i++) {
        if (ulSeqMask & 1ULL << i) {
            ucSeqStartIndex = i;
            break;
        }
    }

    if ((ucSeqStartIndex == 0) || (ucSeqStartIndex == 63)) {
        BS_DBGASSERT(0);
        return BS_BAD_PARA;
    }

    pstCommonHead->stSeqOpt.ulSeqMask = ulSeqMask;
    pstCommonHead->stSeqOpt.ucSeqStartIndex = ucSeqStartIndex;

    return BS_OK;
}

NAP_HANDLE NAP_Create(NAP_PARAM_S *p)
{
    _NAP_HEAD_COMMON_S *pstHeadCommon = NULL;

    if (p->uiMaxNum > 0x7fffffff) {
        BS_DBGASSERT(0);
        return NULL;
    }

    NAP_PARAM_S param = *p;

    param.uiNodeSize += sizeof(_NAP_NODE_COMMON_S);

    switch (p->enType) {
        case NAP_TYPE_ARRAY:
            pstHeadCommon = _NAP_ArrayCreate(&param);
            break;

        case NAP_TYPE_PTR_ARRAY:
            pstHeadCommon = _NAP_PtrArrayCreate(&param);
            break;

        case NAP_TYPE_HASH:
            pstHeadCommon = _NAP_HashCreate(&param);
            break;

        case NAP_TYPE_AVL:
            pstHeadCommon = _NAP_AvlCreate(&param);
            break;

        default:
            break;
    }

    if (NULL != pstHeadCommon) {
        if (BS_OK != nap_CommonInit(pstHeadCommon, p)) {
            NAP_Destory(pstHeadCommon);
            pstHeadCommon = NULL;
        }
    }

    return pstHeadCommon;
}

VOID NAP_Destory(IN NAP_HANDLE hNAPHandle)
{
    _NAP_HEAD_COMMON_S *pstCommonHead = hNAPHandle;

    if (NULL == hNAPHandle) {
        return;
    }

    nap_CommonFini(pstCommonHead);

    pstCommonHead->pstFuncTbl->pfDestory(hNAPHandle);
}

void * NAP_GetMemCap(NAP_HANDLE hNAPHandle)
{
    _NAP_HEAD_COMMON_S *pstCommonHead = hNAPHandle;
    return pstCommonHead->memcap;
}

UINT NAP_GetNodeSize(IN NAP_HANDLE hNapHandle)
{
    _NAP_HEAD_COMMON_S *pstCommonHead = hNapHandle;

    return pstCommonHead->uiNodeSize;
}


VOID * NAP_Alloc(IN NAP_HANDLE hNapHandle)
{
    _NAP_HEAD_COMMON_S *pstCommonHead = hNapHandle;
    _NAP_NODE_COMMON_S *pstCommonNode;
    UINT uiIndex;

    if (NULL == hNapHandle) {
        return NULL;
    }

    uiIndex = nap_AllocIndex(pstCommonHead);
    if (NAP_INVALID_INDEX == uiIndex) {
        return NULL;
    }

    pstCommonNode = pstCommonHead->pstFuncTbl->pfAlloc(hNapHandle, uiIndex);
    if (NULL == pstCommonNode) {
        nap_FreeIndex(pstCommonHead, uiIndex);
        return NULL;
    }

    pstCommonNode->ulID = nap_CacleIDByIndex(pstCommonHead, uiIndex);

    pstCommonHead->uiCount ++;

    return pstCommonNode + 1;
}

VOID * NAP_ZAlloc(IN NAP_HANDLE hNapHandle)
{
    VOID *pMem;
    _NAP_HEAD_COMMON_S *pstCommonHead = hNapHandle;

    pMem = NAP_Alloc(hNapHandle);
    if (NULL != pMem)
    {
        Mem_Zero(pMem, pstCommonHead->uiNodeSize);
    }

    return pMem;
}


VOID * NAP_AllocByIndex(NAP_HANDLE hNapHandle, UINT uiSpecIndex)
{
    _NAP_HEAD_COMMON_S *pstCommonHead = hNapHandle;
    _NAP_NODE_COMMON_S *pstCommonNode;
    UINT uiIndex;

    if (uiSpecIndex == NAP_INVALID_ID) {
        return NAP_Alloc(hNapHandle);
    }

    if (NULL == hNapHandle) {
        return NULL;
    }

    uiIndex = nap_AllocSpecIndex(pstCommonHead, uiSpecIndex);
    if (NAP_INVALID_INDEX == uiIndex) {
        return NULL;
    }
    
    pstCommonNode = pstCommonHead->pstFuncTbl->pfAlloc(hNapHandle, uiIndex);
    if (NULL == pstCommonNode) {
        nap_FreeIndex(pstCommonHead, uiIndex);
        return NULL;
    }

    pstCommonNode->ulID = nap_CacleIDByIndex(hNapHandle, uiIndex);
    pstCommonHead->uiCount ++;

    return pstCommonNode + 1;
}

VOID * NAP_ZAllocByIndex(IN NAP_HANDLE hNapHandle, IN UINT uiSpecIndex)
{
    VOID *pMem;
    _NAP_HEAD_COMMON_S *pstCommonHead = hNapHandle;

    pMem = NAP_AllocByIndex(hNapHandle, uiSpecIndex);
    if (NULL != pMem) {
        Mem_Zero(pMem, pstCommonHead->uiNodeSize);
    }

    return pMem;
}


VOID * NAP_AllocByID(IN NAP_HANDLE hNapHandle, IN UINT ulSpecID)
{
    _NAP_HEAD_COMMON_S *pstCommonHead = hNapHandle;
    _NAP_NODE_COMMON_S *pstCommonNode;
    UINT uiIndex;
    UINT uiSpecIndex;

    if (ulSpecID == NAP_INVALID_ID) {
        return NAP_Alloc(hNapHandle);
    }

    if (NULL == hNapHandle) {
        return NULL;
    }

    uiSpecIndex = nap_CacleIndexByID(pstCommonHead, ulSpecID);

    uiIndex = nap_AllocSpecIndex(pstCommonHead, uiSpecIndex);
    if (NAP_INVALID_INDEX == uiIndex) {
        return NULL;
    }
    
    pstCommonNode = pstCommonHead->pstFuncTbl->pfAlloc(hNapHandle, uiIndex);
    if (NULL == pstCommonNode) {
        nap_FreeIndex(pstCommonHead, uiIndex);
        return NULL;
    }

    pstCommonNode->ulID = ulSpecID;

    return pstCommonNode + 1;
}

VOID * NAP_ZAllocByID(IN NAP_HANDLE hNapHandle, IN UINT ulSpecID)
{
    VOID *pMem;
    _NAP_HEAD_COMMON_S *pstCommonHead = hNapHandle;

    pMem = NAP_AllocByID(hNapHandle, ulSpecID);
    if (NULL != pMem) {
        Mem_Zero(pMem, pstCommonHead->uiNodeSize);
    }

    return pMem;
}


VOID NAP_Free(IN NAP_HANDLE hNapHandle, IN VOID *pstNode)
{
    _NAP_HEAD_COMMON_S *pstCommonHead = hNapHandle;
    _NAP_NODE_COMMON_S *pstCommonNode;
    UINT uiIndex;

    if ((NULL == hNapHandle) || (NULL == pstNode)) {
        return;
    }

    pstCommonNode = (VOID*)((UCHAR*)pstNode - sizeof(_NAP_NODE_COMMON_S));

    uiIndex = nap_CacleIndexByID(pstCommonHead, pstCommonNode->ulID);

    nap_FreeIndex(pstCommonHead, uiIndex);
    
    pstCommonHead->pstFuncTbl->pfFree(hNapHandle, pstCommonNode, uiIndex);

    pstCommonHead->uiCount --;
}

VOID NAP_FreeByID(IN NAP_HANDLE hNapHandle, IN UINT ulID)
{
    NAP_Free(hNapHandle, NAP_GetNodeByID(hNapHandle, ulID));
}

VOID NAP_FreeByIndex(IN NAP_HANDLE hNapHandle, IN UINT index)
{
    NAP_Free(hNapHandle, NAP_GetNodeByIndex(hNapHandle, index));
}

VOID NAP_FreeAll(IN NAP_HANDLE hNapHandle)
{
    UINT uiIndex = NAP_INVALID_INDEX;

    while ((uiIndex = NAP_GetNextIndex(hNapHandle, uiIndex))
            != NAP_INVALID_INDEX)
    {
        NAP_Free(hNapHandle, NAP_GetNodeByIndex(hNapHandle, uiIndex));
    }

    return;
}

VOID * NAP_GetNodeByID(IN NAP_HANDLE hNAPHandle, IN UINT ulID)
{
    _NAP_HEAD_COMMON_S *pstCommonHead = hNAPHandle;
    _NAP_NODE_COMMON_S *pstCommonNode;
    UINT uiIndex;

    if ((hNAPHandle == NULL) || (ulID == NAP_INVALID_ID)) {
        return NULL;
    }

    uiIndex = nap_CacleIndexByID(pstCommonHead, ulID);

    if (! nap_IsIndexSetted(pstCommonHead, uiIndex)) {
        return NULL;
    }

    pstCommonNode = pstCommonHead->pstFuncTbl->pfGetNodeByIndex(hNAPHandle, uiIndex);
    if (NULL == pstCommonNode) {
        return NULL;
    }

    if (pstCommonNode->ulID != ulID) {
        return NULL;
    }

    return pstCommonNode + 1;
}

VOID * NAP_GetNodeByIndex(IN NAP_HANDLE hNAPHandle, IN UINT uiIndex)
{
    _NAP_HEAD_COMMON_S *pstCommonHead = hNAPHandle;
    _NAP_NODE_COMMON_S *pstCommonNode;

    if (! nap_IsIndexSetted(pstCommonHead, uiIndex)) {
        return NULL;
    }

    pstCommonNode = pstCommonHead->pstFuncTbl->pfGetNodeByIndex(hNAPHandle, uiIndex);
    if (NULL == pstCommonNode) {
        return NULL;
    }

    return pstCommonNode + 1;
}

UINT NAP_GetIDByNode(IN NAP_HANDLE hNAPHandle, IN VOID *pstNode)
{
    _NAP_NODE_COMMON_S *pstCommonNode;

    if (NULL == pstNode) {
        return NAP_INVALID_ID;
    }

    pstCommonNode = (VOID*)((UCHAR*)pstNode - sizeof(_NAP_NODE_COMMON_S));

    return pstCommonNode->ulID;
}

UINT NAP_GetIDByIndex(IN NAP_HANDLE hNAPHandle, UINT index)
{
    return NAP_GetIDByNode(hNAPHandle, NAP_GetNodeByIndex(hNAPHandle, index));
}

UINT NAP_GetIndexByID(IN NAP_HANDLE hNAPHandle, UINT id)
{
    _NAP_HEAD_COMMON_S *pstCommonHead = hNAPHandle;
    return nap_CacleIndexByID(pstCommonHead, id);
}

UINT NAP_GetIndexByNode(IN NAP_HANDLE hNAPHandle, void *node)
{
    UINT id;

    id = NAP_GetIDByNode(hNAPHandle, node);
    if (id == NAP_INVALID_ID) {
        return NAP_INVALID_INDEX;
    }

    return NAP_GetIndexByID(hNAPHandle, id);
}


BS_STATUS NAP_ChangeNodeID(IN NAP_HANDLE hNAPHandle, IN VOID *pstNode, IN UINT uiNewNodeID)
{
    _NAP_HEAD_COMMON_S *pstCommonHead = hNAPHandle;
    _NAP_NODE_COMMON_S *pstCommonNode;

    UINT uiOldNodeID = NAP_GetIDByNode(hNAPHandle, pstNode);

    if (uiOldNodeID == NAP_INVALID_ID) {
        return BS_NO_SUCH;
    }

    if (nap_CacleIndexByID(pstCommonHead, uiOldNodeID)
            != nap_CacleIndexByID(pstCommonHead, uiNewNodeID)) {
        BS_DBGASSERT(0);
        return BS_NOT_SUPPORT;
    }

    pstCommonNode = (VOID*)((UCHAR*)pstNode - sizeof(_NAP_NODE_COMMON_S));

    pstCommonNode->ulID = uiNewNodeID;

    return BS_OK;
}


BS_STATUS NAP_EnableSeq(HANDLE hNAPHandle, UINT ulSeqMask, UINT uiSeqCount)
{
    _NAP_HEAD_COMMON_S *pstCommonHead = hNAPHandle;

    
    if (ulSeqMask == 0) {
        ulSeqMask = ~pstCommonHead->ulIndexMask;
        ulSeqMask &= 0xffff0000;
    }

    if (ulSeqMask == 0) {
        return BS_ERR;
    }

    if (uiSeqCount == 0) {
        return BS_BAD_PARA;
    }

    if (BS_OK != nap_SetSeqMask(hNAPHandle, ulSeqMask)) {
        return BS_ERR;
    }

    if (pstCommonHead->stSeqOpt.seq_array == NULL) {
        pstCommonHead->stSeqOpt.seq_array =
            MemCap_Malloc(pstCommonHead->memcap, sizeof(USHORT) * uiSeqCount);
        if (NULL == pstCommonHead->stSeqOpt.seq_array) {
            return BS_NO_MEMORY;
        }
        pstCommonHead->stSeqOpt.uiSeqArrayCount = uiSeqCount;
    }

    pstCommonHead->stSeqOpt.bEnable = TRUE;

    return BS_OK;
}


UINT NAP_GetNextIndex(IN NAP_HANDLE hNAPHandle, IN UINT uiCurrentIndex)
{
    _NAP_HEAD_COMMON_S *pstCommonHead = hNAPHandle;
    UINT uiIndex = NAP_INVALID_INDEX;

    if (uiCurrentIndex == NAP_INVALID_INDEX) {
        LBitMap_GetFirstBusyBit(pstCommonHead->hLBitmap, &uiIndex);
    } else {
        LBitMap_GetNextBusyBit(pstCommonHead->hLBitmap, uiCurrentIndex, &uiIndex);
    }

    return uiIndex;
}

UINT NAP_GetNextID(IN NAP_HANDLE hNAPHandle, IN UINT ulCurrentID)
{
    _NAP_HEAD_COMMON_S *pstCommonHead = hNAPHandle;
    _NAP_NODE_COMMON_S *pstCommonNode;
    UINT uiIndex;
    UINT uiCurrentIndex;

    if (ulCurrentID == NAP_INVALID_ID) {
        uiCurrentIndex = NAP_INVALID_INDEX;
    } else {
        uiCurrentIndex = nap_CacleIndexByID(pstCommonHead, ulCurrentID);
    }

    uiIndex = NAP_GetNextIndex(hNAPHandle, uiCurrentIndex);

    if (NAP_INVALID_INDEX == uiIndex) {
        return NAP_INVALID_ID;
    }

    pstCommonNode = pstCommonHead->pstFuncTbl->pfGetNodeByIndex(hNAPHandle, uiIndex);
    if (NULL == pstCommonNode) {
        BS_DBGASSERT(0);
        return NAP_INVALID_ID;
    }
    
    return pstCommonNode->ulID;
}

UINT NAP_GetCount(IN NAP_HANDLE hNAPHandle)
{
    _NAP_HEAD_COMMON_S *pstCommonHead = hNAPHandle;

    return pstCommonHead->uiCount;
}

