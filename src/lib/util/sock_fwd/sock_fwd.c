/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2008-3-23
* Description: 我们将Host看作为A, Remote看作为B
* History:     
******************************************************************************/

#define RETCODE_FILE_NUM RETCODE_FILE_NUM_SOCKFWD

#include "bs.h"

#include "utl/net.h"        
#include "utl/socket_utl.h"        
#include "utl/ssltcp_utl.h"        
#include "utl/hookapi.h"
#include "utl/bit_opt.h"
#include "utl/rbuf.h"
#include "utl/sock_ses.h"
#include "utl/sock_fwd.h"

typedef struct tagSOCK_FWD_ACCEPT_EVENT_CALL_BACK_S
{
    DLL_NODE_S stDllNode;
    UINT ulEvent;
    PF_SOCK_FWD_ACCEPT_EVENT pfFunc;
    UINT ulUserHandle;
}_SOCK_FWD_ACCEPT_EVENT_S;

typedef struct tagSOCK_FWD_TRANS_EVENT_CALL_BACK_S
{
    DLL_NODE(tagSOCK_FWD_TRANS_EVENT_CALL_BACK_S) stDllNode;
    UINT ulEvent;
    PF_SOCK_FWD_TRANS_EVENT pfFunc;
    UINT ulUserHandle;
}_SOCK_FWD_TRANS_EVENT_S;


static UINT  g_ulSockFwdListenSslTcpId = 0;
static USHORT g_usSockFwdListenPort = 0;
static HANDLE  g_hSockFwdSesId = 0;
static UINT  g_ulSockFwdRemoteIp = 0;
static USHORT g_usSockFwdRemotePort = 0;
static DLL_HEAD_S g_stSockFwdAcceptEventList = DLL_HEAD_INIT_VALUE(&g_stSockFwdAcceptEventList);
static DLL_HEAD_S g_stSockFwdTransEventList = DLL_HEAD_INIT_VALUE(&g_stSockFwdTransEventList);



static BS_STATUS _SockFwd_Read(IN SOCK_SES_SIDE_S *pstSesSide)
{
    UCHAR *pucData;
    UINT ulDataLen;
    UINT ulReadLen;

    while (BIT_ISSET(pstSesSide->ulStatus, SOCK_SES_READABLE)
        && (! RBUF_IsFull(pstSesSide->hRbufId)))
    {
        RBUF_GetContinueWritePtr(pstSesSide->hRbufId, &pucData, &ulDataLen);
        
        if (BS_OK != SSLTCP_Read(pstSesSide->ulSslTcpId, pucData, ulDataLen, &ulReadLen))
        {
            SSLTCP_Close(pstSesSide->ulSslTcpId);
            pstSesSide->ulSslTcpId = 0;
            BIT_CLR(pstSesSide->ulStatus, SOCK_SES_READABLE);
            BIT_CLR(pstSesSide->ulStatus, SOCK_SES_WRITEABLE);
            BIT_SET(pstSesSide->ulStatus, SOCK_SES_CLOSED);
            break;
        }
    
        RBUF_MoveWriteIndex(pstSesSide->hRbufId, (INT)ulReadLen);
    
        if (ulDataLen > ulReadLen) 
        {
            BIT_CLR(pstSesSide->ulStatus, SOCK_SES_READABLE);
            break;
        }
    }

    return BS_OK;
}

static BS_STATUS _SockFwd_Write(IN SOCK_SES_SIDE_S *pstSesSideFrom, IN SOCK_SES_SIDE_S *pstSesSideTo)
{
    UCHAR *pucData;
    UINT ulDataLen;
    UINT ulWriteLen;

    if (! BIT_ISSET(pstSesSideTo->ulStatus, SOCK_SES_WRITEABLE))
    {
        return BS_OK;
    }

    if (RBUF_IsEmpty(pstSesSideFrom->hRbufId))
    {
        return BS_OK;
    }

    RBUF_ReadNoDel(pstSesSideFrom->hRbufId, &pucData, &ulDataLen);

    if (BS_OK != SSLTCP_Write(pstSesSideTo->ulSslTcpId, pucData, ulDataLen, &ulWriteLen))
    {
        SSLTCP_Close(pstSesSideTo->ulSslTcpId);
        pstSesSideTo->ulSslTcpId = 0;
        BIT_CLR(pstSesSideTo->ulStatus, SOCK_SES_READABLE);
        BIT_CLR(pstSesSideTo->ulStatus, SOCK_SES_WRITEABLE);
        BIT_SET(pstSesSideTo->ulStatus, SOCK_SES_CLOSED);
        return BS_OK;
    }

    RBUF_MoveReadIndex(pstSesSideFrom->hRbufId, ulWriteLen);
    
    if (ulWriteLen < ulDataLen)
    {
        BIT_CLR(pstSesSideTo->ulStatus, SOCK_SES_WRITEABLE);
    }

    return BS_OK;
}

static VOID _SockFwd_CloseIfNeed(IN SOCK_SES_SIDE_S *pstCloseIfNeed, IN SOCK_SES_SIDE_S *pstPeerSide)
{
    
    
    if (BIT_ISSET(pstPeerSide->ulStatus, SOCK_SES_CLOSED)
        && (RBUF_IsEmpty(pstPeerSide->hRbufId))
        && (! BIT_ISSET(pstCloseIfNeed->ulStatus, SOCK_SES_CLOSED)))
    {
        SSLTCP_Close(pstCloseIfNeed->ulSslTcpId);
        BIT_CLR(pstCloseIfNeed->ulStatus, SOCK_SES_READABLE);
        BIT_CLR(pstCloseIfNeed->ulStatus, SOCK_SES_WRITEABLE);
        BIT_SET(pstCloseIfNeed->ulStatus, SOCK_SES_CLOSED);
    }
}


static BS_STATUS _SockFwd_DealConnectionStaus(IN SOCK_SES_S *pstSesNode)
{
    _SockFwd_CloseIfNeed(&pstSesNode->stSideA, &pstSesNode->stSideB);
    _SockFwd_CloseIfNeed(&pstSesNode->stSideB, &pstSesNode->stSideA);

    
    if (BIT_ISSET(pstSesNode->stSideA.ulStatus, SOCK_SES_CLOSED) && BIT_ISSET(pstSesNode->stSideB.ulStatus, SOCK_SES_CLOSED))
    {
        SockSes_DelNode(g_hSockFwdSesId, pstSesNode);
    }

    return BS_OK;
}

static BS_STATUS _SockFwd_TransEventNotify(IN UINT ulEvent, IN SOCK_SES_S *pstSesNode)
{
    _SOCK_FWD_TRANS_EVENT_S *pstNode;
    
    DLL_SCAN(&g_stSockFwdTransEventList, pstNode)
    {
        if (pstNode->ulEvent & SOCK_FWD_EVENT_ACCEPT)
        {
            pstNode->pfFunc(SOCK_FWD_EVENT_ACCEPT, pstSesNode, pstNode->ulUserHandle);
        }
    }

    return BS_OK;
}

static VOID _SockFwd_Forward(IN SOCK_SES_S *pstSesNode, IN BOOL_T bIsFromAtoB)
{
    UINT ulEventBefore;
    UINT ulEventAfter;
    SOCK_SES_SIDE_S *pstFrom;
    SOCK_SES_SIDE_S *pstTo;

    if (bIsFromAtoB == TRUE)
    {
        pstFrom = &pstSesNode->stSideA;
        pstTo = &pstSesNode->stSideB;
        ulEventBefore = SOCK_FWD_EVENT_HOST_BEFORE_READ;
        ulEventAfter  = SOCK_FWD_EVENT_HOST_AFTER_READ;
    }
    else
    {
        pstFrom = &pstSesNode->stSideB;
        pstTo = &pstSesNode->stSideA;
        ulEventBefore = SOCK_FWD_EVENT_SERVER_BEFORE_READ;
        ulEventAfter  = SOCK_FWD_EVENT_SERVER_AFTER_READ;
    }
    
    while ((BIT_ISSET(pstFrom->ulStatus, SOCK_SES_READABLE) && (! RBUF_IsFull(pstFrom->hRbufId)))
        || (BIT_ISSET(pstTo->ulStatus, SOCK_SES_WRITEABLE) && (! RBUF_IsEmpty(pstFrom->hRbufId))))
    {
        if (BIT_ISSET(pstFrom->ulStatus, SOCK_SES_READABLE) && (! RBUF_IsFull(pstFrom->hRbufId)))
        {
            _SockFwd_TransEventNotify(ulEventBefore, pstSesNode);
            _SockFwd_Read(pstFrom);
            _SockFwd_TransEventNotify(ulEventAfter, pstSesNode);
        }

        if (BIT_ISSET(pstTo->ulStatus, SOCK_SES_WRITEABLE) && (! RBUF_IsEmpty(pstFrom->hRbufId)))
        {
            _SockFwd_Write(pstFrom, pstTo);
        }
    }

    _SockFwd_DealConnectionStaus(pstSesNode);

    if (BIT_ISSET(pstSesNode->stSideA.ulStatus, SOCK_SES_CLOSED))
    {
        _SockFwd_TransEventNotify(SOCK_FWD_EVENT_HOST_CLOSE, pstSesNode);
    }
    
    if (BIT_ISSET(pstSesNode->stSideB.ulStatus, SOCK_SES_CLOSED))
    {
        _SockFwd_TransEventNotify(SOCK_FWD_EVENT_SERVER_CLOSE, pstSesNode);
    }
    
}

static VOID _SockFwd_DealAReadMsg(IN SOCK_SES_S *pstSesNode)
{
    BIT_SET(pstSesNode->stSideA.ulStatus, SOCK_SES_READABLE);
    _SockFwd_Forward(pstSesNode, TRUE);
}

static VOID _SockFwd_DealBReadMsg(IN SOCK_SES_S *pstSesNode)
{
    BIT_SET(pstSesNode->stSideB.ulStatus, SOCK_SES_READABLE);
    _SockFwd_Forward(pstSesNode, FALSE);
}

static VOID _SockFwd_DealAWriteMsg(IN SOCK_SES_S *pstSesNode)
{
    BIT_SET(pstSesNode->stSideA.ulStatus, SOCK_SES_WRITEABLE);
    _SockFwd_Forward(pstSesNode, FALSE);
}

static VOID _SockFwd_DealBWriteMsg(IN SOCK_SES_S *pstSesNode)
{
    BIT_SET(pstSesNode->stSideB.ulStatus, SOCK_SES_WRITEABLE);
    _SockFwd_Forward(pstSesNode, TRUE);
}

static BS_STATUS _SockFwd_SocketEventFunc(IN UINT hSslTcpId, IN UINT ulEvent, IN USER_HANDLE_S *pstUserHandle)
{
    SOCK_SES_S *pstSesNode;
    UINT ulIsHost = HANDLE_UINT(pstUserHandle->ahUserHandle[0]);

    if (ulIsHost == TRUE)
    {
        pstSesNode = SockSes_GetNodeBySideA(g_hSockFwdSesId, hSslTcpId);
    }
    else
    {
        pstSesNode = SockSes_GetNodeBySideB(g_hSockFwdSesId, hSslTcpId);
    }
    
    if (NULL == pstSesNode)
    {
        SSLTCP_Close(hSslTcpId);
        return BS_OK;
    }

    if (ulEvent & SSLTCP_EVENT_READ)
    {
        if (ulIsHost == TRUE)
        {
            _SockFwd_DealAReadMsg(pstSesNode);
        }
        else
        {
            _SockFwd_DealBReadMsg(pstSesNode);
        }
    }

    if (ulEvent & SSLTCP_EVENT_WRITE)
    {
        if (ulIsHost == TRUE)
        {
            _SockFwd_DealAWriteMsg(pstSesNode);
        }
        else
        {
            _SockFwd_DealBWriteMsg(pstSesNode);
        }
    }

    return BS_OK;
}

static BS_STATUS _SockFwd_Accept(IN UINT hListenSslTcpId)
{
    UINT hAcceptSslTcpId = 0;
    UINT ulRemoteSslTcpId;
    BS_STATUS eRet;
    _SOCK_FWD_ACCEPT_EVENT_S *pstNode;
    USER_HANDLE_S stUserHandle;

    if (BS_OK != SSLTCP_Accept(hListenSslTcpId, &hAcceptSslTcpId))
    {
        RETURN(BS_ERR);
    }

    
    if (0 == (ulRemoteSslTcpId = SSLTCP_Create("tcp", AF_INET, NULL)))
    {
        SSLTCP_Close(hAcceptSslTcpId);
        RETURN(BS_ERR);
    }

    if (BS_OK != SSLTCP_Connect(ulRemoteSslTcpId, g_ulSockFwdRemoteIp, g_usSockFwdRemotePort))
    {
        SSLTCP_Close(ulRemoteSslTcpId);
        SSLTCP_Close(hAcceptSslTcpId);

        RETURN(BS_CAN_NOT_CONNECT);
    }

    if (BS_OK != (eRet = SockSes_AddNode(g_hSockFwdSesId, hAcceptSslTcpId, ulRemoteSslTcpId)))
    {
        SSLTCP_Close(ulRemoteSslTcpId);
        SSLTCP_Close(hAcceptSslTcpId);

        return eRet;
    }

    DLL_SCAN(&g_stSockFwdAcceptEventList, pstNode)
    {
        if (pstNode->ulEvent & SOCK_FWD_EVENT_ACCEPT)
        {
            pstNode->pfFunc(SOCK_FWD_EVENT_ACCEPT, hAcceptSslTcpId, pstNode->ulUserHandle);
        }
    }

    stUserHandle.ahUserHandle[0] = (HANDLE)TRUE;
    SSLTCP_SetAsyn(NULL, hAcceptSslTcpId, SSLTCP_EVENT_ALL, _SockFwd_SocketEventFunc, &stUserHandle);
    stUserHandle.ahUserHandle[0] = (HANDLE)FALSE;
    SSLTCP_SetAsyn(NULL, ulRemoteSslTcpId, SSLTCP_EVENT_ALL, _SockFwd_SocketEventFunc, &stUserHandle);

    return BS_OK;
}

static BS_STATUS _SockFwd_ListenSocketEventFunc(IN UINT hSslTcpId, IN UINT ulEvent, IN USER_HANDLE_S *pstUserHandle)
{
    if (ulEvent & SSLTCP_EVENT_READ)
    {
        _SockFwd_Accept(hSslTcpId);
    }

    if (ulEvent & SSLTCP_EVENT_EXECPT)
    {
        RETURN(BS_ERR);
    }

    return BS_OK;
}

static BS_STATUS _SockFwd_Initlisten()
{
    BS_STATUS eRet;
	USER_HANDLE_S stUserHandle;

    
    g_ulSockFwdListenSslTcpId = SSLTCP_Create("tcp", AF_INET, NULL);
    if (0 == g_ulSockFwdListenSslTcpId)
    {
        BS_WARNNING(("Can't create ssltcp"));
        RETURN(BS_ERR);
    }

    eRet = SSLTCP_Listen(g_ulSockFwdListenSslTcpId, LOOPBACK_IP, 0, 10);
    if (BS_OK != eRet)
    {
        SSLTCP_Close(g_ulSockFwdListenSslTcpId);
        g_ulSockFwdListenSslTcpId = 0;
        BS_WARNNING(("Can't listen ssltcp, ret value=0x%08x", eRet));
        return eRet;
    }

    g_usSockFwdListenPort = SSLTCP_GetHostPort(g_ulSockFwdListenSslTcpId);
    if (0 == g_usSockFwdListenPort)
    {
        SSLTCP_Close(g_ulSockFwdListenSslTcpId);
        g_ulSockFwdListenSslTcpId = 0;
        BS_WARNNING(("Can't get host port, ret value=0x%08x", eRet));
        return eRet;
    }
    
    SSLTCP_SetAsyn(NULL, g_ulSockFwdListenSslTcpId, SSLTCP_EVENT_ALL, _SockFwd_ListenSocketEventFunc, &stUserHandle);

    return BS_OK;
}

USHORT SockFwd_GetRdtListenPort()
{
    return g_usSockFwdListenPort;
}

BS_STATUS SockFwd_AddSend2ServerData(IN SOCK_SES_S *pstSesNode, IN UCHAR *pucData, IN UINT ulDataLen, OUT UINT *pulWriteLen)
{
    UINT ulWriteLen;

    if (BS_OK != RBUF_Write(pstSesNode->stSideA.hRbufId, pucData, ulDataLen, &ulWriteLen))
    {
        RETURN(BS_ERR);
    }

    *pulWriteLen = ulWriteLen;
    
    return BS_OK;
}

BS_STATUS SockFwd_RegAcceptEvent(IN UINT ulEvent, IN PF_SOCK_FWD_ACCEPT_EVENT pfFunc, IN UINT ulUserHandle)
{
    _SOCK_FWD_ACCEPT_EVENT_S *pstNode;

    pstNode = MEM_ZMalloc(sizeof(_SOCK_FWD_ACCEPT_EVENT_S));
    if (NULL == pstNode)
    {
        RETURN(BS_NO_MEMORY);
    }

    pstNode->ulEvent = ulEvent;
    pstNode->pfFunc = pfFunc;
    pstNode->ulUserHandle = ulUserHandle;

    DLL_ADD((DLL_HEAD_S*)&g_stSockFwdAcceptEventList, pstNode);

    return BS_OK;
}

BS_STATUS SockFwd_RegTransEvent(IN UINT ulEvent, IN PF_SOCK_FWD_TRANS_EVENT pfFunc, IN UINT ulUserHandle)
{
    _SOCK_FWD_TRANS_EVENT_S *pstNode;

    pstNode = MEM_ZMalloc(sizeof(_SOCK_FWD_TRANS_EVENT_S));
    if (NULL == pstNode)
    {
        RETURN(BS_NO_MEMORY);
    }

    pstNode->ulEvent = ulEvent;
    pstNode->pfFunc = pfFunc;
    pstNode->ulUserHandle = ulUserHandle;

    DLL_ADD((DLL_HEAD_S*)&g_stSockFwdTransEventList, pstNode);

    return BS_OK;
}


BS_STATUS SockFwd_Init(IN UINT ulServerIp, IN USHORT usServerPort)
{
    BS_STATUS eRet;

    g_ulSockFwdRemoteIp = ulServerIp;
    g_usSockFwdRemotePort = usServerPort;

    eRet = SockSes_CreateInstance(TRUE, &g_hSockFwdSesId);
    if (BS_OK != eRet)
    {
        return eRet;
    }

    eRet = _SockFwd_Initlisten();
    if (BS_OK != eRet)
    {
        SockSes_DeleteInstance(g_hSockFwdSesId);
        g_hSockFwdSesId = 0;
        return eRet;
    }    

    return BS_OK;
}


