/******************************************************************************
* Copyright (C), LiXingang
* Author:      lixingang  Version: 1.0  Date: 2012-2-8
* Description: 缓冲器. 缓冲到满或者Flush时触发回调函数调用.
* History:     
******************************************************************************/

#include "bs.h"

#include "utl/txt_utl.h"
#include "utl/buffer_utl.h"

typedef struct
{
    PF_BUFFER_OUT_FUNC pfFunc;
    USER_HANDLE_S stUserHandle;
    UINT uiBufferSize;
    UINT uiDataLen;
    UCHAR aucData[0];
}BUFFER_S;

static inline VOID buffer_FlushBuffer(IN BUFFER_S *pstBuffer)
{
    UINT uiDataLen;

    pstBuffer->aucData[pstBuffer->uiDataLen] = '\0';
    uiDataLen = pstBuffer->uiDataLen;
    pstBuffer->uiDataLen = 0;
    if (NULL != pstBuffer->pfFunc)
    {
        pstBuffer->pfFunc(pstBuffer->aucData, uiDataLen, &pstBuffer->stUserHandle);
    }
}

BUFFER_HANDLE BUFFER_Create(IN UINT uiBufferSize,
        IN PF_BUFFER_OUT_FUNC pfFunc, IN USER_HANDLE_S *pstUserHandle)
{
    BUFFER_S *pstBuffer;

    pstBuffer = MEM_Malloc(sizeof(BUFFER_S) + uiBufferSize + 1);  /* +1是给\0留一个位置 */
    if (NULL == pstBuffer)
    {
        return NULL;
    }

    Mem_Zero(pstBuffer, sizeof(BUFFER_S));

    pstBuffer->pfFunc = pfFunc;
    pstBuffer->stUserHandle = *pstUserHandle;
    pstBuffer->uiBufferSize = uiBufferSize;

    return (BUFFER_HANDLE)pstBuffer;
}

VOID BUFFER_Destory(IN BUFFER_HANDLE hBuffer)
{
    if (NULL != hBuffer)
    {
        MEM_Free(hBuffer);
    }
}

BS_STATUS BUFFER_Write(IN BUFFER_HANDLE hBuffer, IN VOID *pData, IN UINT uiDataLen)
{
    BUFFER_S *pstBuffer = hBuffer;

    if (hBuffer == NULL)
    {
        return BS_ERR;
    }

    if (pstBuffer->uiDataLen > 0)
    {
        if (uiDataLen + pstBuffer->uiDataLen >= pstBuffer->uiBufferSize)
        {
            buffer_FlushBuffer(pstBuffer);
        }
    }

    if (uiDataLen >= pstBuffer->uiBufferSize)
    {
        pstBuffer->pfFunc(pData, uiDataLen, &pstBuffer->stUserHandle);
    }
    else
    {
        memcpy(pstBuffer->aucData + pstBuffer->uiDataLen, pData, uiDataLen);
        pstBuffer->uiDataLen += uiDataLen;
    }

    return BS_OK;
}

BS_STATUS BUFFER_WriteString(IN BUFFER_HANDLE hBuffer, IN CHAR *pcData)
{
    if (hBuffer == NULL)
    {
        return BS_ERR;
    }

    return BUFFER_Write(hBuffer, pcData, strlen(pcData));
}

static VOID _buffer_Print(IN CHAR *pcMsg, BUFFER_HANDLE hBuffer)
{
     BUFFER_Write(hBuffer, pcMsg, strlen(pcMsg));
}

BS_STATUS BUFFER_Print(IN BUFFER_HANDLE hBuffer, IN CHAR *pcFmt, ...)
{
    if (hBuffer == NULL)
    {
        return BS_ERR;
    }

    TXT_ARGS_PRINT(_buffer_Print, hBuffer);

    return BS_OK;
}

BS_STATUS BUFFER_Flush(IN BUFFER_HANDLE hBuffer)
{
    BUFFER_S *pstBuffer = hBuffer;

    if (hBuffer == NULL)
    {
        return BS_ERR;
    }

    if (pstBuffer->uiDataLen > 0)
    {
        buffer_FlushBuffer(pstBuffer);
    }

    return BS_OK;
}

