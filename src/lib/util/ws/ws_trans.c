/******************************************************************************
* Copyright (C),    LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2014-7-6
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/bit_opt.h"
#include "utl/socket_utl.h"
#include "utl/ws_utl.h"

#include "ws_def.h"
#include "ws_conn.h"
#include "ws_trans.h"
#include "ws_event.h"
#include "ws_trans_mempool.h"

typedef enum
{
    _WS_TRANS_INNER_EVENT_READ = 0,
    _WS_TRANS_INNER_EVENT_WRITE,
    _WS_TRANS_INNER_EVENT_ERR,
    _WS_TRANS_INNER_EVENT_STEP,

    _WS_CONN_EVENT_MAX
}_WS_TRANS_INNER_EVENT_E;

static BS_STATUS ws_trans_Err(IN FSM_S *pstFsm, IN UINT uiEvent);
static BS_STATUS ws_trans_RecvHead(IN FSM_S *pstFsm, IN UINT uiEvent);
static BS_STATUS ws_trans_RecvBody(IN FSM_S *pstFsm, IN UINT uiEvent);
static BS_STATUS ws_trans_RecvBodyOK(IN FSM_S *pstFsm, IN UINT uiEvent);
static BS_STATUS ws_trans_BuildReplyHead(IN FSM_S *pstFsm, IN UINT uiEvent);
static BS_STATUS ws_trans_SendHead(IN FSM_S *pstFsm, IN UINT uiEvent);
static BS_STATUS ws_trans_BuildBody(IN FSM_S *pstFsm, IN UINT uiEvent);
static BS_STATUS ws_trans_FormatBody(IN FSM_S *pstFsm, IN UINT uiEvent);
static BS_STATUS ws_trans_SendBody(IN FSM_S *pstFsm, IN UINT uiEvent);
static BS_STATUS ws_trans_SendBodyOK(IN FSM_S *pstFsm, IN UINT uiEvent);
static BS_STATUS ws_trans_Fini(IN FSM_S *pstFsm, IN UINT uiEvent);

static FSM_SWITCH_TBL g_hWsTransFsmSwitchTbl = NULL;

static FSM_STATE_MAP_S g_astWsTransFsmStateMap[WS_STATE_MAX] = 
{
    {"S.RecvHead",      WS_STATE_RECV_HEAD},
    {"S.RecvBody",      WS_STATE_RECV_BODY},
    {"S.RecvBodyOK",    WS_STATE_RECV_BODY_OK},
    {"S.PrepareHead",   WS_STATE_PREPARE_HEAD},
    {"S.BuildHead",     WS_STATE_BUILD_HEAD},
    {"S.SendHead",      WS_STATE_SEND_HEAD},
    {"S.BuildBody",     WS_STATE_BUILD_BODY},
    {"S.FormatBody",    WS_STATE_FORMAT_BODY},
    {"S.SendBody",      WS_STATE_SEND_BODY},
    {"S.SendBodyOK",    WS_STATE_SEND_BODY_OK},
    {"S.Fini",          WS_STATE_FINI},
};

static FSM_EVENT_MAP_S g_astWsTransFsmEventMap[_WS_CONN_EVENT_MAX] = 
{
    {"E.ConnRead",   _WS_TRANS_INNER_EVENT_READ},
    {"E.ConnWrite",  _WS_TRANS_INNER_EVENT_WRITE},
    {"E.Err",        _WS_TRANS_INNER_EVENT_ERR},
    {"E.Step",       _WS_TRANS_INNER_EVENT_STEP},
};

static FSM_SWITCH_MAP_S g_astWsTransFsmSwichMap[] =
{
    {"*",             "E.Err",              "@", ws_trans_Err},
    {"S.RecvHead",    "E.ConnRead,E.Step",  "@", ws_trans_RecvHead},
    {"S.RecvBody",    "E.ConnRead,E.Step",  "@", ws_trans_RecvBody},
    {"S.RecvBodyOK",  "E.Step",             "@", ws_trans_RecvBodyOK},
    {"S.BuildHead",   "E.ConnWrite,E.Step", "@", ws_trans_BuildReplyHead},
    {"S.SendHead",    "E.ConnWrite,E.Step", "@", ws_trans_SendHead},
    {"S.BuildBody",   "E.ConnWrite,E.Step", "@", ws_trans_BuildBody},
    {"S.FormatBody",  "E.Step",             "@", ws_trans_FormatBody},
    {"S.SendBody",    "E.ConnWrite,E.Step", "@", ws_trans_SendBody},
    {"S.SendBodyOK",  "E.ConnWrite,E.Step", "@", ws_trans_SendBodyOK},
    {"S.Fini",        "E.Step,E.ConnWrite", "@", ws_trans_Fini},
};

static VOID ws_trans_Destory(IN WS_TRANS_S *pstTrans)
{
    WS_Event_IssuEvent(pstTrans, WS_TRANS_EVENT_DESTORY);

    if (pstTrans->hHttpHeadRequest != NULL)
    {
        HTTP_DestoryHeadParser(pstTrans->hHttpHeadRequest);
        pstTrans->hHttpHeadRequest = NULL;
    }

    if (pstTrans->hHttpHeadReply != NULL)
    {
        HTTP_DestoryHeadParser(pstTrans->hHttpHeadReply);
        pstTrans->hHttpHeadReply = NULL;
    }

    if (NULL != pstTrans->pstRecvMbuf)
    {
        MBUF_Free(pstTrans->pstRecvMbuf);
        pstTrans->pstRecvMbuf = NULL;
    }

    if (NULL != pstTrans->pstSendMbuf)
    {
        MBUF_Free(pstTrans->pstSendMbuf);
        pstTrans->pstSendMbuf = NULL;
    }

    if (NULL != pstTrans->hWsConn)
    {
        WS_Conn_SetUserData(pstTrans->hWsConn, NULL);
    }

    if (NULL != pstTrans->hQuery)
    {
        MIME_Destroy(pstTrans->hQuery);
        pstTrans->hQuery = NULL;
    }

    if (NULL != pstTrans->hCookie)
    {
        MIME_Destroy(pstTrans->hCookie);
        pstTrans->hCookie = NULL;
    }

    if (NULL != pstTrans->hBodyMime)
    {
        MIME_Destroy(pstTrans->hBodyMime);
        pstTrans->hBodyMime = NULL;
    }

    if (NULL != pstTrans->hKD)
    {
        KD_Destory(pstTrans->hKD);
        pstTrans->hKD = NULL;
    }

    WS_TransMemPool_Fini(pstTrans);

    MEM_Free(pstTrans);
}

static BS_STATUS ws_trans_Recv(IN WS_TRANS_S *pstTrans, IN UINT uiWantReadLen, OUT UINT *puiReadLen)
{
    UCHAR aucData[1024];
    INT iReadLen;
    UINT uiWantLen;
    UINT uiSize = uiWantReadLen;
    UINT uiTotalReadLen = 0;

    *puiReadLen = 0;

    if (uiWantReadLen == 0)
    {
        return BS_OK;
    }

    while (uiSize > 0)
    {
        uiWantLen = MIN(sizeof(aucData), uiSize);

        iReadLen = WS_Conn_Read(pstTrans->hWsConn, aucData, uiWantLen);
        if (iReadLen <= 0)
        {
            if (iReadLen != SOCKET_E_AGAIN)
            {
                return BS_ERR;
            }

            break;
        }

        if ((pstTrans->uiFlag & WS_TRANS_FLAG_DROP_REQ_BODY) == 0)
        {
            if (pstTrans->pstRecvMbuf == NULL)
            {
                pstTrans->pstRecvMbuf = MBUF_CreateByCopyBuf(0, aucData, iReadLen, MBUF_DATA_DATA);
                if (NULL == pstTrans->pstRecvMbuf)
                {
                    return BS_ERR;
                }
            }
            else
            {
                if (BS_OK != MBUF_CopyFromBufToMbuf(pstTrans->pstRecvMbuf,
                    MBUF_TOTAL_DATA_LEN(pstTrans->pstRecvMbuf), aucData, iReadLen))
                {
                    return BS_ERR;
                }
            }
        }

        uiSize -= iReadLen;
        uiTotalReadLen += iReadLen;
    }

    *puiReadLen = uiTotalReadLen;

    return BS_OK;
}

static BS_STATUS ws_trans_RecvHeadData(IN WS_TRANS_S *pstTrans)
{
    UINT uiReadLen;

    return ws_trans_Recv(pstTrans, 1024*16 /*SSL记录块大小*/, &uiReadLen);
}

static BS_STATUS ws_trans_Err(IN FSM_S *pstFsm, IN UINT uiEvent)
{
    return BS_ERR;
}

static BS_STATUS ws_trans_ParseQuery(IN WS_TRANS_S *pstTrans)
{
    CHAR *pcQuery;

    pcQuery = HTTP_GetUriQuery(pstTrans->hHttpHeadRequest);
    if (NULL == pcQuery)
    {
        return BS_OK;
    }

    pstTrans->hQuery = MIME_Create();
    if (NULL == pstTrans->hQuery)
    {
        return BS_ERR;
    }

    return MIME_ParseData(pstTrans->hQuery, pcQuery);
}

static BS_STATUS ws_trans_ParseCookie(IN WS_TRANS_S *pstTrans)
{
    CHAR *pcCookie;

    pcCookie = HTTP_GetHeadField(pstTrans->hHttpHeadRequest, HTTP_FIELD_COOKIE);
    if (NULL == pcCookie)
    {
        return BS_OK;
    }

    pstTrans->hCookie = MIME_Create();
    if (NULL == pstTrans->hCookie)
    {
        return BS_ERR;
    }

    return MIME_ParseCookie(pstTrans->hCookie, pcCookie);
}

static inline VOID ws_trans_DebugRequestHead(IN _WS_S *pstWs, IN CHAR *pcHead, IN UINT uiHeadLen)
{
    CHAR szInfo[WS_TRANS_MAX_HEAD_LEN + 1];
    UINT uiCopyLen;

    if (_WS_DEBUG_SWICH_TEST(pstWs, WS_DBG_PACKET))
    {
        uiCopyLen = MIN(sizeof(szInfo) - 1, uiHeadLen);
        memcpy(szInfo, pcHead, uiCopyLen);
        szInfo[uiCopyLen] = '\0';

        _WS_DEBUG_PKT(pstWs, ("Recv request head:\r\n%s", szInfo));
    }
}

static BS_STATUS ws_trans_RecvHead(IN FSM_S *pstFsm, IN UINT uiEvent)
{
    WS_TRANS_S *pstTrans;
    UINT uiHeadLen;
    _WS_S *pstWs;
    BS_STATUS eRet = BS_OK;
    UINT64 uiContentLen;
    HANDLE hMemHandle;
    UCHAR *pucData;
    UINT uiDataLen;
    MBUF_S *pstMbuf;
    MBUF_S *pstMBufTmp;

    pstTrans = FSM_GetPrivateData(pstFsm);
    pstWs = pstTrans->pstWs;

    if (BS_OK != ws_trans_RecvHeadData(pstTrans))
    {
        _WS_DEBUG_ERR(pstWs, ("Read connection data failed.\r\n"));
        return BS_ERR;
    }

    pstMbuf = pstTrans->pstRecvMbuf;
    if (NULL == pstMbuf)
    {
        return BS_OK;
    }

    uiDataLen = MBUF_TOTAL_DATA_LEN(pstMbuf);
    pucData = MBUF_GetContinueMem(pstMbuf, 0, uiDataLen, &hMemHandle);

    uiHeadLen = HTTP_GetHeadLen((CHAR*)pucData, uiDataLen);
    if (0 == uiHeadLen) {
        if (uiDataLen >= WS_TRANS_MAX_HEAD_LEN) {
            MBUF_FreeContinueMem(hMemHandle);
            _WS_DEBUG_ERR(pstWs, ("Http head too long.\r\n"));
            return BS_TOO_LONG;
        }

        return BS_OK;
    }

    ws_trans_DebugRequestHead(pstWs, (CHAR*)pucData, uiHeadLen);

    eRet = HTTP_ParseHead(pstTrans->hHttpHeadRequest, (CHAR*)pucData, uiHeadLen, HTTP_REQUEST);
    eRet |= ws_trans_ParseCookie(pstTrans);
    eRet |= ws_trans_ParseQuery(pstTrans);

    MBUF_FreeContinueMem(hMemHandle);

    if (eRet != BS_OK)
    {
        _WS_DEBUG_ERR(pstWs, ("Http head parse failed.\r\n"));
        return BS_ERR;
    }

    MBUF_CutHead(pstMbuf, uiHeadLen);

    pstTrans->pcRequestFile = HTTP_GetUriAbsPath(pstTrans->hHttpHeadRequest);

    if (BS_OK != HTTP_GetContentLen(pstTrans->hHttpHeadRequest, &uiContentLen))
    {
        uiContentLen = 0;
    }

    pstTrans->uiContentLen = (UINT)uiContentLen;

    if (MBUF_TOTAL_DATA_LEN(pstMbuf) > uiContentLen)
    {
        pstMBufTmp = MBUF_Fragment(pstMbuf, (UINT)uiContentLen);
        if (NULL == pstMBufTmp)
        {
            return BS_ERR;
        }
        pstTrans->pstRecvMbuf = pstMBufTmp;
        WS_Conn_NextReq(pstTrans->hWsConn, pstMbuf);
        pstTrans->uiRemainContentLen = 0;
    }
    else
    {
        pstTrans->uiRemainContentLen = pstTrans->uiContentLen - MBUF_TOTAL_DATA_LEN(pstMbuf);
    }

    if (MBUF_TOTAL_DATA_LEN(pstTrans->pstRecvMbuf) == 0)
    {
        MBUF_Free(pstTrans->pstRecvMbuf);
        pstTrans->pstRecvMbuf = NULL;
    }

    FSM_SetState(pstFsm, WS_STATE_RECV_BODY);

    return WS_Event_IssuEvent(pstTrans, WS_TRANS_EVENT_RECV_HEAD_OK);
}

static BS_STATUS ws_trans_RecvBody(IN FSM_S *pstFsm, IN UINT uiEvent)
{
    WS_TRANS_S *pstTrans;
    UINT uiReadLen;

    pstTrans = FSM_GetPrivateData(pstFsm);

    if (pstTrans->uiRemainContentLen > 0)
    {
        if (BS_OK != ws_trans_Recv(pstTrans, pstTrans->uiRemainContentLen, &uiReadLen))
        {
            return BS_ERR;
        }

        pstTrans->uiRemainContentLen -= uiReadLen;
    }

    if ((pstTrans->pstRecvMbuf != NULL) && (MBUF_TOTAL_DATA_LEN(pstTrans->pstRecvMbuf) >= WS_TRANS_MAX_BODY_BUF_LEN))
    {
        if (BS_OK != WS_Event_IssuEvent(pstTrans, WS_TRANS_EVENT_RECV_BODY))
        {
            return BS_ERR;
        }
    }

    if (pstTrans->uiRemainContentLen == 0)
    {
        if (pstTrans->pstRecvMbuf != NULL)
        {
            if (BS_OK != WS_Event_IssuEvent(pstTrans, WS_TRANS_EVENT_RECV_BODY))
            {
                return BS_ERR;
            }
        }

        if (pstTrans->uiFlag & WS_TRANS_FLAG_REPLY_AFTER_BODY)
        {
            WS_Trans_SetHeadFieldFinish(pstTrans);
        }
        else
        {
            FSM_SetState(pstFsm, WS_STATE_RECV_BODY_OK);
        }
    }

    return BS_OK;
}

static BS_STATUS ws_trans_RecvBodyOK(IN FSM_S *pstFsm, IN UINT uiEvent)
{
    WS_TRANS_S *pstTrans;

    pstTrans = FSM_GetPrivateData(pstFsm);

    FSM_SetState(pstFsm, WS_STATE_PREPARE_HEAD);

    WS_Conn_SetEvent(pstTrans->hWsConn, 0);

    if (BS_OK != WS_Event_IssuEvent(pstTrans, WS_TRANS_EVENT_RECV_BODY_OK))
    {
        return BS_ERR;
    }

    return BS_OK;
}

static BS_STATUS ws_trans_BuildReplyHead(IN FSM_S *pstFsm, IN UINT uiEvent)
{
    WS_TRANS_S *pstTrans;
    ULONG ulHeadLen;
    CHAR *pcHead;

    pstTrans = FSM_GetPrivateData(pstFsm);

    HTTP_SetVersion(pstTrans->hHttpHeadReply, HTTP_VERSION_1_1);

    if (BS_OK != WS_Event_IssuEvent(pstTrans, WS_TRANS_EVENT_PRE_BUILD_HEAD))
    {
        return BS_ERR;
    }

    pcHead = HTTP_BuildHttpHead(pstTrans->hHttpHeadReply, HTTP_RESPONSE, &ulHeadLen);
    if (NULL == pcHead)
    {
        return BS_NO_MEMORY;
    }

    pstTrans->stReplyHead.pucData = (UCHAR*)pcHead;
    pstTrans->stReplyHead.uiDataLen = ulHeadLen;
    pstTrans->stReplyHead.uiOffset = 0;

    FSM_SetState(pstFsm, WS_STATE_SEND_HEAD);

    return BS_OK;
}

static BS_STATUS ws_trans_SendHead(IN FSM_S *pstFsm, IN UINT uiEvent)
{
    WS_TRANS_S *pstTrans;
    INT iWriteLen;
    UINT64 uiContextLen = 0;

    pstTrans = FSM_GetPrivateData(pstFsm);

    if (pstTrans->stReplyHead.uiDataLen > pstTrans->stReplyHead.uiOffset)
    {
        iWriteLen = WS_Conn_Write(pstTrans->hWsConn, pstTrans->stReplyHead.pucData + pstTrans->stReplyHead.uiOffset,
                        pstTrans->stReplyHead.uiDataLen - pstTrans->stReplyHead.uiOffset);
        if (iWriteLen < 0)
        {
            if (iWriteLen == SOCKET_E_AGAIN)
            {
                return BS_OK;
            }
            else
            {
                return BS_ERR;
            }
        }

        pstTrans->stReplyHead.uiOffset += iWriteLen;
    }

    if (pstTrans->stReplyHead.uiDataLen <= pstTrans->stReplyHead.uiOffset)
    {
        if ((BS_OK == HTTP_GetContentLen(pstTrans->hHttpHeadReply, &uiContextLen))
            && (uiContextLen == 0))
        {
            FSM_SetState(pstFsm, WS_STATE_SEND_BODY_OK);
        }
        else
        {
            FSM_SetState(pstFsm, WS_STATE_BUILD_BODY);
        }
    }

    return BS_OK;
}

static BS_STATUS ws_trans_BuildBody(IN FSM_S *pstFsm, IN UINT uiEvent)
{
    WS_TRANS_S *pstTrans;
    BS_STATUS eRet;

    pstTrans = FSM_GetPrivateData(pstFsm);

    if (NULL != pstTrans->pstSendMbuf)
    {
        FSM_SetState(pstFsm, WS_STATE_FORMAT_BODY);
        return BS_OK;
    }

    if (pstTrans->uiFlag & WS_TRANS_FLAG_ADD_REPLY_BODY_FINISHED)
    {
        FSM_SetState(pstFsm, WS_STATE_SEND_BODY_OK);
        return BS_OK;
    }

    eRet = WS_Event_IssuEvent(pstTrans, WS_TRANS_EVENT_BUILD_BODY);
    if (eRet != BS_OK)
    {
        return eRet;
    }

    if (NULL != pstTrans->pstSendMbuf)
    {
        FSM_SetState(pstFsm, WS_STATE_FORMAT_BODY);
    }
    else
    {
        if (pstTrans->uiFlag & WS_TRANS_FLAG_ADD_REPLY_BODY_FINISHED)
        {
            FSM_SetState(pstFsm, WS_STATE_SEND_BODY_OK);
        }
        else
        {
            WS_Conn_SetEvent(pstTrans->hWsConn, 0);
        }
    }

    return BS_OK;
}

static BS_STATUS ws_trans_FormatBody(IN FSM_S *pstFsm, IN UINT uiEvent)
{
    WS_TRANS_S *pstTrans;
    BS_STATUS eRet;

    pstTrans = FSM_GetPrivateData(pstFsm);

    eRet = WS_Event_IssuEvent(pstTrans, WS_TRANS_EVENT_FORMAT_BODY);
    if (eRet != BS_OK)
    {
        return eRet;
    }

    FSM_SetState(pstFsm, WS_STATE_SEND_BODY);

    return BS_OK;
}

static BS_STATUS ws_trans_SendBody(IN FSM_S *pstFsm, IN UINT uiEvent)
{
    WS_TRANS_S *pstTrans;
    UCHAR *pucData;
    UINT uiDataLen;
    INT iWriteLen;
    UINT uiTotleWriteLen = 0;

    pstTrans = FSM_GetPrivateData(pstFsm);

    MBUF_SCAN_DATABLOCK_BEGIN(pstTrans->pstSendMbuf, pucData, uiDataLen)
    {
        iWriteLen = WS_Conn_Write(pstTrans->hWsConn, pucData, uiDataLen);
        if (iWriteLen == SOCKET_E_AGAIN)
        {
            break;
        }
        if (iWriteLen < 0)
        {
            return BS_ERR;
        }

        uiTotleWriteLen += iWriteLen;
    }MBUF_SCAN_END();    

    if (uiTotleWriteLen > 0)
    {
        if ((UINT)uiTotleWriteLen < MBUF_TOTAL_DATA_LEN(pstTrans->pstSendMbuf))
        {
            MBUF_CutHead(pstTrans->pstSendMbuf, uiTotleWriteLen);
        }
        else
        {
            MBUF_Free(pstTrans->pstSendMbuf);
            pstTrans->pstSendMbuf = NULL;
            if (pstTrans->uiFlag & WS_TRANS_FLAG_ADD_REPLY_BODY_FINISHED)
            {
                FSM_SetState(pstFsm, WS_STATE_SEND_BODY_OK);
            }
            else
            {
                FSM_SetState(pstFsm, WS_STATE_BUILD_BODY);
            }
        }
    }

    return BS_OK;
}

static BS_STATUS ws_trans_SendBodyOK(IN FSM_S *pstFsm, IN UINT uiEvent)
{
    WS_TRANS_S *pstTrans;
    BS_STATUS eRet;

    pstTrans = FSM_GetPrivateData(pstFsm);

    eRet = WS_Event_IssuEvent(pstTrans, WS_TRANS_EVENT_SEND_BODY_OK);

    FSM_SetState(pstFsm, WS_STATE_FINI);

    return eRet;
}

static BS_STATUS ws_trans_Fini(IN FSM_S *pstFsm, IN UINT uiEvent)
{
    WS_TRANS_S *pstTrans;

    pstTrans = FSM_GetPrivateData(pstFsm);

    if (HTTP_CONNECTION_CLOSE == HTTP_GetConnection(pstTrans->hHttpHeadReply))
    {
        return BS_STOP;
    }

    return BS_OK;
}

static BS_STATUS ws_trans_StateStep(IN WS_TRANS_S *pstTrans, IN UINT uiEvent)
{
    UINT uiState;
    BS_STATUS eRet;
    UINT uiEventTmp;

    uiEventTmp = uiEvent;

    while (1)
    {
        uiState = FSM_GetCurrentState(&pstTrans->stFsm);
        eRet = FSM_EventHandle(&pstTrans->stFsm, uiEventTmp);
        if (eRet != BS_OK)
        {
            break;
        }
        if (uiState == FSM_GetCurrentState(&pstTrans->stFsm))
        {
            break;
        }
        if (BIT_ISSET(pstTrans->uiFlag, WS_TRANS_FLAG_PAUSE))
        {
            break;
        }

        uiEventTmp = _WS_TRANS_INNER_EVENT_STEP;
    }

    if (eRet != BS_OK)
    {
        FSM_SetState(&pstTrans->stFsm, WS_STATE_FINI);
    }

    return eRet;
}

static BS_STATUS ws_trans_Create(IN WS_CONN_HANDLE hWsConn)
{
    _WS_S *pstWs;
    WS_TRANS_S *pstTrans;

    pstWs = WS_Conn_GetWS(hWsConn);

    pstTrans = MEM_ZMalloc(sizeof(WS_TRANS_S));
    if (NULL == pstTrans)
    {
        return BS_NO_MEMORY;
    }

    pstTrans->pstWs = pstWs;
    pstTrans->hWsConn = hWsConn;

    pstTrans->hHttpHeadRequest = HTTP_CreateHeadParser();
    if (NULL == pstTrans->hHttpHeadRequest)
    {
        ws_trans_Destory(pstTrans);
        return BS_ERR;
    }

    pstTrans->hHttpHeadReply = HTTP_CreateHeadParser();
    if (NULL == pstTrans->hHttpHeadReply)
    {
        ws_trans_Destory(pstTrans);
        return BS_ERR;
    }

    FSM_Init(&pstTrans->stFsm, g_hWsTransFsmSwitchTbl);
    FSM_InitState(&pstTrans->stFsm, WS_STATE_RECV_HEAD);
    FSM_SetPrivateData(&pstTrans->stFsm, pstTrans);
    if (BS_OK != WS_TransMemPool_Init(pstTrans))
    {
        ws_trans_Destory(pstTrans);
        return BS_ERR;
    }

    pstTrans->hKD = KD_Create();
    if (NULL == pstTrans->hKD)
    {
        ws_trans_Destory(pstTrans);
        return BS_ERR;
    }

    WS_Conn_SetUserData(hWsConn, pstTrans);

    if (BS_OK != WS_Event_IssuEvent(pstTrans, WS_TRANS_EVENT_CREATE))
    {
        ws_trans_Destory(pstTrans);
        return BS_ERR;
    }

    return BS_OK;
}

static BOOL_T ws_trans_CanFsmRun(IN WS_TRANS_S *pstTrans)
{
    if (FSM_GetCurrentState(&pstTrans->stFsm) == WS_STATE_FINI)
    {
        return FALSE;
    }

    if (BIT_ISSET(pstTrans->uiFlag, WS_TRANS_FLAG_PAUSE) != 0)
    {
        WS_Conn_SetEvent(pstTrans->hWsConn, 0);
        return FALSE;
    }

    return TRUE;
}

BS_STATUS _WS_Trans_EventInput(IN WS_CONN_HANDLE hWsConn, IN UINT uiEvent)
{
    WS_TRANS_S *pstTrans;
    BS_STATUS eRet = BS_OK;
    UINT uiState;
    BOOL_T need_step = 0;

    pstTrans = WS_Conn_GetUserData(hWsConn);

    if (NULL == pstTrans)
    {
        if (BS_OK != ws_trans_Create(hWsConn))
        {
            return BS_ERR;
        }

        pstTrans = WS_Conn_GetUserData(hWsConn);
    }

    if (WS_TRANS_FLAG_WILL_CONTINUE & pstTrans->uiFlag)
    {
        BIT_CLR(pstTrans->uiFlag, WS_TRANS_FLAG_WILL_CONTINUE);
        need_step = 1;
        uiEvent = 0;

        uiState = FSM_GetCurrentState(&pstTrans->stFsm);
        if (uiState <= WS_STATE_RECV_BODY)
        {
            WS_Conn_SetEvent(pstTrans->hWsConn, WS_CONN_EVENT_READ);
        }
        else if (uiState >= WS_STATE_SEND_HEAD)
        {
            WS_Conn_SetEvent(pstTrans->hWsConn, WS_CONN_EVENT_WRITE);
        }
    }

    if (uiEvent & WS_CONN_EVENT_ERROR)
    {
        eRet = ws_trans_StateStep(pstTrans , _WS_TRANS_INNER_EVENT_ERR);
    }

    if ((uiEvent & WS_CONN_EVENT_READ) && ws_trans_CanFsmRun(pstTrans))
    {
        eRet = ws_trans_StateStep(pstTrans , _WS_TRANS_INNER_EVENT_READ);
    }

    if ((uiEvent & WS_CONN_EVENT_WRITE) && ws_trans_CanFsmRun(pstTrans))
    {
        eRet = ws_trans_StateStep(pstTrans , _WS_TRANS_INNER_EVENT_WRITE);
    }

    if ((need_step) && ws_trans_CanFsmRun(pstTrans))
    {
        eRet = ws_trans_StateStep(pstTrans , _WS_TRANS_INNER_EVENT_STEP);
    }

    if (FSM_GetCurrentState(&pstTrans->stFsm) == WS_STATE_FINI)
    {
        WS_Conn_SetEvent(pstTrans->hWsConn, WS_CONN_EVENT_READ);
        ws_trans_Destory(pstTrans);
    }

    if (BS_OK != eRet)
    {
        WS_Conn_Destory(hWsConn);
    }

    return eRet;
}

BS_STATUS _WS_Trans_Init()
{
    SPLX_P();

    if (g_hWsTransFsmSwitchTbl == NULL)
    {
        g_hWsTransFsmSwitchTbl = FSM_CreateSwitchTbl(g_astWsTransFsmStateMap,
                WS_STATE_MAX, g_astWsTransFsmEventMap, _WS_CONN_EVENT_MAX,
                g_astWsTransFsmSwichMap,
                sizeof(g_astWsTransFsmSwichMap)/sizeof(FSM_SWITCH_MAP_S));
    }

    SPLX_V();

    if (NULL == g_hWsTransFsmSwitchTbl)
    {
        return BS_ERR;
    }

    return BS_OK;
}

BS_STATUS _WS_Trans_RegFsmStateListener(IN PF_FSM_STATE_LISTEN pfListenFunc,IN USER_HANDLE_S * pstUserHandle)
{
    return FSM_RegStateListener(g_hWsTransFsmSwitchTbl, pfListenFunc, pstUserHandle);
}

CHAR * _WS_Trans_GetEventName(IN UINT uiEvent)
{
    return FSM_GetEventName(g_hWsTransFsmSwitchTbl, uiEvent);
}

CHAR * _WS_Trans_GetStateName(IN UINT uiState)
{
    return FSM_GetStateName(g_hWsTransFsmSwitchTbl, uiState);
}

VOID _WS_Trans_SetState(IN WS_TRANS_S *pstTrans, IN UINT uiState)
{
    FSM_SetState(&pstTrans->stFsm, uiState);
}

MBUF_S * WS_Trans_GetBodyData(IN WS_TRANS_HANDLE hTrans)
{
    WS_TRANS_S *pstTrans = hTrans;
    MBUF_S *pstMbuf;

    pstMbuf = pstTrans->pstRecvMbuf;

    pstTrans->pstRecvMbuf = NULL;

    return pstMbuf;
}

WS_CONN_HANDLE WS_Trans_GetConn(IN WS_TRANS_HANDLE hTrans)
{
    WS_TRANS_S *pstTrans = hTrans;

    return pstTrans->hWsConn;
}

VOID WS_Trans_SetHeadFieldFinish(IN WS_TRANS_HANDLE hWsTrans)
{
    WS_TRANS_S *pstTrans = hWsTrans;

    if (FSM_GetCurrentState(&pstTrans->stFsm) < WS_STATE_BUILD_HEAD)
    {
        WS_Conn_SetEvent(pstTrans->hWsConn, WS_CONN_EVENT_WRITE);
        _WS_Trans_SetState(pstTrans, WS_STATE_BUILD_HEAD);
    }
}

VOID WS_Trans_Close(IN WS_TRANS_HANDLE hTrans)
{
    ws_trans_Destory(hTrans);
}

VOID WS_Trans_Err(IN WS_TRANS_HANDLE hTrans)
{
    WS_TRANS_S *pstTrans = hTrans;
    WS_CONN_HANDLE hWsConn = pstTrans->hWsConn;

    ws_trans_Destory(pstTrans);
    WS_Conn_Destory(hWsConn);
}

VOID _WS_Trans_SetFlag(IN WS_TRANS_S *pstTrans, IN UINT uiFlag)
{
    pstTrans->uiFlag |= uiFlag;
}

BS_STATUS WS_Trans_Reply(IN WS_TRANS_HANDLE hTrans, IN UINT uiStatusCode, IN UINT uiFlag /* WS_TRANS_REPLY_FLAG_XXX */)
{
    BS_STATUS eRet;
    WS_TRANS_S *pstTrans = hTrans;

    _WS_Trans_SetFlag(pstTrans, WS_TRANS_FLAG_DROP_REQ_BODY | WS_TRANS_FLAG_REPLY_AFTER_BODY);
    eRet = HTTP_SetStatusCode(pstTrans->hHttpHeadReply, uiStatusCode);

    if (uiFlag & WS_TRANS_REPLY_FLAG_WITHOUT_BODY)
    {
        HTTP_SetContentLen(pstTrans->hHttpHeadReply, 0);
    }

    if (FSM_GetCurrentState(&pstTrans->stFsm) == WS_STATE_PREPARE_HEAD)
    {
        WS_Trans_SetHeadFieldFinish(pstTrans);
    }
    else
    {
        if (uiFlag & WS_TRANS_REPLY_FLAG_IMMEDIATELY)
        {
            WS_Trans_SetHeadFieldFinish(pstTrans);
        }
    }

    return eRet;
}

BS_STATUS WS_Trans_Redirect(IN WS_TRANS_HANDLE hTrans, IN CHAR *pcRedirectTo)
{
    BS_STATUS eRet;
    WS_TRANS_S *pstTrans = hTrans;

    eRet = HTTP_SetHeadField(pstTrans->hHttpHeadReply, HTTP_FIELD_LOCATION, pcRedirectTo);
    eRet |= WS_Trans_Reply(pstTrans, HTTP_STATUS_MOVED_TEMP, WS_TRANS_REPLY_FLAG_WITHOUT_BODY);

    return eRet;
}

VOID WS_Trans_Pause(IN WS_TRANS_HANDLE hTrans)
{
    WS_TRANS_S *pstTrans = hTrans;

    pstTrans->uiFlag |= WS_TRANS_FLAG_PAUSE;

    WS_Conn_SetEvent(pstTrans->hWsConn, 0);
}

VOID WS_Trans_Continue(IN WS_TRANS_HANDLE hTrans)
{
    WS_TRANS_S *pstTrans = hTrans;

    if (pstTrans->uiFlag & WS_TRANS_FLAG_PAUSE)
    {
        BIT_CLR(pstTrans->uiFlag, WS_TRANS_FLAG_PAUSE);
        BIT_SET(pstTrans->uiFlag, WS_TRANS_FLAG_WILL_CONTINUE);
        WS_Conn_SetEvent(pstTrans->hWsConn, WS_CONN_EVENT_READ | WS_CONN_EVENT_WRITE);
    }
}

CHAR * WS_Trans_GetRequestFile(IN WS_TRANS_HANDLE hTrans)
{
    WS_TRANS_S *pstTrans = hTrans;

    return pstTrans->pcRequestFile;
}

WS_CONTEXT_HANDLE WS_Trans_GetContext(IN WS_TRANS_HANDLE hTrans)
{
    WS_TRANS_S *pstTrans = hTrans;

    return pstTrans->hContext;
}

BS_STATUS WS_Trans_SetUserHandle(IN WS_TRANS_HANDLE hTrans, IN CHAR *pcKey, IN HANDLE hUserHandle)
{
    WS_TRANS_S *pstTrans = hTrans;

    return KD_SetKeyHandle(pstTrans->hKD, pcKey, hUserHandle);
}

HANDLE WS_Trans_GetUserHandle(IN WS_TRANS_HANDLE hTrans, IN CHAR *pcKey)
{
    WS_TRANS_S *pstTrans = hTrans;
    
    return KD_GetKeyHandle(pstTrans->hKD, pcKey);
}

HTTP_HEAD_PARSER WS_Trans_GetHttpRequestParser(IN WS_TRANS_HANDLE hTrans)
{
    WS_TRANS_S *pstTrans = hTrans;

    return pstTrans->hHttpHeadRequest;
}

HTTP_HEAD_PARSER WS_Trans_GetHttpEncap(IN WS_TRANS_HANDLE hTrans)
{
    WS_TRANS_S *pstTrans = hTrans;

    return pstTrans->hHttpHeadReply;
}

BS_STATUS WS_Trans_AddReplyBody(IN WS_TRANS_HANDLE hTrans, IN MBUF_S *pstMbuf)
{
    WS_TRANS_S *pstTrans = hTrans;

    if (pstTrans->pstSendMbuf == NULL)
    {
        pstTrans->pstSendMbuf = pstMbuf;
    }
    else
    {
        if (BS_OK != MBUF_Cat(pstTrans->pstSendMbuf, pstMbuf))
        {
            return BS_ERR;
        }
    }

    if (FSM_GetCurrentState(&pstTrans->stFsm) >= WS_STATE_BUILD_BODY)
    {
        WS_Conn_SetEvent(pstTrans->hWsConn, WS_CONN_EVENT_WRITE);
    }

    return BS_OK;
}

BS_STATUS WS_Trans_AddReplyBodyByBuf(IN WS_TRANS_HANDLE hTrans, IN void *buf, IN UINT uiDataLen)
{
    WS_TRANS_S *pstTrans = hTrans;

    if (pstTrans->pstSendMbuf != NULL)
    {
        if (BS_OK != MBUF_CopyFromBufToMbuf(pstTrans->pstSendMbuf,
                MBUF_TOTAL_DATA_LEN(pstTrans->pstSendMbuf), buf, uiDataLen))
        {
            return BS_NO_MEMORY;
        }
    }
    else
    {
        pstTrans->pstSendMbuf = MBUF_CreateByCopyBuf(0, buf, uiDataLen, MBUF_DATA_DATA);
        if (NULL == pstTrans->pstSendMbuf)
        {
            return BS_NO_MEMORY;
        }
    }

    if (FSM_GetCurrentState(&pstTrans->stFsm) >= WS_STATE_BUILD_BODY)
    {
        WS_Conn_SetEvent(pstTrans->hWsConn, WS_CONN_EVENT_WRITE);
    }

    return BS_OK;
}

VOID WS_Trans_ReplyBodyFinish(IN WS_TRANS_HANDLE hTrans)
{
    WS_TRANS_S *pstTrans = hTrans;

    pstTrans->uiFlag |= WS_TRANS_FLAG_ADD_REPLY_BODY_FINISHED;

    if (FSM_GetCurrentState(&pstTrans->stFsm) >= WS_STATE_BUILD_BODY)
    {
        WS_Conn_SetEvent(pstTrans->hWsConn, WS_CONN_EVENT_WRITE);
    }
}

MIME_HANDLE WS_Trans_GetQueryMime(IN WS_TRANS_HANDLE hTrans)
{
    WS_TRANS_S *pstTrans = hTrans;

    return pstTrans->hQuery;
}

MIME_HANDLE WS_Trans_GetBodyMime(IN WS_TRANS_HANDLE hTrans)
{
    WS_TRANS_S *pstTrans = hTrans;

    return pstTrans->hBodyMime;
}

MIME_HANDLE WS_Trans_GetCookieMime(IN WS_TRANS_HANDLE hTrans)
{
    WS_TRANS_S *pstTrans = hTrans;

    return pstTrans->hCookie;
}

