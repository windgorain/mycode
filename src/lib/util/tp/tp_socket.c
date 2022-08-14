
#include "bs.h"

#include "utl/tp_utl.h"
#include "utl/nap_utl.h"
#include "utl/bit_opt.h"

#include "tp_inner.h"

#define _TP_SEND_MBUF_COUNT 16   /* 发送缓冲区大小, 单位为MBuf个数  */
#define _TP_RECV_MBUF_COUNT 16   /* 接收缓冲区大小, 单位为MBuf个数  */

static VOID tp_socket_DeleteAllTimer(IN _TP_CTRL_S *pstCtrl, IN _TP_SOCKET_S *pstTpSocket)
{
    KA_Stop(&pstTpSocket->stKeepAlive);

    if (pstTpSocket->stResend.hResendTImer != NULL)
    {
        VCLOCK_DestroyTimer(pstCtrl->hVclockHandle, pstTpSocket->stResend.hResendTImer);
        pstTpSocket->stResend.hResendTImer = NULL;
    }
}

BS_STATUS _TP_Socket_Init(_TP_CTRL_S *pstCtrl)
{
    NAP_PARAM_S param = {0};

    param.enType = NAP_TYPE_HASH;
    param.uiMaxNum = _TP_ID_MAX;
    param.uiNodeSize = sizeof(_TP_SOCKET_S);

    pstCtrl->hSocketNap = NAP_Create(&param);
    if (NULL == pstCtrl->hSocketNap)
    {
        return BS_NO_MEMORY;
    }

    NAP_EnableSeq(pstCtrl->hSocketNap, 0, _TP_ID_MAX);

    return BS_OK;
}

VOID _TP_Socket_UnInit(_TP_CTRL_S *pstCtrl)
{
    if (NULL == pstCtrl->hSocketNap)
    {
        NAP_Destory(pstCtrl->hSocketNap);
        pstCtrl->hSocketNap = NULL;
    }
}

_TP_SOCKET_S * _TP_Socket_NewSocket
(
    IN _TP_CTRL_S *pstCtrl,
    IN TP_TYPE_E eType
)
{
    _TP_SOCKET_S * pstTpSocket;
    HANDLE phPropertys = NULL;

    pstTpSocket = NAP_ZAlloc(pstCtrl->hSocketNap);
    if (NULL == pstTpSocket)
    {
        return NULL;
    }

    pstTpSocket->hEvent = Event_Create();
    if (NULL == pstTpSocket->hEvent)
    {
        NAP_Free(pstCtrl->hSocketNap, pstTpSocket);
        return NULL;
    }

    if (pstCtrl->uiMaxPropertys > 0)
    {
        phPropertys = MEM_ZMalloc(sizeof(HANDLE) * pstCtrl->uiMaxPropertys);
        if (NULL == phPropertys)
        {
            Event_Delete(pstTpSocket->hEvent);
            NAP_Free(pstCtrl->hSocketNap, pstTpSocket);
            return NULL;
        }
    }

    MBUF_QUE_INIT(&pstTpSocket->stSendMbufQue, _TP_SEND_MBUF_COUNT);
    MBUF_QUE_INIT(&pstTpSocket->stRecvMbufQue, _TP_RECV_MBUF_COUNT);
    pstTpSocket->eType = eType;
    pstTpSocket->uiLocalTpId = (UINT)NAP_GetIDByNode(pstCtrl->hSocketNap, pstTpSocket);
    pstTpSocket->phPropertys = phPropertys;

    _TP_PKT_InitKeepAlive(pstCtrl, pstTpSocket);

    return pstTpSocket;
}

VOID _TP_Socket_FreeSocket(IN _TP_CTRL_S *pstCtrl, IN _TP_SOCKET_S *pstTpSocket)
{
    MBUF_QUE_FREE_ALL(&pstTpSocket->stRecvMbufQue);
    MBUF_QUE_FREE_ALL(&pstTpSocket->stSendMbufQue);

    tp_socket_DeleteAllTimer(pstCtrl, pstTpSocket);

    Event_Delete(pstTpSocket->hEvent);

    if (pstTpSocket->phPropertys != NULL)
    {
        MEM_Free(pstTpSocket->phPropertys);
    }

    NAP_Free(pstCtrl->hSocketNap, pstTpSocket);
}

_TP_SOCKET_S * _TP_Socket_FindSocket(IN _TP_CTRL_S *pstCtrl, IN TP_ID uiTpId)
{
    _TP_SOCKET_S *pstSocket;

    pstSocket = NAP_GetNodeByID(pstCtrl->hSocketNap, uiTpId);

    return pstSocket;
}

static BS_STATUS tp_socket_SetOptNbio
(
    IN _TP_SOCKET_S *pstTpSocket,
    IN VOID *pValue
)
{
    UINT *puiNbio = pValue;

    if (*puiNbio)
    {
        BIT_SET(pstTpSocket->uiFlag, _TP_SOCKET_FLAG_NBIO);
    }
    else
    {
        BIT_CLR(pstTpSocket->uiFlag, _TP_SOCKET_FLAG_NBIO);
    }

    return BS_OK;
}

static BS_STATUS tp_socket_SetOptKeepAliveTime
(
    IN _TP_CTRL_S *pstCtrl,
    IN _TP_SOCKET_S *pstTpSocket,
    IN VOID *pValue
)
{
    TP_OPT_KEEP_ALIVE_S *pstKeepAlive = pValue;

    pstTpSocket->stKeepAlive.stSet.usIdle = pstKeepAlive->usIdle;
    pstTpSocket->stKeepAlive.stSet.usIntval = pstKeepAlive->usIntval;
    pstTpSocket->stKeepAlive.stSet.usMaxProbeCount = pstKeepAlive->usMaxProbeCount;

    _TP_PKT_ResetKeepAlive(pstCtrl, pstTpSocket);

    return BS_OK;
}

BS_STATUS _TP_Socket_SetOpt
(
    IN _TP_CTRL_S *pstCtrl,
    IN _TP_SOCKET_S *pstTpSocket,
    IN TP_OPT_E enOpt,
    IN VOID *pValue
)
{
    BS_STATUS eRet = BS_OK;

    switch (enOpt)
    {
        case TP_OPT_NBIO:
        {
            eRet = tp_socket_SetOptNbio(pstTpSocket, pValue);
            break;
        }

        case TP_OPT_KEEPALIVE_TIME:
        {
            eRet = tp_socket_SetOptKeepAliveTime(pstCtrl, pstTpSocket, pValue);
            break;
        }

        default:
        {
            eRet = BS_NOT_SUPPORT;
            break;
        }
    }

    return eRet;
}

VOID _TP_Socket_WakeUp
(
    IN _TP_CTRL_S *pstCtrl,
    IN _TP_SOCKET_S *pstTpSocket,
    IN UINT uiEvent
)
{
	Event_Write(pstTpSocket->hEvent, uiEvent);
    pstCtrl->pfEventFunc(pstCtrl, pstTpSocket->uiLocalTpId, uiEvent, &pstCtrl->stUserHandle);
}

VOID _TP_Socket_Abort(IN _TP_CTRL_S *pstCtrl, IN _TP_SOCKET_S *pstTpSocket)
{
    _TP_PROTOCOL_S * pstProtocol;
    
    if (pstTpSocket->uiFlag & _TP_SOCKET_FLAG_ACCEPTING)
    {
        pstProtocol = _TP_Protocol_FindProtocol(pstCtrl, pstTpSocket->uiProtocolId);
        if (NULL != pstProtocol)
        {
            _TP_Protocol_DelAccepting(pstProtocol, pstTpSocket);
        }
    }

    pstTpSocket->uiStatus = _TP_STATUS_CLOSED;
    _TP_Socket_WakeUp(pstCtrl, pstTpSocket, TP_EVENT_ERR);
}

BS_STATUS _TP_Socket_Walk
(
    IN _TP_CTRL_S *pstCtrl,
    IN PF_TP_SOCKET_WALK_FUNC pfFunc,
    IN HANDLE hUserHandle
)
{
    _TP_SOCKET_S *pstNode;
    UINT uiIndex = NAP_INVALID_INDEX;

    while ((uiIndex = NAP_GetNextIndex(pstCtrl->hSocketNap, uiIndex))
            != NAP_INVALID_INDEX) {
        pstNode = NAP_GetNodeByIndex(pstCtrl->hSocketNap, uiIndex);
        pfFunc(pstCtrl, pstNode, hUserHandle);
    }

    return BS_OK;
}

