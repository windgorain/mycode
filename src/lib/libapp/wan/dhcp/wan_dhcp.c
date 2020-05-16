/******************************************************************************
* Copyright (C) Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2013-4-1
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/mbuf_utl.h"
#include "utl/ippool_utl.h"
#include "utl/bit_opt.h"
#include "utl/vf_utl.h"
#include "utl/eth_utl.h"
#include "utl/dhcp_utl.h"
#include "utl/dhcps_utl.h"

#include "comp/comp_wan.h"

#include "../h/wan_vrf.h"
#include "../h/wan_ifnet.h"
#include "../h/wan_udp_service.h"
#include "../h/wan_ip_addr.h"

/* Debug 选项 */
#define _WAN_DHCP_DBG_PACKET 0x1
#define _WAN_DHCP_DBG_ERROR 0x2

#define _WAN_DHCP_FLAG_ENABLE 0x1   /* 使能dhcps服务 */

typedef struct
{
    RCU_NODE_S stRcu;
    UINT uiFlag;
    IF_INDEX ifIndex;
    UINT uiVrfID;
    DHCPS_HANDLE hDhcpsHandle;
}_WAN_DHCP_S;

static UINT g_uiWanDhcpDebugFlag = 0;

#if 0 /* 因为编译告警此函数没用到 */
static UINT wan_dhcp_GetHostIp(IN UINT uiVrfID)
{
    return 0;
}

static BS_STATUS wan_dhcp_SendPkt
(
    IN MBUF_S *pstMbuf,
    IN USER_HANDLE_S *pstUserHandle,
    IN WAN_UDP_SERVICE_PARAM_S *pstParam
)
{
    UINT uiVrfID;
    
    uiVrfID = HANDLE_UINT(pstUserHandle->ahUserHandle[0]);
    
    MBUF_SET_OUTVPNID(pstMbuf, uiVrfID);

    if ((pstParam->uiLocalIP == 0) || (pstParam->uiLocalIP == 0xffffffff))
    {
        pstParam->uiLocalIP = wan_dhcp_GetHostIp(uiVrfID);
    }

    if (pstParam->uiPeerIP == 0)
    {
        pstParam->uiPeerIP = 0xffffffff;
    }

    BS_DBG_OUTPUT(g_uiWanDhcpDebugFlag, _WAN_DHCP_DBG_PACKET,
        ("DHCP: Send dhcp packet.\r\n"));

    return WanUdpService_Output(pstMbuf, pstParam);
}

static VOID wan_dhcp_AddMacIpBinding(IN CHAR *pcMac, IN CHAR *pcIp, IN HANDLE hUserHandle)
{
    MAC_ADDR_S stMacAddr;
    UINT uiIP;

    DHCPS_HANDLE hDhcpHandle = hUserHandle;

    STRING_2_MAC_ADDR(pcMac, stMacAddr.aucMac);
    uiIP = Socket_NameToIpHost(pcIp);

    if (uiIP == 0)
    {
        return;
    }

    DHCPS_SignIP(hDhcpHandle, &stMacAddr, htonl(uiIP));
}

/* 使能dhcps服务 */
static BS_STATUS wan_dhcp_Enable(IN _WAN_DHCP_S *pstDhcp)
{
    DHCP_IP_CONF_S stConf;
    USER_HANDLE_S stUserHandle;
    WAN_IP_ADDR_INFO_S stAddrInfo = {0};
    
    pstDhcp->uiFlag |= _WAN_DHCP_FLAG_ENABLE;

    stConf.uiStartIp = 0xC0A82001;
    stConf.uiEndIp = 0xC0A820FE;
    stConf.uiMask = 0xffffff00;
    stConf.uiGateway = 0xC0A820FE;

    
    stUserHandle.ahUserHandle[0] = UINT_HANDLE(pstDhcp->uiVrfID);

    pstDhcp->hDhcpsHandle = DHCPS_CreateInstance(&stConf, (PF_DHCPS_SEND_FUNC)wan_dhcp_SendPkt, &stUserHandle);
    if (NULL == pstDhcp->hDhcpsHandle)
    {
        MEM_Free(pstDhcp);
        return BS_NO_MEMORY;
    }

    stAddrInfo.uiIfIndex = pstDhcp->ifIndex;
    stAddrInfo.uiIP = htonl(stConf.uiGateway);
    stAddrInfo.uiMask = htonl(stConf.uiMask);

    WAN_IPAddr_AddIp(&stAddrInfo);

    return BS_OK;
}
#endif

static BS_STATUS wan_dhcp_VFEventCreate(IN UINT uiVrfID)
{
    _WAN_DHCP_S *pstDhcp;

    pstDhcp = MEM_ZMalloc(sizeof(_WAN_DHCP_S));
    if (NULL == pstDhcp)
    {
        return BS_NO_MEMORY;
    }

    WanVrf_SetData(uiVrfID, WAN_VRF_PROPERTY_INDEX_DHCP, pstDhcp);

    return BS_OK;
}

static VOID _wan_dhcp_RcuFree(IN VOID *pstRcuNode)
{
    _WAN_DHCP_S *pstDhcp = pstRcuNode;

    if (NULL != pstDhcp->hDhcpsHandle)
    {    
        DHCPS_DestoryInstance(pstDhcp->hDhcpsHandle);
    }

    MEM_Free(pstDhcp);
}

static VOID wan_dhcp_VFEventDestory(IN UINT uiVrfID)
{
    _WAN_DHCP_S *pstDhcp;

    pstDhcp = WanVrf_GetData(uiVrfID, WAN_VRF_PROPERTY_INDEX_DHCP);
    WanVrf_SetData(uiVrfID, WAN_VRF_PROPERTY_INDEX_DHCP, NULL);

    if (pstDhcp != NULL)
    {
        RcuBs_Free(&pstDhcp->stRcu, _wan_dhcp_RcuFree);
    }
}

static BS_STATUS wan_dhcp_VFEvent
(
    IN UINT uiEvent,
    IN UINT uiVrfID,
    IN USER_HANDLE_S *pstUserHandle
)
{
    switch (uiEvent)
    {
        case VF_EVENT_CREATE_VF:
        {
            wan_dhcp_VFEventCreate(uiVrfID);
            break;
        }

        case VF_EVENT_DESTORY_VF:
        {
            wan_dhcp_VFEventDestory(uiVrfID);
            break;
        }
    }
    
    return BS_OK;
}

static BS_STATUS wan_dhcp_DealDhcpPacket 
(
    IN UINT uiVrfID,
    IN MBUF_S *pstMbuf,
    IN WAN_UDP_SERVICE_PARAM_S *pstParam
)
{
    _WAN_DHCP_S *pstDhcp;
    BS_STATUS eRet;
    UINT uiPhase;

    uiPhase = RcuBs_Lock();
    pstDhcp = WanVrf_GetData(uiVrfID, WAN_VRF_PROPERTY_INDEX_DHCP);
    if ((pstDhcp != NULL) && (pstDhcp->hDhcpsHandle != NULL))
    {
        eRet = DHCPS_PktInput(pstDhcp->hDhcpsHandle, pstMbuf, pstParam);
    }
    else
    {
        MBUF_Free(pstMbuf);
        eRet = BS_ERR;
    }
    RcuBs_UnLock(uiPhase);

    return eRet;
}

static BS_STATUS wan_dhcp_PktInput
(
    IN MBUF_S *pstMbuf,
    IN WAN_UDP_SERVICE_PARAM_S *pstParam,
    IN USER_HANDLE_S *pstUserHandle
)
{
    UINT uiVrfID;

    BS_DBG_OUTPUT(g_uiWanDhcpDebugFlag, _WAN_DHCP_DBG_PACKET,
        ("DHCP: Dhcp packet input.\r\n"));

    if (MBUF_TOTAL_DATA_LEN(pstMbuf) < sizeof(DHCP_HEAD_S))
    {
        BS_DBG_OUTPUT(g_uiWanDhcpDebugFlag, _WAN_DHCP_DBG_ERROR,
            ("DHCP: Dhcp packet length too small.\r\n."));
        MBUF_Free(pstMbuf);
        return BS_NO_MEMORY;
    }

    uiVrfID = MBUF_GET_INVPNID(pstMbuf);

    return wan_dhcp_DealDhcpPacket (uiVrfID, pstMbuf, pstParam);
}


/* 返回网络序IP */
UINT WAN_DHCP_GetServerIP(IN UINT uiVrfID)
{
    _WAN_DHCP_S *pstDhcp;
    UINT uiIP = 0;
    UINT uiPhase;

    uiPhase = RcuBs_Lock();
    pstDhcp = WanVrf_GetData(uiVrfID, WAN_VRF_PROPERTY_INDEX_DHCP);
    if ((pstDhcp != NULL) && (pstDhcp->hDhcpsHandle != NULL))
    {
        uiIP = DHCPS_GetServerIP(pstDhcp->hDhcpsHandle);
    }
    RcuBs_UnLock(uiPhase);

    return uiIP;
}

/* 返回网络序Mask */
UINT WAN_DHCP_GetMask(IN UINT uiVrfID)
{
    _WAN_DHCP_S *pstDhcp;
    UINT uiMask = 0;
    UINT uiPhase;

    uiPhase = RcuBs_Lock();
    pstDhcp = WanVrf_GetData(uiVrfID, WAN_VRF_PROPERTY_INDEX_DHCP);
    if ((pstDhcp != NULL) && (pstDhcp->hDhcpsHandle != NULL))
    {
        uiMask = DHCPS_GetMask(pstDhcp->hDhcpsHandle);
    }
    RcuBs_UnLock(uiPhase);

    return uiMask;
}

BS_STATUS WAN_DHCP_Init()
{
    if (BS_OK != WanVrf_RegEventListener(WAN_VRF_REG_PRI_NORMAL, wan_dhcp_VFEvent, NULL))
    {
        return BS_ERR;
    }

    WanUdpService_RegService(htons(DHCP_DFT_SERVER_PORT),
        WAN_UDP_SERVICE_FLAG_CUT_HEAD, wan_dhcp_PktInput, NULL);

    return BS_OK;
}

/* debug dhcp packet */
PLUG_API void WAN_DHCP_DebugPacket
(
    IN UINT ulArgc,
    IN CHAR **argv
)
{
    BIT_SET(g_uiWanDhcpDebugFlag, _WAN_DHCP_DBG_PACKET);
}

/* no debug dhcp packet */
PLUG_API void WAN_DHCP_NoDebugPacket
(
    IN UINT ulArgc,
    IN CHAR **argv
)
{
    BIT_CLR(g_uiWanDhcpDebugFlag, _WAN_DHCP_DBG_PACKET);
}

/* debug dhcp error */
PLUG_API void WAN_DHCP_DebugError
(
    IN UINT ulArgc,
    IN CHAR **argv
)
{
    BIT_SET(g_uiWanDhcpDebugFlag, _WAN_DHCP_DBG_ERROR);
}

/* no debug dhcp error */
PLUG_API void WAN_DHCP_NoDebugError
(
    IN UINT ulArgc,
    IN CHAR **argv
)
{
    BIT_CLR(g_uiWanDhcpDebugFlag, _WAN_DHCP_DBG_ERROR);
}


