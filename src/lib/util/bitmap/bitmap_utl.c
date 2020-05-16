/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2007-7-2
* Description: 
* History:     
******************************************************************************/
#include "bs.h"
#include "utl/num_utl.h"
#include "utl/bitmap_utl.h"

/* 初始化方法之一 */
void BITMAP_Init(BITMAP_S *pstBitMap, UINT bitnum, UINT *bitmap_mem/* 必须是由UINT构成的数组 */)
{
    memset(pstBitMap, 0, sizeof(BITMAP_S));

    pstBitMap->pulBitMap = bitmap_mem;
    pstBitMap->ulBitNum = bitnum;
    pstBitMap->uiUintNum = BITMAP_NEED_UINTS(bitnum);

    memset(bitmap_mem, 0, pstBitMap->uiUintNum * 4);
}

void BITMAP_Fini(BITMAP_S *pstBitMap)
{
    memset(pstBitMap, 0, sizeof(BITMAP_S));
}

/* 初始化方法之二 */
BS_STATUS BITMAP_Create(IN BITMAP_S * pstBitMap, IN UINT ulBitNum)
{
    if (pstBitMap == NULL)
    {
        RETURN(BS_NULL_PARA);
    }

    memset(pstBitMap, 0, sizeof(BITMAP_S));

    pstBitMap->pulBitMap = MEM_ZMalloc(BITMAP_NEED_BYTES(ulBitNum));
    if (pstBitMap->pulBitMap == NULL)
    {
        RETURN(BS_NO_MEMORY);
    }
    
    pstBitMap->ulBitNum = ulBitNum;
    pstBitMap->uiUintNum = BITMAP_NEED_UINTS(ulBitNum);

    return BS_OK;
}

/* 销毁方法之二 */
BS_STATUS BITMAP_Destory(IN BITMAP_S * pstBitMap)
{
    if (NULL != pstBitMap->pulBitMap)
    {
        MEM_Free(pstBitMap->pulBitMap);
    }
    pstBitMap->pulBitMap = NULL;
    pstBitMap->ulBitNum = 0;
    pstBitMap->uiUintNum = 0;

    return BS_OK;
}

/* 获取一个setted位的Index */
UINT BITMAP_GetASettedBitIndex(IN BITMAP_S *pstBitMap)
{
    UINT uiIndex;

    BITMAP_SCAN_SETTED_BEGIN(pstBitMap, uiIndex)
    {
        return uiIndex;
    }BITMAP_SCAN_END();

    return BITMAP_INVALID_INDEX;
}

/* 获取某个指定位置之后的setted位的Index */
UINT BITMAP_GetASettedBitIndexAfter(IN BITMAP_S *pstBitMap, IN UINT uiIndex)
{
    UINT _i, _j;
    UINT uiIndexFound = BITMAP_INVALID_INDEX;
    UINT uiStartUint;
    UINT uiStartBit;

    uiIndex ++;
    if (uiIndex > pstBitMap->ulBitNum) {
        uiIndex = 0;
    }

    uiStartUint = uiIndex / 32;
    uiStartBit = uiIndex % 32;

    for (_i=uiStartUint; _i<pstBitMap->uiUintNum; _i++)
    {
        if (pstBitMap->pulBitMap[_i] != 0)
        {
            for (_j=uiStartBit; _j<32; _j++)
            {
                if (pstBitMap->pulBitMap[_i] & (1 << _j))
                {
                    uiIndexFound = _i*32 + _j;
                    break;
                }
            }
            if (uiIndexFound != BITMAP_INVALID_INDEX)
            {
                break;
            }
        }

        uiStartBit = 0;
    }

    return uiIndexFound;
}

/* 获取一个unsetted位的Index */
UINT BITMAP_GetAUnsettedBitIndex(IN BITMAP_S *pstBitMap)
{
    UINT uiIndex;

    BITMAP_SCAN_UNSETTED_BEGIN(pstBitMap, uiIndex) {
        return uiIndex;
    }BITMAP_SCAN_END();

    return BITMAP_INVALID_INDEX;
}


/* 环绕形式的获取一个unsetted位的Index */
UINT BITMAP_GetAUnsettedBitIndexCycle(IN BITMAP_S *pstBitMap)
{
    UINT _i, _j;
    UINT uiIndex = BITMAP_INVALID_INDEX;
    UINT uiStartUint;
    UINT uiStartBit;

    uiStartUint = pstBitMap->uiScanIndex / 32;
    uiStartBit = pstBitMap->uiScanIndex % 32;

    for (_i=uiStartUint; _i<pstBitMap->uiUintNum; _i++)
    {
        if (pstBitMap->pulBitMap[_i] != 0xffffffff)
        {
            for (_j=uiStartBit; _j<32; _j++)
            {
                if (pstBitMap->pulBitMap[_i] & (1 << _j))
                {
                    continue;
                }
                uiIndex = _i*32 + _j;

                break;
            }
            if (uiIndex != BITMAP_INVALID_INDEX)
            {
                break;
            }
        }

        uiStartBit = 0;
    }

    if (uiIndex >= pstBitMap->ulBitNum)
    {
        uiIndex = BITMAP_INVALID_INDEX;
    }

    if (uiIndex == BITMAP_INVALID_INDEX) /* 扫描到结尾了,还没找到,则从头开始找 */ \
    {
        for (_i=0; _i<=uiStartUint; _i++)
        {
            if (pstBitMap->pulBitMap[_i] == 0xffffffff)
            {
                continue;
            }

            for (_j = 0; _j<32; _j++)
            {
                if ((pstBitMap)->pulBitMap[_i] & (1 << _j))
                {
                    continue;
                }
                uiIndex = _i*32 + _j;
                break;
            }
            break;
        }

        if (uiIndex >= pstBitMap->ulBitNum)
        {
            uiIndex = BITMAP_INVALID_INDEX;
        }
    }

    if (uiIndex != BITMAP_INVALID_INDEX)
    {
        pstBitMap->uiScanIndex = uiIndex + 1;
        if (pstBitMap->uiScanIndex >= pstBitMap->ulBitNum) {
            pstBitMap->uiScanIndex = 0;
        }
    }

    return uiIndex;
}

/* 从两个bitmap中获取都被设置的位的Index */
UINT BITMAP_2GetSettedIndex(IN BITMAP_S * pstBitMap1, IN BITMAP_S * pstBitMap2)
{
    UINT uiIndex;
    
    BITMAP_SCAN_SETTED_BEGIN(pstBitMap1, uiIndex) {
        if (! BITMAP_ISSET(pstBitMap2, uiIndex)) {
            return uiIndex;
        }
    }BITMAP_SCAN_END();

    return BITMAP_INVALID_INDEX;
}

/* 从两个bitmap中获取都未被设置的位的Index */
UINT BITMAP_2GetUnsettedIndex(IN BITMAP_S * pstBitMap1, IN BITMAP_S * pstBitMap2)
{
    UINT uiIndex;
    
    BITMAP_SCAN_UNSETTED_BEGIN(pstBitMap1, uiIndex)
    {
        if (! BITMAP_ISSET(pstBitMap2, uiIndex))
        {
            return uiIndex;
        }
    }BITMAP_SCAN_END();

    return BITMAP_INVALID_INDEX;
}

VOID BITMAP_Zero(IN BITMAP_S * pstBitMap)
{
    UINT i;

    for (i=0; i<pstBitMap->uiUintNum; i++)
    {
        pstBitMap->pulBitMap[i] = 0;
    }
}

static inline BS_STATUS bitmap_SetUintTo(BITMAP_S *pstBitMap, UINT uiStartUint,
        UINT uiCount, UINT to)
{
    UINT i;
    UINT count;

    if (uiStartUint >= pstBitMap->uiUintNum) {
        return BS_OUT_OF_RANGE;
    }

    count = MIN(pstBitMap->uiUintNum - uiStartUint, uiCount);

    for (i=0; i<count; i++) {
        pstBitMap->pulBitMap[uiStartUint + i] = to;
    }

    return BS_OK;
}

static inline BOOL_T bitmap_TestUint(BITMAP_S *pstBitMap, UINT uiStartUint,
        UINT uiCount)
{
    UINT i;
    UINT count;

    if (uiStartUint >= pstBitMap->uiUintNum) {
        BS_DBGASSERT(0);
        return FALSE;
    }

    count = MIN(pstBitMap->uiUintNum - uiStartUint, uiCount);

    for (i=0; i<count; i++) {
        if (pstBitMap->pulBitMap[uiStartUint + i] != 0xffffffff) {
            return FALSE;
        }
    }

    return TRUE;
}

void BITMAP_SetTo(BITMAP_S *pstBitMap, UINT index, UINT to)
{
    if (to) {
        BITMAP_SET(pstBitMap, index);
    } else {
        BITMAP_CLR(pstBitMap, index);
    }
}

static inline BS_STATUS bitmap_SetBitsTo(BITMAP_S *pstBitMap, UINT uiStartBit,
        UINT uiCount, UINT to)
{
    UINT uiEndBit = uiStartBit + uiCount;
    UINT uiTmpBitEnd;
    UINT uiStartUint, uiEndUint;
    UINT i;

    if (uiStartBit >= pstBitMap->ulBitNum) {
        return BS_OUT_OF_RANGE;
    }

    if (uiEndBit > pstBitMap->ulBitNum) {
        uiEndBit = pstBitMap->ulBitNum;
    }

    uiTmpBitEnd = NUM_UP_ALIGN(uiStartBit, 32);
    uiTmpBitEnd = MIN(uiTmpBitEnd, uiEndBit);

    for (i=uiStartBit; i<uiTmpBitEnd; i++) {
        BITMAP_SetTo(pstBitMap, i, to);
    }

    uiStartUint = uiTmpBitEnd/32;
    uiEndUint = uiEndBit/32;

    if (uiEndUint - uiStartUint) {
        bitmap_SetUintTo(pstBitMap, uiStartUint,
                uiEndUint - uiStartUint, to);
    }

    for (i=uiEndUint*32; i<uiEndBit; i++) {
        BITMAP_SetTo(pstBitMap, i, to);
    }

    return BS_OK;
}

static inline BOOL_T bitmap_TestBits(BITMAP_S *pstBitMap, UINT uiStartBit,
        UINT uiCount)
{
    UINT uiEndBit = uiStartBit + uiCount;
    UINT uiTmpBitEnd;
    UINT uiStartUint, uiEndUint;
    UINT i;

    if (uiStartBit >= pstBitMap->ulBitNum) {
        BS_DBGASSERT(0);
        return FALSE;
    }

    if (uiEndBit > pstBitMap->ulBitNum) {
        uiEndBit = pstBitMap->ulBitNum;
    }

    uiTmpBitEnd = NUM_UP_ALIGN(uiStartBit, 32);
    uiTmpBitEnd = MIN(uiTmpBitEnd, uiEndBit);

    for (i=uiStartBit; i<uiTmpBitEnd; i++) {
        if (! BITMAP_ISSET(pstBitMap, i)) {
            return FALSE;
        }
    }

    if (uiEndBit >= 32) {
        uiStartUint = uiTmpBitEnd/32;
        uiEndUint = uiEndBit/32;

        if (uiEndUint - uiStartUint) {
            if (! bitmap_TestUint(pstBitMap, uiStartUint,
                    uiEndUint - uiStartUint)) {
                return FALSE;
            }
        }

        for (i=uiEndUint*32; i<uiEndBit; i++) {
            if (! BITMAP_ISSET(pstBitMap, i)) {
                return FALSE;
            }
        }
    }

    return TRUE;
}

BS_STATUS BITMAP_SetUint(BITMAP_S * pstBitMap, UINT uiStartUint, UINT uiCount)
{
    return bitmap_SetUintTo(pstBitMap, uiStartUint, uiCount, 0xffffffff);
}

void BITMAP_ClrAll(BITMAP_S *pstBitMap)
{
    memset(pstBitMap->pulBitMap, 0, pstBitMap->uiUintNum * sizeof(UINT));
}

BS_STATUS BITMAP_ClrUint(BITMAP_S * pstBitMap, UINT uiStartUint, UINT uiCount)
{
    return bitmap_SetUintTo(pstBitMap, uiStartUint, uiCount, 0);
}

BS_STATUS BITMAP_SetBits(BITMAP_S *pstBitMap, UINT uiStartBit, UINT uiCount)
{
    return bitmap_SetBitsTo(pstBitMap, uiStartBit, uiCount, 0xffffffff);
}

BS_STATUS BITMAP_ClrBits(BITMAP_S *pstBitMap, UINT uiStartBit, UINT uiCount)
{
    return bitmap_SetBitsTo(pstBitMap, uiStartBit, uiCount, 0);
}

BOOL_T BITMAP_IsSetBits(BITMAP_S *pstBitMap, UINT uiStartBit, UINT uiCount)
{
    return bitmap_TestBits(pstBitMap, uiStartBit, uiCount);
}

BOOL_T BITMAP_IsAllSetted(BITMAP_S *pstBitMap)
{
    return BITMAP_IsSetBits(pstBitMap, 0, pstBitMap->ulBitNum);
}
