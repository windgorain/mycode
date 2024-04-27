/******************************************************************************
* Copyright (C), 2000-2006,  LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2017-7-4
* Description: dgram stream: 将stream流,比如tcp,封装成有边界的数据段格式
*   数据格式为: Length+Data, Length为4个字节,表示Data的长度
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/socket_utl.h"
#include "utl/dstream_utl.h"

static BS_STATUS _dstream_WriteSyn(IN DSTREAM_S *pstDStream, IN UCHAR *pucData, IN UINT uiDataLen)
{
    if (BS_OK != Socket_WriteUntilFinish(pstDStream->iFd, (UCHAR*)&uiDataLen, sizeof(UINT), 0))
    {
        return BS_ERR;
    }

    if (BS_OK != Socket_WriteUntilFinish(pstDStream->iFd, pucData, uiDataLen, 0))
    {
        return BS_ERR;
    }

    return BS_OK;
}

static BS_STATUS _dstream_WriteVBuf(IN DSTREAM_S *pstDStream)
{
    INT iRet;
    UINT uiSendLen;

    iRet = Socket_Write(pstDStream->iFd, VBUF_GetData(&pstDStream->stWriteVBuf), VBUF_GetDataLength(&pstDStream->stWriteVBuf), 0);
    if (iRet == SOCKET_E_AGAIN)
    {
        uiSendLen = 0;
    }
    else if (iRet < 0)
    {
        return BS_ERR;
    }
    else
    {
        uiSendLen = (UINT)iRet;
    }

    VBUF_EarseHead(&pstDStream->stWriteVBuf, uiSendLen);

    return BS_OK;
}

static BS_STATUS _dstream_WriteOrSave(IN DSTREAM_S *pstDStream, IN UCHAR *pucData, IN UINT uiDataLen)
{
    INT iRet;
    UINT uiSendLen;

    iRet = Socket_Write(pstDStream->iFd, pucData, uiDataLen, 0);
    if (iRet == SOCKET_E_AGAIN)
    {
        uiSendLen = 0;
    }
    else if (iRet < 0)
    {
        return BS_ERR;
    }
    else
    {
        uiSendLen = (UINT)iRet;
    }

    if (uiSendLen < uiDataLen)
    {
        VBUF_CatBuf(&pstDStream->stWriteVBuf, pucData, uiDataLen);
    }

    return BS_OK;
}

static BS_STATUS _dstream_WriteAsyn(IN DSTREAM_S *pstDStream, IN UCHAR *pucData, IN UINT uiDataLen)
{
    if (VBUF_GetDataLength(&pstDStream->stWriteVBuf) > 0)
    {
        _dstream_WriteVBuf(pstDStream);

        if (VBUF_GetDataLength(&pstDStream->stWriteVBuf) > 0)
        {
            return BS_AGAIN;
        }
    }
    
    if (BS_OK != _dstream_WriteOrSave(pstDStream, (UCHAR*)&uiDataLen, sizeof(UINT)))
    {
        return BS_ERR;
    }

    if (BS_OK != _dstream_WriteOrSave(pstDStream, pucData, uiDataLen))
    {
        return BS_ERR;
    }

    return BS_OK;
}

static BS_STATUS _dstream_ReadSyn(IN DSTREAM_S *pstDStream)
{
    INT iLen;
    UINT uiDataLen;

    while (pstDStream->uiRemainReadLen > 0)
    {
        iLen = Socket_Read(pstDStream->iFd, VBUF_GetTailFreeBuf(&pstDStream->stReadVBuf), pstDStream->uiRemainReadLen, 0);
        if (iLen < 0)
        {
            return BS_ERR;
        }
        if (iLen == 0)
        {
            return BS_PEER_CLOSED;
        }

        pstDStream->uiRemainReadLen -= iLen;

        if (pstDStream->enState == DSTREAM_RECV_STATE_HEAD)
        {
            pstDStream->enState = DSTREAM_RECV_STATE_DATA;
            uiDataLen = *((UINT*)VBUF_GetData(&pstDStream->stReadVBuf));
            pstDStream->uiRemainReadLen = ntohl(uiDataLen);
            VBUF_EarseHead(&pstDStream->stReadVBuf, sizeof(UINT));
            if (BS_OK != VBUF_ExpandTo(&pstDStream->stReadVBuf, pstDStream->uiRemainReadLen))
            {
                return BS_NO_MEMORY;
            }
        }
    }

    pstDStream->enState = DSTREAM_RECV_STATE_HEAD;
    pstDStream->uiRemainReadLen = 4;

    return BS_OK;
}

static BS_STATUS _dstream_ReadAsyn(IN DSTREAM_S *pstDStream)
{
    INT iLen;
    UINT uiDataLen;

    while (pstDStream->uiRemainReadLen > 0)
    {
        iLen = Socket_Read(pstDStream->iFd, VBUF_GetTailFreeBuf(&pstDStream->stReadVBuf), pstDStream->uiRemainReadLen, 0);
        if (iLen == SOCKET_E_AGAIN)
        {
            return BS_AGAIN;
        }
        if (iLen < 0)
        {
            return BS_ERR;
        }
        if (iLen == 0)
        {
            return BS_PEER_CLOSED;
        }

        pstDStream->uiRemainReadLen -= iLen;

        if (pstDStream->enState == DSTREAM_RECV_STATE_HEAD)
        {
            pstDStream->enState = DSTREAM_RECV_STATE_DATA;
            uiDataLen = *((UINT*)VBUF_GetData(&pstDStream->stReadVBuf));
            pstDStream->uiRemainReadLen = ntohl(uiDataLen);
            VBUF_EarseHead(&pstDStream->stReadVBuf, sizeof(UINT));
            if (BS_OK != VBUF_ExpandTo(&pstDStream->stReadVBuf, pstDStream->uiRemainReadLen))
            {
                return BS_NO_MEMORY;
            }
        }
    }

    pstDStream->enState = DSTREAM_RECV_STATE_HEAD;
    pstDStream->uiRemainReadLen = 4;

    return BS_OK;
}


BS_STATUS DSTREAM_Init(IN DSTREAM_S *pstDStream)
{
    memset(pstDStream, 0, sizeof(DSTREAM_S));

    VBUF_Init(&pstDStream->stWriteVBuf);
    VBUF_Init(&pstDStream->stReadVBuf);

    if (BS_OK != VBUF_ExpandTo(&pstDStream->stReadVBuf, 256))    
    {
        return BS_ERR;
    }

    return BS_OK;
}

VOID DSTREAM_Finit(IN DSTREAM_S *pstDStream)
{
    if (NULL != pstDStream)
    {
        VBUF_Finit(&pstDStream->stWriteVBuf);
        VBUF_Finit(&pstDStream->stReadVBuf);
    }
}

VOID DSTREAM_SetFd(IN DSTREAM_S *pstDStream, IN INT iFd)
{
    pstDStream->iFd = iFd;
}

BS_STATUS DSTREAM_Write(IN DSTREAM_S *pstDStream, IN UCHAR *pucData, IN UINT uiDataLen)
{
    if (pstDStream->bitIsBlock)
    {
        return _dstream_WriteSyn(pstDStream, pucData, uiDataLen);
    }
    else
    {
        return _dstream_WriteAsyn(pstDStream, pucData, uiDataLen);
    }
}


BS_STATUS DSTREAM_Read(IN DSTREAM_S *pstDStream)
{
    if (pstDStream->bitIsBlock)
    {
        return _dstream_ReadSyn(pstDStream);
    }
    else
    {
        return _dstream_ReadAsyn(pstDStream);
    }
}


VBUF_S * DSTREAM_GetData(IN DSTREAM_S *pstDStream)
{
    return &pstDStream->stReadVBuf;
}

