/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2013-4-9
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/arp_utl.h"
#include "utl/socket_utl.h"
#include "utl/exec_utl.h"

#include "nat_main.h"
#include "nat_link.h"
#include "nat_phy.h"

#define _NAT_ARP_DBG_FLAG 0x1


#define _NAT_ARP_TIME_OF_TICK 1000 
#define _NAT_ARP_TIME_OUT_TIME (((60 * 1000 * 10) + _NAT_ARP_TIME_OF_TICK - 1) / _NAT_ARP_TIME_OF_TICK)  

static ARP_HANDLE g_hNatArp;
static UINT g_uiNatArpDebugFlag = 0;
static MTIMER_S g_stNatArpMTimer;

static BS_STATUS nat_arp_SendPacket(IN MBUF_S *pstMbuf, IN USER_HANDLE_S *pstUserHandle)
{
    return NAT_Link_OutPut(MBUF_GET_SEND_IF_INDEX(pstMbuf), pstMbuf, ETH_P_ARP);
}

static BOOL_T nat_arp_IsHostIP(IN IF_INDEX uiIfIndex, ARP_HEADER_S *pstArpHeader, IN USER_HANDLE_S *pstUserHandle)
{
    if (NAT_PHY_GetIP(uiIfIndex) == pstArpHeader->ulDstIp)
    {
        return TRUE;
    }

    return FALSE;
}


static UINT nat_arp_GetHostIp(IN IF_INDEX ifIndex, IN UINT uiDstIP, IN USER_HANDLE_S *pstUserHandle)
{
    return NAT_PHY_GetIP(ifIndex);
}

static VOID nat_arp_TimeOut(IN HANDLE hTimerHandle, IN USER_HANDLE_S *pstUserHandle)
{
    ARP_TimerStep(g_hNatArp);
}

BS_STATUS NAT_ARP_Init()
{
    g_hNatArp = ARP_CreateInstance(_NAT_ARP_TIME_OUT_TIME, TRUE);
    if (NULL == g_hNatArp)
    {
        return BS_ERR;
    }

    ARP_SetSendPacketFunc(g_hNatArp, nat_arp_SendPacket, NULL);
    ARP_SetIsHostIpFunc(g_hNatArp, nat_arp_IsHostIP, NULL);
    ARP_SetGetHostIpFunc(g_hNatArp, nat_arp_GetHostIp, NULL);

    MTimer_Add(&g_stNatArpMTimer, 1000, TIMER_FLAG_CYCLE, nat_arp_TimeOut, NULL);

    return BS_OK;
}

BS_STATUS NAT_ARP_PacketInput(IN MBUF_S *pstArpPacket)
{
    return ARP_PacketInput(g_hNatArp, pstArpPacket);
}


BS_STATUS NAT_ARP_GetMacByIp
(
    IN UINT uiIfIndex,
    IN UINT ulIpToResolve ,
    IN MBUF_S *pstMbuf,
    OUT MAC_ADDR_S *pstMacAddr
)
{
    return ARP_GetMacByIp(g_hNatArp, uiIfIndex, ulIpToResolve, pstMbuf, pstMacAddr);
}

PLUG_API BS_STATUS NAT_ARP_Debug(IN UINT ulArgc, IN UCHAR **argv)
{
    g_uiNatArpDebugFlag |= _NAT_ARP_DBG_FLAG;

	return BS_OK;
}

PLUG_API BS_STATUS NAT_ARP_NoDebug(IN UINT ulArgc, IN UCHAR **argv)
{
    g_uiNatArpDebugFlag &= ~((UINT)_NAT_ARP_DBG_FLAG);
    
	return BS_OK;
}

static int nat_arp_ShowEach(IN ARP_NODE_S *pstArpNode, IN HANDLE hUserHandle)
{
    EXEC_OutInfo(" %-15pI4 %pM %s\r\n",
        &pstArpNode->uiIp,
        &pstArpNode->stMac,
        pstArpNode->eType == ARP_TYPE_STATIC ? "Static" : "Dynamic");
    return 0;
}


PLUG_API BS_STATUS NAT_ARP_ShowArp
(
    IN UINT ulArgc,
    IN CHAR **argv
)
{
    EXEC_OutString(" IP              MAC               Type\r\n"
        "------------------------------------------------\r\n");

    ARP_Walk(g_hNatArp, nat_arp_ShowEach, NULL);

    EXEC_OutString("\r\n");

    return BS_OK;
}


PLUG_API BS_STATUS NAT_ARP_AddStaticArp
(
    IN UINT ulArgc,
    IN CHAR **argv
)
{
    UINT uiIP;
    MAC_ADDR_S stMacAddr;

    uiIP = Socket_NameToIpNet(argv[2]);
    if (uiIP == 0)
    {
        EXEC_OutString("Bad IP.\r\n");
        return BS_ERR;
    }

    if (BS_OK != ETH_String2Mac(argv[3], stMacAddr.aucMac))
    {
        EXEC_OutString("Bad MAC.\r\n");
        return BS_ERR;
    }
    
    if (BS_OK != ARP_AddStaticARP(g_hNatArp, uiIP, &stMacAddr))
    {
        EXEC_OutString("Add failed.\r\n");
        return BS_ERR;
    }

    return BS_OK;
}

static int nat_arp_SaveStaticArp(IN ARP_NODE_S *pstArpNode, IN HANDLE hFile)
{
    if (pstArpNode->eType == ARP_TYPE_STATIC)
    {
        CMD_EXP_OutputCmd(hFile, "arp static %pI4 %pM",
            &pstArpNode->uiIp,
            &pstArpNode->stMac);
    }

    return 0;
}

VOID NAT_ARP_Save(IN HANDLE hFile)
{
    ARP_Walk(g_hNatArp, nat_arp_SaveStaticArp, hFile);
}


