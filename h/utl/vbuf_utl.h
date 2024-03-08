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
#endif 

typedef struct {
    ULONG ulTotleLen;
    ULONG ulUsedLen;
    ULONG ulOffset;  
    UINT  double_mem: 1; 
    UCHAR *pucData;
}VBUF_S;

extern VOID VBUF_Init(IN VBUF_S *pstVBuf);
extern VOID VBUF_Finit(IN VBUF_S *pstVBuf);
extern VOID VBUF_SetMemDouble(OUT VBUF_S *pstVBuf, BOOL_T enable);

extern VOID VBUF_Clear(IN VBUF_S *pstVBuf);
void VBUF_ClearData(IN VBUF_S *pstVBuf);

VOID VBUF_SetDataLength(IN VBUF_S *pstVBuf, IN ULONG ulDataLength);

VOID VBUF_AddDataLength(IN VBUF_S *pstVBuf, IN ULONG ulAddDataLength);
extern ULONG VBUF_GetDataLength(IN VBUF_S *pstVBuf);

ULONG VBUF_GetHeadFreeLength(IN VBUF_S *pstVBuf);

ULONG VBUF_GetTailFreeLength(IN VBUF_S *pstVBuf);

BS_STATUS VBUF_ExpandTo(IN VBUF_S *pstVBuf, IN ULONG ulLen);

BS_STATUS VBUF_Expand(IN VBUF_S *pstVBuf, IN ULONG ulLen);

BS_STATUS VBUF_MoveData(IN VBUF_S *pstVBuf, IN ULONG ulOffset);

int VBUF_Cut(VBUF_S *vbuf, ULONG offset, ULONG cut_len);

extern BS_STATUS VBUF_CutHead(IN VBUF_S *pstVBuf, IN ULONG ulCutLen);

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
extern VOID * VBUF_GetTailFreeBuf(IN VBUF_S *pstVBuf);
extern long VBUF_Ptr2Offset(VBUF_S *vbuf, void *ptr);
extern int VBUF_WriteFile(char *filename, VBUF_S *vbuf);
extern int VBUF_ReadFP(void *fp, OUT VBUF_S *vbuf);
extern int VBUF_ReadFile(char *filename, OUT VBUF_S *vbuf);

#ifdef __cplusplus
    }
#endif 

#endif 


