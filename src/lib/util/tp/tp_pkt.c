/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2012-8-24
* Description: syn报文占一个sn. ack/keepalive/rst不占sn
* History:     
******************************************************************************/

#include "bs.h"

#include "utl/tp_utl.h"

#include "tp_inner.h"

#define _TP_PKT_RESERVED_MBUF_LEN 200

#define _TP_PKT_RESEND_INTVAL 5    
#define _TP_PKT_MAX_RESEND_COUNT 5 


#define _TP_PKT_FLAG_DATA 0x1   
#define _TP_PKT_FLAG_SYN 0x2    
#define _TP_PKT_FLAG_ACK 0x4    
#define _TP_PKT_FLAG_KEEP_ALIVE 0x8 
#define _TP_PKT_FLAG_FIN 0x10   
#define _TP_PKT_FLAG_RST 0x20   

#define _TP_PKT_FLAG_STRING_LEN 32

typedef struct
{
    USHORT usVer;
    USHORT usReserved;
    UINT uiProtocolId;
    UINT uiDstTpId;
    UINT uiSrcTpId;
    INT iSn;
    INT iAckSn;
    UINT uiPktFlag;
    UINT uiPayloadLen;
}_TP_PKT_HEAD_S;

typedef struct
{
    _TP_PKT_HEAD_S stPktHeader;
    TP_CHANNEL_S *pstChannel;
}_TP_PKT_INFO_S;

static VOID tp_ptk_SendQueMbuf
(
    IN _TP_CTRL_S *pstCtrl,
    IN _TP_SOCKET_S *pstTpSocket
);

static BS_STATUS tp_pkt_StartKeepAlive
(
    IN _TP_CTRL_S *pstCtrl,
    IN _TP_SOCKET_S *pstTpSocket
)
{
    if (pstTpSocket->uiStatus != _TP_STATUS_ESTABLISH)
    {
        return BS_ERR;
    }
    
    return KA_Start(pstCtrl->hVclockHandle, &pstTpSocket->stKeepAlive);
}

VOID _TP_PKT_ResetKeepAlive
(
    IN _TP_CTRL_S *pstCtrl,
    IN _TP_SOCKET_S *pstTpSocket
)
{
    KA_Reset(&pstTpSocket->stKeepAlive);
}

static VOID tp_pkt_ResendTimeOut(IN HANDLE hTimerHandle, IN USER_HANDLE_S *pstUserHandle)
{
    _TP_CTRL_S *pstCtrl;
    _TP_SOCKET_S *pstTpSocket;

    pstCtrl = pstUserHandle->ahUserHandle[0];
    pstTpSocket = pstUserHandle->ahUserHandle[1];

    if (pstTpSocket->stResend.usResendCount >= _TP_PKT_MAX_RESEND_COUNT)
    {
        _TP_Socket_Abort(pstCtrl, pstTpSocket);
        return;
    }

    pstTpSocket->stResend.usResendCount ++;

    tp_ptk_SendQueMbuf(pstCtrl, pstTpSocket);
}

static BS_STATUS tp_pkt_StartResendTimer
(
    IN _TP_CTRL_S *pstCtrl,
    IN _TP_SOCKET_S *pstTpSocket
)
{
    USER_HANDLE_S stUserHandle;

    if (pstTpSocket->stResend.hResendTImer == NULL)
    {
        stUserHandle.ahUserHandle[0] = pstCtrl;
        stUserHandle.ahUserHandle[1] = pstTpSocket;

        pstTpSocket->stResend.hResendTImer = 
            VCLOCK_CreateTimer(pstCtrl->hVclockHandle, _TP_PKT_RESEND_INTVAL,
                    _TP_PKT_RESEND_INTVAL, TIMER_FLAG_CYCLE, tp_pkt_ResendTimeOut, &stUserHandle);
        if (NULL == pstTpSocket->stResend.hResendTImer)
        {
            _TP_Socket_Abort(pstCtrl, pstTpSocket);
            return BS_ERR;
        }
    }
    else
    {
        VCLOCK_Resume(pstCtrl->hVclockHandle, pstTpSocket->stResend.hResendTImer, 0);
    }

    return BS_OK;
}

static VOID tp_pkt_StopResendTimer
(
    IN _TP_CTRL_S *pstCtrl,
    IN _TP_SOCKET_S *pstTpSocket
)
{
    if (pstTpSocket->stResend.hResendTImer == NULL)
    {
        return;
    }

    VCLOCK_Pause(pstCtrl->hVclockHandle, pstTpSocket->stResend.hResendTImer);
    pstTpSocket->stResend.usResendCount = 0;
}

static CHAR * tp_pkt_GetPktFlagString(IN UINT uiFlag, OUT CHAR szFlagString[_TP_PKT_FLAG_STRING_LEN + 1])
{
    UINT uiIndex = 0;
    
    if (uiFlag & _TP_PKT_FLAG_DATA)
    {
        szFlagString[uiIndex] = 'D';
        uiIndex ++;
    }

    if (uiFlag & _TP_PKT_FLAG_SYN)
    {
        szFlagString[uiIndex] = 'S';
        uiIndex ++;
    }

    if (uiFlag & _TP_PKT_FLAG_ACK)
    {
        szFlagString[uiIndex] = 'A';
        uiIndex ++;
    }

    if (uiFlag & _TP_PKT_FLAG_KEEP_ALIVE)
    {
        szFlagString[uiIndex] = 'K';
        uiIndex ++;
    }

    if (uiFlag & _TP_PKT_FLAG_FIN)
    {
        szFlagString[uiIndex] = 'F';
        uiIndex ++;
    }

    if (uiFlag & _TP_PKT_FLAG_RST)
    {
        szFlagString[uiIndex] = 'R';
        uiIndex ++;
    }

    szFlagString[uiIndex] = '\0';
    
    return szFlagString;
}

static VOID tp_pkt_FillPktHeader
(
    IN _TP_SOCKET_S *pstTpSocket,
    IN UINT uiPktFlag,
    IN UINT uiPayLoadLen,
    INOUT _TP_PKT_HEAD_S *pstPktHeader
)
{
    pstPktHeader->uiProtocolId = htonl(pstTpSocket->uiProtocolId);
    pstPktHeader->uiDstTpId = htonl(pstTpSocket->uiPeerTpId);
    pstPktHeader->uiSrcTpId = htonl(pstTpSocket->uiLocalTpId);
    pstPktHeader->iSn = htonl(pstTpSocket->iSn);
    pstPktHeader->iAckSn = htonl(pstTpSocket->iAckSn);
    pstPktHeader->uiPktFlag = htonl(uiPktFlag);
    pstPktHeader->uiPayloadLen = htonl(uiPayLoadLen);
}

static inline MBUF_S * tp_pkt_BuildPkt
(
    IN _TP_PKT_HEAD_S *pstPktHead   
)
{
    return MBUF_CreateByCopyBuf(_TP_PKT_RESERVED_MBUF_LEN, (UCHAR*)pstPktHead, sizeof(_TP_PKT_HEAD_S), 0);
}

static MBUF_S * tp_pkt_BuildProtocolPkt
(
    IN _TP_SOCKET_S *pstTpSocket,
    IN UINT uiPktFlag
)
{
    _TP_PKT_HEAD_S stPkt = {0};

    tp_pkt_FillPktHeader(pstTpSocket, uiPktFlag, 0, &stPkt);

    return tp_pkt_BuildPkt(&stPkt);
}


static inline BOOL_T tp_pkt_IsPktTakeSn(IN UINT uiPktFlag)
{
    if (uiPktFlag & (_TP_PKT_FLAG_DATA | _TP_PKT_FLAG_SYN | _TP_PKT_FLAG_FIN))
    {
        return TRUE;
    }

    return FALSE;
}

static MBUF_S * tp_pkt_BuildRstPkt
(
    IN _TP_PKT_HEAD_S *pstRecvedPktInfo
)
{
    _TP_PKT_HEAD_S stPkt = {0};

    stPkt.uiProtocolId = htonl(pstRecvedPktInfo->uiProtocolId);
    stPkt.uiDstTpId = htonl(pstRecvedPktInfo->uiSrcTpId);
    stPkt.uiSrcTpId = htonl(pstRecvedPktInfo->uiDstTpId);
    stPkt.uiSrcTpId = htonl(pstRecvedPktInfo->uiDstTpId);
    stPkt.iSn = htonl(pstRecvedPktInfo->iAckSn);
    if (tp_pkt_IsPktTakeSn(pstRecvedPktInfo->uiPktFlag))
    {
        stPkt.iAckSn = pstRecvedPktInfo->iSn + 1;
    }
    else
    {
        stPkt.iAckSn = pstRecvedPktInfo->iSn;
    }
    stPkt.iAckSn = htonl(stPkt.iAckSn);
    stPkt.uiPktFlag = htonl(_TP_PKT_FLAG_RST);

    return tp_pkt_BuildPkt(&stPkt);
}

static BS_STATUS tp_pkt_GetPktHeader(IN MBUF_S *pstMbuf, OUT _TP_PKT_HEAD_S *pstPktHeader)
{
    _TP_PKT_HEAD_S *pstPktHead;

    if (BS_OK != MBUF_MakeContinue(pstMbuf, sizeof(_TP_PKT_HEAD_S)))
    {
        return BS_ERR;
    }

    pstPktHead = MBUF_MTOD(pstMbuf);
    pstPktHeader->uiProtocolId = ntohl(pstPktHead->uiProtocolId);
    pstPktHeader->uiDstTpId = ntohl(pstPktHead->uiDstTpId);
    pstPktHeader->uiSrcTpId = ntohl(pstPktHead->uiSrcTpId);
    pstPktHeader->iSn = ntohl(pstPktHead->iSn);
    pstPktHeader->iAckSn = ntohl(pstPktHead->iAckSn);
    pstPktHeader->uiPktFlag = ntohl(pstPktHead->uiPktFlag);
    pstPktHeader->uiPayloadLen = ntohl(pstPktHead->uiPayloadLen);

    return BS_OK;
}

static BS_STATUS tp_pkt_SendRst
(
    IN _TP_CTRL_S *pstCtrl,
    IN _TP_PKT_INFO_S *pstPktInfo
)
{
    MBUF_S *pstMbuf;

    pstMbuf = tp_pkt_BuildRstPkt(&pstPktInfo->stPktHeader);
    if (NULL == pstMbuf)
    {
        return BS_NO_MEMORY;
    }

    return pstCtrl->pfSendFunc(pstCtrl, pstPktInfo->pstChannel, pstMbuf, &pstCtrl->stUserHandle);
}

static inline BS_STATUS tp_pkt_SendPkt
(
    IN _TP_CTRL_S *pstCtrl,
    IN TP_CHANNEL_S *pstChannel,
    IN MBUF_S *pstMbuf
)
{
    _TP_PKT_HEAD_S *pstHead;
    CHAR szFlagString[_TP_PKT_FLAG_STRING_LEN + 1];

    pstHead = MBUF_MTOD(pstMbuf);

   BS_DBG_OUTPUT(pstCtrl->uiDbgFlag, TP_DBG_FLAG_PKT,
        ("TP Send: Protocol:%d,Dst:%x,Src:%x,Sn:%d,AckSn:%d,Flag:%s,Payload:%d \r\n",
         ntohl(pstHead->uiProtocolId),
         ntohl(pstHead->uiDstTpId),
         ntohl(pstHead->uiSrcTpId),
         ntohl(pstHead->iSn),
         ntohl(pstHead->iAckSn),
         tp_pkt_GetPktFlagString(ntohl(pstHead->uiPktFlag), szFlagString),
         ntohl(pstHead->uiPayloadLen)
         ));
    (VOID) pstCtrl->pfSendFunc(pstCtrl, pstChannel, pstMbuf, &pstCtrl->stUserHandle);

    return BS_OK;
}

static inline BS_STATUS tp_pkt_RecordAndSendPkt
(
    IN _TP_CTRL_S *pstCtrl,
    IN _TP_SOCKET_S *pstTpSocket,
    IN MBUF_S *pstMbuf
)
{
    MBUF_S *pstNewMbuf;

    MBUF_QUE_PUSH(&pstTpSocket->stSendMbufQue, pstMbuf);
    pstTpSocket->iSn ++;

    if (MBUF_QUE_GET_COUNT(&pstTpSocket->stSendMbufQue) == 1)
    {
        pstNewMbuf = MBUF_RawCopy(pstMbuf, 0, MBUF_TOTAL_DATA_LEN(pstMbuf), _TP_PKT_RESERVED_MBUF_LEN);
        if (NULL != pstNewMbuf)
        {
            tp_pkt_SendPkt(pstCtrl, &pstTpSocket->stChannel, pstNewMbuf);
        }

        tp_pkt_StartResendTimer(pstCtrl, pstTpSocket);
    }

    return BS_OK;
}

static BS_STATUS tp_pkt_SendSynAck
(
    IN _TP_CTRL_S *pstCtrl,
    IN _TP_SOCKET_S *pstTpSocket
)
{
    MBUF_S *pstMbuf;

    pstMbuf = tp_pkt_BuildProtocolPkt(pstTpSocket, _TP_PKT_FLAG_SYN | _TP_PKT_FLAG_ACK);
    if (NULL == pstMbuf)
    {
        return BS_NO_MEMORY;
    }

    return tp_pkt_RecordAndSendPkt(pstCtrl, pstTpSocket, pstMbuf);
}

static BS_STATUS tp_pkt_SendAck
(
    IN _TP_CTRL_S *pstCtrl,
    IN _TP_SOCKET_S *pstTpSocket
)
{
    MBUF_S *pstMbuf;

    pstMbuf = tp_pkt_BuildProtocolPkt(pstTpSocket, _TP_PKT_FLAG_ACK);
    if (NULL == pstMbuf)
    {
        return BS_NO_MEMORY;
    }

    return tp_pkt_SendPkt(pstCtrl, &pstTpSocket->stChannel, pstMbuf);
}

static BS_STATUS tp_pkt_SynInput
(
    IN _TP_CTRL_S *pstCtrl,
    IN _TP_PKT_INFO_S *pstPktInfo
)
{
    UINT uiProtocolId = pstPktInfo->stPktHeader.uiProtocolId;
    _TP_PROTOCOL_S *pstProtocol;
    _TP_SOCKET_S *pstTpSocket;

    if ((pstPktInfo->stPktHeader.uiPktFlag & _TP_PKT_FLAG_SYN) == 0)
    {
        tp_pkt_SendRst(pstCtrl, pstPktInfo);
        return BS_NO_SUCH;
    }

    pstProtocol = _TP_Protocol_FindProtocol(pstCtrl, uiProtocolId);
    if (NULL == pstProtocol)
    {
        tp_pkt_SendRst(pstCtrl, pstPktInfo);
        return BS_NO_SUCH;
    }

    if (FALSE == _TP_Protocol_CanAccepting(pstProtocol))
    {
        tp_pkt_SendRst(pstCtrl, pstPktInfo);
        return BS_NO_RESOURCE;
    }

    pstTpSocket = _TP_Socket_NewSocket(pstCtrl, TP_TYPE_CONN);
    if (NULL == pstTpSocket)
    {
        tp_pkt_SendRst(pstCtrl, pstPktInfo);
        return BS_NO_MEMORY;
    }

    pstTpSocket->uiProtocolId = uiProtocolId;
    pstTpSocket->uiPeerTpId = pstPktInfo->stPktHeader.uiSrcTpId;
    pstTpSocket->iAckSn = pstPktInfo->stPktHeader.iSn + 1;
    if (NULL != pstPktInfo->pstChannel)
    {
        pstTpSocket->stChannel = *(pstPktInfo->pstChannel);
    }
    pstTpSocket->uiStatus = _TP_STATUS_SYN_RCVD;
    pstTpSocket->uiListenTpId = pstProtocol->uiLocalTpId;

    if (BS_OK != _TP_Protocol_AddAccepting(pstProtocol, pstTpSocket))
    {
        _TP_Socket_FreeSocket(pstCtrl, pstTpSocket);
        tp_pkt_SendRst(pstCtrl, pstPktInfo);
        return BS_NO_RESOURCE;
    }

    if (BS_OK != tp_pkt_SendSynAck(pstCtrl, pstTpSocket))
    {
        _TP_Protocol_DelAccepting(pstProtocol, pstTpSocket);
        _TP_Socket_FreeSocket(pstCtrl, pstTpSocket);
        return BS_ERR;
    }

    return BS_OK;
}

static VOID tp_ptk_SendQueMbuf
(
    IN _TP_CTRL_S *pstCtrl,
    IN _TP_SOCKET_S *pstTpSocket
)
{
    MBUF_S *pstMbuf;
    MBUF_S *pstNewMbuf;

    if (! MBUF_QUE_IS_EMPTY(&pstTpSocket->stSendMbufQue))
    {
        pstMbuf = MBUF_QUE_PEEK(&pstTpSocket->stSendMbufQue);
        pstNewMbuf = MBUF_RawCopy(pstMbuf, 0, MBUF_TOTAL_DATA_LEN(pstMbuf), _TP_PKT_RESERVED_MBUF_LEN);
        if (NULL != pstNewMbuf)
        {        
            tp_pkt_SendPkt(pstCtrl, &pstTpSocket->stChannel, pstNewMbuf);
        }
    }
}

static BS_STATUS tp_pkt_AckInput
(
    IN _TP_CTRL_S *pstCtrl,
    IN _TP_SOCKET_S *pstTpSocket,
    IN _TP_PKT_INFO_S *pstPktInfo
)
{
    MBUF_S *pstMbuf;
    _TP_PKT_HEAD_S *pstHeader;
    INT iSn;
    BOOL_T bAcked = FALSE;

    while (1)
    {
        pstMbuf = MBUF_QUE_PEEK(&pstTpSocket->stSendMbufQue);
        if (NULL == pstMbuf)
        {
            break;
        }
        pstHeader = MBUF_MTOD(pstMbuf);
        iSn = ntohl(pstHeader->iSn);
        if ((pstPktInfo->stPktHeader.iAckSn - iSn) <= 0)
        {
            break;
        }
        MBUF_QUE_POP(&pstTpSocket->stSendMbufQue, pstMbuf);
        MBUF_Free(pstMbuf);
        bAcked = TRUE;
    }

    if (bAcked)
    {
        pstTpSocket->stResend.usResendCount = 0;
    }

    if (MBUF_QUE_IS_EMPTY(&pstTpSocket->stSendMbufQue))
    {
        tp_pkt_StopResendTimer(pstCtrl, pstTpSocket);
    }
    else
    {
        tp_ptk_SendQueMbuf(pstCtrl, pstTpSocket);
    }

    return BS_OK;
}

static BS_STATUS tp_pkt_RstInput
(
    IN _TP_CTRL_S *pstCtrl,
    IN _TP_SOCKET_S *pstTpSocket,
    IN _TP_PKT_INFO_S *pstPktInfo
)
{
    if ((pstPktInfo->stPktHeader.iAckSn == pstTpSocket->iSn)
        && (pstPktInfo->stPktHeader.iSn == pstTpSocket->iAckSn)
        && (pstTpSocket->uiStatus != _TP_STATUS_CLOSED))
    {
        _TP_Socket_Abort(pstCtrl, pstTpSocket);
    }

    return BS_OK;
}


static BS_STATUS tp_pkt_Ack1Input
(
    IN _TP_CTRL_S *pstCtrl,
    IN _TP_SOCKET_S *pstTpSocket,
    IN _TP_PKT_INFO_S *pstPktInfo
)
{
    if ((pstPktInfo->stPktHeader.iAckSn == pstTpSocket->iSn)
        && (pstTpSocket->uiStatus == _TP_STATUS_SYN_SEND))
    {
        pstTpSocket->uiStatus = _TP_STATUS_ESTABLISH;

        if (BS_OK != tp_pkt_StartKeepAlive(pstCtrl, pstTpSocket))
        {
            _TP_Socket_Abort(pstCtrl, pstTpSocket);
            return BS_ERR;
        }

        tp_pkt_AckInput(pstCtrl, pstTpSocket, pstPktInfo);
        _TP_Socket_WakeUp(pstCtrl, pstTpSocket, TP_EVENT_CONNECT);
    }

    return BS_OK;
}


static BS_STATUS tp_pkt_Ack2Input
(
    IN _TP_CTRL_S *pstCtrl,
    IN _TP_SOCKET_S *pstTpSocket,
    IN _TP_PKT_INFO_S *pstPktInfo
)
{
    _TP_SOCKET_S *pstListenTpSocket;

    if ((pstTpSocket->iSn != pstPktInfo->stPktHeader.iAckSn)
        || (pstTpSocket->iAckSn != pstPktInfo->stPktHeader.iSn))
    {
        return BS_OUT_OF_RANGE;
    }

    pstTpSocket->uiStatus = _TP_STATUS_ESTABLISH;

    if (BS_OK != tp_pkt_StartKeepAlive(pstCtrl, pstTpSocket))
    {
        _TP_Socket_Abort(pstCtrl, pstTpSocket);
        return BS_ERR;
    }

    tp_pkt_AckInput(pstCtrl, pstTpSocket, pstPktInfo);

    pstListenTpSocket = _TP_Socket_FindSocket(pstCtrl, pstTpSocket->uiListenTpId);
    BS_DBGASSERT(NULL != pstListenTpSocket);
    
    _TP_Socket_WakeUp(pstCtrl, pstListenTpSocket, TP_EVENT_ACCEPT);

    return BS_OK;
}

static BS_STATUS tp_pkt_KeepAliveInput
(
    IN _TP_CTRL_S *pstCtrl,
    IN _TP_SOCKET_S *pstTpSocket,
    IN _TP_PKT_INFO_S *pstPktInfo
)
{
    if ((pstPktInfo->stPktHeader.iAckSn == pstTpSocket->iSn)
        && (pstPktInfo->stPktHeader.iSn == pstTpSocket->iAckSn)
        && (pstTpSocket->uiStatus == _TP_STATUS_ESTABLISH))
    {
        return tp_pkt_SendAck(pstCtrl, pstTpSocket);
    }

    return BS_OK;
}

static VOID tp_pkt_KeepAliveFailed(IN KA_S *pstKa)
{
    _TP_CTRL_S *pstCtrl = pstKa->stUserHandle.ahUserHandle[0];
    _TP_SOCKET_S *pstTpSocket = pstKa->stUserHandle.ahUserHandle[1];

    _TP_Socket_Abort(pstCtrl, pstTpSocket);
}

static VOID tp_pkt_SendKeepAlive(IN KA_S *pstKa)
{
    _TP_CTRL_S *pstCtrl = pstKa->stUserHandle.ahUserHandle[0];
    _TP_SOCKET_S *pstTpSocket = pstKa->stUserHandle.ahUserHandle[1];
    MBUF_S *pstMbuf;

    if (pstTpSocket->uiStatus != _TP_STATUS_ESTABLISH)
    {
        return;
    }

    pstMbuf = tp_pkt_BuildProtocolPkt(pstTpSocket, _TP_PKT_FLAG_KEEP_ALIVE);
    if (NULL == pstMbuf)
    {
        return;
    }

    tp_pkt_SendPkt(pstCtrl, &pstTpSocket->stChannel, pstMbuf);
}

_TP_SOCKET_S * tp_pkt_FindSocket(IN _TP_CTRL_S *pstCtrl, IN _TP_PKT_INFO_S *pstPktInfo)
{
    _TP_SOCKET_S *pstTpSocket;

    pstTpSocket = _TP_Socket_FindSocket(pstCtrl, pstPktInfo->stPktHeader.uiDstTpId);
    if (pstTpSocket == NULL)
    {
        return NULL;
    }

    if ((pstTpSocket->uiPeerTpId != 0)
        && (pstTpSocket->uiPeerTpId != pstPktInfo->stPktHeader.uiSrcTpId))
    {
        return NULL;
    }

    return pstTpSocket;
}

static BS_STATUS tp_pkt_ProtocolPktIn
(
    IN _TP_CTRL_S *pstCtrl,
    IN _TP_PKT_INFO_S *pstPktInfo
)
{
    UINT uiProtocolId = pstPktInfo->stPktHeader.uiProtocolId;
    UINT uiPktFlag = pstPktInfo->stPktHeader.uiPktFlag;
    UINT uiDstTpId = pstPktInfo->stPktHeader.uiDstTpId;
    _TP_SOCKET_S *pstTpSocket;

    if ((uiProtocolId == 0)
        || (uiProtocolId > _TP_PROTO_MAX))
    {
        return BS_NOT_SUPPORT;
    }

    if (uiDstTpId == 0)
    {
        return tp_pkt_SynInput(pstCtrl, pstPktInfo);
    }

    pstTpSocket = tp_pkt_FindSocket(pstCtrl, pstPktInfo);
    
    if (NULL == pstTpSocket)
    {
        if ((uiPktFlag & _TP_PKT_FLAG_RST) == 0)
        {
            tp_pkt_SendRst(pstCtrl, pstPktInfo);
        }
        return BS_NO_SUCH;
    }

    _TP_PKT_ResetKeepAlive(pstCtrl, pstTpSocket);

    if (pstTpSocket->uiStatus == _TP_STATUS_SYN_SEND)
    {
        if ((uiPktFlag & _TP_PKT_FLAG_ACK) == 0)
        {
            return BS_ERR;
        }

        pstTpSocket->uiPeerTpId = pstPktInfo->stPktHeader.uiSrcTpId;
    }

    if (uiPktFlag & _TP_PKT_FLAG_SYN)
    {
        pstTpSocket->iAckSn = pstPktInfo->stPktHeader.iSn + 1;
        (VOID) tp_pkt_SendAck(pstCtrl, pstTpSocket);
    }

    if (uiPktFlag & _TP_PKT_FLAG_ACK)
    {
        if (pstTpSocket->uiStatus == _TP_STATUS_SYN_SEND)
        {
            tp_pkt_Ack1Input(pstCtrl, pstTpSocket, pstPktInfo);
        }
        else if (pstTpSocket->uiStatus == _TP_STATUS_SYN_RCVD)
        {
            tp_pkt_Ack2Input(pstCtrl, pstTpSocket, pstPktInfo);
        }
        else
        {        
            tp_pkt_AckInput(pstCtrl, pstTpSocket, pstPktInfo);
        }
    }

    if (uiPktFlag & _TP_PKT_FLAG_KEEP_ALIVE)
    {
        tp_pkt_KeepAliveInput(pstCtrl, pstTpSocket, pstPktInfo);
    }

    if (uiPktFlag & _TP_PKT_FLAG_RST)
    {
        tp_pkt_RstInput(pstCtrl, pstTpSocket, pstPktInfo);
    }

    return BS_OK;
}

static BS_STATUS tp_pkt_DataInput
(
    IN _TP_CTRL_S *pstCtrl,
    IN MBUF_S *pstMbuf,
    IN _TP_PKT_INFO_S *pstPktInfo
)
{
    _TP_SOCKET_S *pstTpSocket;

    if (MBUF_TOTAL_DATA_LEN(pstMbuf) == 0)
    {
        MBUF_Free(pstMbuf);
        return BS_OK;
    }

    pstTpSocket = tp_pkt_FindSocket(pstCtrl, pstPktInfo);
    if ((NULL == pstTpSocket)
       || (pstTpSocket->eType != TP_TYPE_CONN))
    {
        tp_pkt_SendRst(pstCtrl, pstPktInfo);
        MBUF_Free(pstMbuf);
        return BS_NO_SUCH;
    }

    _TP_PKT_ResetKeepAlive(pstCtrl, pstTpSocket);

    if ((pstTpSocket->uiStatus != _TP_STATUS_ESTABLISH)
        || (MBUF_QUE_IS_FULL(&pstTpSocket->stRecvMbufQue)))
    {
        MBUF_Free(pstMbuf);
        return BS_ERR;
    }

    if (pstPktInfo->stPktHeader.iSn == pstTpSocket->iAckSn)
    {
        pstTpSocket->iAckSn ++;
        tp_pkt_SendAck(pstCtrl, pstTpSocket);
        MBUF_QUE_PUSH(&pstTpSocket->stRecvMbufQue, pstMbuf);
        _TP_Socket_WakeUp(pstCtrl, pstTpSocket, TP_EVENT_READ);
    }
    else
    {
        tp_pkt_SendAck(pstCtrl, pstTpSocket);
    }

    return BS_OK;
}

VOID _TP_PKT_InitKeepAlive
(
    IN _TP_CTRL_S *pstCtrl,
    IN _TP_SOCKET_S *pstTpSocket
)
{
    pstTpSocket->stKeepAlive.stSet.usIdle = pstCtrl->usDftIdle;
    pstTpSocket->stKeepAlive.stSet.usIntval = pstCtrl->usDftIntval;
    pstTpSocket->stKeepAlive.stSet.usMaxProbeCount = pstCtrl->usDftMaxProbeCount;
    pstTpSocket->stKeepAlive.stSet.pfSendKeepAlive = tp_pkt_SendKeepAlive;
    pstTpSocket->stKeepAlive.stSet.pfKeepAliveFailed = tp_pkt_KeepAliveFailed;

    pstTpSocket->stKeepAlive.stUserHandle.ahUserHandle[0] = pstCtrl;
    pstTpSocket->stKeepAlive.stUserHandle.ahUserHandle[1] = pstTpSocket;
}

BS_STATUS _TP_PKT_SendSyn
(
    IN _TP_CTRL_S *pstCtrl,
    IN _TP_SOCKET_S *pstTpSocket
)
{
    MBUF_S *pstMbuf;
    
    pstTpSocket->uiStatus = _TP_STATUS_SYN_SEND;

    pstMbuf = tp_pkt_BuildProtocolPkt(pstTpSocket, _TP_PKT_FLAG_SYN);
    if (NULL == pstMbuf)
    {
        return BS_NO_MEMORY;
    }

    return tp_pkt_RecordAndSendPkt(pstCtrl, pstTpSocket, pstMbuf);
}


BS_STATUS _TP_PKT_SendData
(
    IN _TP_CTRL_S *pstCtrl,
    IN _TP_SOCKET_S *pstTpSocket,
    IN MBUF_S *pstMbuf
)
{
    _TP_PKT_HEAD_S *pstPktHeader;
    UINT uiPayloadLen;

    if (pstTpSocket->eType != TP_TYPE_CONN)
    {
        MBUF_Free(pstMbuf);
        return BS_NOT_SUPPORT;
    }

    if (pstTpSocket->uiStatus != _TP_STATUS_ESTABLISH)
    {
        MBUF_Free(pstMbuf);
        return BS_NOT_READY;
    }

    if (MBUF_QUE_IS_FULL(&pstTpSocket->stSendMbufQue))
    {
        MBUF_Free(pstMbuf);
        return BS_AGAIN;
    }

    uiPayloadLen = MBUF_TOTAL_DATA_LEN(pstMbuf);

    if (BS_OK != MBUF_Prepend(pstMbuf, sizeof(_TP_PKT_HEAD_S)))
    {
        MBUF_Free(pstMbuf);
        return BS_NO_MEMORY;
    }

    pstPktHeader = MBUF_MTOD(pstMbuf);

    tp_pkt_FillPktHeader(pstTpSocket, _TP_PKT_FLAG_DATA, uiPayloadLen, pstPktHeader);

    return tp_pkt_RecordAndSendPkt(pstCtrl, pstTpSocket, pstMbuf);
}

MBUF_S * TP_PktBuildRstByPkt(IN MBUF_S *pstMbuf)
{
    _TP_PKT_INFO_S stPktInfo;

    if (BS_OK != tp_pkt_GetPktHeader(pstMbuf, &stPktInfo.stPktHeader))
    {
        return NULL;
    }

    return tp_pkt_BuildRstPkt(&stPktInfo.stPktHeader);
}

BS_STATUS TP_PktInput
(
    IN TP_HANDLE hTpHandle,
    IN MBUF_S *pstMbuf,
    IN TP_CHANNEL_S *pstChannel
)
{
    _TP_CTRL_S *pstCtrl = hTpHandle;
    _TP_PKT_INFO_S stPktInfo;
    BS_STATUS eRet;
    CHAR szFlagString[_TP_PKT_FLAG_STRING_LEN + 1];

    if (pstMbuf == NULL)
    {
        return BS_BAD_PARA;
    }

    if (BS_OK != tp_pkt_GetPktHeader(pstMbuf, &stPktInfo.stPktHeader))
    {
        MBUF_Free(pstMbuf);
        return BS_ERR;
    }

    stPktInfo.pstChannel = pstChannel;

    BS_DBG_OUTPUT(pstCtrl->uiDbgFlag, TP_DBG_FLAG_PKT,
        ("TP Recv: Protocol:%d,Dst:%x,Src:%x,Sn:%d,AckSn:%d,Flag:%s,Payload:%d\r\n",
         stPktInfo.stPktHeader.uiProtocolId,
         stPktInfo.stPktHeader.uiDstTpId,
         stPktInfo.stPktHeader.uiSrcTpId,
         stPktInfo.stPktHeader.iSn,
         stPktInfo.stPktHeader.iAckSn,
         tp_pkt_GetPktFlagString(stPktInfo.stPktHeader.uiPktFlag, szFlagString),
         stPktInfo.stPktHeader.uiPayloadLen
         ));

    if ((stPktInfo.stPktHeader.uiPktFlag & _TP_PKT_FLAG_DATA) == 0)
    {
        eRet = tp_pkt_ProtocolPktIn(pstCtrl, &stPktInfo);
        MBUF_Free(pstMbuf);
    }
    else
    {
        MBUF_CutHead(pstMbuf, sizeof(_TP_PKT_HEAD_S));
        eRet = tp_pkt_DataInput(pstCtrl, pstMbuf, &stPktInfo);
    }

    return eRet;
}

BOOL_T TP_IsSynPkt(IN MBUF_S *pstMbuf)
{
    UINT uiPktFlag;
    _TP_PKT_HEAD_S *pstHeader;
    
    if (BS_OK != MBUF_MakeContinue(pstMbuf, sizeof(_TP_PKT_HEAD_S)))
    {
        return FALSE;
    }

    pstHeader = MBUF_MTOD(pstMbuf);

    uiPktFlag = ntohl(pstHeader->uiPktFlag);

    if (uiPktFlag & _TP_PKT_FLAG_SYN)
    {
        return TRUE;
    }

    return FALSE;
}

BOOL_T TP_NeedAck(IN MBUF_S *pstMbuf)
{
    UINT uiPktFlag;
    _TP_PKT_HEAD_S *pstHeader;
    
    if (BS_OK != MBUF_MakeContinue(pstMbuf, sizeof(_TP_PKT_HEAD_S)))
    {
        return FALSE;
    }

    pstHeader = MBUF_MTOD(pstMbuf);

    uiPktFlag = ntohl(pstHeader->uiPktFlag);

    if (uiPktFlag & (_TP_PKT_FLAG_DATA | _TP_PKT_FLAG_SYN | _TP_PKT_FLAG_KEEP_ALIVE | _TP_PKT_FLAG_FIN))
    {
        return TRUE;
    }

    return FALSE;
}
