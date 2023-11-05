/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-7-5
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/cjson.h"
#include "utl/txt_utl.h"
#include "utl/arp_utl.h"
#include "utl/conn_utl.h"
#include "utl/ssl_utl.h"
#include "utl/lasterr_utl.h"
#include "utl/mime_utl.h"
#include "utl/http_lib.h"
#include "utl/msgque_utl.h"
#include "utl/vbuf_utl.h"
#include "utl/eth_utl.h"
#include "utl/vnic_lib.h"
#include "utl/vnic_agent.h"
#include "utl/vnic_tap.h"
#include "utl/ip_list.h"
#include "utl/mbuf_utl.h"
#include "utl/my_ip_helper.h"

#include "app/svpn_pub.h"

#include "../h/svpnc_log.h"
#include "../h/svpnc_conf.h"
#include "../h/svpnc_utl.h"

#define SVPNC_IP_TUNNEL_MYPOLL_TRIGER_EVENT_PKT 0x1

#define SVPNC_IP_TUNNEL_THREAD_NUM 2
#define SVPNC_IP_TUNNEL_DATA_SIZE 2048

#define SVPNC_IP_TUNNEL_DBG_FLAG_PKT 0x1

typedef struct
{
    THREAD_ID uiTid;
    MYPOLL_HANDLE hMyPoller;
    HANDLE hMsgQue;
    MBUF_S *pstMbufSendding; 
    CONN_HANDLE hConn;
    UINT uiAdapterIndex;
    UINT uiHostIP;      
    VNIC_HANDLE hVnic;
    VNIC_AGENT_HANDLE hVnicAgent;
    VBUF_S stVBuf;
    IPLIST_S stIpList;
    UINT uiDns1;
    UINT uiDns2;
    BOOL_T bStarted;
}_SVPNC_IP_TUNNEL_S;

static _SVPNC_IP_TUNNEL_S g_stSvpncIpTunnel = {0};
static UINT g_uiSvpncIpTunnelDbgFlag = 0;
static MAC_ADDR_S g_stSvpncIpTunnelMac = {0x0, 0xff, 0x59, 0x01, 0x01, 0x01};
static MAC_ADDR_S g_stSvpncVnicMac;

static VOID _svpnc_iptunnel_ConnErr()
{
    CONN_Free(g_stSvpncIpTunnel.hConn);
    g_stSvpncIpTunnel.hConn = NULL;

    
    if (g_stSvpncIpTunnel.pstMbufSendding != NULL)
    {
        MBUF_Free(g_stSvpncIpTunnel.pstMbufSendding);
        g_stSvpncIpTunnel.pstMbufSendding = NULL;
    }

    
    VBUF_CutAll(&g_stSvpncIpTunnel.stVBuf);

    g_stSvpncIpTunnel.bStarted = FALSE;
}

static BS_STATUS _svpnc_iptunnel_Main(IN USER_HANDLE_S *pstUserHandle)
{
    MyPoll_Run(g_stSvpncIpTunnel.hMyPoller);

	return BS_OK;
}

static int _svpnc_iptunnel_UserEvent(UINT uiTriggerEvent, USER_HANDLE_S *pstUserHandle)
{
    if (g_stSvpncIpTunnel.pstMbufSendding != NULL) {
        return 0;    
    }

    if (uiTriggerEvent & SVPNC_IP_TUNNEL_MYPOLL_TRIGER_EVENT_PKT)
    {
        CONN_AddEvent(g_stSvpncIpTunnel.hConn, MYPOLL_EVENT_OUT);
    }

    return 0;
}

static BS_STATUS _svpnc_iptunnel_ProcessArp(IN HANDLE hVnicAgentId, IN MBUF_S *pstMbuf)
{
    ETH_ARP_PACKET_S *pstArp;
    MBUF_S *pstReply;

    if (BS_OK != MBUF_MakeContinue (pstMbuf, sizeof (ETH_ARP_PACKET_S)))
    {
        MBUF_Free(pstMbuf);
        return BS_ERR;
    }

    pstArp = MBUF_MTOD (pstMbuf);
    if (pstArp->usOperation != htons(ARP_REQUEST))          
    {
        MBUF_Free(pstMbuf);
        return BS_OK;
    }

    
    if (pstArp->ulDstIp == g_stSvpncIpTunnel.uiHostIP)
    {
        MBUF_Free(pstMbuf);
        return BS_OK;
    }

    
    if ((pstArp->ulDstIp & htonl(0xffffff00)) != (g_stSvpncIpTunnel.uiHostIP & htonl(0xffffff00)))
    {
        MBUF_Free(pstMbuf);
        return BS_OK;
    }

    pstReply = ARP_BuildPacketWithEthHeader(pstArp->ulDstIp, pstArp->ulSenderIp,
        g_stSvpncIpTunnelMac.aucMac, pstArp->aucSMac, ARP_REPLY);

    MBUF_Free(pstMbuf);

    if (NULL == pstReply)
    {
        return BS_ERR;
    }

    ETH_PadPacket(pstReply, TRUE);

    return VNIC_Agent_Write(hVnicAgentId, pstReply);
}

static BS_STATUS _svpnc_iptunnel_PktInput(IN HANDLE hVnicAgentId, IN MBUF_S *pstMbuf, IN HANDLE hUserHandle)
{
    MSGQUE_MSG_S stMsg;
    UINT uiNum;
    SVPN_IPTUNNEL_HEAD_S stHead = {0};
    ETH_PKT_INFO_S stEthInfo;

    if (g_stSvpncIpTunnel.bStarted != TRUE)
    {
        MBUF_Free(pstMbuf);
        return BS_ERR;
    }

    if ((BS_OK != ETH_GetEthHeadInfoByMbuf(pstMbuf, &stEthInfo)))
    {
        MBUF_Free(pstMbuf);
        return BS_ERR;
    }

    if (stEthInfo.usType == ETH_P_ARP)
    {
        return _svpnc_iptunnel_ProcessArp(hVnicAgentId, pstMbuf);
    }

    if (stEthInfo.usType != ETH_P_IP)
    {
        MBUF_Free(pstMbuf);
        return BS_OK;
    }

    MBUF_CutHead(pstMbuf, stEthInfo.usHeadLen);

    BS_DBG_OUTPUT(g_uiSvpncIpTunnelDbgFlag, SVPNC_IP_TUNNEL_DBG_FLAG_PKT,
        ("Recv a packet from vnic, packet length=%d.\r\n", MBUF_TOTAL_DATA_LEN(pstMbuf)));

    stHead.ucType = SVPN_IPTUNNEL_PKT_TYPE_DATA;
    stHead.uiPktLen = MBUF_TOTAL_DATA_LEN(pstMbuf);
    stHead.uiPktLen = htonl(stHead.uiPktLen);

    if ((BS_OK != MBUF_PrependData(pstMbuf, (UCHAR*)&stHead, sizeof(SVPN_IPTUNNEL_HEAD_S)))
        || (BS_OK != MBUF_MakeContinue(pstMbuf, MBUF_TOTAL_DATA_LEN(pstMbuf))))
    {
        BS_DBG_OUTPUT(g_uiSvpncIpTunnelDbgFlag, SVPNC_IP_TUNNEL_DBG_FLAG_PKT,
            ("Failed to add ip tunnel header to packet.\r\n"));
        MBUF_Free(pstMbuf);
        return BS_ERR;
    }

    stMsg.ahMsg[0] = pstMbuf;
    
    if (BS_OK != MSGQUE_WriteMsg(g_stSvpncIpTunnel.hMsgQue, &stMsg)) {
        MBUF_Free(pstMbuf);
    }

    uiNum = MSGQUE_Count(g_stSvpncIpTunnel.hMsgQue);

    if (uiNum == 1)
    {
        MyPoll_PostUserEvent(g_stSvpncIpTunnel.hMyPoller, SVPNC_IP_TUNNEL_MYPOLL_TRIGER_EVENT_PKT);
    }

    return BS_OK;
}

static BS_STATUS _svpnc_iptunnel_Init()
{
    if (g_stSvpncIpTunnel.hMyPoller == NULL)
    {
        g_stSvpncIpTunnel.hMyPoller = MyPoll_Create();
        if (NULL == g_stSvpncIpTunnel.hMyPoller)
        {
            return BS_ERR;
        }

        MyPoll_SetUserEventProcessor(g_stSvpncIpTunnel.hMyPoller, _svpnc_iptunnel_UserEvent, NULL);
    }

    if (NULL == g_stSvpncIpTunnel.hMsgQue)
    {
        g_stSvpncIpTunnel.hMsgQue = MSGQUE_Create(128);
        if (NULL == g_stSvpncIpTunnel.hMsgQue)
        {
            return BS_ERR;
        }
    }

    VBUF_Init(&g_stSvpncIpTunnel.stVBuf);

    if (BS_OK != VBUF_ExpandTo(&g_stSvpncIpTunnel.stVBuf, SVPNC_IP_TUNNEL_DATA_SIZE))
    {
        return BS_ERR;
    }

    return BS_OK;
}

static BS_STATUS _svpnc_iptunnel_VnicInit()
{
    UINT uiStatus = 1;
    UINT uiLen;

    g_stSvpncIpTunnel.hVnic = VNIC_Dev_Open();
    if (NULL == g_stSvpncIpTunnel.hVnic)
    {
        return BS_ERR;
    }

    g_stSvpncIpTunnel.hVnicAgent = VNIC_Agent_Create();
    if (NULL == g_stSvpncIpTunnel.hVnicAgent)
    {
        VNIC_Delete(g_stSvpncIpTunnel.hVnic);
        g_stSvpncIpTunnel.hVnic = NULL;
        return BS_ERR;
    }

    VNIC_Agent_SetVnic(g_stSvpncIpTunnel.hVnicAgent, g_stSvpncIpTunnel.hVnic);

    VNIC_Ioctl (g_stSvpncIpTunnel.hVnic, TAP_WIN_IOCTL_SET_MEDIA_STATUS,
        (UCHAR*)&uiStatus, 4, (UCHAR*)&uiStatus, 4, &uiLen);

    VNIC_Ioctl (g_stSvpncIpTunnel.hVnic, TAP_WIN_IOCTL_GET_MAC,
        (UCHAR*)&g_stSvpncVnicMac, 6, (UCHAR*)&g_stSvpncVnicMac, 6, &uiLen);

    VNIC_GetAdapterIndex(g_stSvpncIpTunnel.hVnic, &g_stSvpncIpTunnel.uiAdapterIndex);

    VNIC_Agent_Start(g_stSvpncIpTunnel.hVnicAgent, _svpnc_iptunnel_PktInput, NULL);

    return BS_OK;
}

static BS_STATUS _svpnc_iptunnel_TaskInit()
{
    g_stSvpncIpTunnel.uiTid = THREAD_Create("svpnc_iptunnel", NULL, _svpnc_iptunnel_Main, NULL);
    if (g_stSvpncIpTunnel.uiTid == THREAD_ID_INVALID)
    {
        return BS_ERR;
    }

    return BS_OK;
}

static CHAR * svpnc_iptunnel_GetJsonValueByName(IN cJSON *pstJson, IN CHAR *pcName)
{
    cJSON *pstNode;

    pstNode = cJSON_GetObjectItem(pstJson, pcName);
    if ((NULL == pstNode) || (pstNode->valuestring == NULL))
    {
        return "";
    }

    return pstNode->valuestring;
}

static BS_STATUS svpnc_iptunnel_AddRouteList(IN CHAR *pcAddressList)
{
    IPLIST_S stIpList;
    LSTR_S stAddress;

    IPList_Init(&stIpList);

    stAddress.pcData = pcAddressList;
    stAddress.uiLen = strlen(pcAddressList);

    IPList_ParseString(&stIpList, &stAddress);

    IPLIst_Cat(&g_stSvpncIpTunnel.stIpList, &stIpList);

    IPList_Finit(&stIpList);

    return BS_OK;
}

static VOID _svpnc_iptunnel_ProcessJsonNode(IN cJSON *pstTcpRelayNode)
{
    CHAR *pcName, *pcDesc, *pcAddress;

    pcName = svpnc_iptunnel_GetJsonValueByName(pstTcpRelayNode, "Name");
    pcDesc = svpnc_iptunnel_GetJsonValueByName(pstTcpRelayNode, "Description");
    pcAddress = svpnc_iptunnel_GetJsonValueByName(pstTcpRelayNode, "Address");

    svpnc_iptunnel_AddRouteList(pcAddress);

    return;
}

static VOID _svpnc_iptunnel_ParseJsonString(IN CHAR *pcString)
{
    cJSON *pstJson;
    cJSON *pstJsonData;
    cJSON *pstTcpRelayNode;
    INT iSize;
    INT i;

    pstJson = cJSON_Parse(pcString);
    if (NULL == pstJson)
    {
        return;
    }

    pstJsonData = cJSON_GetObjectItem(pstJson, "Data");
    if (NULL != pstJsonData)
    {
        iSize = cJSON_GetArraySize(pstJsonData);
        for (i=0; i<iSize; i++)
        {
            pstTcpRelayNode = cJSON_GetArrayItem(pstJsonData, i);
            _svpnc_iptunnel_ProcessJsonNode(pstTcpRelayNode);
        }
    }

    cJSON_Delete(pstJson);
}


static UINT _svpnc_iptunnel_GetNextHot()
{
    UINT uiNextHop;

    uiNextHop = ntohl(g_stSvpncIpTunnel.uiHostIP);
    if (((uiNextHop & 0xff) == 0xff) || ((uiNextHop & 0xfe) == 0xfe))
    {
        uiNextHop --;
    }
    else
    {
        uiNextHop ++;
    }

    return htonl(uiNextHop);
}

static VOID _svpnc_iptunnel_SetRoute(IN UINT uiDstIp, IN UINT uiMask)
{
    UINT uiNextHop;
    CHAR szInfo[256];
    UINT uiDstIpNet, uiMaskNet;

    uiNextHop = _svpnc_iptunnel_GetNextHot();
    uiDstIpNet = htonl(uiDstIp);
    uiMaskNet = htonl(uiMask);

    SVPNC_Log("[SVPNC.IpTunnel] Set route %pI4/%pI4.\r\n", &uiDstIpNet, &uiMaskNet);
    
    if (BS_OK != My_IP_Helper_AddRoute(uiDstIpNet, uiMaskNet, uiNextHop, g_stSvpncIpTunnel.uiAdapterIndex))
    {
        LastErr_SPrint(szInfo);
        SVPNC_Log("[SVPNC.IpTunnel] Set route failed. Error:%s\r\n", szInfo);
    }
}

static VOID _svpnc_iptunnel_ExitNotify(IN INT lExitNum, IN USER_HANDLE_S *pstUserHandle)
{
    WIN_EnableDebugPrivilege();

    My_IP_Helper_DeleteAllIpAddress(g_stSvpncIpTunnel.uiAdapterIndex);
    My_IP_Helper_DelAllRouteByAdapterindex(g_stSvpncIpTunnel.uiAdapterIndex);

    if (g_stSvpncIpTunnel.uiDns1 != 0)
    {
        VNIC_DelDns(g_stSvpncIpTunnel.hVnic, g_stSvpncIpTunnel.uiDns1);
    }

    if (g_stSvpncIpTunnel.uiDns2 != 0)
    {
        VNIC_DelDns(g_stSvpncIpTunnel.hVnic, g_stSvpncIpTunnel.uiDns2);
    }
}

static VOID _svpnc_iptunnel_SendIpPacket2Vnic(IN UCHAR *pucIpPacket, IN UINT uiPktLen)
{
    MBUF_S *pstMbuf;
    ETH_HEADER_S *pstEthHeader;

    pstMbuf = MBUF_CreateByCopyBuf(200, pucIpPacket, uiPktLen, MBUF_DATA_DATA);
    if (NULL == pstMbuf)
    {
        return;
    }

    MBUF_Prepend(pstMbuf, sizeof(ETH_HEADER_S));
    if (BS_OK != MBUF_MakeContinue(pstMbuf, sizeof(ETH_HEADER_S)))
    {
        MBUF_Free(pstMbuf);
        return;
    }

    pstEthHeader = MBUF_MTOD(pstMbuf);

    pstEthHeader->stDMac = g_stSvpncVnicMac;
    pstEthHeader->stSMac = g_stSvpncIpTunnelMac;
    pstEthHeader->usProto = htons(ETH_P_IP);
    
    VNIC_Agent_Write(g_stSvpncIpTunnel.hVnicAgent, pstMbuf);
}

static VOID _svpnc_iptunnel_ProcessConnPacket()
{
    SVPN_IPTUNNEL_HEAD_S *pstHeader;
    UINT uiPktLen;
    UINT uiRecordLen;

    while (VBUF_GetDataLength(&g_stSvpncIpTunnel.stVBuf) >= sizeof(SVPN_IPTUNNEL_HEAD_S))
    {
        pstHeader = VBUF_GetData(&g_stSvpncIpTunnel.stVBuf);
        uiPktLen = ntohl(pstHeader->uiPktLen);
        uiRecordLen = uiPktLen + sizeof(SVPN_IPTUNNEL_HEAD_S);
        if (uiRecordLen > VBUF_GetDataLength(&g_stSvpncIpTunnel.stVBuf))
        {
            break;
        }
        _svpnc_iptunnel_SendIpPacket2Vnic((UCHAR*)(pstHeader + 1), uiPktLen);
        VBUF_EarseHead(&g_stSvpncIpTunnel.stVBuf, uiRecordLen);
    }

    VBUF_MoveData(&g_stSvpncIpTunnel.stVBuf, 0);

    return;
}

static VOID _svpnc_iptunnel_ReadEvent()
{
    UCHAR *pucData;
    UINT uiSize;
    INT iLen;

    if (g_stSvpncIpTunnel.hConn == NULL)    
    {
        return;
    }

    for (;;)
    {
        pucData = VBUF_GetTailFreeBuf(&g_stSvpncIpTunnel.stVBuf);
        uiSize = VBUF_GetTailFreeLength(&g_stSvpncIpTunnel.stVBuf);

        if (uiSize > 0)
        {
            iLen = CONN_Read(g_stSvpncIpTunnel.hConn, pucData, uiSize);
            if (iLen == SOCKET_E_AGAIN)
            {
                break;
            }

            if (iLen <= 0)
            {
                _svpnc_iptunnel_ConnErr();
                break;
            }

            VBUF_AddDataLength(&g_stSvpncIpTunnel.stVBuf, iLen);
        }

        _svpnc_iptunnel_ProcessConnPacket();
    }

    return;
}

static MBUF_S * _svpnc_iptunnel_GetC2SMbuf()
{
    MBUF_S *pstMbuf;
    MSGQUE_MSG_S stMsg;
    SVPN_IPTUNNEL_HEAD_S stHead = {0};

    pstMbuf = g_stSvpncIpTunnel.pstMbufSendding;
    if (NULL != pstMbuf)
    {
        return pstMbuf;
    }

    if (BS_OK != MSGQUE_ReadMsg(g_stSvpncIpTunnel.hMsgQue, &stMsg))
    {
        return NULL;
    }

    pstMbuf = stMsg.ahMsg[0];

    g_stSvpncIpTunnel.pstMbufSendding = pstMbuf;

    return pstMbuf;
}

static VOID _svpnc_iptunnel_WriteEvent()
{
    MBUF_S *pstMbuf;
    UINT uiLen;
    INT iLen;
    
    if (g_stSvpncIpTunnel.hConn == NULL)    
    {
        return;
    }

    pstMbuf = _svpnc_iptunnel_GetC2SMbuf();
    if (NULL == pstMbuf)
    {
        CONN_DelEvent(g_stSvpncIpTunnel.hConn, MYPOLL_EVENT_OUT);
        return;
    }

    uiLen = MBUF_TOTAL_DATA_LEN(pstMbuf);

    iLen = CONN_Write(g_stSvpncIpTunnel.hConn, MBUF_MTOD(pstMbuf), uiLen);
    if (iLen == SOCKET_E_AGAIN)
    {
        return;
    }
    if (iLen <= 0)
    {
        _svpnc_iptunnel_ConnErr();
        return;
    }

    if (iLen == uiLen)
    {
        g_stSvpncIpTunnel.pstMbufSendding = NULL;
        MBUF_Free(pstMbuf);
        return;
    }

    MBUF_CutHead(pstMbuf, iLen);

    return;
}


static int _svpnc_iptunnel_ConnEventNotify(int iSocketId, UINT uiEvent, USER_HANDLE_S *pstUserHandle)
{
    if (uiEvent & MYPOLL_EVENT_IN) {
        _svpnc_iptunnel_ReadEvent();
    }

    if (uiEvent & MYPOLL_EVENT_OUT) {
        _svpnc_iptunnel_WriteEvent();
    }

    if (uiEvent & MYPOLL_EVENT_ERR) {
        _svpnc_iptunnel_ConnErr();
    }

    return 0;
}

static BS_STATUS _svpnc_iptunnel_ProcessHandshake(IN CONN_HANDLE hConn, IN HANDLE hHttpParser)
{
    CHAR *pcIP;
    
    if (HTTP_STATUS_OK != HTTP_GetStatusCode(hHttpParser))
    {
        return BS_ERR;
    }

    pcIP = HTTP_GetHeadField(hHttpParser, "IP");
    if (NULL == pcIP)
    {
        return BS_ERR;
    }

    g_stSvpncIpTunnel.uiHostIP = Socket_NameToIpNet(pcIP);
    if (0 == g_stSvpncIpTunnel.uiHostIP)
    {
        return BS_ERR;
    }

    if (BS_OK != _svpnc_iptunnel_VnicInit())
    {
        return BS_ERR;
    }

    My_IP_Helper_DelAllRouteByAdapterindex(g_stSvpncIpTunnel.uiAdapterIndex);
    My_IP_Helper_DeleteAllIpAddress(g_stSvpncIpTunnel.uiAdapterIndex);

    if (BS_OK != VNIC_AddIP(g_stSvpncIpTunnel.hVnic, g_stSvpncIpTunnel.uiHostIP, htonl(0xffffff00)))
    {
        return BS_ERR;
    }

    
    while (2 > My_IP_Helper_CountRouteByAdapterIndex(g_stSvpncIpTunnel.uiAdapterIndex))
    {
        Sleep(500);
    }

    Socket_SetNoBlock(CONN_GetFD(hConn), TRUE);

    g_stSvpncIpTunnel.bStarted = TRUE;

    CONN_SetPoller(hConn, g_stSvpncIpTunnel.hMyPoller);
    g_stSvpncIpTunnel.hConn = hConn;

    CONN_SetEvent(hConn, MYPOLL_EVENT_IN, _svpnc_iptunnel_ConnEventNotify, NULL);

    return BS_OK;
}

static BS_STATUS _svpnc_iptunnel_RecvHandshake
(
    IN CONN_HANDLE hConn,
    IN VBUF_S *pstVBuf,
    IN HANDLE hHttpParser
)
{
    INT iLen;
    UINT uiHeadLen;
    UCHAR acuData[1024];
    BS_STATUS eRet = BS_ERR;
    
    while (1)
    {
        iLen = CONN_Read(hConn, acuData, sizeof(acuData));
        if (iLen <= 0)
        {
            return BS_ERR;
        }

        if (BS_OK != VBUF_CatFromBuf(pstVBuf, acuData, iLen))
        {
            return BS_ERR;
        }

        uiHedLen = HTTP_GetHeadLen(VBUF_GetData(pstVBuf),
                VBUF_GetDataLength(pstVBuf));
        if (uiHeadLen > 0) {
            VBUF_CutTail(pstVBuf, VBUF_GetDataLength(pstVBuf) - uiHeadLen);
            break;
        }
    }

    if (BS_OK != HTTP_ParseHead(hHttpParser, VBUF_GetData(pstVBuf),
                VBUF_GetDataLength(pstVBuf), HTTP_RESPONSE))
    {
        return BS_ERR;
    }

    return _svpnc_iptunnel_ProcessHandshake(hConn, hHttpParser);
}

static BS_STATUS _svpnc_iptunnel_SendHandshake(IN CONN_HANDLE hConn)
{
    CHAR szString[512];
    
    snprintf(szString, sizeof(szString),
        "IP-Tunnel / HTTP/1.1\r\n"
        "Cookie: svpnuid=%s\r\n\r\n", SVPNC_GetCookie());

    if (CONN_WriteString(hConn, szString) < 0)
    {
        return BS_ERR;
    }

    return BS_OK;
}

static BS_STATUS _svpnc_iptunnel_StartHandshake()
{
    VBUF_S stVBuf;
    CONN_HANDLE hConn;
    HANDLE hHttpPaser;
    BS_STATUS eRet;

    VBUF_Init(&stVBuf);

    hHttpPaser = HTTP_CreateHeadParser();
    if (NULL == hHttpPaser)
    {
        VBUF_Finit(&stVBuf);
        return BS_NO_MEMORY;
    }
    
    hConn = SVPNC_SynConnectServer();
    if (hConn == NULL)
    {
        HTTP_DestoryHeadParser(hHttpPaser);
        VBUF_Finit(&stVBuf);
        return BS_ERR;
    }

    eRet = _svpnc_iptunnel_SendHandshake(hConn);
    if (eRet == BS_OK)
    {
        eRet = _svpnc_iptunnel_RecvHandshake(hConn, &stVBuf, hHttpPaser);
    }

    HTTP_DestoryHeadParser(hHttpPaser);
    VBUF_Finit(&stVBuf);

    if (eRet != BS_OK)
    {
        CONN_Free(hConn);
    }

    return eRet;
}

static BS_STATUS _svpnc_iptunnel_ProcessRoutes()
{
    CONN_HANDLE hConn;
    CHAR szString[4096];
    INT iLen;
    UINT uiBeginIP;
    UINT uiEndIP;
    UINT uiMask;

    hConn = SVPNC_SynConnectServer();
    if (hConn == NULL)
    {
        SVPNC_Log("[SVPNC.IpTunnel] Route request failed because connect error.\r\n");
        return BS_ERR;
    }

    snprintf(szString, sizeof(szString),
        "GET /request.cgi?_do=IpRes.List HTTP/1.1\r\n"
        "Cookie: svpnuid=%s\r\n\r\n", SVPNC_GetCookie());

    if (CONN_WriteString(hConn, szString) < 0)
    {
        CONN_Free(hConn);
        SVPNC_Log("[SVPNC.IpTunnel] Route request failed because write error.\r\n");
        return BS_ERR;
    }

    SVPNC_Log("[SVPNC.IpTunnel] Route request %s\r\n", szString);

    iLen = SVPNC_ReadHttpBody(hConn, szString, sizeof(szString)-1);
    CONN_Free(hConn);
    if (iLen < 0)
    {
        return BS_ERR;
    }

    szString[iLen] = '\0';

    SVPNC_Log("[SVPNC.IpTunnel] Route Reply %s\r\n", szString);

    IPList_Init(&g_stSvpncIpTunnel.stIpList);
    _svpnc_iptunnel_ParseJsonString(szString);
    IPList_Continue(&g_stSvpncIpTunnel.stIpList);

    WIN_EnableDebugPrivilege();

    IPLIST_SCAN_BEGIN(&g_stSvpncIpTunnel.stIpList, uiBeginIP, uiEndIP)
    {
        uiMask = IP_GetMiniMask(uiBeginIP, uiEndIP);
        _svpnc_iptunnel_SetRoute(uiBeginIP, uiMask);
    }IPLIST_SCAN_END();

    return BS_OK;
}

static BS_STATUS _svpnc_iptunnel_ProcessDns()
{
    CONN_HANDLE hConn;
    CHAR szString[4096];
    INT iLen;
    cJSON *pstJson;
    CHAR *pcMainDNS;
    CHAR *pcSlaveDNS;
    UINT uiDns1;
    UINT uiDns2;

    hConn = SVPNC_SynConnectServer();
    if (hConn == NULL)
    {
        SVPNC_Log("[SVPNC.IpTunnel] Dns request failed because connect error.\r\n");
        return BS_ERR;
    }

    snprintf(szString, sizeof(szString),
        "GET /request.cgi?_do=InnerDNS.Get HTTP/1.1\r\n"
        "Cookie: svpnuid=%s\r\n\r\n", SVPNC_GetCookie());

    if (CONN_WriteString(hConn, szString) < 0)
    {
        SVPNC_Log("[SVPNC.IpTunnel] Dns request failed because write error.\r\n");
        CONN_Free(hConn);
        return BS_ERR;
    }

    SVPNC_Log("[SVPNC.IpTunnel] DNS request %s\r\n", szString);

    iLen = SVPNC_ReadHttpBody(hConn, szString, sizeof(szString)-1);
    CONN_Free(hConn);
    if (iLen < 0)
    {
        return BS_ERR;
    }

    szString[iLen] = '\0';

    SVPNC_Log("[SVPNC.IpTunnel] DNS Reply %s\r\n", szString);

    pstJson = cJSON_Parse(szString);
    if (NULL == pstJson)
    {
        return BS_ERR;
    }

    pcMainDNS = svpnc_iptunnel_GetJsonValueByName(pstJson, "MainDNS");
    pcSlaveDNS = svpnc_iptunnel_GetJsonValueByName(pstJson, "SlaveDNS");

    uiDns1 = Socket_NameToIpNet(pcMainDNS);
    uiDns2 = Socket_NameToIpNet(pcSlaveDNS);

    cJSON_Delete(pstJson);

    if (uiDns1 == 0)
    {
        uiDns1 = uiDns2;
        uiDns2 = 0;
    }

    g_stSvpncIpTunnel.uiDns1 = uiDns1;
    g_stSvpncIpTunnel.uiDns2 = uiDns2;

    if (uiDns1 != 0)
    {
        VNIC_AddDns(g_stSvpncIpTunnel.hVnic, uiDns1, 1);
    }

    if (uiDns2 != 0)
    {
        VNIC_AddDns(g_stSvpncIpTunnel.hVnic, uiDns2, 2);
    }

    return BS_OK;
}

static BS_STATUS _svpnc_iptunnel_Start()
{
    _svpnc_iptunnel_Init();

    _svpnc_iptunnel_TaskInit();

    SYSRUN_RegExitNotifyFunc(_svpnc_iptunnel_ExitNotify, NULL);

    _svpnc_iptunnel_StartHandshake();

    return BS_OK;
}

BS_STATUS SVPNC_IpTunnel_Start()
{
    if (g_stSvpncIpTunnel.bStarted == TRUE)
    {
        return BS_OK;
    }

    _svpnc_iptunnel_Start();

    _svpnc_iptunnel_ProcessRoutes();
    _svpnc_iptunnel_ProcessDns();

    return BS_OK;
}


