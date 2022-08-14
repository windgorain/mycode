/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2013-1-24
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/mbuf_utl.h"
#include "utl/exec_utl.h"
#include "utl/fib_utl.h"
#include "utl/ipfwd_service.h"
#include "utl/vf_utl.h"
#include "utl/arp_utl.h"
#include "utl/bit_opt.h"
#include "comp/comp_if.h"

#include "../h/wan_vrf.h"
#include "../h/wan_ifnet.h"
#include "../h/wan_ipfwd_service.h"
#include "../h/wan_fib.h"
#include "../h/wan_arp.h"
#include "../h/wan_eth_link.h"
#include "../h/wan_ipfwd.h"
#include "../h/wan_bridge.h"
#include "../h/wan_ip_addr.h"
#include "../h/wan_blackhole.h"
#include "../h/wan_vrf_cmd.h"
#include "../h/wan_arp_agent.h"

#define _WAN_ARP_TIME_OUT_TICK WAN_VRF_TIME_TO_TICK(10 * 60 * 1000)  /* 10分钟 */

#define _WAN_ARP_DBG_FLAG_PACKET 0x1
#define _WAN_ARP_DBG_FLAG_EVENT  0x2

typedef struct
{
    RCU_NODE_S stRcu;
    ARP_HANDLE hArp;
}_WAN_ARP_S;

static UINT g_uiWanArpDebugFlag = 0;
static UINT g_uiWanArpVfIndex = 0;

static BS_STATUS wan_arp_SendPacket(IN MBUF_S *pstMbuf, IN USER_HANDLE_S *pstUserHandle)
{
    UINT uiVFID;
    UINT uiIfIndex;

    uiVFID = HANDLE_UINT(pstUserHandle->ahUserHandle[0]);

    MBUF_SET_OUTVPNID(pstMbuf, uiVFID);

    uiIfIndex = MBUF_GET_SEND_IF_INDEX(pstMbuf);

    BS_DBG_OUTPUT(g_uiWanArpDebugFlag, _WAN_ARP_DBG_FLAG_PACKET,
        ("ARP: Send arp packet.\r\n"));

    return IFNET_LinkOutput(uiIfIndex, pstMbuf, htons(ETH_P_ARP));
}

static BOOL_T wan_arp_IsHostIP(IN UINT uiIfIndex, ARP_HEADER_S *pstArpHeader, IN USER_HANDLE_S *pstUserHandle)
{
    if (pstArpHeader->ulDstIp != pstArpHeader->ulSenderIp)  /* 冲突检查arp不进行代理 */
    {
        if (WAN_ArpAgent_IsAgentOn(uiIfIndex, ntohl(pstArpHeader->ulDstIp)))
        {
            return TRUE;
        }
    }
    
    return WAN_IPAddr_IsInterfaceIp(uiIfIndex, pstArpHeader->ulDstIp);
}

/* 返回网络序IP */
static UINT wan_arp_GetHostIp
(
    IN IF_INDEX ifIndex,
    IN UINT uiDstIP, /* 网络序 */
    IN USER_HANDLE_S *pstUserHandle
)
{
    WAN_IP_ADDR_INFO_S stAddr;

    if (BS_OK != WAN_IPAddr_MatchBestNet(ifIndex, uiDstIP, &stAddr))
    {
        return 0;
    }

    return stAddr.uiIP;
}

static BS_STATUS wan_arp_VFEventCreate(IN UINT uiVrfID)
{
    _WAN_ARP_S *pstArp;
    USER_HANDLE_S stUserHandle;

    pstArp = MEM_ZMalloc(sizeof(_WAN_ARP_S));
    if (NULL == pstArp)
    {
        return BS_NO_MEMORY;
    }

    pstArp->hArp = ARP_CreateInstance(_WAN_ARP_TIME_OUT_TICK, TRUE);
    if (NULL == pstArp->hArp)
    {
        return (BS_NO_MEMORY);
    }

    stUserHandle.ahUserHandle[0] = UINT_HANDLE(uiVrfID);

    ARP_SetSendPacketFunc(pstArp->hArp, wan_arp_SendPacket, &stUserHandle);
    ARP_SetIsHostIpFunc(pstArp->hArp, wan_arp_IsHostIP, &stUserHandle);
    ARP_SetGetHostIpFunc(pstArp->hArp, wan_arp_GetHostIp, &stUserHandle);
    ARP_SetHostMac(pstArp->hArp, WAN_Bridge_GetMac());

    WanVrf_SetData(uiVrfID, WAN_VRF_PROPERTY_INDEX_ARP, pstArp);

    return BS_OK;
}

static VOID _wan_arp_RcuFree(IN VOID *pstRcuNode)
{
    _WAN_ARP_S *pstArp = pstRcuNode;

    ARP_DestoryInstance(pstArp->hArp);
    MEM_Free(pstArp);
}

static VOID wan_arp_VFEventDestory(IN UINT uiVrfID)
{
    _WAN_ARP_S *pstArp;

    pstArp = WanVrf_GetData(uiVrfID, WAN_VRF_PROPERTY_INDEX_ARP);
    WanVrf_SetData(uiVrfID, WAN_VRF_PROPERTY_INDEX_ARP, NULL);

    if (NULL != pstArp)
    {
        RcuEngine_Call(&pstArp->stRcu, _wan_arp_RcuFree);
    }

    return;
}

static VOID wan_arp_VFEventTimer(IN UINT uiVrfID)
{
    _WAN_ARP_S *pstArp;

    pstArp = WanVrf_GetData(uiVrfID, WAN_VRF_PROPERTY_INDEX_ARP);

    if ((NULL != pstArp) && (NULL != pstArp->hArp))
    {
        ARP_TimerStep(pstArp->hArp);
    }

    return;
}

static BS_STATUS wan_arp_VFEvent(IN UINT uiEvent, IN UINT uiVrfID, IN USER_HANDLE_S *pstUserHandle)
{
    CHAR szName[WAN_VRF_MAX_NAME_LEN + 1];
    
    BS_DBG_OUTPUT(g_uiWanArpDebugFlag, _WAN_ARP_DBG_FLAG_EVENT,
            ("ARP: Recv VF %s event.\r\n", WanVrf_GetNameByID2(uiEvent, szName)));

    switch (uiEvent)
    {
        case VF_EVENT_CREATE_VF:
        {
            wan_arp_VFEventCreate(uiVrfID);
            break;
        }

        case VF_EVENT_DESTORY_VF:
        {
            wan_arp_VFEventDestory(uiVrfID);
            break;
        }

        case VF_EVENT_TIMER:
        {
            wan_arp_VFEventTimer(uiVrfID);
            break;
        }
    }
    
    return BS_OK;
}

static BS_WALK_RET_E wan_arp_ShowEach(IN ARP_NODE_S *pstArpNode, IN HANDLE hUserHandle)
{
    EXEC_OutInfo(" %-15pI4 %pM\r\n", &pstArpNode->uiIp, &pstArpNode->stMac);
    return BS_WALK_CONTINUE;
}

BS_STATUS WAN_ARP_Init()
{
    g_uiWanArpVfIndex = WanVrf_RegEventListener(WAN_VRF_REG_PRI_NORMAL, wan_arp_VFEvent, NULL);
    if (VF_INVALID_USER_INDEX == g_uiWanArpVfIndex)
    {
        return BS_ERR;
    }

    return BS_OK;
}

BS_STATUS WAN_ARP_PacketInput(IN MBUF_S *pstArpPacket)
{
    _WAN_ARP_S *pstArp;
    UINT uiVrfID;
    BS_STATUS eRet;
    UINT uiPhase;

    BS_DBG_OUTPUT(g_uiWanArpDebugFlag, _WAN_ARP_DBG_FLAG_PACKET,
        ("ARP: Recv arp packet.\r\n"));

    uiVrfID = MBUF_GET_INVPNID(pstArpPacket);

    uiPhase = RcuEngine_Lock();
    pstArp = WanVrf_GetData(uiVrfID, WAN_VRF_PROPERTY_INDEX_ARP);
    if ((NULL != pstArp) && (NULL != pstArp->hArp))
    {
        eRet = ARP_PacketInput(pstArp->hArp, pstArpPacket);
    }
    else
    {
        BS_DBG_OUTPUT(g_uiWanArpDebugFlag, _WAN_ARP_DBG_FLAG_PACKET,
            ("ARP: No such arp handle.\r\n"));
        MBUF_Free(pstArpPacket);
        eRet = BS_NO_SUCH;
    }
    RcuEngine_UnLock(uiPhase);

    return eRet;
}

/* 根据IP得到MAC，如果得不到,则发送ARP请求，并返回BS_PROCESSED. */
BS_STATUS WAN_ARP_GetMacByIp
(
    IN UINT uiIfIndex,
    IN UINT ulIpToResolve /* 网络序 */,
    IN MBUF_S *pstMbuf,
    OUT MAC_ADDR_S *pstMacAddr
)
{
    _WAN_ARP_S *pstArp;
    UINT uiVrfID;
    BS_STATUS eRet;
    UINT uiPhase;

    uiVrfID = MBUF_GET_OUTVPNID(pstMbuf);

    uiPhase = RcuEngine_Lock();
    pstArp = WanVrf_GetData(uiVrfID, WAN_VRF_PROPERTY_INDEX_ARP);
    if ((NULL != pstArp) && (NULL != pstArp->hArp))
    {
        eRet = ARP_GetMacByIp(pstArp->hArp, uiIfIndex, ulIpToResolve, pstMbuf, pstMacAddr);
    }
    else
    {
        eRet = BS_NO_SUCH;
    }
    RcuEngine_UnLock(uiPhase);

    return eRet;
}

/* debug arp packet */
PLUG_API void WAN_ARP_DebugPacket
(
    IN UINT ulArgc,
    IN CHAR **argv
)
{
    BIT_SET(g_uiWanArpDebugFlag, _WAN_ARP_DBG_FLAG_PACKET);
}

/* no debug arp packet */
PLUG_API void WAN_ARP_NoDebugPacket
(
    IN UINT ulArgc,
    IN CHAR **argv
)
{
    BIT_CLR(g_uiWanArpDebugFlag, _WAN_ARP_DBG_FLAG_PACKET);
}

/* debug arp event */
PLUG_API void WAN_ARP_DebugEvent
(
    IN UINT ulArgc,
    IN CHAR **argv
)
{
    BIT_SET(g_uiWanArpDebugFlag, _WAN_ARP_DBG_FLAG_EVENT);
}

/* no debug arp event */
PLUG_API void WAN_ARP_NoDebugEvent
(
    IN UINT ulArgc,
    IN CHAR **argv
)
{
    BIT_CLR(g_uiWanArpDebugFlag, _WAN_ARP_DBG_FLAG_EVENT);
}

/*
    VF视图下:
    show arp
*/
PLUG_API BS_STATUS WAN_ARP_ShowArp
(
    IN UINT ulArgc,
    IN CHAR **argv,
    IN VOID *pEnv
)
{
    UINT uiVrfID;
    _WAN_ARP_S *pstArp;
    UINT uiPhase;

    uiVrfID = WAN_VrfCmd_GetVrfByEnv(pEnv);

    uiPhase = RcuEngine_Lock();
    pstArp = WanVrf_GetData(uiVrfID, WAN_VRF_PROPERTY_INDEX_ARP);
    if ((NULL != pstArp) && (NULL != pstArp->hArp))
    {
        EXEC_OutString(" IP              MAC\r\n"
            "--------------------------------------\r\n");

        ARP_Walk(pstArp->hArp, wan_arp_ShowEach, NULL);

        EXEC_OutString("\r\n");
    }
    RcuEngine_UnLock(uiPhase);

    return BS_OK;
}


