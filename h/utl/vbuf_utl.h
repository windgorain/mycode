/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2007-8-15
* Description: 
* History:     
******************************************************************************/

#ifndef __VBUF_UTL_H_
#define __VBUF_UTL_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

typedef struct {
    ULONG ulTotleLen;
    ULONG ulUsedLen;
    ULONG ulOffset;  /* 数据的起始偏移 */
    UINT  double_mem: 1; /* 0: 节省内存模式, 用多少申请多少. 1: 按照*2来自动扩展 */
    UCHAR *pucData;
}VBUF_S;

extern VOID VBUF_Init(IN VBUF_S *pstVBuf);
extern VOID VBUF_Finit(IN VBUF_S *pstVBuf);
extern VOID VBUF_SetMemDouble(OUT VBUF_S *pstVBuf, BOOL_T enable);
/* 清空数据并释放对应内存 */
extern VOID VBUF_Clear(IN VBUF_S *pstVBuf);
/* 设置VBuf中的数据区长度 */
VOID VBUF_SetDataLength(IN VBUF_S *pstVBuf, IN ULONG ulDataLength);
/* 增加VBuf中的数据区长度 */
VOID VBUF_AddDataLength(IN VBUF_S *pstVBuf, IN ULONG ulAddDataLength);
extern ULONG VBUF_GetDataLength(IN VBUF_S *pstVBuf);
/* 获取头部的空闲区长度 */
ULONG VBUF_GetHeadFreeLength(IN VBUF_S *pstVBuf);
/* 获取尾部的空闲区长度 */
ULONG VBUF_GetTailFreeLength(IN VBUF_S *pstVBuf);
/* 扩展空间,将空间加大到ulLen字节 */
BS_STATUS VBUF_ExpandTo(IN VBUF_S *pstVBuf, IN ULONG ulLen);
/* 扩展空间,将空间加大ulLen字节 */
BS_STATUS VBUF_Expand(IN VBUF_S *pstVBuf, IN ULONG ulLen);
/* 将数据移动到Offset位置 */
BS_STATUS VBUF_MoveData(IN VBUF_S *pstVBuf, IN ULONG ulOffset);
/* 砍掉头部数据,并将后面数据移动到开始位置 */
extern BS_STATUS VBUF_CutHead(IN VBUF_S *pstVBuf, IN ULONG ulCutLen);
/* 擦除头部数据，并不会移动数据 */
extern BS_STATUS VBUF_EarseHead(IN VBUF_S *pstVBuf, IN ULONG ulCutLen);
extern VOID VBUF_CutAll(IN VBUF_S *pstVBuf);
extern BS_STATUS VBUF_CutTail(IN VBUF_S *pstVBuf, IN ULONG ulCutLen);
extern BS_STATUS VBUF_CatFromBuf(IN VBUF_S *pstVBuf, IN void *buf, IN ULONG ulLen);
extern BS_STATUS VBUF_CatFromVBuf(IN VBUF_S *pstVBufDst, IN VBUF_S *pstVBufSrc);
extern BS_STATUS VBUF_CpyFromBuf(IN VBUF_S *pstVBuf, IN void *buf, IN ULONG ulLen);
extern BS_STATUS VBUF_CpyFromVBuf(IN VBUF_S *pstVBufDst, IN VBUF_S *pstVBufSrc);
extern INT VBUF_CmpByBuf(IN VBUF_S *pstVBuf, IN void *buf, IN ULONG ulLen);
extern INT VBUF_CmpByVBuf(IN VBUF_S *pstVBuf1, IN VBUF_S *pstVBuf2);
extern VOID * VBUF_GetData(IN VBUF_S *pstVBuf);
VOID * VBUF_GetTailFreeBuf(IN VBUF_S *pstVBuf);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__VBUF_UTL_H_*/


