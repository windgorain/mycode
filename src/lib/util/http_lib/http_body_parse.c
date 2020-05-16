/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2015-11-21
* Description: 
* History:     
******************************************************************************/

#include "bs.h"

#include "utl/http_lib.h"


#define HTTP_BODY_PARSE_READ_SIZE 512

typedef struct
{
    HTTP_HEAD_PARSER hHeadParser;
    HTTP_BODY_TRAN_TYPE_E eTranType;
    UINT uiRemainLen;   /* 剩余多少数据未读,当eTranType==HTTP_BODY_TRAN_TYPE_CONTENT_LENGTH时有效 */
    HTTP_CHUNK_HANDLE hChunkParser; /* 当eTranType==HTTP_BODY_TRAN_TYPE_CHUNKED时有效 */
    BOOL_T bIsFinish;   /* 处理Body完成 */

    PF_HTTP_BODY_FUNC pfBodyFunc;
    USER_HANDLE_S stUserHandle;
}HTTP_BODY_PSRSER_S;

static BS_STATUS http_body_ChunkDate(IN UCHAR *pucData, IN UINT uiDataLen, IN VOID *pUserPointer)
{
    USER_HANDLE_S *pstUserPointer = pUserPointer;
    HTTP_BODY_PSRSER_S *pstBodyParser = pstUserPointer->ahUserHandle[0];

    return pstBodyParser->pfBodyFunc(pucData, uiDataLen, &pstBodyParser->stUserHandle);
}

static BS_STATUS http_body_ParseByContentLength
(
    IN HTTP_BODY_PSRSER_S *pstParser,
    IN UCHAR *pucData,
    IN UINT uiDataLen,
    OUT UINT *puiParsedLen
)
{
    UINT uiLen;
    BS_STATUS eRet;

    if (pstParser->uiRemainLen == 0) {
        pstParser->bIsFinish = TRUE;
        return BS_OK;
    }

    uiLen = MIN(uiDataLen, pstParser->uiRemainLen);

    eRet = pstParser->pfBodyFunc(pucData, uiLen, &pstParser->stUserHandle);
    if (eRet != BS_OK) {
        return eRet;
    }

    pstParser->uiRemainLen -= uiLen;
    *puiParsedLen = uiLen;

    if (pstParser->uiRemainLen > 0) {
        return BS_NOT_COMPLETE;
    }

    pstParser->bIsFinish = TRUE;
    return BS_OK;
}

static BS_STATUS http_body_ParseByClosed
(
    IN HTTP_BODY_PSRSER_S *pstParser,
    IN UCHAR *pucData,
    IN UINT uiDataLen,
    OUT UINT *puiParsedLen
)
{
    BS_STATUS eRet;

    if (pstParser->bIsFinish)
    {
        return BS_OK;
    }

    if (uiDataLen == 0)
    {
        pstParser->bIsFinish = TRUE;
        return BS_OK;
    }

    eRet = pstParser->pfBodyFunc(pucData, uiDataLen, &pstParser->stUserHandle);
    if (eRet != BS_OK)
    {
        return eRet;
    }

    *puiParsedLen = uiDataLen;

    return BS_NOT_COMPLETE;
}

static BS_STATUS http_body_ParseByChunked
(
    IN HTTP_BODY_PSRSER_S *pstParser,
    IN UCHAR *pucData,
    IN UINT uiDataLen,
    OUT UINT *puiParsedLen
)
{
    BS_STATUS eRet;

    if (pstParser->bIsFinish)
    {
        return BS_OK;
    }

    eRet = HTTP_Chunk_Parse(pstParser->hChunkParser, pucData, uiDataLen, puiParsedLen);
    if (eRet == BS_NOT_COMPLETE)
    {
        return BS_NOT_COMPLETE;
    }

    if (BS_OK == eRet)
    {
        pstParser->bIsFinish = TRUE;
    }

    return eRet;
}


HTTP_BODY_PARSER HTTP_BODY_CreateParser
(
    IN HTTP_HEAD_PARSER hHeadParser,
    IN PF_HTTP_BODY_FUNC pfBodyFunc,
    IN USER_HANDLE_S *pstUserHandle
)
{
    HTTP_BODY_TRAN_TYPE_E eTranType;
    UINT64 uiContentLen = 0;
    BOOL_T bIsFinish = FALSE;
    HTTP_CHUNK_HANDLE hChunkParser = NULL;
    HTTP_BODY_PSRSER_S *pstBodyParser;
    USER_HANDLE_S stUserHandle;

    eTranType = HTTP_GetBodyTranType(hHeadParser);
    if (HTTP_BODY_TRAN_TYPE_NONE == eTranType) {
        return NULL;
    }

    if (eTranType == HTTP_BODY_TRAN_TYPE_CONTENT_LENGTH) {
        if (BS_OK != HTTP_GetContentLen(hHeadParser, &uiContentLen)) {
            uiContentLen = 0;
        }

        if (uiContentLen == 0) {
            bIsFinish = TRUE;
        }
    }

    pstBodyParser = MEM_ZMalloc(sizeof(HTTP_BODY_PSRSER_S));
    if (NULL == pstBodyParser) {
        return NULL;
    }

    if (eTranType == HTTP_BODY_TRAN_TYPE_CHUNKED) {
        stUserHandle.ahUserHandle[0] = pstBodyParser;

        hChunkParser = HTTP_Chunk_CreateParser(http_body_ChunkDate, &stUserHandle);
        if (NULL == hChunkParser) {
            MEM_Free(pstBodyParser);
            return NULL;
        }
    }

    pstBodyParser->hHeadParser = hHeadParser;
    pstBodyParser->eTranType = eTranType;
    pstBodyParser->uiRemainLen = (UINT)uiContentLen;
    pstBodyParser->hChunkParser = hChunkParser;
    pstBodyParser->bIsFinish = bIsFinish;
    pstBodyParser->pfBodyFunc = pfBodyFunc;
    if (NULL != pstUserHandle) {
        pstBodyParser->stUserHandle = *pstUserHandle;
    }

    return pstBodyParser;
}

VOID HTTP_BODY_DestroyParser(IN HTTP_BODY_PARSER hBodyParser)
{
    HTTP_BODY_PSRSER_S *pstParser = hBodyParser;

    if (NULL != pstParser) {
        if (NULL != pstParser->hChunkParser) {
            HTTP_Chunk_DestoryParser(pstParser->hChunkParser);
        }

        MEM_Free(pstParser);
    }
}

/*
    OK: 接收完成
    NOT_COMPLETE: 需要继续接收数据
    其他: 错误原因
*/
BS_STATUS HTTP_BODY_Parse
(
    IN HTTP_BODY_PARSER hBodyParser,
    IN UCHAR *pucData,
    IN UINT uiDataLen,      /* 为0表示对端Closed了 */
    OUT UINT *puiParsedLen  /* 解析了多少数据 */
)
{
    HTTP_BODY_PSRSER_S *pstParser = hBodyParser;
    BS_STATUS eRet = BS_OK;

    *puiParsedLen = 0;

    switch (pstParser->eTranType) {
        case HTTP_BODY_TRAN_TYPE_CONTENT_LENGTH:
            eRet = http_body_ParseByContentLength(pstParser, pucData, uiDataLen, puiParsedLen);
            break;

        case HTTP_BODY_TRAN_TYPE_CLOSED:
            eRet = http_body_ParseByClosed(pstParser, pucData, uiDataLen, puiParsedLen);
            break;

        case HTTP_BODY_TRAN_TYPE_CHUNKED:
            eRet = http_body_ParseByChunked(pstParser, pucData, uiDataLen, puiParsedLen);
            break;

        default:
            BS_DBGASSERT(0);
            eRet = BS_NOT_SUPPORT;
            break;
    }

    return eRet;
}

BOOL_T HTTP_BODY_IsFinish(IN HTTP_BODY_PARSER hBodyParser)
{
    HTTP_BODY_PSRSER_S *pstParser = hBodyParser;

    return pstParser->bIsFinish;
}

