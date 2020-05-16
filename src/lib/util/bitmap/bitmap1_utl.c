/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2007-7-2
* Description: Index从1开始计算.0是无效值
* History:     
******************************************************************************/
/* retcode所需要的宏 */
#define RETCODE_FILE_NUM RETCODE_FILE_NUM_BITMAP

#include "bs.h"

#include "utl/bitmap_utl.h"
#include "utl/bitmap1_utl.h"

UINT BITMAP_Index2IndexFrom1(UINT uiIndex)
{
    if (uiIndex == BITMAP_INVALID_INDEX) {
        return 0;
    }

    return uiIndex + 1;
}

UINT BITMAP_IndexFrom12Index(UINT uiIndexFrom1)
{
    if (uiIndexFrom1 == 0) {
        return BITMAP_INVALID_INDEX;
    }

    return uiIndexFrom1 - 1;
}

/* 获取一个setted位的Index */
UINT BITMAP1_GetASettedBitIndex(IN BITMAP_S *pstBitMap)
{
    UINT uiIndex = BITMAP_GetASettedBitIndex(pstBitMap);

    return BITMAP_Index2IndexFrom1(uiIndex);
}

/* 获取某个指定位置之后的setted位的Index */
UINT BITMAP1_GetASettedBitIndexAfter(IN BITMAP_S *pstBitMap, IN UINT uiIndexFrom1)
{
    UINT uiIndex = BITMAP_GetASettedBitIndexAfter(pstBitMap, BITMAP_IndexFrom12Index(uiIndexFrom1));

    return BITMAP_Index2IndexFrom1(uiIndex);
}

/* 获取一个unsetted位的Index */
UINT BITMAP1_GetAUnsettedBitIndex(IN BITMAP_S *pstBitMap)
{
    UINT uiIndex = BITMAP_GetAUnsettedBitIndex(pstBitMap);

    return BITMAP_Index2IndexFrom1(uiIndex);
}


/* 环绕形式的获取一个unsetted位的Index */
UINT BITMAP1_GetAUnsettedBitIndexCycle(IN BITMAP_S *pstBitMap)
{
    UINT uiIndex = BITMAP_GetAUnsettedBitIndexCycle(pstBitMap);
    return BITMAP_Index2IndexFrom1(uiIndex);
}

/* 从两个bitmap中获取都被设置的位的IndexFrom1 */
UINT BITMAP1_2GetSettedIndex(IN BITMAP_S * pstBitMap1, IN BITMAP_S * pstBitMap2)
{
    return BITMAP_Index2IndexFrom1(BITMAP_2GetSettedIndex(pstBitMap1, pstBitMap2));
}

/* 从两个bitmap中获取都未被设置的位的IndexFrom1 */
UINT BITMAP1_2GetUnsettedIndex(IN BITMAP_S * pstBitMap1, IN BITMAP_S * pstBitMap2)
{
    return BITMAP_Index2IndexFrom1(BITMAP_2GetUnsettedIndex(pstBitMap1, pstBitMap2));
}

