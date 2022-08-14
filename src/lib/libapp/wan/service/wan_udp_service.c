/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2013-3-29
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/ip_utl.h"
#include "utl/udp_utl.h"

#include "../h/wan_ipfwd.h"
#include "../h/wan_udp_service.h"


/* Debug 选项 */
#define _WAN_UDP_SERVICE_DBG_PACKET 0x1

typedef struct
{
    UINT uiFlag;
    PF_WAN_SERVICE_FUNC pfFunc;
    USER_HANDLE_S stUserHandle;
}_WAN_UDP_SERVICE_TBL_S;

static UINT g_uiWanUdpServiceDebugFlag = 0;
static _WAN_UDP_SERVICE_TBL_S g_astWanUdpService[65535];  /* 以网络序端口为下标 */

BS_STATUS WanUdpService_Input(IN MBUF_S *pstMbuf)
{
    IP_HEAD_S *pstIpHead;
    UDP_HEAD_S *pstUdpHeader;
    UINT uiIpHeadSize;
    UINT uiFlag;
    USHORT usDstPort;
    PF_WAN_SERVICE_FUNC pfServiceFunc;
    WAN_UDP_SERVICE_PARAM_S stParam;

    pstIpHead = MBUF_MTOD (pstMbuf);

    uiIpHeadSize = IP_HEAD_LEN(pstIpHead);

    if (uiIpHeadSize >= MBUF_TOTAL_DATA_LEN(pstMbuf))
    {
        MBUF_Free(pstMbuf);
        return BS_BAD_PARA;
    }

    if (BS_OK != MBUF_MakeContinue(pstMbuf, uiIpHeadSize + sizeof(UDP_HEAD_S)))
    {
        MBUF_Free(pstMbuf);
        return BS_NO_MEMORY;
    }

    pstIpHead = MBUF_MTOD (pstMbuf);

    pstUdpHeader = (UDP_HEAD_S*)(((UCHAR*)pstIpHead) + uiIpHeadSize);

    usDstPort = pstUdpHeader->usDstPort;

    uiFlag = g_astWanUdpService[usDstPort].uiFlag;
    pfServiceFunc = g_astWanUdpService[usDstPort].pfFunc;
    if (pfServiceFunc == NULL)
    {
        BS_DBG_OUTPUT(g_uiWanUdpServiceDebugFlag, _WAN_UDP_SERVICE_DBG_PACKET,
                ("UDP_Service: Drop packet to port %d.\r\n", ntohs(usDstPort)));
        MBUF_Free(pstMbuf);
        return BS_NO_SUCH;
    }

    BS_DBG_OUTPUT(g_uiWanUdpServiceDebugFlag, _WAN_UDP_SERVICE_DBG_PACKET,
            ("UDP_Service: Recv packet to port %d.\r\n", ntohs(usDstPort)));

    stParam.uiLocalIP = pstIpHead->unDstIp.uiIp;
    stParam.uiPeerIP = pstIpHead->unSrcIp.uiIp;
    stParam.usLocalPort = pstUdpHeader->usDstPort;
    stParam.usPeerPort = pstUdpHeader->usSrcPort;

    if (uiFlag & WAN_UDP_SERVICE_FLAG_CUT_HEAD)
    {
        MBUF_CutHead(pstMbuf, uiIpHeadSize + sizeof(UDP_HEAD_S));
    }

    return pfServiceFunc(pstMbuf, &stParam, &g_astWanUdpService[usDstPort].stUserHandle);
}


BS_STATUS WanUdpService_Output(IN MBUF_S *pstMbuf, IN WAN_UDP_SERVICE_PARAM_S *pstParam)
{
    UDP_HEAD_S *pstUdpHeader;
    USHORT usDataLen;

    if (BS_OK != MBUF_Prepend(pstMbuf, sizeof(UDP_HEAD_S)))
    {
        MBUF_Free(pstMbuf);
        return BS_NO_MEMORY;
    }

    if (BS_OK != MBUF_MakeContinue(pstMbuf, sizeof(UDP_HEAD_S)))
    {
        MBUF_Free(pstMbuf);
        return BS_NO_MEMORY;
    }

	usDataLen = MBUF_TOTAL_DATA_LEN(pstMbuf);

    pstUdpHeader = MBUF_MTOD(pstMbuf);

    pstUdpHeader->usDstPort = pstParam->usPeerPort;
    pstUdpHeader->usSrcPort = pstParam->usLocalPort;
    pstUdpHeader->usDataLength = htons (usDataLen);
    pstUdpHeader->usCrc = 0;
    pstUdpHeader->usCrc = UDP_CheckSum ((UCHAR*)pstUdpHeader, 
                          usDataLen,
                          (UCHAR *)&pstParam->uiLocalIP,
                          (UCHAR *)&pstParam->uiPeerIP);

    BS_DBG_OUTPUT(g_uiWanUdpServiceDebugFlag, _WAN_UDP_SERVICE_DBG_PACKET,
            ("UDP_Service: Send packet to port %d.\r\n", ntohs(pstParam->usPeerPort)));

    return WAN_IpFwd_Output(pstMbuf, pstParam->uiPeerIP, pstParam->uiLocalIP, IP_PROTO_UDP);
}

PLUG_API BS_STATUS WanUdpService_RegService
(
    IN USHORT usPort/* 网络序 */,
    IN UINT uiFlag, /* WAN_UDP_SERVICE_FLAG_XXX */
    IN PF_WAN_SERVICE_FUNC pfServiceFunc,
    IN USER_HANDLE_S *pstUserHandle
)
{
    g_astWanUdpService[usPort].uiFlag = uiFlag;
    g_astWanUdpService[usPort].pfFunc = pfServiceFunc;
    if (pstUserHandle != NULL)
    {
        g_astWanUdpService[usPort].stUserHandle = *pstUserHandle;
    }

    return BS_OK;
}


/* debug udp packet */
PLUG_API VOID WAN_UDP_Service_DebugPacket(IN UINT ulArgc, IN CHAR **argv)
{
    g_uiWanUdpServiceDebugFlag |= _WAN_UDP_SERVICE_DBG_PACKET;
}

/* no debug udp packet */
PLUG_API VOID WAN_UDP_Service_NoDebugPacket(IN UINT ulArgc, IN CHAR **argv)
{
    g_uiWanUdpServiceDebugFlag &= ~_WAN_UDP_SERVICE_DBG_PACKET;
}


