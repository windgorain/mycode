
#include "bs.h"

#include "utl/tp_utl.h"
#include "utl/nap_utl.h"
#include "utl/bit_opt.h"

#include "tp_inner.h"

BS_STATUS _TP_Protocol_Init(IN _TP_CTRL_S *pstCtrl)
{
    NAP_PARAM_S param = {0};

    param.enType = NAP_TYPE_HASH;
    param.uiMaxNum = _TP_PROTO_MAX;
    param.uiNodeSize = sizeof(_TP_PROTOCOL_S);

    pstCtrl->hProtocolNap = NAP_Create(&param);
    if (NULL == pstCtrl->hProtocolNap)
    {
        return BS_NO_MEMORY;
    }

    return BS_OK;
}

VOID _TP_Protocol_UnInit(IN _TP_CTRL_S *pstCtrl)
{
    if (pstCtrl->hProtocolNap)
    {
        NAP_Destory(pstCtrl->hProtocolNap);
        pstCtrl->hProtocolNap = NULL;
    }
}

_TP_PROTOCOL_S * _TP_Protocol_FindProtocol(IN _TP_CTRL_S *pstCtrl, IN UINT uiProtocolId)
{
    return NAP_GetNodeByID(pstCtrl->hProtocolNap, uiProtocolId);
}

BS_STATUS _TP_Protocol_BindProtocol
(
    IN _TP_CTRL_S *pstCtrl,
    IN _TP_SOCKET_S *pstTpSocket,
    IN UINT uiProtocolId
)
{
    _TP_PROTOCOL_S *pstProtocol;

    if (pstTpSocket->eType != TP_TYPE_PROTOCOL)
    {
        return BS_NOT_SUPPORT;
    }

    if (pstTpSocket->uiProtocolId != 0)
    {
        return BS_ALREADY_EXIST;
    }

    if (_TP_Protocol_FindProtocol(pstCtrl, uiProtocolId) != NULL)
    {
        return BS_CONFLICT;
    }

    pstProtocol = NAP_ZAllocByID(pstCtrl->hProtocolNap, uiProtocolId);
    if (NULL == pstProtocol)
    {
        return BS_NO_MEMORY;
    }

    pstProtocol->uiProtocolId = uiProtocolId;
    pstProtocol->uiLocalTpId = pstTpSocket->uiLocalTpId;
    pstTpSocket->uiProtocolId = uiProtocolId;

    return BS_OK;
}

BS_STATUS _TP_Protocol_UnBindProtocol
(
    IN _TP_CTRL_S *pstCtrl,
    IN _TP_SOCKET_S *pstTpSocket
)
{
    _TP_PROTOCOL_S *pstProtocol;
    UINT i;

    if (pstTpSocket->eType != TP_TYPE_PROTOCOL)
    {
        return BS_OK;
    }

    if (pstTpSocket->uiProtocolId == 0)
    {
        return BS_OK;
    }

    pstProtocol = _TP_Protocol_FindProtocol(pstCtrl, pstTpSocket->uiProtocolId);
    BS_DBGASSERT(NULL != pstProtocol);

    for (i=0; i<_TP_PROTOCOL_MAX_ACCEPT_NUM; i++)
    {
        if (pstProtocol->astAcceptting[i] != NULL)
        {
            _TP_Socket_FreeSocket(pstCtrl, pstProtocol->astAcceptting[i]);
            pstProtocol->astAcceptting[i] = NULL;
        }
    }

    NAP_Free(pstCtrl->hProtocolNap, pstProtocol);

    pstTpSocket->uiProtocolId = 0;

    return BS_OK;
}

BS_STATUS _TP_Protocol_Listen(IN _TP_CTRL_S *pstCtrl, IN _TP_SOCKET_S *pstTpSocket)
{
    _TP_PROTOCOL_S *pstProtocol;


    if (pstTpSocket->eType != TP_TYPE_PROTOCOL)
    {
        return BS_NOT_SUPPORT;
    }

    if (pstTpSocket->uiProtocolId == 0)
    {
        return BS_NOT_READY;
    }

    pstProtocol = _TP_Protocol_FindProtocol(pstCtrl, pstTpSocket->uiProtocolId);
    BS_DBGASSERT(NULL != pstProtocol);
    BS_DBGASSERT(pstProtocol->uiLocalTpId == pstTpSocket->uiLocalTpId);

    BIT_SET(pstProtocol->uiFlag, _TP_PROTOCOL_FLAG_LISTEN);

    return BS_OK;
}

BS_STATUS _TP_Protocol_Accept
(
    IN _TP_CTRL_S *pstCtrl,
    IN _TP_SOCKET_S *pstTpSocket,
    OUT TP_ID *puiAcceptTpId
)
{
    _TP_PROTOCOL_S *pstProtocol;
    UINT i;
    _TP_SOCKET_S *pstTpAcceptSocket;
    UINT uiAcceptTpId = 0;

    if (pstTpSocket->eType != TP_TYPE_PROTOCOL)
    {
        return BS_NOT_SUPPORT;
    }

    if (pstTpSocket->uiProtocolId == 0)
    {
        return BS_NOT_READY;
    }
    
    pstProtocol = _TP_Protocol_FindProtocol(pstCtrl, pstTpSocket->uiProtocolId);
    BS_DBGASSERT(NULL != pstProtocol);

    for (i=0; i<_TP_PROTOCOL_MAX_ACCEPT_NUM; i++)
    {
        if (pstProtocol->astAcceptting[i] != NULL)
        {
            pstTpAcceptSocket = pstProtocol->astAcceptting[i];
            if (pstTpAcceptSocket->uiStatus == _TP_STATUS_ESTABLISH)
            {
                uiAcceptTpId = pstTpAcceptSocket->uiLocalTpId;
                BIT_CLR(pstTpAcceptSocket->uiFlag, _TP_SOCKET_FLAG_ACCEPTING);
                pstProtocol->astAcceptting[i] = NULL;
                pstProtocol->uiAccepttingNum --;
                break;
            }
        }
    }

    *puiAcceptTpId = uiAcceptTpId;

    return BS_OK;
}

BOOL_T _TP_Protocol_CanAccepting
(
    IN _TP_PROTOCOL_S *pstProtocol
)
{
    if ((pstProtocol->uiFlag & _TP_PROTOCOL_FLAG_LISTEN) == 0)
    {
        return FALSE;
    }

    if (pstProtocol->uiAccepttingNum >= _TP_PROTOCOL_MAX_ACCEPT_NUM)
    {
        return FALSE;
    }

    return TRUE;
}

BS_STATUS _TP_Protocol_AddAccepting
(
    IN _TP_PROTOCOL_S *pstProtocol,
    IN _TP_SOCKET_S *pstTpSocket
)
{
    UINT i;
    BS_STATUS eRet = BS_NO_RESOURCE;

    for (i=0; i<_TP_PROTOCOL_MAX_ACCEPT_NUM; i++)
    {
        if (pstProtocol->astAcceptting[i] == NULL)
        {
            pstProtocol->astAcceptting[i] = pstTpSocket;
            BIT_SET(pstTpSocket->uiFlag, _TP_SOCKET_FLAG_ACCEPTING);
            pstProtocol->uiAccepttingNum ++;
            eRet = BS_OK;
            break;
        }
    }

    return eRet;
}

VOID _TP_Protocol_DelAccepting
(
    IN _TP_PROTOCOL_S *pstProtocol,
    IN _TP_SOCKET_S *pstTpSocket
)
{
    UINT i;

    if (pstTpSocket == 0)
    {
        return;
    }

    for (i=0; i<_TP_PROTOCOL_MAX_ACCEPT_NUM; i++)
    {
        if (pstProtocol->astAcceptting[i] == pstTpSocket)
        {
            BIT_CLR(pstTpSocket->uiFlag, _TP_SOCKET_FLAG_ACCEPTING);
            pstProtocol->astAcceptting[i] = NULL;
            pstProtocol->uiAccepttingNum --;
            break;
        }
    }
}


