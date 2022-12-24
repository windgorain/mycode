/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2009-4-3
* Description: 
* History:     
******************************************************************************/

#ifndef __VLDBMP_UTL_H_
#define __VLDBMP_UTL_H_

#include "utl/endian_utl.h"

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

#pragma pack(1)

typedef struct
{
    USHORT usType;      /* 类型, 必须为BM */
    UINT  uiSize;      /* 图片文件大小, 字节为单位 */
    USHORT usReserved1;
    USHORT usReserved2;
    UINT  uiOffBits;   /* 位图数据的起始位置 */
}VLDBMP_HEADER_S;

typedef struct
{
    UINT  uiSize;         /* 本结构大小 */
    UINT  uiWidth;        /* 位图宽度, 像素为单位 */
    UINT  uiHeight;       /* 位图高度, 像素为单位 */
    USHORT usPlanes;       /* 目标设备级别, 必须为1 */
    USHORT usBitCount;     /*1,4,8,24. 24表示真彩色 */
    UINT  uiCompression;  /* 是否压缩. 0表示不压缩 */
    UINT  uiSizeImage;    /* 位图大小 */
    UINT uiXPelsPerMeter; /* 水平分辨率 */
    UINT uiYPelsPerMeter; /* 垂直分辨率 */
    UINT uiClrUsed;       /* 使用的颜色数 */
    UINT uiClrImportant;
}VLDBMP_INFO_S;

typedef struct
{
    UCHAR ucBule;
    UCHAR ucGreen;
    UCHAR ucRed;
 }VLDBMP_DATA_S;

typedef struct
{
    VLDBMP_HEADER_S stBmpHead;
    VLDBMP_INFO_S   stInfo;
    VLDBMP_DATA_S   pstData[0];
}VLDBMP_S;

#pragma pack()

#define VLDBMP_FLAG_YWAP        0x1 /* 噪点 */
#define VLDBMP_FLAG_DRIFT       0x2 /* 漂移 */
#define VLDBMP_FLAG_RAND_COLOR  0x4 /* 随机字体颜色 */


typedef struct
{
    UINT uiLineRadii; /* 线条宽度半径 */
    UINT uiFlag;
    UCHAR ucXCompressMin; /* X方向上的最小压缩率 */
    UCHAR ucXCompressMax; /* X方向上的最大压缩率 */
    UCHAR ucYCompressMin; /* Y方向上的最小压缩率 */
    UCHAR ucYCompressMax; /* Y方向上的最大压缩率 */
    UCHAR ucMaxRotate;    /* 最大旋转角度 */
}VLDBMP_OPT_S;

#define VLDBMP_SET_RADII(_pstOpt,_uiRadii) ((_pstOpt)->uiLineRadii = (_uiRadii))
#define VLDBMP_SET_MAX_ROTATE(_pstOpt,_ucRotate) ((_pstOpt)->ucMaxRotate = (_ucRotate))
#define VLDBMP_YAWP_ON(_pstOpt) ((_pstOpt)->uiFlag |= VLDBMP_FLAG_YWAP)
#define VLDBMP_YAWP_OFF(_pstOpt) ((_pstOpt)->uiFlag &= (~VLDBMP_FLAG_YWAP))
#define VLDBMP_DRIFT_ON(_pstOpt) ((_pstOpt)->uiFlag |= VLDBMP_FLAG_DRIFT)
#define VLDBMP_DRIFT_OFF(_pstOpt) ((_pstOpt)->uiFlag &= (~VLDBMP_FLAG_DRIFT))
#define VLDBMP_RAND_COLOR_ON(_pstOpt) ((_pstOpt)->uiFlag |= VLDBMP_FLAG_RAND_COLOR)
#define VLDBMP_RAND_COLOR_OFF(_pstOpt) ((_pstOpt)->uiFlag &= (~VLDBMP_FLAG_RAND_COLOR))


static inline VOID VLDBMP_SetXCompress(IN VLDBMP_OPT_S *pstOpt, IN UCHAR ucMin, IN UCHAR ucMax)
{
    pstOpt->ucXCompressMin = ucMin;
    pstOpt->ucXCompressMax = ucMax;
}

static inline VOID VLDBMP_SetYCompress(IN VLDBMP_OPT_S *pstOpt, IN UCHAR ucMin, IN UCHAR ucMax)
{
    pstOpt->ucYCompressMin = ucMin;
    pstOpt->ucYCompressMax = ucMax;
}

#define VLDBMP_SIZE(pstBmp) (Litter2Host32(pstBmp->stBmpHead.uiSize))

VOID VLDBMP_GenCode(IN UINT ulCount/* 产生随机字符数目 */, OUT CHAR *pszCode);
VLDBMP_S * VLDBMP_CreateBmp
(
    IN const CHAR *pszVldCode,
    IN UINT uiWidth/* 每个字符的宽度 */,
    IN UINT uiHeight/* 每个字符的高度 */,
    IN const VLDBMP_OPT_S *pstOpt /* VLDBMP_FLAG_COMPRESS等 */
);
/* 创建所有字符图片的总表图片 */
VLDBMP_S * VLDBMP_CreateListBmp
(
    IN UINT uiWidth/* 每个字符的宽度 */,
    IN UINT uiHeight/* 每个字符的高度 */,
    IN VLDBMP_OPT_S *pstOpt  /* VLDBMP_FLAG_COMPRESS等 */
);
VOID VLDBMP_Destory(IN const VLDBMP_S *pstBmp);
BS_STATUS VLDBMP_SaveToFile(IN VLDBMP_S *pstBmp, IN CHAR *pszPath);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__VLDBMP_UTL_H_*/


