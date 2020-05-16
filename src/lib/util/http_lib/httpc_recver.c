/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-6-23
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/vbuf_utl.h"
#include "utl/lstr_utl.h"
#include "utl/http_lib.h"

#define HTTPC_RECVER_MAX_HEAD_SIZE 2048

typedef enum
{
    HTTPC_RECVER_STATE_HEAD = 0,
    HTTPC_RECVER_STATE_BODY,
    HTTPC_RECVER_STATE_FINISH
}HTTPC_RECVER_STATE_E;


typedef struct
{
    HTTPC_RECVER_PARAM_S stParam;
    HTTPC_RECVER_STATE_E enState;
    VBUF_S stVBuf;
    HTTP_HEAD_PARSER hHttpParser;
    HTTP_BODY_TRAN_TYPE_E eTransType;
    UINT64 uiRemainLen; /* 剩余体长度,或者是当前trunk块当前剩余长度 */
}HTTPC_RECVER_S;

static INT httpcrecver_ReadBodyOfVBuf(IN HTTPC_RECVER_S *pstRecver, IN UCHAR *pucBuf, IN UINT uiMaxReadLen)
{
    UINT uiDataLen;
    INT uiLen = 0;
    
    uiDataLen = VBUF_GetDataLength(&pstRecver->stVBuf);
    if (uiDataLen > 0)
    {
        uiLen = MIN(uiDataLen, uiMaxReadLen);
        memcpy(pucBuf, VBUF_GetData(&pstRecver->stVBuf), uiLen);
        VBUF_CutHead(&pstRecver->stVBuf, uiLen);
    }

    return uiLen;
}

static INT httpcrecver_ReadBodyByLen(IN HTTPC_RECVER_S *pstRecver, IN UCHAR *pucBuf, IN UINT uiLen)
{
    INT iLen;

    iLen = httpcrecver_ReadBodyOfVBuf(pstRecver, pucBuf, uiLen);
    if (iLen > 0)
    {
        return iLen;
    }

    iLen = pstRecver->stParam.pfRecv(pstRecver->stParam.ulFd, pucBuf, uiLen);
    if (iLen == HTTPC_E_PEER_CLOSED)
    {
        pstRecver->enState = HTTPC_RECVER_STATE_FINISH;
        return HTTPC_E_PEER_CLOSED;
    }

    return iLen;
}

/* 处理Content-length模式的体数据 */
static INT httpcrecver_ReadBodyTypeLength(IN HTTPC_RECVER_S *pstRecver, IN UCHAR *pucBuf, IN UINT uiBufSize)
{
    INT iLen;
    
    if (pstRecver->uiRemainLen == 0)
    {
        pstRecver->enState = HTTPC_RECVER_STATE_FINISH;
        return HTTPC_E_FINISH;
    }

    iLen = httpcrecver_ReadBodyByLen(pstRecver, pucBuf, MIN(uiBufSize, (UINT)pstRecver->uiRemainLen));
    if (iLen > 0)
    {
        pstRecver->uiRemainLen -= iLen;
        if (pstRecver->uiRemainLen == 0)
        {
            pstRecver->enState = HTTPC_RECVER_STATE_FINISH;
        }
    }

    return iLen;
}

static INT httpcrecver_ReadBodyTypeClosed(IN HTTPC_RECVER_S *pstRecver, IN UCHAR *pucBuf, IN UINT uiBufSize)
{
    INT iLen;

    iLen = httpcrecver_ReadBodyOfVBuf(pstRecver, pucBuf, uiBufSize);
    if (iLen > 0)
    {
        return iLen;
    }

    iLen = pstRecver->stParam.pfRecv(pstRecver->stParam.ulFd, pucBuf, uiBufSize);
    if (iLen == HTTPC_E_PEER_CLOSED)
    {
        pstRecver->enState = HTTPC_RECVER_STATE_FINISH;
        return HTTPC_E_FINISH;
    }

    return iLen;
}

static BOOL_T httpcrecver_ParseChunkedFlag(IN HTTPC_RECVER_S *pstRecver)
{
    LSTR_S stLstr;
    CHAR *pcFind;
    UINT uiNum;
    
    stLstr.pcData = VBUF_GetData(&pstRecver->stVBuf);
    stLstr.uiLen = VBUF_GetDataLength(&pstRecver->stVBuf);

    pcFind = LSTR_Strstr(&stLstr, "\r\n");
    if (NULL == pcFind)
    {
        return FALSE;
    }

    stLstr.uiLen = pcFind - stLstr.pcData;    
    LSTR_XAtoui(&stLstr, &uiNum);
    VBUF_CutHead(&pstRecver->stVBuf, stLstr.uiLen + 2);

    pstRecver->uiRemainLen = uiNum;

    if (pstRecver->uiRemainLen == 0)
    {
        pstRecver->enState = HTTPC_RECVER_STATE_FINISH;
    }

    return TRUE;
}

static BS_STATUS httpcrecver_tryChunkedLine(IN HTTPC_RECVER_S *pstRecver)
{
    INT iLen;

    VBUF_MoveData(&pstRecver->stVBuf, 0);

    if (TRUE == httpcrecver_ParseChunkedFlag(pstRecver))
    {
        return BS_OK;
    }

    if (VBUF_GetDataLength(&pstRecver->stVBuf) >= 32)
    {
        return BS_ERR;
    }

    iLen = pstRecver->stParam.pfRecv(pstRecver->stParam.ulFd, VBUF_GetTailFreeBuf(&pstRecver->stVBuf), 32);
    if (iLen < 0)
    {
        if (iLen == HTTPC_E_AGAIN)
        {
            return BS_NOT_COMPLETE;
        }

        return BS_ERR;
    }

    VBUF_AddDataLength(&pstRecver->stVBuf, iLen);

    if (TRUE == httpcrecver_ParseChunkedFlag(pstRecver))
    {
        return BS_OK;
    }

    if (VBUF_GetDataLength(&pstRecver->stVBuf) >= 32)
    {
        return BS_ERR;
    }
    
    return BS_NOT_COMPLETE;
}

static INT httpcrecver_ReadBodyTypeChunked(IN HTTPC_RECVER_S *pstRecver, IN UCHAR *pucBuf, IN UINT uiBufSize)
{
    INT iLen;
    BS_STATUS eRet;
    
    if (pstRecver->uiRemainLen == 0)
    {
        eRet = httpcrecver_tryChunkedLine(pstRecver);
        if (eRet == BS_NOT_COMPLETE)
        {
            return HTTPC_E_AGAIN;
        }
        if (eRet != BS_OK)
        {
            return HTTPC_E_ERR;
        }
        if (pstRecver->uiRemainLen == 0)
        {
            return HTTPC_E_FINISH;
        }
    }

    iLen = httpcrecver_ReadBodyByLen(pstRecver, pucBuf, (UINT)pstRecver->uiRemainLen);
    if (iLen > 0)
    {
        pstRecver->uiRemainLen -= iLen;
    }

    return iLen;
}

static INT httpcrecver_ReadBody(IN HTTPC_RECVER_S *pstRecver, IN UCHAR *pucBuf, IN UINT uiBufSize)
{
    INT iRet = HTTPC_E_ERR;

    switch(pstRecver->eTransType)
    {
        case HTTP_BODY_TRAN_TYPE_CONTENT_LENGTH:
        {
            iRet = httpcrecver_ReadBodyTypeLength(pstRecver, pucBuf, uiBufSize);
            break;
        }

        case HTTP_BODY_TRAN_TYPE_CLOSED:
        {
            iRet = httpcrecver_ReadBodyTypeClosed(pstRecver, pucBuf, uiBufSize);
            break;
        }

        case HTTP_BODY_TRAN_TYPE_CHUNKED:
        {
            iRet = httpcrecver_ReadBodyTypeChunked(pstRecver, pucBuf, uiBufSize);
            break;
        }

        default:
        {
            break;
        }
    }

    return iRet;
}

static BS_STATUS httpcrecver_ParseTransType(IN HTTPC_RECVER_S *pstRecver)
{
    pstRecver->eTransType = HTTP_GetBodyTranType(pstRecver->hHttpParser);
    if (pstRecver->eTransType == HTTP_BODY_TRAN_TYPE_NONE)
    {
        return BS_ERR;
    }

    if (pstRecver->eTransType == HTTP_BODY_TRAN_TYPE_CONTENT_LENGTH)
    {
        if (BS_OK != HTTP_GetContentLen(pstRecver->hHttpParser, &pstRecver->uiRemainLen))
        {
            return BS_ERR;
        }
    }

    return BS_OK;
}

static INT httpcrecver_ReadHead(IN HTTPC_RECVER_S *pstRecver, IN UCHAR *pucBuf, IN UINT uiBufSize)
{
    UCHAR *pucData;
    UINT uiDataLen;
    INT iRet;
    UINT uiHeadLen;
    UCHAR *pucTmpBuf;
    ULONG ulBufSize;

    pucTmpBuf = VBUF_GetTailFreeBuf(&pstRecver->stVBuf);
    ulBufSize = VBUF_GetTailFreeLength(&pstRecver->stVBuf);
    if (ulBufSize == 0)
    {
        return HTTPC_E_ERR;
    }

    iRet = pstRecver->stParam.pfRecv(pstRecver->stParam.ulFd, pucTmpBuf, ulBufSize);
    if (iRet < 0) {
        if (iRet == HTTPC_E_PEER_CLOSED) {
            return HTTPC_E_ERR;
        }
        return iRet;
    }

    VBUF_AddDataLength(&pstRecver->stVBuf, iRet);

    pucData = VBUF_GetData(&pstRecver->stVBuf);
    uiDataLen = VBUF_GetDataLength(&pstRecver->stVBuf);

    uiHeadLen = HTTP_GetHeadLen((void*)pucData, uiDataLen);
    if (uiHeadLen == 0) {
        return HTTPC_E_AGAIN;
    }

    if (BS_OK != HTTP_ParseHead(pstRecver->hHttpParser,
                (void*)pucData, uiHeadLen, HTTP_RESPONSE)) {
        return HTTPC_E_ERR;
    }

    if (BS_OK != httpcrecver_ParseTransType(pstRecver)) {
        return HTTPC_E_ERR;
    }

    VBUF_CutHead(&pstRecver->stVBuf, uiHeadLen);

    return httpcrecver_ReadBody(pstRecver, pucBuf, uiBufSize);
}

HTTPC_RECVER_HANDLE HttpcRecver_Create(IN HTTPC_RECVER_PARAM_S *pstParam)
{
    HTTPC_RECVER_S *pstRecver;

    pstRecver = MEM_ZMalloc(sizeof(HTTPC_RECVER_S));
    if (NULL == pstRecver)
    {
        return NULL;
    }

    VBUF_Init(&pstRecver->stVBuf);
    if (BS_OK != VBUF_Expand(&pstRecver->stVBuf, HTTPC_RECVER_MAX_HEAD_SIZE))
    {
        VBUF_Finit(&pstRecver->stVBuf);
        MEM_Free(pstRecver);
        return NULL;
    }

    pstRecver->hHttpParser = HTTP_CreateHeadParser();
    if (NULL == pstRecver->hHttpParser)
    {
        VBUF_Finit(&pstRecver->stVBuf);
        MEM_Free(pstRecver);
        return NULL;
    }

    pstRecver->stParam = *pstParam;

    return pstRecver;
}

VOID HttpcRecver_Destroy(IN HTTPC_RECVER_HANDLE hRecver)
{
    HTTPC_RECVER_S *pstRecver = hRecver;

    if (NULL != pstRecver->hHttpParser)
    {
        HTTP_DestoryHeadParser(pstRecver->hHttpParser);
    }

    VBUF_Finit(&pstRecver->stVBuf);

    MEM_Free(pstRecver);
}

INT HttpcRecver_Read(IN HTTPC_RECVER_HANDLE hRecver, IN UCHAR *pucBuf, IN UINT uiBufSize)
{
    HTTPC_RECVER_S *pstRecver = hRecver;
    INT iRet;

    if (pstRecver == NULL)
    {
        return HTTPC_E_ERR;
    }

    switch (pstRecver->enState)
    {
        case HTTPC_RECVER_STATE_HEAD:
        {
            iRet = httpcrecver_ReadHead(pstRecver, pucBuf, uiBufSize);
            break;
        }

        case HTTPC_RECVER_STATE_BODY:
        {
            iRet = httpcrecver_ReadBody(pstRecver, pucBuf, uiBufSize);
            break;
        }

        case HTTPC_RECVER_STATE_FINISH:
        {
            iRet = HTTPC_E_FINISH;
            break;
        }

        default:
        {
            iRet = HTTPC_E_ERR;
            break;
        }
    }

    return iRet;
}

HTTP_HEAD_PARSER HttpcRecver_GetHttpParser(IN HTTPC_RECVER_HANDLE hRecver)
{
    HTTPC_RECVER_S *pstRecver = hRecver;

    return pstRecver->hHttpParser;
}

