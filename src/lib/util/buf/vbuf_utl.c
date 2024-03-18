/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2007-8-15
* Description: 动态扩展空间的buf
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/vbuf_utl.h"



int VBUF_AddHead(INOUT VBUF_S *vbuf, ULONG len)
{
    if (vbuf->ulOffset < len) {
        int ret = VBUF_MoveData(vbuf, len);
        if (ret < 0) {
            return ret;
        }
    }

    vbuf->ulOffset -= len;
    vbuf->ulUsedLen += len;

    return 0;
}


int VBUF_AddHeadBuf(INOUT VBUF_S *vbuf, void *buf, ULONG len)
{
    int ret;

    ret = VBUF_AddHead(vbuf, len);
    if (ret < 0) {
        return ret;
    }

    void *d = VBUF_GetData(vbuf);
    memcpy(d, buf, len);

    return 0;
}

BS_STATUS VBUF_CatFromVBuf(IN VBUF_S *pstVBufDst, IN VBUF_S *pstVBufSrc)
{
    BS_DBGASSERT(0 != pstVBufDst);
    BS_DBGASSERT(0 != pstVBufSrc);

    return VBUF_CatFromBuf(pstVBufDst, pstVBufSrc->pucData, pstVBufSrc->ulUsedLen);
}

BS_STATUS VBUF_CpyFromVBuf(IN VBUF_S *pstVBufDst, IN VBUF_S *pstVBufSrc)
{
    BS_DBGASSERT(0 != pstVBufDst);
    BS_DBGASSERT(0 != pstVBufSrc);

    return VBUF_CpyFromBuf(pstVBufDst, pstVBufSrc->pucData, pstVBufSrc->ulUsedLen);
}

INT VBUF_CmpByBuf(IN VBUF_S *pstVBuf, IN void *buf, IN ULONG ulLen)
{
    BS_DBGASSERT(0 != pstVBuf);

    return memcmp(pstVBuf->pucData + pstVBuf->ulOffset, buf, MIN(pstVBuf->ulUsedLen, ulLen));
}

INT VBUF_CmpByVBuf(IN VBUF_S *pstVBuf1, IN VBUF_S *pstVBuf2)
{
    if ((pstVBuf1 == 0) && (pstVBuf2 == 0))
    {
        return 0;
    }
    
    if (pstVBuf1 == 0) 
    {
        return -1;
    }

    if (pstVBuf2 == 0)
    {
        return 1;
    }

    return VBUF_CmpByBuf(pstVBuf1, pstVBuf2->pucData, pstVBuf2->ulUsedLen);
}


long VBUF_Ptr2Offset(VBUF_S *vbuf, void *ptr)
{
    long offset;
    char *data = VBUF_GetData(vbuf);
    char *tmp = ptr;

    if (data > tmp) {
        return -1;
    }

    offset = tmp - data;
    if (offset >= vbuf->ulUsedLen) {
        return -1;
    }

    return offset;
}

