/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2007-8-15
* Description: 动态扩展空间的buf
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/vbuf_utl.h"

static int _vbuf_resize_double_up_to(VBUF_S *pstVBuf, ULONG min)
{
    ULONG new_size = pstVBuf->ulTotleLen;

    if (new_size == 0) {
        new_size = 1;
    }

    while (new_size < min) {
        new_size  = new_size * 2;
    }

    return VBUF_ExpandTo(pstVBuf, new_size);
}

static int _vbuf_resize_up_to(VBUF_S *pstVBuf, ULONG len)
{
    if (pstVBuf->double_mem) {
        return _vbuf_resize_double_up_to(pstVBuf, len);
    }
    return VBUF_ExpandTo(pstVBuf, len);
}

static int _vbuf_resize_up(VBUF_S *pstVBuf, ULONG len)
{
    return _vbuf_resize_up_to(pstVBuf, pstVBuf->ulTotleLen + len);
}

static int _vbuf_pre_cat(IN VBUF_S *pstVBuf, IN ULONG ulLen)
{
    ULONG ulTailLen;

    ulTailLen = (pstVBuf->ulTotleLen - pstVBuf->ulOffset) - pstVBuf->ulUsedLen;

    if (ulTailLen >= ulLen) {
        return 0;
    }

    if (pstVBuf->ulUsedLen + ulLen <= pstVBuf->ulTotleLen) {
        return VBUF_MoveData(pstVBuf, 0);
    }

    return _vbuf_resize_up_to(pstVBuf, pstVBuf->ulUsedLen + ulLen);
}

VOID VBUF_Init(OUT VBUF_S *pstVBuf)
{
    Mem_Zero(pstVBuf, sizeof(VBUF_S));
}

VOID VBUF_Finit(IN VBUF_S *pstVBuf)
{
    MEM_SafeFree(pstVBuf->pucData);
}

VOID VBUF_SetMemDouble(OUT VBUF_S *pstVBuf, BOOL_T enable)
{
    pstVBuf->double_mem = enable;
}


VOID VBUF_Clear(IN VBUF_S *pstVBuf)
{
    if (! pstVBuf) {
        return;
    }

    if (pstVBuf->pucData) {
        MEM_Free(pstVBuf->pucData);
    }

    Mem_Zero(pstVBuf, sizeof(VBUF_S));
}


void VBUF_ClearData(IN VBUF_S *pstVBuf)
{
    pstVBuf->ulUsedLen = 0;
    pstVBuf->ulOffset = 0;
}


VOID VBUF_SetDataLength(IN VBUF_S *pstVBuf, IN ULONG ulDataLength)
{
    BS_DBGASSERT(0 != pstVBuf);

    pstVBuf->ulUsedLen = ulDataLength;
}


VOID VBUF_AddDataLength(IN VBUF_S *pstVBuf, IN ULONG ulAddDataLength)
{
    BS_DBGASSERT(0 != pstVBuf);

    pstVBuf->ulUsedLen += ulAddDataLength;
}


ULONG VBUF_GetDataLength(IN VBUF_S *pstVBuf)
{
    BS_DBGASSERT(0 != pstVBuf);

    return pstVBuf->ulUsedLen;
}


ULONG VBUF_GetHeadFreeLength(IN VBUF_S *pstVBuf)
{
    return pstVBuf->ulOffset;
}


ULONG VBUF_GetTailFreeLength(IN VBUF_S *pstVBuf)
{
    BS_DBGASSERT(pstVBuf->ulTotleLen >= (pstVBuf->ulOffset + pstVBuf->ulUsedLen));

    return (pstVBuf->ulTotleLen - pstVBuf->ulOffset) - pstVBuf->ulUsedLen;
}


BS_STATUS VBUF_ExpandTo(IN VBUF_S *pstVBuf, IN ULONG ulLen)
{
    UCHAR *pucTmp;
    UINT uiNewTotleLen = ulLen;

    if (uiNewTotleLen <= pstVBuf->ulTotleLen) {
        return BS_OK;
    }

    pucTmp = MEM_MallocAndCopy(pstVBuf->pucData + pstVBuf->ulOffset, pstVBuf->ulUsedLen, uiNewTotleLen);
    if (NULL == pucTmp) {
        RETURN(BS_NO_MEMORY);
    }

    if (pstVBuf->pucData != NULL) {
        MEM_Free(pstVBuf->pucData);
    }

    pstVBuf->ulTotleLen = uiNewTotleLen;
    pstVBuf->pucData = pucTmp;
    pstVBuf->ulOffset = 0;

    return BS_OK;
}


BS_STATUS VBUF_Expand(IN VBUF_S *pstVBuf, IN ULONG ulLen)
{
    UINT uiNewTotleLen;

    uiNewTotleLen = ulLen + pstVBuf->ulTotleLen;

    return VBUF_ExpandTo(pstVBuf, uiNewTotleLen);
}


BS_STATUS VBUF_MoveData(IN VBUF_S *pstVBuf, IN ULONG ulOffset)
{
    if (pstVBuf->ulOffset == ulOffset) {
        return BS_OK;
    }

    if (pstVBuf->ulUsedLen + ulOffset > pstVBuf->ulTotleLen) {
        if (BS_OK != _vbuf_resize_up(pstVBuf, pstVBuf->ulUsedLen + ulOffset - pstVBuf->ulTotleLen)) {
            return BS_NO_MEMORY;
        }
    }

    if (pstVBuf->ulUsedLen > 0) {
        memmove(pstVBuf->pucData + ulOffset, pstVBuf->pucData + pstVBuf->ulOffset, pstVBuf->ulUsedLen);
    }

    pstVBuf->ulOffset = ulOffset;

    return BS_OK;
}


int VBUF_Cut(VBUF_S *vbuf, ULONG offset, ULONG cut_len)
{
    if (offset + cut_len >= vbuf->ulUsedLen) {
        vbuf->ulUsedLen = offset;
        return 0;
    }

    char *data = VBUF_GetData(vbuf);
    char *dst = data + offset;
    char *src = dst + cut_len;
    ULONG src_len = vbuf->ulUsedLen - (offset + cut_len);

    memmove(dst, src, src_len);

    vbuf->ulUsedLen -= cut_len;

    return 0;

}


BS_STATUS VBUF_CutHead(IN VBUF_S *pstVBuf, IN ULONG ulCutLen)
{
    if (ulCutLen < pstVBuf->ulUsedLen) {
        pstVBuf->ulUsedLen -= ulCutLen;
        memmove(pstVBuf->pucData, pstVBuf->pucData + pstVBuf->ulOffset + ulCutLen, pstVBuf->ulUsedLen);
    } else {
        pstVBuf->ulUsedLen = 0;
    }

    pstVBuf->ulOffset = 0;

    return BS_OK;
    
}


BS_STATUS VBUF_EarseHead(IN VBUF_S *pstVBuf, IN ULONG ulCutLen)
{
    BS_DBGASSERT(0 != pstVBuf);

    if (ulCutLen < pstVBuf->ulUsedLen) {
        pstVBuf->ulUsedLen -= ulCutLen;
        pstVBuf->ulOffset = pstVBuf->ulOffset + ulCutLen;
    } else {
        pstVBuf->ulUsedLen = 0;
        pstVBuf->ulOffset = 0;
    }

    return BS_OK;
}

VOID VBUF_CutAll(IN VBUF_S *pstVBuf)
{
    BS_DBGASSERT(0 != pstVBuf);

    pstVBuf->ulUsedLen = 0;
    pstVBuf->ulOffset = 0;
}

BS_STATUS VBUF_CutTail(IN VBUF_S *pstVBuf, IN ULONG ulCutLen)
{
    BS_DBGASSERT(0 != pstVBuf);

    if (ulCutLen < pstVBuf->ulUsedLen)
    {
        pstVBuf->ulUsedLen -= ulCutLen;
    }
    else
    {
        pstVBuf->ulUsedLen = 0;
    }

    return BS_OK;
}

BS_STATUS VBUF_CatFromBuf(IN VBUF_S *pstVBuf, IN VOID *buf, IN ULONG ulLen)
{
    int ret;

    BS_DBGASSERT(0 != pstVBuf);
    BS_DBGASSERT(NULL != buf);

    if (pstVBuf->ulUsedLen == 0) {
        return VBUF_CpyFromBuf(pstVBuf, buf, ulLen);
    }

    ret = _vbuf_pre_cat(pstVBuf, ulLen);
    if (ret < 0) {
        return ret;
    }

    MEM_Copy(pstVBuf->pucData + pstVBuf->ulOffset + pstVBuf->ulUsedLen, buf, ulLen);
    pstVBuf->ulUsedLen += ulLen;

    return BS_OK;
}

BS_STATUS VBUF_CatFromVBuf(IN VBUF_S *pstVBufDst, IN VBUF_S *pstVBufSrc)
{
    BS_DBGASSERT(0 != pstVBufDst);
    BS_DBGASSERT(0 != pstVBufSrc);

    return VBUF_CatFromBuf(pstVBufDst, pstVBufSrc->pucData, pstVBufSrc->ulUsedLen);
}

BS_STATUS VBUF_CpyFromBuf(IN VBUF_S *pstVBuf, IN void *buf, IN ULONG ulLen)
{
    BS_DBGASSERT(0 != pstVBuf);

    if (pstVBuf->ulTotleLen < ulLen) {
        UCHAR *pucTmp = MEM_Malloc(ulLen);
        if (NULL == pucTmp)
        {
            RETURN(BS_NO_MEMORY);
        }

        if (NULL != pstVBuf->pucData)
        {
            MEM_Free(pstVBuf->pucData);
        }

        pstVBuf->ulTotleLen = ulLen;
        pstVBuf->pucData = pucTmp;
    }

    MEM_Copy(pstVBuf->pucData, buf, ulLen);
    pstVBuf->ulUsedLen = ulLen;
    pstVBuf->ulOffset = 0;

    return BS_OK;
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

VOID * VBUF_GetData(IN VBUF_S *pstVBuf)
{
    BS_DBGASSERT(pstVBuf != 0);

    return pstVBuf->pucData + pstVBuf->ulOffset;
}

VOID * VBUF_GetTailFreeBuf(IN VBUF_S *pstVBuf)
{
    return pstVBuf->pucData + pstVBuf->ulOffset + pstVBuf->ulUsedLen;
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

