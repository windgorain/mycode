/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2015-7-25
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/file_utl.h"
#include "utl/txt_utl.h"
#include "utl/socket_utl.h"
#include "utl/dns_utl.h"
#include "utl/vbuf_utl.h"
#include "utl/ws_utl.h"

#include "../h/svpn_def.h"
#include "../h/svpn_debug.h"
#include "../h/svpn_context.h"

#include "svpn_tcprelay_inner.h"

#define SVPN_TCPRELAY_MAX_BUF_CACHED_SIZE 4096


static BS_STATUS svpn_tcprelay_FlushData(IN HANDLE hVBuf, IN CONN_HANDLE hConn)
{
    UCHAR *pucData;
    UINT uiLen;
    INT iWriteLen;

    uiLen = VBUF_GetDataLength(hVBuf);

    if (uiLen == 0)
    {
        return BS_OK;
    }

    pucData = VBUF_GetData(hVBuf);
    if (NULL == pucData)
    {
        return BS_ERR;
    }

    iWriteLen = CONN_Write(hConn, pucData, uiLen);
    if (iWriteLen == SOCKET_E_AGAIN)
    {
        return BS_OK;
    }

    if (iWriteLen < 0)
    {
        return BS_ERR;
    }

    if ((UINT)iWriteLen == uiLen)
    {
        VBUF_Clear(hVBuf);
    }
    else
    {
        VBUF_EarseHead(hVBuf, (UINT)iWriteLen);
    }

    return BS_OK;
}

static void svpn_tcprelay_Close(SVPN_TCPRELAY_NODE_S *pstNode, CONN_HANDLE hConn)
{
    CONN_Free(hConn);
    if (hConn == pstNode->hDownConn)
    {
        pstNode->hDownConn = NULL;
        if (VBUF_GetDataLength(&pstNode->stDownVBuf) == 0)
        {
            CONN_Free(pstNode->hUpConn);
            pstNode->hUpConn = NULL;
        }
    }
    else
    {
        pstNode->hUpConn= NULL;
        if (VBUF_GetDataLength(&pstNode->stUpVBuf) == 0)
        {
            CONN_Free(pstNode->hDownConn);
            pstNode->hDownConn = NULL;
        }
    }
}

static BS_STATUS svpn_tcprelay_FwdData
(
    IN SVPN_TCPRELAY_NODE_S *pstNode,
    IN HANDLE hVBuf,
    IN CONN_HANDLE hSrcConn,
    IN CONN_HANDLE hDstConn
)
{
    UCHAR aucData[1024];
    INT iReadLen;

    if (hDstConn == NULL)
    {
        VBUF_Clear(hVBuf);
        CONN_DelEvent(hSrcConn, MYPOLL_EVENT_IN);
        return BS_OK;
    }

    if (BS_OK != svpn_tcprelay_FlushData(hVBuf, hDstConn))
    {
        return BS_ERR;
    }

    if (VBUF_GetDataLength(hVBuf) > 0)
    {
        return BS_OK;
    }

    if (hSrcConn == NULL)
    {
        return BS_STOP;
    }

    while (VBUF_GetDataLength(hVBuf) < SVPN_TCPRELAY_MAX_BUF_CACHED_SIZE)
    {
        iReadLen = CONN_Read(hSrcConn, aucData, sizeof(aucData));
        if (iReadLen <= 0)
        {
            if (iReadLen != SOCKET_E_AGAIN)
            {
                svpn_tcprelay_Close(pstNode, hSrcConn);
                hSrcConn = NULL;
            }
            break;
        }
        else
        {
            VBUF_CatBuf(hVBuf, aucData, iReadLen);
        }
    }

    if ((pstNode->hDownConn == NULL) && (pstNode->hUpConn == NULL))
    {
        return BS_ERR;
    }

    if ((VBUF_GetDataLength(hVBuf) == 0) && (CONN_SslPending(hSrcConn) == 0))
    {
        CONN_AddEvent(hSrcConn, MYPOLL_EVENT_IN);
        CONN_DelEvent(hDstConn, MYPOLL_EVENT_OUT);
    }
    else
    {
        CONN_DelEvent(hSrcConn, MYPOLL_EVENT_IN);
        CONN_AddEvent(hDstConn, MYPOLL_EVENT_OUT);
    }

    return BS_OK;
}

static int svpn_tcprelay_DownEvent(IN INT iSocketId, IN UINT uiEvent, IN USER_HANDLE_S *pstUserHandle)
{
    SVPN_TCPRELAY_NODE_S *pstNode = pstUserHandle->ahUserHandle[0];
    BS_STATUS eRet = BS_OK;

    SVPN_DBG_OUTPUT(SVPN_DBG_ID_TCP_RELAY, SVPN_DBG_FLAG_TR_EVENT,
            "Down socket(%d) event 0x%x.\r\n", iSocketId, uiEvent);

    if (uiEvent & MYPOLL_EVENT_ERR)
    {   
        if ((pstNode->hUpConn != NULL) && (VBUF_GetDataLength(&pstNode->stDownVBuf) > 0))
        {
            CONN_Free(pstNode->hDownConn);
            pstNode->hDownConn = NULL;
        }
        else
        {
            SVPN_TcpRelayNode_Free(pstNode);
        }

        return 0;
    }

    if (uiEvent & MYPOLL_EVENT_IN)
    {
        eRet = svpn_tcprelay_FwdData(pstNode, &pstNode->stDownVBuf, pstNode->hDownConn, pstNode->hUpConn);
    }

    if ((eRet == BS_OK) && (uiEvent & MYPOLL_EVENT_OUT))
    {
        eRet = svpn_tcprelay_FwdData(pstNode, &pstNode->stUpVBuf, pstNode->hUpConn, pstNode->hDownConn);
    }

    if ((eRet != BS_OK) || ((pstNode->hDownConn == NULL) && (pstNode->hUpConn == NULL)))
    {
        SVPN_TcpRelayNode_Free(pstNode);
    }

    return 0;
}

static int svpn_tcprelay_UpEvent(IN INT iSocketId, IN UINT uiEvent, IN USER_HANDLE_S *pstUserHandle)
{
    SVPN_TCPRELAY_NODE_S *pstNode = pstUserHandle->ahUserHandle[0];
    BS_STATUS eRet = BS_OK;

    SVPN_DBG_OUTPUT(SVPN_DBG_ID_TCP_RELAY, SVPN_DBG_FLAG_TR_EVENT,
            "Up socket(%d) event 0x%x.\r\n", iSocketId, uiEvent);

    if (uiEvent & MYPOLL_EVENT_ERR)
    {
        if ((pstNode->hDownConn != NULL) && (VBUF_GetDataLength(&pstNode->stUpVBuf) > 0))
        {
            CONN_Free(pstNode->hUpConn);
            pstNode->hUpConn = NULL;
        }
        else
        {
            SVPN_TcpRelayNode_Free(pstNode);
        }

        return 0;
    }

    if (uiEvent & MYPOLL_EVENT_IN)
    {
        eRet = svpn_tcprelay_FwdData(pstNode, &pstNode->stUpVBuf, pstNode->hUpConn, pstNode->hDownConn);
    }

    if ((eRet == BS_OK) && (uiEvent & MYPOLL_EVENT_OUT))
    {
        eRet = svpn_tcprelay_FwdData(pstNode, &pstNode->stDownVBuf, pstNode->hDownConn, pstNode->hUpConn);
    }

    if ((eRet != BS_OK) || ((pstNode->hDownConn == NULL) && (pstNode->hUpConn == NULL)))
    {
        SVPN_TcpRelayNode_Free(pstNode);
    }

    return 0;
}

static BS_STATUS svpn_tcprelay_StartFwd(IN SVPN_TCPRELAY_NODE_S *pstNode)
{
    USER_HANDLE_S stUserHandle;
    BS_STATUS eRet = BS_OK;

    stUserHandle.ahUserHandle[0] = pstNode;

    eRet = CONN_SetEvent(pstNode->hDownConn, MYPOLL_EVENT_IN, svpn_tcprelay_DownEvent, &stUserHandle);
    eRet |= CONN_SetEvent(pstNode->hUpConn, MYPOLL_EVENT_IN, svpn_tcprelay_UpEvent, &stUserHandle);

    if (eRet != BS_OK)
    {
        return BS_ERR;
    }

    return BS_OK;
}

static int svpn_tcprelay_ConnectEvent(IN INT iSocketId, IN UINT uiEvent, IN USER_HANDLE_S *pstUserHandle)
{
    SVPN_TCPRELAY_NODE_S *pstNode = pstUserHandle->ahUserHandle[0];

    SVPN_DBG_OUTPUT(SVPN_DBG_ID_TCP_RELAY, SVPN_DBG_FLAG_TR_EVENT,
            "Up socket(%d) event 0x%x.\r\n", iSocketId, uiEvent);

    if (uiEvent & MYPOLL_EVENT_ERR)
    {
        CONN_WriteString(pstNode->hDownConn, "HTTP/1.1 503 ERR\r\n\r\n");
        SVPN_TcpRelayNode_Free(pstNode);
        return 0;
    }

    CONN_WriteString(pstNode->hDownConn, "HTTP/1.1 200 OK\r\n\r\n");
    
    if (BS_OK != svpn_tcprelay_StartFwd(pstNode))
    {
        SVPN_TcpRelayNode_Free(pstNode);
        return 0;
    }

    return 0;
}

static BS_STATUS svpn_tcprelay_ParserServerField
(
    IN WS_TRANS_HANDLE hWsTrans,
    OUT CHAR szServer[DNS_MAX_LABEL_LEN + 1],
    OUT USHORT *pusPort
)
{
    HTTP_HEAD_PARSER hParser;
    CHAR *pcServer;
    LSTR_S stServer;
    LSTR_S stPort;
    UINT uiPort;

    hParser = WS_Trans_GetHttpRequestParser(hWsTrans);
    if (NULL == hParser)
    {
        return BS_ERR;
    }

    pcServer = HTTP_GetHeadField(hParser, "server");
    if (NULL == pcServer)
    {
        return BS_ERR;
    }

    TXT_StrSplit(pcServer, ':', &stServer, &stPort);

    if ((stServer.uiLen == 0) || (stServer.uiLen > DNS_MAX_LABEL_LEN))
    {
        return BS_ERR;
    }

    if (stPort.uiLen == 0)
    {
        return BS_ERR;
    }

    if (BS_OK != TXT_AtouiWithCheck(stPort.pcData, &uiPort))
    {
        return BS_ERR;
    }

    if ((uiPort == 0) || (uiPort >= 65535))
    {
        return BS_ERR;
    }

    TXT_Strlcpy(szServer, stServer.pcData, stServer.uiLen + 1);
    *pusPort = uiPort;

    return BS_OK;
}

static BS_STATUS svpn_tcprelay_ConnectServer(IN SVPN_TCPRELAY_NODE_S *pstNode)
{
    INT iSocket;
    UINT uiIp;
    CONN_HANDLE hUpConn;
    BS_STATUS eRet;
    USER_HANDLE_S stUserHandle;

    uiIp = Socket_NameToIpHost(pstNode->szServer);
    if (0 == uiIp)
    {
        return BS_ERR;
    }

    iSocket = Socket_Create(AF_INET, SOCK_STREAM);
    if (iSocket < 0)
    {
        return BS_ERR;
    }

    SVPN_DBG_OUTPUT(SVPN_DBG_ID_TCP_RELAY, SVPN_DBG_FLAG_TR_PROCESS,
            "Connect socket(%d) to server %s:%d.\r\n", iSocket, pstNode->szServer, pstNode->usPort);

    Socket_SetNoBlock(iSocket, TRUE);

    hUpConn = CONN_New(iSocket);
    if (NULL == hUpConn)
    {
        Socket_Close(iSocket);
        return BS_ERR;
    }

    CONN_SetUserData(hUpConn, CONN_USER_DATA_INDEX_0, CONN_GetUserData(pstNode->hDownConn, CONN_USER_DATA_INDEX_0));

    CONN_SetPoller(hUpConn, CONN_GetPoller(pstNode->hDownConn));

    pstNode->hUpConn = hUpConn;

    stUserHandle.ahUserHandle[0] = pstNode;
    CONN_SetEvent(pstNode->hUpConn, MYPOLL_EVENT_OUT | MYPOLL_EVENT_ERR,
        svpn_tcprelay_ConnectEvent, &stUserHandle);
    CONN_SetEvent(pstNode->hDownConn, 0, svpn_tcprelay_DownEvent, &stUserHandle);

    eRet = Socket_Connect(iSocket, uiIp, pstNode->usPort);
    if ((eRet != BS_OK) && (eRet != SOCKET_E_AGAIN))
    {
        CONN_Free(hUpConn);
        pstNode->hUpConn = NULL;
        return BS_ERR;
    }

    return BS_OK;
}

static BOOL_T svpn_tcprelay_CheckPermit(IN WS_TRANS_HANDLE hWsTrans, IN CHAR *pcServer, IN USHORT usPort)
{
    return TRUE;
}

static VOID svpn_tcprelay_RecvHeadOK(IN WS_TRANS_HANDLE hWsTrans)
{
    WS_CONN_HANDLE hWsConn;
    CONN_HANDLE hDownConn;
    CHAR szServer[DNS_MAX_LABEL_LEN + 1];
    USHORT usPort;
    SVPN_TCPRELAY_NODE_S *pstNode;
    SVPN_CONTEXT_HANDLE hSvpnContext;

    hWsConn = WS_Trans_GetConn(hWsTrans);
    hDownConn = WS_Conn_GetRawConn(hWsConn);
    WS_Conn_ClearRawConn(hWsConn);
    CONN_SetEvent(hDownConn, 0, NULL, NULL);

    hSvpnContext = SVPN_Context_GetContextByWsTrans(hWsTrans);
    if (NULL == hSvpnContext)
    {
        CONN_WriteString(hDownConn, "HTTP/1.1 900 ERROR\r\ninfo: Can not get context\r\n\r\n");
        CONN_Free(hDownConn);
        return;
    }

    if (BS_OK != svpn_tcprelay_ParserServerField(hWsTrans, szServer, &usPort))
    {
        CONN_WriteString(hDownConn, "HTTP/1.1 900 ERROR\r\ninfo: Server field is wrong\r\n\r\n");
        CONN_Free(hDownConn);
        return;
    }

    SVPN_DBG_OUTPUT(SVPN_DBG_ID_TCP_RELAY, SVPN_DBG_FLAG_TR_PACKET,
        "Recv request %s:%d.\r\n", szServer, usPort);

    
    if (TRUE != svpn_tcprelay_CheckPermit(hWsTrans, szServer, usPort))
    {
        SVPN_DBG_OUTPUT(SVPN_DBG_ID_TCP_RELAY, SVPN_DBG_FLAG_TR_PACKET,
            "CheckPermit: Drop request %s:%d.\r\n", szServer, usPort);
        CONN_WriteString(hDownConn, "HTTP/1.1 901 ERROR\r\ninfo: Not permit to access\r\n\r\n");
        CONN_Free(hDownConn);
        return;
    }

    pstNode = SVPN_TcpRelayNode_New(szServer, usPort);
    if (NULL == pstNode)
    {
        SVPN_DBG_OUTPUT(SVPN_DBG_ID_TCP_RELAY, SVPN_DBG_FLAG_TR_ERROR,
            "Memory: Drop request %s:%d.\r\n", szServer, usPort);
        CONN_WriteString(hDownConn, "HTTP/1.1 900 ERROR\r\ninfo: Alloc memory failed\r\n\r\n");
        CONN_Free(hDownConn);
        return;
    }

    pstNode->hDownConn = hDownConn;
    pstNode->hSvpnContext = hSvpnContext;

    if (BS_OK != svpn_tcprelay_ConnectServer(pstNode))
    {
        SVPN_DBG_OUTPUT(SVPN_DBG_ID_TCP_RELAY, SVPN_DBG_FLAG_TR_ERROR,
            "Connect %s:%d failed.\r\n", szServer, usPort);
        CONN_WriteString(hDownConn, "HTTP/1.1 900 ERROR\r\ninfo: Connect server failed\r\n\r\n");
        SVPN_TcpRelayNode_Free(pstNode);
        return;
    }

    return;
}

WS_DELIVER_RET_E SVPN_TcpRelay_RequestIn(IN WS_TRANS_HANDLE hTrans, IN UINT uiEvent)
{
    switch (uiEvent)
    {
        case WS_TRANS_EVENT_RECV_HEAD_OK:
        {
            svpn_tcprelay_RecvHeadOK(hTrans);
            break;
        }

        default:
        {
            break;
        }
    }

    return WS_DELIVER_RET_INHERIT;
}

