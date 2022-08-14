/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2008-10-5
* Description: 
* History:     
******************************************************************************/
/* retcode所需要的宏 */
#define RETCODE_FILE_NUM VNET_RETCODE_FILE_NUM_UDPPHY
    
#include "bs.h"
    
#include "utl/mbuf_utl.h"
#include "utl/msgque_utl.h"
#include "utl/socket_utl.h"
#include "utl/exec_utl.h"
#include "comp/comp_if.h"

#include "../../../inc/vnet_conf.h"
#include "../../../inc/vnet_ifnet.h"

#include "../../../vnet/inc/vnet_ipport.h"

#include "../../inc/vnets_context.h"
#include "../../inc/vnets_ses.h"
#include "../../inc/vnets_vpn_link.h"

/* 头部预留空间 */
#define _VNETS_UDP_PHY_RESERVED_MBUF_HEAD_SPACE 0

/* VNET UDP PHY的事件 */
#define _VNETS_UDP_PHY_SEND_DATA_EVENT 0x1

/* VNET UDP PHY的消息 */
#define _VNETS_UDP_PHY_SEND_DATA_MSG   0x1

/* udp PHY的Debug选项*/
#define _VNETS_UDP_PHY_DBG_FLAG_PACKET    0x1

/* var */
static INT g_iVnetsUdpPhySocketId = 0;
static UINT g_ulVnetsUdpPhyIfIndex = 0;     /* UDP PHY的接口IfIndex */
static THREAD_ID g_ulVnetsUdpPhyRecvTid = 0;     /* 接收数据的线程 */
static THREAD_ID g_ulVnetsUdpPhySendTid = 0;     /* 发送数据的线程 */
static MSGQUE_HANDLE g_hVnetsUdpPhySendQid = NULL;     /* 发送报文队列 */
static EVENT_HANDLE g_hVnetsUdpPhySendEventid = 0;
static UINT g_ulVnetsUdpPhyDbgFlag = 0;

/* 每个pstMbuf中都包含一个且仅一个完整的 报文 */
static BS_STATUS _VNETS_UDP_PHY_OutPut
(
    IN UINT ulIfIndex,
    IN MBUF_S *pstMbuf
)
{
    MSGQUE_MSG_S stMsg;
    
    stMsg.ahMsg[0] = UINT_HANDLE(_VNETS_UDP_PHY_SEND_DATA_MSG);
    stMsg.ahMsg[1] = pstMbuf;

    if (BS_OK != MSGQUE_WriteMsg (g_hVnetsUdpPhySendQid, &stMsg))
    {
        MBUF_Free(pstMbuf);
        RETURN(BS_FULL);
    }

    Event_Write (g_hVnetsUdpPhySendEventid, _VNETS_UDP_PHY_SEND_DATA_EVENT);

    return BS_OK;
}

static BS_STATUS _VNETS_UDP_PHY_CreateIf ()
{
    /* 如果已经创建，则返回OK */ 
    if (g_ulVnetsUdpPhyIfIndex != 0)
    {
        return BS_OK;
    }

    g_ulVnetsUdpPhyIfIndex = IFNET_CreateIf("vnets.l2.udp");
    if (0 == g_ulVnetsUdpPhyIfIndex)
    {
        RETURN(BS_ERR);
    }

    return BS_OK;
}

static BS_STATUS _VNETS_UDP_PHY_Input (IN UINT ulFromIp, IN USHORT usFromPort, IN MBUF_S *pstMbuf)
{
    VNETS_PHY_CONTEXT_S *pstPhyContext;
    
    BS_DBG_OUTPUT(g_ulVnetsUdpPhyDbgFlag, _VNETS_UDP_PHY_DBG_FLAG_PACKET, 
        ("VNET-UDP-PHY:Receive packet form %pI4\r\n", &ulFromIp));

    pstPhyContext = VNETS_Context_GetPhyContext(pstMbuf);

    pstPhyContext->enType = VNETS_PHY_TYPE_UDP;
    pstPhyContext->uiIfIndex = g_ulVnetsUdpPhyIfIndex;
    pstPhyContext->unPhyContext.stUdpPhyContext.uiPeerIp = ulFromIp;
    pstPhyContext->unPhyContext.stUdpPhyContext.usPeerPort = usFromPort;

    MBUF_SET_RECV_IF_INDEX(pstMbuf, g_ulVnetsUdpPhyIfIndex);

    return IFNET_LinkInput(g_ulVnetsUdpPhyIfIndex, pstMbuf);
}

static BS_STATUS _VNETS_UDP_PHY_RecvMbuf(OUT MBUF_S **ppstMbuf, OUT UINT *pulFromIp, OUT USHORT *pusFromPort)
{
    MBUF_S *pstMbuf;
    UINT ulReadLen;
    MBUF_CLUSTER_S *pstCluster;
    UINT ulMaxReadLen;

    pstCluster = MBUF_CreateCluster();
    if (NULL == pstCluster)
    {
        return BS_AGAIN;
    }
    
    ulMaxReadLen = MBUF_CLUSTER_SIZE (pstCluster) - _VNETS_UDP_PHY_RESERVED_MBUF_HEAD_SPACE;
    
    if (BS_OK != Socket_RecvFrom (g_iVnetsUdpPhySocketId, pstCluster->pucData + _VNETS_UDP_PHY_RESERVED_MBUF_HEAD_SPACE, 
            ulMaxReadLen, &ulReadLen, pulFromIp, pusFromPort))
    {
        MBUF_FreeCluster (pstCluster);
        return BS_AGAIN;
    }
    
    if (ulReadLen == 0)
    {
        MBUF_FreeCluster (pstCluster);
        return BS_AGAIN;
    }
    
    pstMbuf = VNETS_Context_CreateMbufByCluster (pstCluster,
            _VNETS_UDP_PHY_RESERVED_MBUF_HEAD_SPACE, ulReadLen);
    
    if (NULL == pstMbuf)
    {
        MBUF_FreeCluster (pstCluster);
        return BS_AGAIN;
    }

    *ppstMbuf = pstMbuf;

    return BS_OK;
}

static void _VNETS_UDP_PHY_RecvMain (USER_HANDLE_S *pstUserHandle)
{
    BS_STATUS eRet;
    MBUF_S *pstMbuf;
    UINT ulFromIp;
    USHORT usFromPort;

    for (;;)
    {
        eRet = _VNETS_UDP_PHY_RecvMbuf(&pstMbuf, &ulFromIp, &usFromPort);
        if (eRet == BS_AGAIN)
        {
            continue;
        }
        else if (eRet != BS_OK)
        {
            break;
        }
        
        (VOID) _VNETS_UDP_PHY_Input (ulFromIp, usFromPort, pstMbuf);
    }

    return;
}

static BS_STATUS _VNETS_UDP_PHY_DealSendDataMsg (IN MSGQUE_MSG_S *pstMsg)
{
    MBUF_S *pstMbuf;
    BS_STATUS eRet;
    UINT ulToIp;
    USHORT usToPort;
    VNETS_PHY_CONTEXT_S *pstPhyContext;

    pstMbuf = (MBUF_S*)pstMsg->ahMsg[1];
    pstPhyContext = VNETS_Context_GetPhyContext(pstMbuf);

    ulToIp = pstPhyContext->unPhyContext.stUdpPhyContext.uiPeerIp;
    usToPort = pstPhyContext->unPhyContext.stUdpPhyContext.usPeerPort;

    if (BS_OK != MBUF_MakeContinue(pstMbuf, MBUF_TOTAL_DATA_LEN(pstMbuf)))
    {
        BS_WARNNING(("Can't continue mbuf, len:%d", MBUF_TOTAL_DATA_LEN(pstMbuf)));
        MBUF_Free(pstMbuf);
        RETURN(BS_ERR);
    }

    BS_DBG_OUTPUT(g_ulVnetsUdpPhyDbgFlag, _VNETS_UDP_PHY_DBG_FLAG_PACKET, 
        ("VNET-UDP-PHY:Send packet to %pI4:%d\r\n", &ulToIp, ntohs(usToPort)));
    
    eRet = Socket_SendTo(g_iVnetsUdpPhySocketId, MBUF_MTOD(pstMbuf), MBUF_TOTAL_DATA_LEN(pstMbuf), ulToIp, usToPort);

    MBUF_Free(pstMbuf);

    return eRet;
}

static BS_STATUS _VNETS_UDP_PHY_DealMsg (IN MSGQUE_MSG_S *pstMsg)
{
    UINT ulMsgType;
    BS_STATUS eRet = 0;

    ulMsgType = HANDLE_UINT(pstMsg->ahMsg[0]);

    switch (ulMsgType)
    {
        case _VNETS_UDP_PHY_SEND_DATA_MSG:
            eRet = _VNETS_UDP_PHY_DealSendDataMsg(pstMsg);
            break;

        default:
            BS_WARNNING(("Not support yet!"));
            break;
    }

    return eRet;
}

static void _VNETS_UDP_PHY_SendMain (IN USER_HANDLE_S *pstUserHandle)
{
    UINT64 uiEvent;
    MSGQUE_MSG_S stMsg;

    for (;;)
    {
        Event_Read (g_hVnetsUdpPhySendEventid, _VNETS_UDP_PHY_SEND_DATA_EVENT, &uiEvent, BS_WAIT, BS_WAIT_FOREVER);

        if (uiEvent & _VNETS_UDP_PHY_SEND_DATA_EVENT)
        {
            while (BS_OK == MSGQUE_ReadMsg (g_hVnetsUdpPhySendQid, &stMsg))
            {
                _VNETS_UDP_PHY_DealMsg (&stMsg);
            }
        }
    }

    MSGQUE_Delete (g_hVnetsUdpPhySendQid);
    g_hVnetsUdpPhySendQid = 0;

    return;
}

static BS_STATUS _VNETS_UDP_PHY_CreateUdpSocket(IN UINT ulIp, IN USHORT usPort)
{
    if ((g_iVnetsUdpPhySocketId = Socket_Create(AF_INET, SOCK_DGRAM)) < 0)
    {
        RETURN(BS_ERR);
    }

    if ((ulIp != 0) || (usPort != 0))
    {
        if (BS_OK != Socket_Bind(g_iVnetsUdpPhySocketId, ulIp, usPort))
        {
            Socket_Close(g_iVnetsUdpPhySocketId);
            g_iVnetsUdpPhySocketId = 0;
            EXEC_OutInfo(" Can't bind Udp port %d.\r\n", ntohs(usPort));
            RETURN(BS_ERR);
        }
    }

    return BS_OK;
}

static BS_STATUS _VNETS_UDP_PHY_Init
(
    IN UINT ulIp,    /* 网络序 */
    IN USHORT usPort /* 网络序 */
)
{
    IF_PHY_PARAM_S stPhyParam;
    IF_LINK_PARAM_S stLinkParam;
    IF_TYPE_PARAM_S stTypeParam = {0};

    if (g_ulVnetsUdpPhyRecvTid != 0)
    {
        return BS_OK;
    }

    stPhyParam.pfPhyOutput = _VNETS_UDP_PHY_OutPut;
    IFNET_SetPhyType("vnets.l2.udp", &stPhyParam);

    stLinkParam.pfLinkInput = VNETS_SES_PktInput;
    stLinkParam.pfLinkOutput = VNETS_VPN_LINK_Output;
    IFNET_SetLinkType("vnets.vpn.link", &stLinkParam);

    stTypeParam.pcLinkType = "vnets.vpn.link";
    stTypeParam.pcPhyType = "vnets.l2.udp";
    stTypeParam.uiFlag = IF_TYPE_FLAG_HIDE;

    IFNET_AddIfType("vnets.l2.udp", &stTypeParam);

    /* 创建UDP接口 */
    if (BS_OK != _VNETS_UDP_PHY_CreateIf ())
    {
        RETURN(BS_ERR);
    }

    if (BS_OK != _VNETS_UDP_PHY_CreateUdpSocket(ulIp, usPort))
    {
        RETURN(BS_ERR);
    }

    if (g_hVnetsUdpPhySendEventid == 0)
    {
        if (NULL == (g_hVnetsUdpPhySendEventid = Event_Create()))
        {
            RETURN(BS_ERR);
        }
    }

    if (g_hVnetsUdpPhySendQid == 0)
    {
        if (NULL == (g_hVnetsUdpPhySendQid = MSGQUE_Create(512)))
        {
            RETURN(BS_ERR);
        }
    }

    if (g_ulVnetsUdpPhyRecvTid == 0)
    {
        if (THREAD_ID_INVALID == (g_ulVnetsUdpPhyRecvTid = THREAD_Create(
                        "VnetUdpRecv", NULL, _VNETS_UDP_PHY_RecvMain, NULL))) {
            RETURN(BS_ERR);
        }
    }

    if (g_ulVnetsUdpPhySendTid == 0)
    {
        if (THREAD_ID_INVALID == (g_ulVnetsUdpPhySendTid = THREAD_Create (
                        "VnetUdpSend", NULL, _VNETS_UDP_PHY_SendMain, NULL))) {
            RETURN(BS_ERR);
        }
    }

    return BS_OK;
}

BS_STATUS VNETS_UDP_PHY_Init
(
    IN UINT ulServerIp/* 主机序 */,
    IN USHORT usServerPort/* 主机序 */
)
{
    BS_STATUS eRet;

    if (BS_OK != (eRet = _VNETS_UDP_PHY_Init(htonl(ulServerIp), htons(usServerPort))))
    {
        EXEC_OutString(" Can't open vnet udp phy.\r\n");
        return eRet;
    }

    if (BS_OK != Socket_SetRecvBufSize(g_iVnetsUdpPhySocketId, 1024*1024 * 5))
    {
        /* 如果设置不成功,也不用返回失败,而是以小缓冲继续工作 */
        BS_WARNNING(("Can't set socket receive buffer size!"));
    }
    return BS_OK;
}

/* debug udp-phy packet */
PLUG_API VOID VNETS_UDP_PHY_DebugPacket(IN UINT ulArgc, IN CHAR **argv)
{
    g_ulVnetsUdpPhyDbgFlag |= _VNETS_UDP_PHY_DBG_FLAG_PACKET;
}

/* no debug udp-phy packet */
PLUG_API VOID VNETS_UDP_PHY_NoDebugPacket(IN UINT ulArgc, IN CHAR **argv)
{
    g_ulVnetsUdpPhyDbgFlag &= ~_VNETS_UDP_PHY_DBG_FLAG_PACKET;
}

