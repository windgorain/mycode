/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      lixingang  Version: 1.0  Date: 2007-7-2
* Description: Index从1开始计算.0是无效值
* History:     
******************************************************************************/

#ifndef __BITMAP1_UTL_H_
#define __BITMAP1_UTL_H_

#include "utl/bitmap_utl.h"

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */


#define BITMAP1_ISSET(pstBitMap,uiIndexFrom1) BITMAP_ISSET(pstBitMap, (uiIndexFrom1)-1)
#define BITMAP1_SET(pstBitMap,uiIndexFrom1)   BITMAP_SET(pstBitMap, (uiIndexFrom1)-1)
#define BITMAP1_CLR(pstBitMap,uiIndexFrom1)   BITMAP_CLR(pstBitMap, (uiIndexFrom1)-1)

#define BITMAP1_SCAN_SETTED_BEGIN(pstBitMap, uiIndexFrom1)  \
    do { \
        UINT _uiIndex; \
        BITMAP_SCAN_SETTED_BEGIN(pstBitMap, _uiIndex) { \
            uiIndexFrom1 = _uiIndex + 1;

#define BITMAP1_SCAN_UNSETTED_BEGIN(pstBitMap, uiIndexFrom1)  \
    do {\
        UINT _uiIndex; \
        BITMAP_SCAN_UNSETTED_BEGIN(pstBitMap, _uiIndex) { \
            uiIndexFrom1 = _uiIndex + 1;
    
#define BITMAP1_SCAN_END() BITMAP_SCAN_END()}}while(0)

/* 获取一个setted位的IndexFrom1 */
extern UINT BITMAP1_GetASettedBitIndex(IN BITMAP_S * pstBitMap);

/* 获取某个指定位置之后的setted位的IndexFrom1 */
extern UINT BITMAP1_GetBusyFrom(BITMAP_S *pstBitMap, UINT from);

/* 获取一个unsetted位的IndexFrom1 */
extern UINT BITMAP1_GetAUnsettedBitIndex(IN BITMAP_S * pstBitMap);

/* 环绕形式的获取一个unsetted位的IndexFrom1 */
extern UINT BITMAP1_GetFreeCycle(IN BITMAP_S *pstBitMap);

/* 从两个bitmap中获取都未被设置的位的IndexFrom1 */
extern UINT BITMAP1_2GetFree(IN BITMAP_S * pstBitMap1, IN BITMAP_S * pstBitMap2);

/* 从两个bitmap中获取都被设置的位的IndexFrom1 */
UINT BITMAP1_2GetBusy(IN BITMAP_S * pstBitMap1, IN BITMAP_S * pstBitMap2);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif




