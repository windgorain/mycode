/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-7-9
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/sprintf_utl.h"
#include "utl/file_utl.h"
#include "utl/txt_utl.h"
#include "utl/socket_utl.h"
#include "utl/dns_utl.h"
#include "utl/vbuf_utl.h"
#include "utl/ws_utl.h"
#include "utl/conn_utl.h"
#include "utl/eth_utl.h"
#include "utl/ip_utl.h"
#include "app/wan_pub.h"
#include "app/if_pub.h"
#include "app/svpn_pub.h"
#include "comp/comp_if.h"

#include "../h/svpn_def.h"

#include "../h/svpn_context.h"
#include "../h/svpn_ippool.h"
#include "../h/svpn_debug.h"

#include "svpn_iptun_node.h"


static IF_INDEX g_ifSvpnIpTunnelInterface = 0;

static VOID _svpn_iptunnel_ConnErr(IN SVPN_IPTUN_NODE_S *pstNode)
{
    SVPN_IpTunNode_Free(pstNode);
}

static VOID _svpn_iptunnel_SendPacket2Up(IN SVPN_IPTUN_NODE_S *pstNode, IN UCHAR *pucPktData, IN UINT uiPktLen)
{
    MBUF_S *pstMbuf;

    SVPN_DBG_OUTPUT(SVPN_DBG_ID_IP_TUN, SVPN_DBG_FLAG_IPTUN_PACKET,
        ("Recv packet from iptunnel, packet length:%d.\r\n", uiPktLen));

    pstMbuf = MBUF_CreateByCopyBuf(300, pucPktData, uiPktLen, MBUF_DATA_DATA);
    if (NULL == pstMbuf)
    {
        return;
    }

    IFNET_ProtoInput(g_ifSvpnIpTunnelInterface, pstMbuf, htons(ETH_P_IP));

    return;
}

static VOID _svpn_iptunnel_ProcessData(IN SVPN_IPTUN_NODE_S *pstNode)
{
    SVPN_IPTUNNEL_HEAD_S *pstHeader;
    UINT uiPktLen;
    UINT uiRecordLen;

    while (VBUF_GetDataLength(&pstNode->stVBuf) >= sizeof(SVPN_IPTUNNEL_HEAD_S))
    {
        pstHeader = VBUF_GetData(&pstNode->stVBuf);
        uiPktLen = ntohl(pstHeader->uiPktLen);
        uiRecordLen = sizeof(SVPN_IPTUNNEL_HEAD_S) + uiPktLen;
        if (uiRecordLen > VBUF_GetDataLength(&pstNode->stVBuf))
        {
            break;
        }
        _svpn_iptunnel_SendPacket2Up(pstNode, (UCHAR*)(pstHeader + 1), uiPktLen);
        VBUF_EarseHead(&pstNode->stVBuf, uiRecordLen);
    }

    VBUF_MoveData(&pstNode->stVBuf, 0);

    return;
}

static BS_STATUS _svpn_iptunnel_ConnInput(IN SVPN_IPTUN_NODE_S *pstNode)
{
    UCHAR *pucData;
    UINT uiLen;
    INT iLen;

    for (;;)
    {
        pucData = VBUF_GetTailFreeBuf(&pstNode->stVBuf);
        uiLen = VBUF_GetTailFreeLength(&pstNode->stVBuf);

        if (uiLen > 0)
        {
            iLen = CONN_Read(pstNode->hDownConn, pucData, uiLen);
            if (iLen == SOCKET_E_AGAIN)
            {
                return BS_OK;
            }
            if (iLen <= 0)
            {
                SVPN_DBG_OUTPUT(SVPN_DBG_ID_IP_TUN, SVPN_DBG_FLAG_IPTUN_ERROR,
                    ("Close connection because read error, errno:%d.\r\n", iLen));
                _svpn_iptunnel_ConnErr(pstNode);
                return BS_STOP;
            }

            VBUF_AddDataLength(&pstNode->stVBuf, iLen);
        }

        _svpn_iptunnel_ProcessData(pstNode);
    }

    return BS_OK;
}

static MBUF_S * _svpnc_iptunnel_GetS2CMbuf(IN SVPN_IPTUN_NODE_S *pstNode)
{
    if (pstNode->pstMbufSending != NULL)
    {
        return pstNode->pstMbufSending;
    }

    MUTEX_P(&pstNode->stMutex);
    MBUF_QUE_POP(&pstNode->stMbufQue, pstNode->pstMbufSending);
    if (pstNode->pstMbufSending == NULL)
    {
        CONN_DelEvent(pstNode->hDownConn, MYPOLL_EVENT_OUT);
    }
    MUTEX_V(&pstNode->stMutex);

    return pstNode->pstMbufSending;
}

static VOID _svpn_iptunnel_ConnOutput(IN SVPN_IPTUN_NODE_S *pstNode)
{
    MBUF_S *pstMbuf;
    UINT uiLen;
    INT iLen;

    pstMbuf = _svpnc_iptunnel_GetS2CMbuf(pstNode);
    if (NULL == pstMbuf)
    {
        return;
    }

    uiLen = MBUF_TOTAL_DATA_LEN(pstMbuf);

    iLen = CONN_Write(pstNode->hDownConn, MBUF_MTOD(pstMbuf), uiLen);
    if (iLen == SOCKET_E_AGAIN)
    {
        return;
    }
    if (iLen <= 0)
    {
        SVPN_DBG_OUTPUT(SVPN_DBG_ID_IP_TUN, SVPN_DBG_FLAG_IPTUN_ERROR,
            ("Close connection because write error, errno:%d.\r\n", iLen));
        _svpn_iptunnel_ConnErr(pstNode);
        return;
    }

    if (iLen == uiLen)
    {
        MBUF_Free(pstMbuf);
        pstNode->pstMbufSending = NULL;
        return;
    }

    MBUF_CutHead(pstMbuf, iLen);

    return;
}

static BS_WALK_RET_E _svpn_iptunnel_ConnEvent(IN INT iSocketId, IN UINT uiEvent, IN USER_HANDLE_S *pstUserHandle)
{
    SVPN_IPTUN_NODE_S *pstNode = pstUserHandle->ahUserHandle[0];

    if (uiEvent & MYPOLL_EVENT_ERR)
    {
        SVPN_DBG_OUTPUT(SVPN_DBG_ID_IP_TUN, SVPN_DBG_FLAG_IPTUN_EVENT, ("Recv connection error event.\r\n"));
        _svpn_iptunnel_ConnErr(pstNode);
        return BS_WALK_CONTINUE;
    }

    if (uiEvent & MYPOLL_EVENT_IN)
    {
        SVPN_DBG_OUTPUT(SVPN_DBG_ID_IP_TUN, SVPN_DBG_FLAG_IPTUN_EVENT, ("Recv connection in event.\r\n"));
        if (BS_OK != _svpn_iptunnel_ConnInput(pstNode))
        {
            return BS_WALK_CONTINUE;
        }
    }

    if (uiEvent & MYPOLL_EVENT_OUT)
    {
        SVPN_DBG_OUTPUT(SVPN_DBG_ID_IP_TUN, SVPN_DBG_FLAG_IPTUN_EVENT, ("Recv connection out event.\r\n"));
        _svpn_iptunnel_ConnOutput(pstNode);
    }

    return BS_WALK_CONTINUE;
}

static BS_STATUS _svpn_iptunnel_Handshake(IN SVPN_IPTUN_NODE_S *pstNode)
{
    CHAR szString[512];
    USER_HANDLE_S stUserHandle;
    INT iFd;

    iFd = CONN_GetFD(pstNode->hDownConn);
    Socket_SetNoDelay(iFd, TRUE);

    BS_Snprintf(szString, sizeof(szString),
        "HTTP/1.1 200 OK\r\n"
        "IP: %pI4\r\n"
        "\r\n", &pstNode->uiVirtualIP);

    SVPN_DBG_OUTPUT(SVPN_DBG_ID_IP_TUN, SVPN_DBG_FLAG_IPTUN_HSK, ("Reply %s.", szString));

    CONN_WriteString(pstNode->hDownConn, szString);

    stUserHandle.ahUserHandle[0] = pstNode;

    CONN_SetEvent(pstNode->hDownConn, MYPOLL_EVENT_IN, _svpn_iptunnel_ConnEvent, &stUserHandle);

	return BS_OK;
}

static VOID svpn_iptunnel_RecvHeadOK(IN WS_TRANS_HANDLE hWsTrans)
{
    WS_CONN_HANDLE hWsConn;
    CONN_HANDLE hDownConn;
    SVPN_IPTUN_NODE_S *pstNode;
    SVPN_CONTEXT_HANDLE hSvpnContext;
    UINT uiIp;

    hWsConn = WS_Trans_GetConn(hWsTrans);
    hDownConn = WS_Conn_GetRawConn(hWsConn);
    if (NULL == hDownConn)
    {
        return;
    }

    SVPN_DBG_OUTPUT(SVPN_DBG_ID_IP_TUN, SVPN_DBG_FLAG_IPTUN_HSK, ("Recv request.\r\n"));

    hSvpnContext = SVPN_Context_GetContextByWsTrans(hWsTrans);

    WS_Conn_ClearRawConn(hWsConn);
    CONN_SetEvent(hDownConn, 0, NULL, NULL);

    if (NULL == hSvpnContext)
    {
        SVPN_DBG_OUTPUT(SVPN_DBG_ID_IP_TUN, SVPN_DBG_FLAG_IPTUN_ERROR, ("Can't get svpn context.\r\n"));
        CONN_WriteString(hDownConn, "HTTP/1.1 900 ERROR\r\ninfo: Can not get context\r\n\r\n");
        CONN_Free(hDownConn);
        return;
    }

    uiIp = SVPN_IPPOOL_AllocIP(hSvpnContext);
    if (uiIp == 0)
    {
        SVPN_DBG_OUTPUT(SVPN_DBG_ID_IP_TUN, SVPN_DBG_FLAG_IPTUN_ERROR, ("Can't alloc memory.\r\n"));
        CONN_WriteString(hDownConn, "HTTP/1.1 900 ERROR\r\ninfo: Alloc IP failed\r\n\r\n");
        CONN_Free(hDownConn);
        return;
    }
    uiIp = htonl(uiIp);

    pstNode = SVPN_IpTunNode_New(hSvpnContext, hDownConn, uiIp);
    if (NULL == pstNode)
    {
        SVPN_DBG_OUTPUT(SVPN_DBG_ID_IP_TUN, SVPN_DBG_FLAG_IPTUN_ERROR, ("Can't create ip tunnel node.\r\n"));
        SVPN_IPPOOL_FreeIP(pstNode->hSvpnContext, uiIp);
        CONN_WriteString(hDownConn, "HTTP/1.1 900 ERROR\r\ninfo: Alloc memory failed\r\n\r\n");
        CONN_Free(hDownConn);
        return;
    }

    if (BS_OK != _svpn_iptunnel_Handshake(pstNode))
    {
        SVPN_IpTunNode_Free(pstNode);
        return;
    }

    return;
}

WS_DELIVER_RET_E SVPN_IpTunnel_RequestIn(IN WS_TRANS_HANDLE hTrans, IN UINT uiEvent)
{
    switch (uiEvent)
    {
        case WS_TRANS_EVENT_RECV_HEAD_OK:
        {
            svpn_iptunnel_RecvHeadOK(hTrans);
            break;
        }

        default:
        {
            break;
        }
    }

    return WS_DELIVER_RET_INHERIT;
}

static VOID _svpn_iptun_ProcessLinkOutput(IN MBUF_S *pstMbuf)
{
    IP_HEAD_S *pstIpHeader;
    SVPN_IPTUN_NODE_S *pstNode;
    SVPN_IPTUNNEL_HEAD_S stHead = {0};

    pstIpHeader = MBUF_MTOD(pstMbuf);

    pstNode = SVPN_IpTunNode_Find(pstIpHeader->unDstIp.uiIp);
    if (NULL == pstNode)
    {
        MBUF_Free(pstMbuf);
        return;
    }

    if (MBUF_QUE_IS_FULL(&pstNode->stMbufQue))
    {
        MBUF_Free(pstMbuf);
        return;
    }

    stHead.ucType = SVPN_IPTUNNEL_PKT_TYPE_DATA;
    stHead.uiPktLen = MBUF_TOTAL_DATA_LEN(pstMbuf);
    stHead.uiPktLen = htonl(stHead.uiPktLen);

    if ((BS_OK != MBUF_PrependData(pstMbuf, (UCHAR*)&stHead, sizeof(SVPN_IPTUNNEL_HEAD_S)))
        || (BS_OK != MBUF_MakeContinue(pstMbuf, MBUF_TOTAL_DATA_LEN(pstMbuf))))
    {
        SVPN_DBG_OUTPUT(SVPN_DBG_ID_IP_TUN, SVPN_DBG_FLAG_IPTUN_ERROR,
            ("Failed to add ip tunnel header to packet.\r\n"));
        MBUF_Free(pstMbuf);
        return;
    }

    MUTEX_P(&pstNode->stMutex);
    MBUF_QUE_PUSH(&pstNode->stMbufQue, pstMbuf);
    CONN_AddEvent(pstNode->hDownConn, MYPOLL_EVENT_OUT);
    MUTEX_V(&pstNode->stMutex);

    return;
}

static BS_STATUS _svpn_iptunnel_LinkOutput
(
    IN IF_INDEX ifIndex,
    IN MBUF_S *pstMbuf,
    IN USHORT usProtoType/* 报文协议类型, 网络序 */
)
{
    IP_HEAD_S *pstIpHeader;
    UINT uiPhase;

    pstIpHeader = MBUF_MTOD(pstMbuf);

    SVPN_DBG_OUTPUT(SVPN_DBG_ID_IP_TUN, SVPN_DBG_FLAG_IPTUN_PACKET,
        ("Packet link output,srcIP:%pI4,dstIP:%pI4,packet length:%d.\r\n",
        &pstIpHeader->unSrcIp.uiIp, &pstIpHeader->unDstIp.uiIp,
        MBUF_TOTAL_DATA_LEN(pstMbuf)));

    uiPhase = RcuEngine_Lock();
    _svpn_iptun_ProcessLinkOutput(pstMbuf);
    RcuEngine_UnLock(uiPhase);

    return BS_OK;
}

BS_STATUS SVPN_IpTunnel_Init()
{
    IF_LINK_PARAM_S stLinkParam = {0};
    IF_TYPE_PARAM_S stTypeParam = {0};
    
    stLinkParam.pfLinkOutput = _svpn_iptunnel_LinkOutput;
    IFNET_SetLinkType("svpn.tunnel", &stLinkParam);

    stTypeParam.pcProtoType = IF_PROTO_IP_TYPE_MAME;
    stTypeParam.pcLinkType = "svpn.tunnel";
    stTypeParam.uiFlag = IF_TYPE_FLAG_HIDE;

    IFNET_AddIfType("svpn.tunnel", &stTypeParam);
    
    g_ifSvpnIpTunnelInterface = IFNET_CreateIf("svpn.tunnel");
    if (g_ifSvpnIpTunnelInterface == 0)
    {
        return BS_ERR;
    }

	return BS_OK;
}

IF_INDEX SVPN_IpTunnel_GetInterface()
{
    return g_ifSvpnIpTunnelInterface;
}

