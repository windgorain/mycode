/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2012-5-27
* Description: trusty pakcet protocol
* History:     
* 
* 它和周边模块的关系为:
                 APP
                  |
                 TP
                  |
         Packet Process Module
******************************************************************************/

#include "bs.h"

#include "utl/tp_utl.h"
#include "utl/bit_opt.h"

#include "tp_inner.h"

#define _TP_DFT_KEEP_ALIVE_IDLE      3600
#define _TP_DFT_KEEP_ALIVE_INTVAL    10
#define _TP_DFT_KEEP_ALIVE_MAX_COUNT 6


typedef struct
{
    DLL_NODE_S stNode;
    PF_TP_CLOSE_NOTIFY_FUNC pfNotifyFunc;
    USER_HANDLE_S stUserHandle;
}_TP_CLOSE_NOTIFY_S;

static VOID tp_WalkEach
(
    IN _TP_CTRL_S *pstCtrl,
    IN _TP_SOCKET_S *pstSocket,
    IN HANDLE hUserHandle
)
{
    USER_HANDLE_S *pstUserHandle = hUserHandle;
    PF_TP_WALK_FUNC pfFunc = pstUserHandle->ahUserHandle[0];

    pfFunc(pstCtrl, pstSocket->uiLocalTpId, pstUserHandle->ahUserHandle[1]);
}

static VOID tp_FreeNotifyList(IN VOID *pstNotifyNode)
{
    MEM_Free(pstNotifyNode);
}

static VOID tp_CloseNotify(IN _TP_CTRL_S *pstCtrl, IN _TP_SOCKET_S *pstTpSocket)
{
    _TP_CLOSE_NOTIFY_S *pstNotifyNode;

    DLL_SCAN(&pstCtrl->stCloseNotifyList, pstNotifyNode)
    {
        pstNotifyNode->pfNotifyFunc(pstTpSocket->uiLocalTpId, pstTpSocket->phPropertys, &pstNotifyNode->stUserHandle);
    }
}

static VOID tp_SetDftKeepAlive
(
    IN _TP_CTRL_S *pstCtrl,
    IN USHORT usIdle,
    IN USHORT usIntval,
    IN USHORT usMaxProbeCount
)
{
    pstCtrl->usDftIdle = usIdle;
    pstCtrl->usDftIntval = usIntval;
    pstCtrl->usDftMaxProbeCount = usMaxProbeCount;
}

TP_HANDLE TP_Create
(
    IN PF_TP_SEND_FUNC pfSendFunc,
    IN PF_TP_EVENT_FUNC pfEventFunc,
    IN USER_HANDLE_S *pstUserHandle,
    IN UINT uiMaxPropertys /* 支持多少个属性 */
)
{
    _TP_CTRL_S *pstCtrl;

    pstCtrl = MEM_ZMalloc(sizeof(_TP_CTRL_S));
    if (NULL == pstCtrl)
    {
        return NULL;
    }

    if (BS_OK != _TP_Socket_Init(pstCtrl))
    {
        TP_Destory(pstCtrl);
        return NULL;
    }

    if (BS_OK != _TP_Protocol_Init(pstCtrl))
    {
        TP_Destory(pstCtrl);
        return NULL;
    }

    tp_SetDftKeepAlive(pstCtrl, _TP_DFT_KEEP_ALIVE_IDLE, _TP_DFT_KEEP_ALIVE_INTVAL, _TP_DFT_KEEP_ALIVE_MAX_COUNT);

    pstCtrl->hVclockHandle = VCLOCK_CreateInstance(FALSE);
    if (NULL == pstCtrl->hVclockHandle)
    {
        TP_Destory(pstCtrl);
        return NULL;
    }

    DLL_INIT(&pstCtrl->stCloseNotifyList);
    pstCtrl->uiMaxPropertys = uiMaxPropertys;
    pstCtrl->pfSendFunc = pfSendFunc;
    pstCtrl->pfEventFunc = pfEventFunc;
    if (NULL != pstUserHandle)
    {
        pstCtrl->stUserHandle = *pstUserHandle;
    }

    return pstCtrl;
}

VOID TP_Destory(IN TP_HANDLE hTpHandle)
{
    _TP_CTRL_S *pstCtrl = hTpHandle;

    if (NULL == pstCtrl)
    {
        return;
    }

    if (NULL != pstCtrl->hVclockHandle)
    {
        VCLOCK_DeleteInstance(pstCtrl->hVclockHandle);
    }

    DLL_FREE(&pstCtrl->stCloseNotifyList, tp_FreeNotifyList);
    _TP_Protocol_UnInit(pstCtrl);
    _TP_Socket_UnInit(pstCtrl);
    MEM_Free(pstCtrl);
}

BS_STATUS TP_SetDftKeepAlive
(
    IN TP_HANDLE hTpHandle,
    IN USHORT usIdle,
    IN USHORT usIntval,
    IN USHORT usMaxProbeCount
)
{
    _TP_CTRL_S *pstCtrl = hTpHandle;

    if (NULL == pstCtrl)
    {
        return BS_NULL_PARA;
    }

    tp_SetDftKeepAlive(pstCtrl, usIdle, usIntval, usMaxProbeCount);

    return BS_OK;
}

BS_STATUS TP_RegCloseNotifyEvent
(
    IN TP_HANDLE hTpHandle,
    IN PF_TP_CLOSE_NOTIFY_FUNC pfFunc,
    IN USER_HANDLE_S *pstUserHandle
)
{
    _TP_CTRL_S *pstCtrl = hTpHandle;
    _TP_CLOSE_NOTIFY_S *pstNotify;

    pstNotify = MEM_ZMalloc(sizeof(_TP_CLOSE_NOTIFY_S));
    if (NULL == pstNotify)
    {
        return BS_ERR;
    }

    pstNotify->pfNotifyFunc = pfFunc;
    if (NULL != pstUserHandle)
    {
        pstNotify->stUserHandle = *pstUserHandle;
    }

    DLL_ADD(&pstCtrl->stCloseNotifyList, pstNotify);

    return BS_OK;
}


VOID TP_Walk(IN TP_HANDLE hTpHandle, IN PF_TP_WALK_FUNC pfFunc, IN HANDLE hUserHandle)
{
    _TP_CTRL_S *pstCtrl = hTpHandle;
    USER_HANDLE_S stUserHandle;

    stUserHandle.ahUserHandle[0] = pfFunc;
    stUserHandle.ahUserHandle[1] = hUserHandle;

    _TP_Socket_Walk(pstCtrl, tp_WalkEach, &stUserHandle);
}

VOID TP_SetDbgFlag(IN TP_HANDLE hTpHandle, IN UINT uiDbgFlag)
{
    _TP_CTRL_S *pstCtrl = hTpHandle;

    pstCtrl->uiDbgFlag |= uiDbgFlag;
}

VOID TP_ClrDbgFlag(IN TP_HANDLE hTpHandle, IN UINT uiDbgFlag)
{
    _TP_CTRL_S *pstCtrl = hTpHandle;

    pstCtrl->uiDbgFlag &= (~uiDbgFlag);
}

TP_ID TP_Socket
(
    IN TP_HANDLE hTpHandle,
    IN TP_TYPE_E eType
)
{
    _TP_CTRL_S *pstCtrl = hTpHandle;
    _TP_SOCKET_S * pstTpSocket;

    pstTpSocket = _TP_Socket_NewSocket(pstCtrl, eType);
    if (NULL == pstTpSocket)
    {
        return 0;
    }

    return pstTpSocket->uiLocalTpId;
}

BS_STATUS TP_SetOpt
(
    IN TP_HANDLE hTpHandle,
    IN TP_ID uiTpId,
    IN TP_OPT_E enOpt,
    IN VOID *pValue
)
{
    _TP_CTRL_S *pstCtrl = hTpHandle;
    _TP_SOCKET_S *pstTpSocket;

    if (uiTpId == 0)
    {
        return BS_BAD_PARA;
    }

    pstTpSocket = _TP_Socket_FindSocket(pstCtrl, uiTpId);
    if (NULL == pstTpSocket)
    {
        return BS_NO_SUCH;
    }

    return _TP_Socket_SetOpt(pstCtrl, pstTpSocket, enOpt, pValue);
}

BS_STATUS TP_SetProperty
(
    IN TP_HANDLE hTpHandle,
    IN TP_ID uiTpId,
    IN UINT uiIndex,
    IN HANDLE hValue
)
{
    _TP_CTRL_S *pstCtrl = hTpHandle;
    _TP_SOCKET_S *pstTpSocket;

    if (uiTpId == 0)
    {
        return BS_BAD_PARA;
    }

    if (uiIndex >= pstCtrl->uiMaxPropertys)
    {
        return BS_OUT_OF_RANGE;
    }

    pstTpSocket = _TP_Socket_FindSocket(pstCtrl, uiTpId);
    if (NULL == pstTpSocket)
    {
        return BS_NO_SUCH;
    }

    pstTpSocket->phPropertys[uiIndex] = hValue;

    return BS_OK;
}

HANDLE TP_GetProperty
(
    IN TP_HANDLE hTpHandle,
    IN TP_ID uiTpId,
    IN UINT uiIndex
)
{
    _TP_CTRL_S *pstCtrl = hTpHandle;
    _TP_SOCKET_S *pstTpSocket;

    if (uiTpId == 0)
    {
        return NULL;
    }

    if (uiIndex >= pstCtrl->uiMaxPropertys)
    {
        return NULL;
    }

    pstTpSocket = _TP_Socket_FindSocket(pstCtrl, uiTpId);
    if (NULL == pstTpSocket)
    {
        return NULL;
    }

    return pstTpSocket->phPropertys[uiIndex];
}

BS_STATUS TP_Bind
(
    IN TP_HANDLE hTpHandle,
    IN TP_ID uiTpId,
    IN UINT uiProtocolId    /* 主机序 */
)
{
    _TP_CTRL_S *pstCtrl = hTpHandle;
    _TP_SOCKET_S *pstTpSocket;
    BS_STATUS eRet;

    if (uiTpId == 0)
    {
        return BS_BAD_PARA;
    }

    if ((uiProtocolId > _TP_PROTO_MAX) || (uiProtocolId == 0))
    {
        return BS_OUT_OF_RANGE;
    }

    pstTpSocket = _TP_Socket_FindSocket(pstCtrl, uiTpId);
    if (NULL == pstTpSocket)
    {
        return BS_NO_SUCH;
    }

    eRet = _TP_Protocol_BindProtocol(pstCtrl, pstTpSocket, uiProtocolId);
    if (BS_OK != eRet)
    {
        return eRet;
    }

    return BS_OK;
}

BS_STATUS TP_Listen
(
    IN TP_HANDLE hTpHandle,
    IN TP_ID uiTpId
)
{
    _TP_CTRL_S *pstCtrl = hTpHandle;
    _TP_SOCKET_S *pstTpSocket;

    if (uiTpId == 0)
    {
        return BS_BAD_PARA;
    }

    pstTpSocket = _TP_Socket_FindSocket(pstCtrl, uiTpId);
    if (NULL == pstTpSocket)
    {
        return BS_NO_SUCH;
    }

    return _TP_Protocol_Listen(pstCtrl, pstTpSocket);
}

BS_STATUS TP_Close
(
    IN TP_HANDLE hTpHandle,
    IN TP_ID uiTpId
)
{
    _TP_CTRL_S *pstCtrl = hTpHandle;
    _TP_SOCKET_S *pstTpSocket;

    if (uiTpId == 0)
    {
        return BS_BAD_PARA;
    }

    pstTpSocket = _TP_Socket_FindSocket(pstCtrl, uiTpId);
    if (NULL == pstTpSocket)
    {
        return BS_NO_SUCH;
    }

    _TP_Protocol_UnBindProtocol(pstCtrl, pstTpSocket);

    tp_CloseNotify(pstCtrl, pstTpSocket);

    _TP_Socket_FreeSocket(pstCtrl, pstTpSocket);

    return BS_OK;
}

BS_STATUS TP_Connect
(
    IN TP_HANDLE hTpHandle,
    IN TP_ID uiTpId,
    IN UINT uiProtocolId,
    IN TP_CHANNEL_S *pstChannel
)
{
    _TP_CTRL_S *pstCtrl = hTpHandle;
    _TP_SOCKET_S *pstTpSocket;
    BS_STATUS eRet;
    UINT64 uiEvent;

    if (uiTpId == 0)
    {
        return BS_BAD_PARA;
    }

    if (uiProtocolId == 0)
    {
        return BS_BAD_PARA;
    }

    pstTpSocket = _TP_Socket_FindSocket(pstCtrl, uiTpId);
    if (NULL == pstTpSocket)
    {
        return BS_NO_SUCH;
    }

    if (pstTpSocket->eType != TP_TYPE_CONN)
    {
        return BS_NOT_SUPPORT;
    }

    if (pstTpSocket->uiStatus != _TP_STATUS_INIT)
    {
        return BS_ALREADY_EXIST;
    }

    pstTpSocket->uiProtocolId = uiProtocolId;

    if (NULL != pstChannel)
    {
        pstTpSocket->stChannel = *pstChannel;
    }

    eRet = _TP_PKT_SendSyn(pstCtrl, pstTpSocket);
    if (eRet != BS_OK)
    {
        return eRet;
    }

    if (BIT_ISSET(pstTpSocket->uiFlag, _TP_SOCKET_FLAG_NBIO))
    {
        return BS_OK;
    }

    eRet = Event_Read(pstTpSocket->hEvent,
                TP_EVENT_CONNECT | TP_EVENT_ERR,
                &uiEvent, EVENT_FLAG_WAIT, 5000);
    if (eRet == BS_OK)
    {
        if (uiEvent & TP_EVENT_ERR)
        {
            return BS_CAN_NOT_CONNECT;
        }

        return BS_OK;
    }

    return BS_TIME_OUT;
}

BS_STATUS TP_Accept
(
    IN TP_HANDLE hTpHandle,
    IN TP_ID uiTpId,
    OUT TP_ID *puiAcceptTpId
)
{
    _TP_CTRL_S *pstCtrl = hTpHandle;
    _TP_SOCKET_S *pstTpSocket;

    *puiAcceptTpId = 0;

    if (uiTpId == 0)
    {
        return BS_BAD_PARA;
    }

    pstTpSocket = _TP_Socket_FindSocket(pstCtrl, uiTpId);
    if (NULL == pstTpSocket)
    {
        return BS_NO_SUCH;
    }

    return _TP_Protocol_Accept(pstCtrl, pstTpSocket, puiAcceptTpId);
}

TP_CHANNEL_S * TP_GetChannel
(
    IN TP_HANDLE hTpHandle,
    IN TP_ID uiTpId
)
{
    _TP_CTRL_S *pstCtrl = hTpHandle;
    _TP_SOCKET_S *pstTpSocket;

    if (uiTpId == 0)
    {
        return NULL;
    }

    pstTpSocket = _TP_Socket_FindSocket(pstCtrl, uiTpId);
    if (NULL == pstTpSocket)
    {
        return NULL;
    }

    return &pstTpSocket->stChannel;
}

BS_STATUS TP_SendMbuf
(
    IN TP_HANDLE hTpHandle,
    IN TP_ID uiTpId,
    IN MBUF_S *pstMbuf
)
{
    _TP_CTRL_S *pstCtrl = hTpHandle;
    _TP_SOCKET_S *pstTpSocket;

    if (uiTpId == 0)
    {
        MBUF_Free(pstMbuf);
        return BS_BAD_PARA;
    }

    pstTpSocket = _TP_Socket_FindSocket(pstCtrl, uiTpId);
    if (NULL == pstTpSocket)
    {
        MBUF_Free(pstMbuf);
        return BS_NO_SUCH;
    }

    return _TP_PKT_SendData(pstCtrl, pstTpSocket, pstMbuf);
}

BS_STATUS TP_SendData
(
    IN TP_HANDLE hTpHandle,
    IN TP_ID uiTpId,
    IN UCHAR *pucData,
    IN UINT uiDataLen
)
{
    MBUF_S *pstMbuf;

    if ((pucData == NULL) || (uiDataLen == 0))
    {
        return BS_BAD_PARA;
    }

    pstMbuf = MBUF_CreateByCopyBuf(0, pucData, uiDataLen, 0);
    if (NULL == pstMbuf)
    {
        return BS_NO_MEMORY;
    }

    return TP_SendMbuf(hTpHandle, uiTpId, pstMbuf);
}

MBUF_S * TP_RecvMbuf
(
    IN TP_HANDLE hTpHandle,
    IN TP_ID uiTpId
)
{
    _TP_CTRL_S *pstCtrl = hTpHandle;
    _TP_SOCKET_S *pstTpSocket;
    MBUF_S *pstMbuf;

    if (uiTpId == 0)
    {
        return NULL;
    }

    pstTpSocket = _TP_Socket_FindSocket(pstCtrl, uiTpId);
    if (NULL == pstTpSocket)
    {
        return NULL;
    }

    MBUF_QUE_POP(&pstTpSocket->stRecvMbufQue, pstMbuf);

    return pstMbuf;
}

BS_STATUS TP_RecvData
(
    IN TP_HANDLE hTpHandle,
    IN TP_ID uiTpId,
    INOUT UCHAR *pucData,
    IN UINT uiMaxDataLen,
    OUT UINT *puiReadLen
)
{
    MBUF_S *pstMbuf;

    pstMbuf = TP_RecvMbuf(hTpHandle, uiTpId);
    if (NULL == pstMbuf)
    {
        return BS_AGAIN;
    }

    if (MBUF_TOTAL_DATA_LEN(pstMbuf) > uiMaxDataLen)
    {
        MBUF_Free(pstMbuf);
        return BS_TOO_LONG;
    }

    MBUF_CopyFromMbufToBuf(pstMbuf, 0, MBUF_TOTAL_DATA_LEN(pstMbuf), pucData);
    *puiReadLen = MBUF_TOTAL_DATA_LEN(pstMbuf);

    MBUF_Free(pstMbuf);

    return BS_OK;
}

/* 触发KeepAlive */
VOID TP_TiggerKeepAlive(IN TP_HANDLE hTpHandle, IN TP_ID uiTpId)
{
    
    _TP_CTRL_S *pstCtrl = hTpHandle;
    _TP_SOCKET_S *pstTpSocket;

    if (uiTpId == 0)
    {
        return;
    }

    pstTpSocket = _TP_Socket_FindSocket(pstCtrl, uiTpId);
    if (NULL == pstTpSocket)
    {
        return;
    }

    KA_TrigerKeepAlive(&pstTpSocket->stKeepAlive);
}

BS_STATUS TP_GetState
(
    IN TP_HANDLE hTpHandle,
    IN TP_ID uiTpId,
    OUT TP_STATE_S *pstState
)
{
    _TP_CTRL_S *pstCtrl = hTpHandle;
    _TP_SOCKET_S *pstTpSocket;

    Mem_Zero(pstState, sizeof(TP_STATE_S));

    if (uiTpId == 0)
    {
        return BS_BAD_PARA;
    }

    pstTpSocket = _TP_Socket_FindSocket(pstCtrl, uiTpId);
    if (NULL == pstTpSocket)
    {
        return BS_NO_SUCH;
    }

    pstState->eType = pstTpSocket->eType;
    pstState->uiFlag = pstTpSocket->uiFlag;
    pstState->uiProtocolId = pstTpSocket->uiProtocolId;
    pstState->uiLocalTpId = uiTpId;
    pstState->uiPeerTpId = pstTpSocket->uiPeerTpId;
    pstState->uiStatus = pstTpSocket->uiStatus;
    pstState->iSn = pstTpSocket->iSn;
    pstState->iAckSn = pstTpSocket->iAckSn;
    pstState->stChannel = pstTpSocket->stChannel;
    pstState->uiSendBufCount = MBUF_QUE_GET_COUNT(&pstTpSocket->stSendMbufQue);
    pstState->uiRecvBufCount = MBUF_QUE_GET_COUNT(&pstTpSocket->stRecvMbufQue);

    return BS_OK;
}

BS_STATUS TP_TimerStep(IN TP_HANDLE hTpHandle)
{
    _TP_CTRL_S *pstCtrl = hTpHandle;

    return VCLOCK_Step(pstCtrl->hVclockHandle);
}

CHAR * TP_GetStatusString(IN UINT uiStatus)
{
    static CHAR * apcString[_TP_STATUS_MAX] = 
    {
        "Init",
        "Syn_send",
        "Syn_recved",
        "Establish",
        "Fin_wait_1",
        "Fin_wait_2",
        "Closing",
        "Time_wait",
        "Close_wait",
        "Last_ack",
        "Closed"
    };

    if (uiStatus >= _TP_STATUS_MAX)
    {
        return "";
    }

    return apcString[uiStatus];
}
