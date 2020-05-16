/******************************************************************************
* Copyright (C),    LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2014-6-16
* Description: 
* History:     
******************************************************************************/
#include <math.h>

#include "bs.h"

#include "utl/endian_utl.h"
#include "utl/rand_utl.h"
#include "utl/file_utl.h"
#include "utl/vldbmp_utl.h"

#define _VLDBMP_CODE_BMP_WIDTH 100   /* 字符图片模板的边长 */

#define _VLDBMP_YAWP_RATE 10  /* 噪点率 */

#define _VLDBMP_COLOR_BG_BLUE    0xFF  /* 背景色 */
#define _VLDBMP_COLOR_BG_RED     0xFF  /* 背景色 */
#define _VLDBMP_COLOR_BG_GREEN   0xFF  /* 背景色 */

typedef struct
{
    UINT uiStartX;
    UINT uiStartY;
    UINT uiStopX;
    UINT uiStopY;
}VLDBMP_LINE_S;

typedef struct
{
    CHAR cCode;
    VLDBMP_LINE_S *pstLines;
    UINT uiLineNum;
}VLDBMP_MAP_S;

/* 以100 * 100的画布, 第1象限为坐标 */

static VLDBMP_LINE_S g_astVlsImg_0[] = 
{
    {20,90, 20,10},
    {20,90, 80,90},
    {80,90, 80,10},
    {20,10, 80,10}
};

static VLDBMP_LINE_S g_astVlsImg_1[] = 
{
    {30,70, 50,90},
    {50,90, 50,10},
    {30,10, 70,10},
};

static VLDBMP_LINE_S g_astVlsImg_2[] = 
{
    {10,90, 90,90},
    {90,90, 90,50},
    {90,50, 10,50},
    {10,50, 10,10},
    {10,10, 90,10},
};

static VLDBMP_LINE_S g_astVlsImg_3[] = 
{
    {10,90, 90,90},
    {90,90, 90,10},
    {30,50, 90,50},
    {10,10, 90,10},
};

static VLDBMP_LINE_S g_astVlsImg_4[] = 
{
    {10,90, 10,50},
    {10,50, 90,50},
    {50,90, 50,10},
};

static VLDBMP_LINE_S g_astVlsImg_5[] = 
{
    {90,90, 10,90},
    {10,90, 10,50},
    {10,50, 90,50},
    {90,50, 90,10},
    {90,10, 10,10},
};

static VLDBMP_LINE_S g_astVlsImg_6[] = 
{
    {90,90, 10,90},
    {10,90, 10,10},
    {10,50, 90,50},
    {90,50, 90,10},
    {10,10, 90,10},
};

static VLDBMP_LINE_S g_astVlsImg_7[] = 
{
    {10,90, 90,90},
    {90,90, 90,10},
};

static VLDBMP_LINE_S g_astVlsImg_8[] = 
{
    {10,90, 90,90},
    {90,90, 90,10},
    {90,10, 10,10},
    {10,10, 10,90},
    {10,50, 90,50},
};

static VLDBMP_LINE_S g_astVlsImg_9[] = 
{
    {10,90, 90,90},
    {90,90, 90,10},
    {90,10, 10,10},
    {10,50, 10,90},
    {10,50, 90,50},
};

static VLDBMP_LINE_S g_astVlsImg_A[] = 
{
    {50,90, 10,10},
    {50,90, 90,10},
    {30,50, 70,50}
};

static VLDBMP_LINE_S g_astVlsImg_E[] = 
{
    {10,90, 80,90},
    {10,90, 10,10},
    {10,50, 60,50},
    {10,10, 80,10}
};

static VLDBMP_LINE_S g_astVlsImg_F[] = 
{
    {10,90, 80,90},
    {10,90, 10,10},
    {10,50, 70,50}
};

static VLDBMP_LINE_S g_astVlsImg_H[] = 
{
    {10,90, 10,10},
    {10,50, 90,50},
    {90,90, 90,10}
};

static VLDBMP_LINE_S g_astVlsImg_I[] = 
{
    {30,90, 70,90},
    {50,90, 50,10},
    {30,10, 70,10}
};

static VLDBMP_LINE_S g_astVlsImg_K[] = 
{
    {10,90, 10,10},
    {70,90, 10,50},
    {10,50, 70,10}
};

static VLDBMP_LINE_S g_astVlsImg_L[] = 
{
    {10,90, 10,10},
    {10,10, 80,10}
};

static VLDBMP_LINE_S g_astVlsImg_M[] = 
{
    {30,90, 10,10},
    {30,90, 50,30},
    {50,30, 70,90},
    {70,90, 90,10},
};

static VLDBMP_LINE_S g_astVlsImg_N[] = 
{
    {10,90, 10,10},
    {10,90, 90,10},
    {90,90, 90,10},
};

static VLDBMP_LINE_S g_astVlsImg_T[] = 
{
    {10,90, 90,90},
    {50,90, 50,10},
};

static VLDBMP_LINE_S g_astVlsImg_V[] = 
{
    {10,90, 50,10},
    {50,10, 90,90},
};

static VLDBMP_LINE_S g_astVlsImg_W[] = 
{
    {10,90, 30,10},
    {30,10, 50,70},
    {50,70, 70,10},
    {70,10, 90,90},
};

static VLDBMP_LINE_S g_astVlsImg_X[] = 
{
    {10,90, 90,10},
    {90,90, 10,10},
};

static VLDBMP_LINE_S g_astVlsImg_Y[] = 
{
    {10,90, 50,50},
    {90,90, 50,50},
    {50,50, 50,10},
};

static VLDBMP_LINE_S g_astVlsImg_Z[] = 
{
    {10,90, 90,90},
    {90,90, 10,10},
    {10,10, 90,10},
};

VLDBMP_MAP_S g_astVldbmpMap[] = 
{
    {'0', g_astVlsImg_0, sizeof(g_astVlsImg_0)/sizeof(VLDBMP_LINE_S)},
    {'1', g_astVlsImg_1, sizeof(g_astVlsImg_1)/sizeof(VLDBMP_LINE_S)},
    {'2', g_astVlsImg_2, sizeof(g_astVlsImg_2)/sizeof(VLDBMP_LINE_S)},
    {'3', g_astVlsImg_3, sizeof(g_astVlsImg_3)/sizeof(VLDBMP_LINE_S)},
    {'4', g_astVlsImg_4, sizeof(g_astVlsImg_4)/sizeof(VLDBMP_LINE_S)},
    {'5', g_astVlsImg_5, sizeof(g_astVlsImg_5)/sizeof(VLDBMP_LINE_S)},
    {'6', g_astVlsImg_6, sizeof(g_astVlsImg_6)/sizeof(VLDBMP_LINE_S)},
    {'7', g_astVlsImg_7, sizeof(g_astVlsImg_7)/sizeof(VLDBMP_LINE_S)},
    {'8', g_astVlsImg_8, sizeof(g_astVlsImg_8)/sizeof(VLDBMP_LINE_S)},
    {'9', g_astVlsImg_9, sizeof(g_astVlsImg_9)/sizeof(VLDBMP_LINE_S)},
    
    {'A', g_astVlsImg_A, sizeof(g_astVlsImg_A)/sizeof(VLDBMP_LINE_S)},
    {'E', g_astVlsImg_E, sizeof(g_astVlsImg_E)/sizeof(VLDBMP_LINE_S)},
    {'F', g_astVlsImg_F, sizeof(g_astVlsImg_F)/sizeof(VLDBMP_LINE_S)},
    {'H', g_astVlsImg_H, sizeof(g_astVlsImg_H)/sizeof(VLDBMP_LINE_S)},
    {'I', g_astVlsImg_I, sizeof(g_astVlsImg_I)/sizeof(VLDBMP_LINE_S)},
    {'K', g_astVlsImg_K, sizeof(g_astVlsImg_K)/sizeof(VLDBMP_LINE_S)},
    {'L', g_astVlsImg_L, sizeof(g_astVlsImg_L)/sizeof(VLDBMP_LINE_S)},
    {'M', g_astVlsImg_M, sizeof(g_astVlsImg_M)/sizeof(VLDBMP_LINE_S)},
    {'N', g_astVlsImg_N, sizeof(g_astVlsImg_N)/sizeof(VLDBMP_LINE_S)},
    {'T', g_astVlsImg_T, sizeof(g_astVlsImg_T)/sizeof(VLDBMP_LINE_S)},
    {'V', g_astVlsImg_V, sizeof(g_astVlsImg_V)/sizeof(VLDBMP_LINE_S)},
    {'W', g_astVlsImg_W, sizeof(g_astVlsImg_W)/sizeof(VLDBMP_LINE_S)},
    {'X', g_astVlsImg_X, sizeof(g_astVlsImg_X)/sizeof(VLDBMP_LINE_S)},
    {'Y', g_astVlsImg_Y, sizeof(g_astVlsImg_Y)/sizeof(VLDBMP_LINE_S)},
    {'Z', g_astVlsImg_Z, sizeof(g_astVlsImg_Z)/sizeof(VLDBMP_LINE_S)},
};

static inline UINT vldbmp_Rand(VOID)
{
    return RAND_Get();
}

static inline VOID vldbmp_FillHead
(
    IN VLDBMP_S *pstBmp,
    IN UINT uiCodeCount,
    IN UINT uiWidth,
    IN UINT uiHeight
 )
{
    UINT    uiDataSize  = uiWidth * uiHeight * uiCodeCount * (UINT)sizeof(VLDBMP_DATA_S);

    Mem_Zero(&pstBmp->stBmpHead, sizeof(VLDBMP_HEADER_S));
    pstBmp->stBmpHead.usType    = 0x4d42;
    pstBmp->stBmpHead.uiSize    = sizeof(VLDBMP_HEADER_S) + sizeof(VLDBMP_INFO_S) + uiDataSize;
    pstBmp->stBmpHead.uiOffBits = sizeof(VLDBMP_HEADER_S) + sizeof(VLDBMP_INFO_S);

    Mem_Zero(&pstBmp->stInfo, sizeof(VLDBMP_INFO_S));
    pstBmp->stInfo.uiSize        = sizeof(VLDBMP_INFO_S);
    pstBmp->stInfo.uiWidth       = uiWidth * uiCodeCount;
    pstBmp->stInfo.uiHeight      = uiHeight;
    pstBmp->stInfo.usPlanes      = 1;
    pstBmp->stInfo.usBitCount    = 24;   /* 24位真彩色 */
    pstBmp->stInfo.uiCompression = 0;
    pstBmp->stInfo.uiSizeImage   = uiDataSize;
}

/* 转成bmp需要的小序 */
VOID vldbmp_EndianHead(IN VLDBMP_S *pstBmp)
{
    pstBmp->stBmpHead.usType    = Host2Litter16(pstBmp->stBmpHead.usType);
    pstBmp->stBmpHead.uiSize    = Host2Litter32(pstBmp->stBmpHead.uiSize);
    pstBmp->stBmpHead.uiOffBits = Host2Litter32(pstBmp->stBmpHead.uiOffBits);

    pstBmp->stInfo.uiSize        = Host2Litter32(pstBmp->stInfo.uiSize);
    pstBmp->stInfo.uiWidth       = Host2Litter32(pstBmp->stInfo.uiWidth);
    pstBmp->stInfo.uiHeight      = Host2Litter32(pstBmp->stInfo.uiHeight);
    pstBmp->stInfo.usPlanes      = Host2Litter16(pstBmp->stInfo.usPlanes);
    pstBmp->stInfo.usBitCount    = Host2Litter16(pstBmp->stInfo.usBitCount);
    pstBmp->stInfo.uiCompression = Host2Litter32(pstBmp->stInfo.uiCompression);
    pstBmp->stInfo.uiSizeImage   = Host2Litter32(pstBmp->stInfo.uiSizeImage);
    pstBmp->stInfo.uiXPelsPerMeter   = Host2Litter32(pstBmp->stInfo.uiXPelsPerMeter);
    pstBmp->stInfo.uiYPelsPerMeter   = Host2Litter32(pstBmp->stInfo.uiYPelsPerMeter);
    pstBmp->stInfo.uiClrUsed         = Host2Litter32(pstBmp->stInfo.uiClrUsed);
    pstBmp->stInfo.uiClrImportant    = Host2Litter32(pstBmp->stInfo.uiClrImportant);
}

static VLDBMP_MAP_S * vldbmp_DupMap(IN const VLDBMP_MAP_S *pstMapSrc)
{
    VLDBMP_MAP_S *pstMapDst;
    VLDBMP_LINE_S *pstLines;
    UINT uiSize;

    pstMapDst = MEM_ZMalloc(sizeof(VLDBMP_MAP_S));
    if (NULL == pstMapDst)
    {
        return NULL;
    }

    uiSize = (UINT)sizeof(VLDBMP_LINE_S) * pstMapSrc->uiLineNum;
    pstLines = MEM_Malloc(uiSize);
    if (NULL == pstLines)
    {
        MEM_Free(pstMapDst);
        return NULL;
    }
    MEM_Copy(pstLines, pstMapSrc->pstLines, uiSize);

    pstMapDst->cCode = pstMapSrc->cCode;
    pstMapDst->uiLineNum = pstMapSrc->uiLineNum;
    pstMapDst->pstLines = pstLines;

    return pstMapDst;
}

static VOID vldbmp_FreeMap(IN const VLDBMP_MAP_S *pstMap)
{
    if (NULL == pstMap)
    {
        return;
    }

    if (NULL != pstMap->pstLines)
    {
        MEM_Free(pstMap->pstLines);
    }

    MEM_Free(pstMap);
}

static VLDBMP_MAP_S * vldbmp_GetMapByCode(IN CHAR cCode)
{
    UINT uiI;
    VLDBMP_MAP_S *pstMap = NULL;

    for (uiI=0; uiI<sizeof(g_astVldbmpMap)/sizeof(VLDBMP_MAP_S); uiI++)
    {
        if (cCode == g_astVldbmpMap[uiI].cCode)
        {
            pstMap = vldbmp_DupMap(&g_astVldbmpMap[uiI]);
            break;
        }
    }

    return pstMap;
}

static inline UINT vldbmp_GetDistance(IN UINT uiPos1, IN UINT uiPos2)
{
    if (uiPos1 > uiPos2)
    {
        return uiPos1 - uiPos2;
    }

    return uiPos2 - uiPos1;
}

/* 获得坐标 */
static UINT vldbmp_GetCoordinate
(
    IN UINT uiStarCoordinate,
    IN UINT uiStopCoordinate,
    IN UINT uiStepNum,
    IN UINT uiStepIndex
)
{
    if (uiStarCoordinate <= uiStopCoordinate)
    {
        return uiStarCoordinate + (((uiStopCoordinate - uiStarCoordinate) * uiStepIndex + uiStepNum/2) / uiStepNum);
    }

    return uiStarCoordinate - (((uiStarCoordinate - uiStopCoordinate) * uiStepIndex + uiStepNum/2) / uiStepNum);
}

static inline VOID vldbmp_DrawPoint
(
    INOUT VLDBMP_S *pstBmp,
    IN UINT uiX,
    IN UINT uiY,
    IN const VLDBMP_DATA_S *pstColor
)
{
    UINT uiPos;

    if (uiX >= pstBmp->stInfo.uiWidth)
    {
        return;
    }

    if (uiY >= pstBmp->stInfo.uiHeight)
    {
        return;
    }

    uiPos = uiY * pstBmp->stInfo.uiWidth + uiX;

    pstBmp->pstData[uiPos].ucBule  = pstColor->ucBule;
    pstBmp->pstData[uiPos].ucRed   = pstColor->ucRed;
    pstBmp->pstData[uiPos].ucGreen = pstColor->ucGreen;
}

static VOID vldbmp_DrawArea
(
    INOUT VLDBMP_S *pstBmp,
    IN UINT uiX,
    IN UINT uiY,
    IN const VLDBMP_OPT_S *pstOpt,
    IN const VLDBMP_DATA_S *pstColor
)
{
    UINT uiI, uiJ;

    vldbmp_DrawPoint(pstBmp, uiX, uiY, pstColor);

    for (uiI=1; uiI<=pstOpt->uiLineRadii; uiI++)
    {
        for (uiJ=1; uiJ<=pstOpt->uiLineRadii; uiJ++)
        {
            vldbmp_DrawPoint(pstBmp, uiX, uiY + uiJ, pstColor);
            vldbmp_DrawPoint(pstBmp, uiX, uiY - uiJ, pstColor);
            vldbmp_DrawPoint(pstBmp, uiX + uiI, uiY, pstColor);
            vldbmp_DrawPoint(pstBmp, uiX + uiI, uiY + uiJ, pstColor);
            vldbmp_DrawPoint(pstBmp, uiX + uiI, uiY - uiJ, pstColor);
            vldbmp_DrawPoint(pstBmp, uiX - uiI, uiY, pstColor);
            vldbmp_DrawPoint(pstBmp, uiX - uiI, uiY + uiJ, pstColor);
            vldbmp_DrawPoint(pstBmp, uiX - uiI, uiY - uiJ, pstColor);
        }
    }
}

static VOID vldbmp_DrawLine
(
    INOUT VLDBMP_S *pstBmp,
    IN const VLDBMP_LINE_S *pstLine,
    IN UINT uiXOffset,  /* X方向上的偏移 */
    IN const VLDBMP_OPT_S *pstOpt,
    IN const VLDBMP_DATA_S *pstColor
)
{
    UINT uiXDistance;
    UINT uiYDistance;
    UINT uiStepNum;
    UINT uiStep;
    UINT uiX, uiY; /* XY坐标 */

    uiXDistance = vldbmp_GetDistance(pstLine->uiStartX, pstLine->uiStopX);
    uiYDistance = vldbmp_GetDistance(pstLine->uiStartY, pstLine->uiStopY);

    uiStepNum = MAX(uiXDistance, uiYDistance);

    for (uiStep=0; uiStep<uiStepNum; uiStep++)
    {
        uiX = vldbmp_GetCoordinate(pstLine->uiStartX, pstLine->uiStopX, uiStepNum, uiStep);
        uiY = vldbmp_GetCoordinate(pstLine->uiStartY, pstLine->uiStopY, uiStepNum, uiStep);

        uiX += uiXOffset;

        vldbmp_DrawArea(pstBmp, uiX, uiY, pstOpt, pstColor);
    }

    return;
}

static VOID vldbmp_FillCode
(
    INOUT VLDBMP_S *pstBmp,
    IN const VLDBMP_MAP_S *pstMap,
    IN UINT uiXOffset,  /* X方向上的偏移 */
    IN const VLDBMP_OPT_S *pstOpt,
    IN const VLDBMP_DATA_S *pstColor
)
{
    UINT uiI;

    for (uiI=0; uiI<pstMap->uiLineNum; uiI++)
    {
        vldbmp_DrawLine(pstBmp, &pstMap->pstLines[uiI], uiXOffset, pstOpt, pstColor);
    }
}

static VOID vldbmp_AddYawp
(
    INOUT VLDBMP_S *pstBmp,
    IN UINT uiWidth,
    IN UINT uiHeight,
    IN UINT uiImgIndex, /* 第几个字符图片 */
    IN const VLDBMP_OPT_S *pstOpt,
    IN const VLDBMP_DATA_S *pstColor
)
{
    UINT uiYawpCount;
    UINT uiDataCount;
    UINT uiIndex;
    UINT uiX;
    UINT uiY;

    if ((pstOpt->uiFlag & VLDBMP_FLAG_YWAP) == 0)
    {
        return;
    }

    uiDataCount = uiWidth * uiHeight;

    uiYawpCount = _VLDBMP_YAWP_RATE * uiDataCount / 100;

    for (uiIndex=0; uiIndex<uiYawpCount; uiIndex++)
    {
        uiX = vldbmp_Rand() % uiWidth;
        uiY = vldbmp_Rand() % uiHeight;

        uiX += (uiWidth * uiImgIndex);

        vldbmp_DrawPoint(pstBmp, uiX, uiY, pstColor);
    }
}

static VOID vldbmp_FillBackColor(IN VLDBMP_S *pstBmp)
{
    UINT uiI;
    UINT uiCount;
    VLDBMP_DATA_S *pstData = pstBmp->pstData;

    uiCount = pstBmp->stInfo.uiWidth * pstBmp->stInfo.uiHeight;

    for (uiI=0; uiI<uiCount; uiI++)
    {
        pstData[uiI].ucBule = _VLDBMP_COLOR_BG_BLUE;
        pstData[uiI].ucRed = _VLDBMP_COLOR_BG_RED;
        pstData[uiI].ucGreen = _VLDBMP_COLOR_BG_GREEN;
    }
}

static UINT vldbmp_Fit2Length(IN UINT uiOffset, IN UINT uiFrom, IN UINT uiTo)
{
    return (uiOffset * uiTo) / uiFrom;
}

/* 适应画布 */
static VOID vldbmp_Fit2Canvas
(
    IN const VLDBMP_MAP_S *pstMap,
    IN UINT uiFromWidth,
    IN UINT uiFromHeight,
    IN UINT uiToWidth,
    IN UINT uiToHeight
)
{
    UINT uiI;

    for (uiI=0; uiI<pstMap->uiLineNum; uiI++)
    {
        pstMap->pstLines[uiI].uiStartX = vldbmp_Fit2Length(pstMap->pstLines[uiI].uiStartX, uiFromWidth, uiToWidth);
        pstMap->pstLines[uiI].uiStopX = vldbmp_Fit2Length(pstMap->pstLines[uiI].uiStopX, uiFromWidth, uiToWidth);
        pstMap->pstLines[uiI].uiStartY = vldbmp_Fit2Length(pstMap->pstLines[uiI].uiStartY, uiFromHeight, uiToHeight);
        pstMap->pstLines[uiI].uiStopY = vldbmp_Fit2Length(pstMap->pstLines[uiI].uiStopY, uiFromHeight, uiToHeight);
    }
}

static UINT vldbmp_RangeRand(IN UINT uiRang1, IN UINT uiRang2)
{
    UINT uiMin;
    UINT uiMax;
    UINT uiDis;
    UINT uiTmp;

    if (uiRang1 == uiRang2)
    {
        return uiRang1;
    }

    uiMin = MIN(uiRang1, uiRang2);
    uiMax = MAX(uiRang1, uiRang2);

    uiDis = uiMax - uiMin;

    uiTmp = vldbmp_Rand() % (uiDis + 1);

    return uiMin + uiTmp;
}

static inline UINT vldbmp_CompressPoint(IN UINT uiPoint, IN UINT uiCompressRate)
{
    UINT uiNewPoint;

    uiNewPoint = uiPoint - (((uiPoint * uiCompressRate) + 50) / 100);

    return uiNewPoint;
}

/* 压缩 */
static VOID vldbmp_Compress
(
    IN const VLDBMP_MAP_S *pstMap,
    IN const VLDBMP_OPT_S *pstOpt
)
{
    UINT uiCompressRateX;
    UINT uiCompressRateY;
    UINT uiIndex;

    uiCompressRateX = vldbmp_RangeRand(pstOpt->ucXCompressMin, pstOpt->ucXCompressMax);
    uiCompressRateY = vldbmp_RangeRand(pstOpt->ucYCompressMin, pstOpt->ucYCompressMax);

    if ((uiCompressRateX == 0) && (uiCompressRateY == 0))
    {
        return;
    }

    for (uiIndex=0; uiIndex<pstMap->uiLineNum; uiIndex++)
    {
        pstMap->pstLines[uiIndex].uiStartX
            = vldbmp_CompressPoint(pstMap->pstLines[uiIndex].uiStartX, uiCompressRateX);
        pstMap->pstLines[uiIndex].uiStopX
            = vldbmp_CompressPoint(pstMap->pstLines[uiIndex].uiStopX, uiCompressRateX);
        pstMap->pstLines[uiIndex].uiStartY
            = vldbmp_CompressPoint(pstMap->pstLines[uiIndex].uiStartY, uiCompressRateY);
        pstMap->pstLines[uiIndex].uiStopY
            = vldbmp_CompressPoint(pstMap->pstLines[uiIndex].uiStopY, uiCompressRateY);
    }

    return;
}

/* 漂移 */
static VOID vldbmp_Drift
(
    IN const VLDBMP_MAP_S *pstMap,
    IN UINT uiWidth,
    IN UINT uiHeight,
    IN const VLDBMP_OPT_S *pstOpt
)
{
    UINT uiMaxXDrift; /* X方向上的最大漂移值 */
    UINT uiMaxYDrift; /* Y方向上的最大漂移值 */
    UINT uiMaxX;
    UINT uiMaxY;
    UINT uiXDrift;
    UINT uiYDrift;
    UINT uiIndex;

    if ((pstOpt->uiFlag & VLDBMP_FLAG_DRIFT) == 0)
    {
        return;
    }

    uiMaxX = 0;
    uiMaxY = 0;

    for (uiIndex = 0; uiIndex<pstMap->uiLineNum; uiIndex++)
    {
        uiMaxX = MAX(uiMaxX, pstMap->pstLines[uiIndex].uiStartX);
        uiMaxX = MAX(uiMaxX, pstMap->pstLines[uiIndex].uiStopX);
        uiMaxY = MAX(uiMaxY, pstMap->pstLines[uiIndex].uiStartY);
        uiMaxY = MAX(uiMaxY, pstMap->pstLines[uiIndex].uiStopY);
    }

    uiMaxX += pstOpt->uiLineRadii;
    uiMaxY += pstOpt->uiLineRadii;

    uiMaxXDrift = 0;
    if (uiWidth > uiMaxX)
    {
        uiMaxXDrift = uiWidth - uiMaxX;
    }

    uiMaxYDrift = 0;
    if (uiHeight > uiMaxY)
    {
        uiMaxYDrift = uiHeight - uiMaxY;
    }

    uiXDrift = 0;
    if (uiMaxXDrift > 0)
    {
        uiXDrift = vldbmp_Rand() % uiMaxXDrift;  /* 取值范围为 0 - (width - 1)*/
    }

    uiYDrift = 0;
    if (uiMaxYDrift > 0)
    {
        uiYDrift = vldbmp_Rand() % uiMaxYDrift;
    }

    for (uiIndex = 0; uiIndex<pstMap->uiLineNum; uiIndex++)
    {
        pstMap->pstLines[uiIndex].uiStartX += uiXDrift;
        pstMap->pstLines[uiIndex].uiStopX += uiXDrift;
        pstMap->pstLines[uiIndex].uiStartY += uiYDrift;
        pstMap->pstLines[uiIndex].uiStopY += uiYDrift;
    }

    return;
}

static inline VOID vldbmp_RoutePoint
(
    IN double fSin,
    IN double fCos,
    IN INT iX1,
    IN INT iY1,
    IN BOOL_T bDeasil, /* 是否顺时针 */
    OUT INT *piX2,
    OUT INT *piY2
)
{
    INT iX2, iY2;

    if (bDeasil == TRUE)
    {
        iX2 = (INT)((double)iX1 * fCos + (double)iY1 * fSin);
        iY2 = (INT)((double)iY1 * fCos - (double)iX1 * fSin);
    }
    else
    {
        iX2 = (INT)((double)iX1 * fCos - (double)iY1 * fSin);
        iY2 = (INT)((double)iY1 * fCos + (double)iX1 * fSin);
    }

    *piX2 = iX2;
    *piY2 = iY2;

    return;
}

/* 移动到第一象限 */
static VOID vldbmp_MoveToQuadrant1(IN VLDBMP_MAP_S *pstMap)
{
    INT iX, iY;
    INT iMinX = 0;
    INT iMinY = 0;
    UINT uiIndex;

    for (uiIndex = 0; uiIndex<pstMap->uiLineNum; uiIndex++)
    {
        iX = pstMap->pstLines[uiIndex].uiStartX;
        iY = pstMap->pstLines[uiIndex].uiStartY;
        iMinX = MIN(iMinX, iX);
        iMinY = MIN(iMinY, iY);
        iX = pstMap->pstLines[uiIndex].uiStopX;
        iY = pstMap->pstLines[uiIndex].uiStopY;
        iMinX = MIN(iMinX, iX);
        iMinY = MIN(iMinY, iY);
    }

    for (uiIndex = 0; uiIndex<pstMap->uiLineNum; uiIndex++)
    {
        pstMap->pstLines[uiIndex].uiStartX -= iMinX;
        pstMap->pstLines[uiIndex].uiStartY -= iMinY;
        pstMap->pstLines[uiIndex].uiStopX -= iMinX;
        pstMap->pstLines[uiIndex].uiStopY -= iMinY;
    }
}

static VOID vldbmp_GetMapSize(IN VLDBMP_MAP_S *pstMap, OUT UINT *puiWidth, OUT UINT *puiHeight)
{
    UINT uiIndex;
    UINT uiMaxX = 0;
    UINT uiMaxY = 0;

    for (uiIndex = 0; uiIndex<pstMap->uiLineNum; uiIndex++)
    {
        uiMaxX = MAX(uiMaxX, pstMap->pstLines[uiIndex].uiStartX);
        uiMaxX = MAX(uiMaxX, pstMap->pstLines[uiIndex].uiStopX);
        uiMaxY = MAX(uiMaxY, pstMap->pstLines[uiIndex].uiStartY);
        uiMaxY = MAX(uiMaxY, pstMap->pstLines[uiIndex].uiStopY);
    }

    *puiWidth = uiMaxX + 1;
    *puiHeight = uiMaxY + 1;
}

/* 旋转 */
static VOID vldbmp_Rotate
(
    IN VLDBMP_MAP_S *pstMap,
    IN UINT uiWidth,
    IN UINT uiHeight,
    IN const VLDBMP_OPT_S *pstOpt
)
{
    UINT uiIndex;
    UINT uiRotateRand;
    double fSin;
    double fCos;
    double fRadian;
    double fPai = 3.14;
    INT iX, iY;
    UINT uiWidthAfterRotate;
    UINT uiHeightAfterRotate;
    BOOL_T bDeasil;

    if (pstOpt->ucMaxRotate == 0)
    {
        return;
    }

    uiRotateRand = (UINT)pstOpt->ucMaxRotate;

    uiRotateRand = vldbmp_Rand() % (uiRotateRand * 2 + 1);
    if (uiRotateRand == 0)
    {
        return;
    }

    bDeasil = FALSE;
    if (uiRotateRand > pstOpt->ucMaxRotate)
    {
        uiRotateRand -= pstOpt->ucMaxRotate;
        bDeasil = TRUE;
    }

    fRadian = fPai * (double)uiRotateRand / 180;

    fSin = sin(fRadian);
    fCos = cos(fRadian);

    for (uiIndex = 0; uiIndex<pstMap->uiLineNum; uiIndex++)
    {
        iX = pstMap->pstLines[uiIndex].uiStartX;
        iY = pstMap->pstLines[uiIndex].uiStartY;
        vldbmp_RoutePoint(fSin, fCos, iX, iY, bDeasil, &iX, &iY);
        pstMap->pstLines[uiIndex].uiStartX = iX;
        pstMap->pstLines[uiIndex].uiStartY = iY;

        iX = pstMap->pstLines[uiIndex].uiStopX;
        iY = pstMap->pstLines[uiIndex].uiStopY;
        vldbmp_RoutePoint(fSin, fCos, iX, iY, bDeasil, &iX, &iY);
        pstMap->pstLines[uiIndex].uiStopX = iX;
        pstMap->pstLines[uiIndex].uiStopY = iY;
    }

    /* 移动到第一象限 */
    vldbmp_MoveToQuadrant1(pstMap);

    /* 可能超出画布范围了,调整一下 */
    vldbmp_GetMapSize(pstMap, &uiWidthAfterRotate, &uiHeightAfterRotate);
    vldbmp_Fit2Canvas(pstMap, uiWidthAfterRotate, uiHeightAfterRotate, uiWidth, uiHeight);

    return;
}

static VOID vldbmp_RandColor(OUT VLDBMP_DATA_S *pstColor)
{
    UCHAR ucBlue;
    UCHAR ucGreen;
    UCHAR ucRed;

    ucBlue = vldbmp_Rand() & 0xff;
    ucGreen = vldbmp_Rand() & 0xff;
    ucRed = vldbmp_Rand() & 0xff;

    if ((ucBlue > 0x7F) && (ucGreen > 0x7F) && (ucRed > 0x7F))
    {
        ucBlue = ucBlue & 0x7f;
        ucGreen = ucGreen & 0x7f;
        ucRed = ucRed & 0x7f;
    }

    pstColor->ucBule = ucBlue;
    pstColor->ucGreen = ucGreen;
    pstColor->ucRed = ucRed;
}

static VOID vldbmp_FillData
(
    IN VLDBMP_S *pstBmp,
    IN const CHAR *pcVldCode,
    IN UINT uiVldCodeLen,
    IN UINT uiWidth,
    IN UINT uiHeight,
    IN const VLDBMP_OPT_S *pstOpt
)
{
    UINT uiImgIndex;
    VLDBMP_MAP_S *pstMap;
    VLDBMP_DATA_S stColor = {0};

    /* 填充背景色 */
    vldbmp_FillBackColor(pstBmp);

    for (uiImgIndex=0; uiImgIndex<uiVldCodeLen; uiImgIndex++)
    {
        pstMap = vldbmp_GetMapByCode(pcVldCode[uiImgIndex]);
        if (NULL == pstMap)
        {
            continue;
        }

        vldbmp_Fit2Canvas(pstMap, _VLDBMP_CODE_BMP_WIDTH, _VLDBMP_CODE_BMP_WIDTH, uiWidth, uiHeight);
        vldbmp_Compress(pstMap, pstOpt);
        vldbmp_Rotate(pstMap, uiWidth, uiHeight, pstOpt);
        vldbmp_Drift(pstMap, uiWidth, uiHeight, pstOpt);

        if (pstOpt->uiFlag & VLDBMP_FLAG_RAND_COLOR)
        {
            vldbmp_RandColor(&stColor);
        }

        /* 添加噪点 */
        vldbmp_AddYawp(pstBmp, uiWidth, uiHeight, uiImgIndex, pstOpt, &stColor);
        
        /* 生成字符图片 */
        vldbmp_FillCode(pstBmp, pstMap, uiImgIndex * uiWidth, pstOpt, &stColor);

        vldbmp_FreeMap(pstMap);
    }

    return;
}

VOID VLDBMP_GenCode(IN UINT uiCount/* 产生随机字符数目 */, OUT CHAR *pcCode)
{
    UINT uiI;
    UINT uiCharCount;
    UINT uiIndex;

    uiCharCount = sizeof(g_astVldbmpMap)/sizeof(VLDBMP_MAP_S);

    for (uiI=0; uiI<uiCount; uiI++)
    {
        uiIndex = vldbmp_Rand() % uiCharCount;
        pcCode[uiI] = g_astVldbmpMap[uiIndex].cCode;
    }

    pcCode[uiCount] = '\0';
}

VLDBMP_S * VLDBMP_CreateBmp
(
    IN const CHAR *pcVldCode,
    IN UINT uiWidth/* 每个字符的宽度 */,
    IN UINT uiHeight/* 每个字符的高度 */,
    IN const VLDBMP_OPT_S *pstOpt
)
{
    VLDBMP_S *pstBmp = NULL;
    UINT    uiCodeCount = (UINT)strlen(pcVldCode);
    UINT    uiDataCount = uiWidth * uiHeight * uiCodeCount;
    UINT    uiDataSize  = uiDataCount * (UINT)sizeof(VLDBMP_DATA_S);

    pstBmp = MEM_Malloc(sizeof(VLDBMP_S) + uiDataSize);
    if (NULL == pstBmp)
    {
        return NULL;
    }

    vldbmp_FillHead(pstBmp, uiCodeCount, uiWidth, uiHeight);
    vldbmp_FillData(pstBmp, pcVldCode, uiCodeCount, uiWidth, uiHeight, pstOpt);
    vldbmp_EndianHead(pstBmp);

    return pstBmp;    
}


VOID VLDBMP_Destory(IN const VLDBMP_S *pstBmp)
{
    if (NULL != pstBmp)
    {
        MEM_Free(pstBmp);
    }
}

/* 创建所有字符图片的总表图片 */
VLDBMP_S * VLDBMP_CreateListBmp
(
    IN UINT uiWidth/* 每个字符的宽度 */,
    IN UINT uiHeight/* 每个字符的高度 */,
    IN VLDBMP_OPT_S *pstOpt
)
{
    CHAR szVldCodes[128];
    UINT uiI;
    UINT uiCount;

    uiCount = sizeof(g_astVldbmpMap)/sizeof(VLDBMP_MAP_S);

    BS_DBGASSERT(uiCount < sizeof(szVldCodes));

    for (uiI=0; uiI<uiCount; uiI++)
    {
        szVldCodes[uiI] = g_astVldbmpMap[uiI].cCode;
    }

    szVldCodes[uiCount] = '\0';

    return VLDBMP_CreateBmp(szVldCodes, uiWidth, uiHeight, pstOpt);
}

BS_STATUS VLDBMP_SaveToFile(IN VLDBMP_S *pstBmp, IN CHAR *pszPath)
{
    FILE *fp;

    if (NULL == pstBmp)
    {
        return (BS_NULL_PARA);
    }

    fp = FILE_Open(pszPath, TRUE, "wb+");
    if (NULL == fp)
    {
        return (BS_CAN_NOT_OPEN);
    }

    fwrite(pstBmp, 1, pstBmp->stBmpHead.uiSize, fp);

    fclose(fp);

    return BS_OK;
}

