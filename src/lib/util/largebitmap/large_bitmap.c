/******************************************************************************
* Copyright (C),    LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2014-8-15
* Description: 大位图管理表.  可分配0-4G的位区间.  使用4级的分级管理. bitIndex从0开始
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/large_bitmap.h"

#define LBITMAP_LEVEL_1 1
#define LBITMAP_LEVEL_2 2
#define LBITMAP_LEVEL_3 3
#define LBITMAP_LEVEL_LEAF 4

#define LBITMAP_NODE_NUM 256
#define LBITMAP_NODE_BITUINT_NUM 8

typedef struct tagLBitMap_LEAF_S
{
    UINT auiBits[LBITMAP_NODE_BITUINT_NUM];
}LBITMAP_BITS_S;

typedef struct
{
    UCHAR ucLevel;
    LBITMAP_BITS_S stBits; /* 对于叶子节点,表示对应位. 对于非叶子节点,表示对应的下一级是否已经full */
}LBITMAP_COMMON_S;

/* 1-3级使用的结构 */
typedef struct
{
    LBITMAP_COMMON_S stCommon;
    LBITMAP_COMMON_S *apstNextLevel[LBITMAP_NODE_NUM]; /* 指向下一级 */
}LBITMAP_LEVEL_S;

/* 第4级是叶子节点, 使用这个结构 */
typedef struct
{
    LBITMAP_COMMON_S stCommon;
}LBITMAP_LEAF_S;

static BS_STATUS lbitmap_Get
(
    IN LBITMAP_COMMON_S *pstCommon,
    IN UINT uiRangeMin,
    IN UINT uiRangeMax,
    OUT UINT *puiBitIndex 
);

static BOOL_T lbitmap_IsBitSetted(IN const LBITMAP_BITS_S *pstBits, IN UCHAR ucBitIndex)
{
    UCHAR ucIndex;
    UCHAR ucPos;

    ucIndex = ((ucBitIndex >> 5) & 0x7);
    ucPos = (ucBitIndex & 0x1F);

    if ((pstBits->auiBits[ucIndex] & (1UL << ucPos)) != 0)
    {
        return TRUE;
    }

    return FALSE;
}

static BOOL_T lbitmap_IsAllSetted(IN const LBITMAP_BITS_S *pstBits)
{
    UINT uiPos;
    BOOL_T bRet = TRUE;

    for (uiPos = 0; uiPos < LBITMAP_NODE_BITUINT_NUM; uiPos++)
    {
        if (pstBits->auiBits[uiPos] != 0xffffffff)
        {
            bRet = FALSE;
            break;
        }
    }

    return bRet;
}

static VOID lbitmap_SetBit(IN LBITMAP_BITS_S *pstBits, IN UCHAR ucBitIndex)
{
    UCHAR ucIndex;
    UCHAR ucPos;

    ucIndex = ((ucBitIndex >> 5) & 0x7);
    ucPos = (ucBitIndex & 0x1F);

    pstBits->auiBits[ucIndex] |= (UINT)(1UL << ucPos);

    return;
}

static VOID lbitmap_ClrBit(IN LBITMAP_BITS_S *pstBits, IN UCHAR ucBitIndex)
{
    UCHAR ucIndex;
    UCHAR ucPos;

    ucIndex = ((ucBitIndex >> 5) & 0x7);
    ucPos = (ucBitIndex & 0x1F);

    pstBits->auiBits[ucIndex] &= ~(1UL << ucPos);

    return;
}

static VOID lbitmap_GetLevelDesc(IN UCHAR ucLevel, OUT UINT *puiOffset, OUT UINT *puiMask)
{
    switch (ucLevel)
    {
        case LBITMAP_LEVEL_1:
        {
            *puiOffset = 24;
            *puiMask = 0xffffff;
            break;
        }

        case LBITMAP_LEVEL_2:
        {
            *puiOffset = 16;
            *puiMask = 0xffff;
            break;
        }

        case LBITMAP_LEVEL_3:
        {
            *puiOffset = 8;
            *puiMask = 0xff;
            break;
        }

        case LBITMAP_LEVEL_LEAF:
        {
            *puiOffset = 0;
            *puiMask = 0x0;
            break;
        }

        default:
        {
            BS_DBGASSERT(0);
            break;
        }
    }
}

static BS_STATUS lbitmap_GetInLeaf
(
    IN LBITMAP_LEAF_S *pstLeaf,
    IN UINT uiRangeMin,
    IN UINT uiRangeMax,
    OUT UINT *puiBitIndex 
)
{
    UINT uiUintPos;
    UINT uiBitPos;
    UCHAR ucStartUint;
    UCHAR ucStopUint;
    UCHAR ucStartBit;
    UCHAR ucStopBit;

    ucStartUint = ((uiRangeMin >> 5) & 0x7);
    ucStopUint = ((uiRangeMax >> 5) & 0x7);

    for (uiUintPos=ucStartUint; uiUintPos<=ucStopUint; uiUintPos++)
    {
        if (pstLeaf->stCommon.stBits.auiBits[uiUintPos] == 0xffffffff)
        {
            continue;
        }

        ucStartBit = 0;
        if (uiUintPos == ucStartUint)
        {
            ucStartBit = uiRangeMin & 0x1F;
        }
        ucStopBit = 31;
        if (uiUintPos == ucStopUint)
        {
            ucStopBit = uiRangeMax & 0x1F;
        }

        for (uiBitPos=ucStartBit; uiBitPos<=ucStopBit; uiBitPos++)
        {
            if (0 == (pstLeaf->stCommon.stBits.auiBits[uiUintPos] & (1 << uiBitPos)))
            {
                pstLeaf->stCommon.stBits.auiBits[uiUintPos] |= (1 << uiBitPos);
                *puiBitIndex = uiUintPos * 32 + uiBitPos;
                return BS_OK;
            }
        }
    }

    return BS_NOT_FOUND;
}

static BS_STATUS lbitmap_GetBusyInLeaf
(
    IN LBITMAP_LEAF_S *pstLeaf,
    IN UINT uiRangeMin,
    IN UINT uiRangeMax,
    OUT UINT *puiBitIndex 
)
{
    UINT uiUintPos;
    UINT uiBitPos;
    UCHAR ucStartUint;
    UCHAR ucStopUint;
    UCHAR ucStartBit;
    UCHAR ucStopBit;

    ucStartUint = ((uiRangeMin >> 5) & 0x7);
    ucStopUint = ((uiRangeMax >> 5) & 0x7);

    for (uiUintPos=ucStartUint; uiUintPos<=ucStopUint; uiUintPos++)
    {
        if (pstLeaf->stCommon.stBits.auiBits[uiUintPos] == 0)
        {
            continue;
        }

        ucStartBit = 0;
        if (uiUintPos == ucStartUint)
        {
            ucStartBit = uiRangeMin & 0x1F;
        }
        ucStopBit = 31;
        if (uiUintPos == ucStopUint)
        {
            ucStopBit = uiRangeMax & 0x1F;
        }

        for (uiBitPos=ucStartBit; uiBitPos<=ucStopBit; uiBitPos++)
        {
            if (pstLeaf->stCommon.stBits.auiBits[uiUintPos] & (1 << uiBitPos))
            {
                *puiBitIndex = uiUintPos * 32 + uiBitPos;
                return BS_OK;
            }
        }
    }

    return BS_NOT_FOUND;
}

static VOID * lbitmap_AllocNode(IN UCHAR ucLevel)
{
    LBITMAP_COMMON_S *pstCommon;

    if (ucLevel == LBITMAP_LEVEL_LEAF)
    {
        pstCommon = MEM_ZMalloc(sizeof(LBITMAP_LEAF_S));
    }
    else
    {
        pstCommon = MEM_ZMalloc(sizeof(LBITMAP_LEVEL_S));
    }

    if (NULL != pstCommon)
    {
        pstCommon->ucLevel = ucLevel;
    }

    return pstCommon;
}

static BS_STATUS lbitmap_GetSpec
(
    IN LBITMAP_LEVEL_S *pstLevel,
    IN UINT uiPos,
    IN UINT uiRangeMin,
    IN UINT uiRangeMax,
    OUT UINT *puiBitIndex 
)
{
    UINT uiOffset = 0;
    UINT uiMask;
    UINT uiBitIndex = 0;
    BS_STATUS eRet;
    
    if (pstLevel->apstNextLevel[uiPos] == NULL)
    {
        pstLevel->apstNextLevel[uiPos] = lbitmap_AllocNode(pstLevel->stCommon.ucLevel + 1);
        if (pstLevel->apstNextLevel[uiPos] == NULL)
        {
            return BS_NO_RESOURCE;
        }
    }

    lbitmap_GetLevelDesc(pstLevel->stCommon.ucLevel, &uiOffset, &uiMask);

    if (TRUE == lbitmap_IsBitSetted(&pstLevel->stCommon.stBits, (UCHAR)uiPos))
    {
        return BS_NOT_FOUND;
    }

    eRet= lbitmap_Get(pstLevel->apstNextLevel[uiPos], uiRangeMin, uiRangeMax, &uiBitIndex);
    if (BS_OK != eRet)
    {
        return eRet;
    }

    uiBitIndex |= (uiPos << uiOffset);
    if (TRUE == lbitmap_IsAllSetted(&pstLevel->apstNextLevel[uiPos]->stBits))
    {
        lbitmap_SetBit(&pstLevel->stCommon.stBits, (UCHAR)uiPos);
    }

    *puiBitIndex = uiBitIndex;

    return BS_OK;
}

static BS_STATUS lbitmap_GetInLevel
(
    IN LBITMAP_LEVEL_S *pstLevel,
    IN UINT uiRangeMin,
    IN UINT uiRangeMax,
    OUT UINT *puiBitIndex 
)
{
    UINT uiIndexStart;
    UINT uiIndexStop;
    UINT uiPos;
    UINT uiNextRangMin;
    UINT uiNextRangMax;
    BS_STATUS eRet = BS_ERR;
    UINT uiOffset = 0;
    UINT uiMask = 0;

    lbitmap_GetLevelDesc(pstLevel->stCommon.ucLevel, &uiOffset, &uiMask);

    uiIndexStart = (uiRangeMin >> uiOffset) & 0xff;
    uiIndexStop = (uiRangeMax >> uiOffset) & 0xff;

    for (uiPos=uiIndexStart; uiPos<=uiIndexStop; uiPos++)
    {
        uiNextRangMin = 0;
        if (uiPos == uiIndexStart)
        {
            uiNextRangMin = uiRangeMin & uiMask;
        }
        uiNextRangMax = uiMask;
        if (uiPos == uiIndexStop)
        {
            uiNextRangMax = uiRangeMax & uiMask;
        }

        if (BS_OK == lbitmap_GetSpec(pstLevel, uiPos, uiNextRangMin, uiNextRangMax, puiBitIndex))
        {
            eRet = BS_OK;
            break;
        }
    }

    return eRet;
}

static BS_STATUS lbitmap_Get
(
    IN LBITMAP_COMMON_S *pstCommon,
    IN UINT uiRangeMin,
    IN UINT uiRangeMax,
    OUT UINT *puiBitIndex 
)
{
    if (pstCommon->ucLevel == LBITMAP_LEVEL_LEAF)
    {
        return lbitmap_GetInLeaf((LBITMAP_LEAF_S*)pstCommon, uiRangeMin, uiRangeMax, puiBitIndex); 
    }
    else
    {
        return lbitmap_GetInLevel((LBITMAP_LEVEL_S*)pstCommon, uiRangeMin, uiRangeMax, puiBitIndex);
    }
}

/* 获取对应Index的Leaf */
static LBITMAP_LEAF_S * lbitmap_GetIndexLeaf(IN LBITMAP_COMMON_S *pstCommon, IN UINT uiIndex)
{
    UINT uiOffset = 0;
    UINT uiMask = 0;
    UINT uiIndexTmp;
    LBITMAP_LEVEL_S *pstLevel;

    if (pstCommon->ucLevel == LBITMAP_LEVEL_LEAF)
    {
        return (LBITMAP_LEAF_S*)pstCommon;
    }

    pstLevel = (LBITMAP_LEVEL_S*)pstCommon;

    lbitmap_GetLevelDesc(pstCommon->ucLevel, &uiOffset, &uiMask);
    uiIndexTmp = (uiIndex >> uiOffset) & 0xff;

    if (pstLevel->apstNextLevel[uiIndexTmp] == NULL)
    {
        return NULL;
    }

    return lbitmap_GetIndexLeaf(pstLevel->apstNextLevel[uiIndexTmp], uiIndex & uiMask);
}

/* 清除对应的位 */
static VOID lbitmap_ClearBit(IN LBITMAP_COMMON_S *pstCommon, IN UINT uiIndex)
{
    UINT uiOffset = 0;
    UINT uiMask = 0;
    UINT uiIndexTmp;
    LBITMAP_LEVEL_S *pstLevel;

    if (pstCommon->ucLevel == LBITMAP_LEVEL_LEAF)
    {
        lbitmap_ClrBit(&pstCommon->stBits, (UCHAR)(uiIndex & 0xff));
        return;
    }

    pstLevel = (LBITMAP_LEVEL_S*)pstCommon;

    lbitmap_GetLevelDesc(pstCommon->ucLevel, &uiOffset, &uiMask);

    uiIndexTmp = (uiIndex >> uiOffset) & 0xff;

    lbitmap_ClrBit(&pstCommon->stBits, uiIndexTmp);

    if (pstLevel->apstNextLevel[uiIndexTmp] == NULL)
    {
        return;
    }

    lbitmap_ClearBit(pstLevel->apstNextLevel[uiIndexTmp], uiIndex & uiMask);

    return;
}

/* 设置对应Index的Bit */
BS_STATUS lbitmap_SetIndexBit(IN LBITMAP_COMMON_S *pstCommon, IN UINT uiIndex)
{
    UINT uiOffset = 0;
    UINT uiMask = 0;
    UINT uiIndexTmp;
    LBITMAP_LEVEL_S *pstLevel;

    if (pstCommon->ucLevel == LBITMAP_LEVEL_LEAF)
    {
        lbitmap_SetBit(&pstCommon->stBits, (UCHAR)(uiIndex & 0xff));
        return BS_OK;
    }
    else
    {
        pstLevel = (LBITMAP_LEVEL_S*)pstCommon;

        lbitmap_GetLevelDesc(pstCommon->ucLevel, &uiOffset, &uiMask);
        uiIndexTmp = (uiIndex >> uiOffset) & 0xff;

        if (pstLevel->apstNextLevel[uiIndexTmp] == NULL)
        {
            pstLevel->apstNextLevel[uiIndexTmp] = lbitmap_AllocNode(pstLevel->stCommon.ucLevel + 1);
            if (pstLevel->apstNextLevel[uiIndexTmp] == NULL)
            {
                return BS_NO_MEMORY;
            }
        }

        return lbitmap_SetIndexBit(pstLevel->apstNextLevel[uiIndexTmp], uiIndex & uiMask);
    }
}

static VOID lbitmap_Destory(IN LBITMAP_COMMON_S *pstCommon)
{
    UINT uiPos;
    LBITMAP_LEVEL_S *pstLevel;

    if (NULL != pstCommon)
    {
        if (pstCommon->ucLevel != LBITMAP_LEVEL_LEAF)
        {
            pstLevel = (LBITMAP_LEVEL_S*)pstCommon;

            for (uiPos = 0; uiPos < LBITMAP_NODE_NUM; uiPos++)
            {
                lbitmap_Destory(pstLevel->apstNextLevel[uiPos]);
                pstLevel->apstNextLevel[uiPos] = NULL;
            }
        }

        MEM_Free(pstCommon);
    }

    return;
}

LBITMAP_HANDLE LBitMap_Create(VOID)
{
    LBITMAP_LEVEL_S *pstCtrl;

    pstCtrl = MEM_ZMalloc(sizeof(LBITMAP_LEVEL_S));
    if (NULL == pstCtrl)
    {
        return NULL;
    }

    pstCtrl->stCommon.ucLevel = LBITMAP_LEVEL_1;

    return pstCtrl;
}

VOID LBitMap_Destory(IN LBITMAP_HANDLE hLBitMap)
{
    lbitmap_Destory(hLBitMap);

    return;
}

/* 从指定区间找到一个空闲位, 但不设置上它 */
BS_STATUS LBitMap_TryByRange
(
    IN LBITMAP_HANDLE hLBitMap,
    IN UINT uiRangeMin,
    IN UINT uiRangeMax,
    OUT UINT *puiBitIndex
)
{
    UINT uiIndex = 0;

    if (BS_OK != lbitmap_Get(hLBitMap, uiRangeMin, uiRangeMax, &uiIndex))
    {
        return BS_NOT_FOUND;
    }

    LBitMap_ClrBit(hLBitMap, uiIndex);

    return uiIndex;
}


/* 从指定区间分配一个位 */
BS_STATUS LBitMap_AllocByRange
(
    IN LBITMAP_HANDLE hLBitMap,
    IN UINT uiRangeMin,
    IN UINT uiRangeMax,
    OUT UINT *puiBitIndex
)
{
    return lbitmap_Get(hLBitMap, uiRangeMin, uiRangeMax, puiBitIndex);
}

BS_STATUS LBitMap_SetBit(IN LBITMAP_HANDLE hLBitMap, IN UINT uiBitIndex)
{
    return lbitmap_SetIndexBit(hLBitMap, uiBitIndex);
}

VOID LBitMap_ClrBit(IN LBITMAP_HANDLE hLBitMap, IN UINT uiBitIndex)
{
    lbitmap_ClearBit(hLBitMap, uiBitIndex);
}

BOOL_T LBitMap_IsBitSetted(IN LBITMAP_HANDLE hLBitMap, IN UINT uiBitIndex)
{
    LBITMAP_LEAF_S *pstLeaf;

    pstLeaf = lbitmap_GetIndexLeaf(hLBitMap, uiBitIndex);
    if (NULL == pstLeaf) {
        return FALSE;
    }

    return lbitmap_IsBitSetted(&pstLeaf->stCommon.stBits, (UCHAR)(uiBitIndex & 0xff));
}

static BS_STATUS lbitmap_GetBusyBit
(
    IN LBITMAP_COMMON_S *pstCommon,
    IN UINT uiRangeMin,
    IN UINT uiRangeMax,
    UINT *puiIndex
)
{
    UINT uiOffset = 0;
    UINT uiMask = 0;
    LBITMAP_LEVEL_S *pstLevel;
    BS_STATUS eRet = BS_NOT_FOUND;
    UCHAR ucStartIndex;
    UCHAR ucStopIndex;
    UINT uiIndex;
    UINT uiRangeMinChild;
    UINT uiRangeMaxChild;
    

    if (pstCommon->ucLevel == LBITMAP_LEVEL_LEAF)
    {
        return lbitmap_GetBusyInLeaf((LBITMAP_LEAF_S*)pstCommon, uiRangeMin, uiRangeMax, puiIndex);
    }

    pstLevel = (LBITMAP_LEVEL_S*)pstCommon;

    lbitmap_GetLevelDesc(pstCommon->ucLevel, &uiOffset, &uiMask);

    ucStartIndex = (uiRangeMin >> uiOffset);
    ucStopIndex = (uiRangeMax >> uiOffset);

    for (uiIndex = ucStartIndex; uiIndex <= ucStopIndex; uiIndex++)
    {
        if (pstLevel->apstNextLevel[uiIndex] == NULL)
        {
            continue;
        }

        uiRangeMinChild = 0;
        if (uiIndex == ucStartIndex)
        {
            uiRangeMinChild = uiRangeMin & uiMask;
        }

        uiRangeMaxChild = uiMask;
        if (uiIndex == ucStopIndex)
        {
            uiRangeMinChild = uiRangeMax & uiMask;
        }

        if (BS_OK != lbitmap_GetBusyBit(pstLevel->apstNextLevel[uiIndex],
            uiRangeMinChild, uiRangeMaxChild, puiIndex))
        {
            continue;
        }

        *puiIndex |= (uiIndex << uiOffset);
        eRet = BS_OK;
        break;
    }

    return eRet;
}

BS_STATUS LBitMap_GetFirstBusyBit(IN LBITMAP_HANDLE hLBitMap, OUT UINT *puiBitIndex)
{
    return lbitmap_GetBusyBit(hLBitMap, 0, 0xffffffff, puiBitIndex);
}

BS_STATUS LBitMap_GetNextBusyBit(IN LBITMAP_HANDLE hLBitMap, IN UINT uiCurrentBitIndex, OUT UINT *puiBitIndex)
{
    return lbitmap_GetBusyBit(hLBitMap, uiCurrentBitIndex + 1, 0xffffffff, puiBitIndex);
}


