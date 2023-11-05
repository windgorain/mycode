/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2013-1-9
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/nat_utl.h"
#include "utl/socket_utl.h"
#include "utl/sprintf_utl.h"
#include "utl/eth_utl.h"
#include "utl/txt_utl.h"
#include "utl/exec_utl.h"
#include "utl/ip_utl.h"
#include "utl/ip_mbuf.h"
#include "utl/ip_protocol.h"
#include "utl/pcap_agent.h"
#include "comp/comp_pcap.h"
#include "comp/comp_pcap.h"

#include "nat_main.h"
#include "nat_link.h"
#include "nat_phy.h"

#define NAT_MAIN_MS_IN_TICK 5000

static BOOL_T g_bNatMainStartted = FALSE;
static CHAR g_szNatPubIpAddress[16] = "";
static UINT g_uiNatPubIpAddress; 
static NAT_HANDLE g_hNatMainHandle = NULL;

static CHAR g_szNatGateWay[16] = "";
static UINT g_uiNatGateWay = 0;   
static CHAR g_szNatPubMac[18] = "";
static MAC_ADDR_S g_stNatPubMac;
static MTIMER_S g_stNatMTimer;

static VOID nat_main_PubPktIn(IN MBUF_S *pstMbuf)
{
    UINT uiIfIndex;
    IP_HEAD_S *pstHead;

    if (BS_OK != NAT_PacketTranslateByMbuf(g_hNatMainHandle, pstMbuf, TRUE, &uiIfIndex))
    {
        MBUF_Free(pstMbuf);
        return;
    }

    pstHead = MBUF_MTOD(pstMbuf);

    MBUF_SET_SEND_IF_INDEX(pstMbuf, uiIfIndex);
    MBUF_SET_NEXT_HOP(pstMbuf, pstHead->unDstIp.uiIp);

    NAT_Link_OutPut(uiIfIndex, pstMbuf, ETH_P_IP);
}


static BS_STATUS nat_main_PrivatePktInput(IN MBUF_S *pstMbuf)
{
    UINT uiIfIndex;
    UINT uiSendIfIndex;
    
    uiIfIndex = MBUF_GET_RECV_IF_INDEX(pstMbuf);

    if (BS_OK != NAT_PacketTranslateByMbuf(g_hNatMainHandle, pstMbuf, FALSE, &uiIfIndex))
    {
        MBUF_Free(pstMbuf);
        return BS_ERR;
    }

    uiSendIfIndex = NAT_PHY_GetPubIfIndex();
    MBUF_SET_SEND_IF_INDEX(pstMbuf, uiSendIfIndex);
    MBUF_SET_NEXT_HOP(pstMbuf, g_uiNatGateWay);

    MBUF_SET_SOURCEMAC(pstMbuf, g_stNatPubMac.aucMac);
    MBUF_SET_ETH_MARKFLAG(pstMbuf, MBUF_L2_FLAG_SRC_MAC);

    return NAT_Link_OutPut(uiSendIfIndex, pstMbuf, ETH_P_IP);
}

static BOOL_T nat_main_IsPrivatePkt(IN MBUF_S *pstMbuf)
{
    UINT uiIfIndex;
    UINT uiIP;
    UINT uiMask;
    IP_HEAD_S *pstHead;

    uiIfIndex = MBUF_GET_RECV_IF_INDEX(pstMbuf);

	if (FALSE == NAT_PHY_IsPrivateIfIndex(uiIfIndex))
	{
		return FALSE;
	}

    uiIP = NAT_PHY_GetIP(uiIfIndex);
    uiMask = NAT_PHY_GetMask(uiIfIndex);

    pstHead = MBUF_MTOD(pstMbuf);

	
    if ((pstHead->unDstIp.uiIp & uiMask) == (uiIP & uiMask))
    {
        return FALSE;
    }

    
    if ((pstHead->unSrcIp.uiIp & uiMask) != (uiIP & uiMask))
    {
        return FALSE;
    }

    
    if (pstHead->unSrcIp.uiIp == uiIP)
    {
        return FALSE;
    }

    return TRUE;
}

static BOOL_T nat_main_IsPubPkt(IN MBUF_S *pstMbuf)
{
    UINT uiIfIndex;
    UINT uiIP;
    IP_HEAD_S *pstHead;

    uiIfIndex = MBUF_GET_RECV_IF_INDEX(pstMbuf);

	if (NAT_PHY_GetPubIfIndex() != uiIfIndex)
	{
		return FALSE;
	}

    uiIP = NAT_PHY_GetIP(uiIfIndex);
    pstHead = MBUF_MTOD(pstMbuf);

	
    if (pstHead->unDstIp.uiIp != uiIP)
    {
        return FALSE;
    }

    return TRUE;
}

VOID NAT_Main_PktInput(IN MBUF_S *pstMbuf)
{
    if (g_bNatMainStartted == FALSE)
    {
        MBUF_Free(pstMbuf);
        return;
    }

    if (BS_OK != IP_ValidPkt(pstMbuf))
    {
        MBUF_Free(pstMbuf);
        return;
    }

    if (BS_OK != MBUF_MakeContinue(pstMbuf, sizeof(IP_HEAD_S)))
    {
        MBUF_Free(pstMbuf);
        return;
    }

    if (nat_main_IsPrivatePkt(pstMbuf))
    {
        nat_main_PrivatePktInput(pstMbuf);
    }
    else if (nat_main_IsPubPkt(pstMbuf))
    {
        nat_main_PubPktIn(pstMbuf);
    }
    else
    {
        MBUF_Free(pstMbuf);
        return;
    }

    return;
}

static VOID nat_cmd_TimeOut(IN HANDLE hTimerHandle, IN USER_HANDLE_S *pstUserHandle)
{
    NAT_TimerStep(g_hNatMainHandle);
}

static VOID nat_main_AutoConfig()
{
    UINT auiPubIp[NAT_MAX_PUB_IP_NUM];

    if (g_uiNatGateWay == 0)
    {
        g_uiNatGateWay = NAT_PHY_GetPubPcapGateWayIP();
    }

    if (g_uiNatPubIpAddress == 0)
    {
        g_uiNatPubIpAddress = NAT_PHY_GetPubPcapIP();
        if (g_hNatMainHandle != NULL)
        {
            Mem_Zero(auiPubIp, sizeof(auiPubIp));
            auiPubIp[0] = g_uiNatPubIpAddress;        
            NAT_SetPubIp(g_hNatMainHandle, auiPubIp);
        }
    }

    if (g_szNatPubMac[0] == '\0')
    {
        g_stNatPubMac = *NAT_PHY_GetPubPcapMAC();
    }

    return;
}

BS_STATUS NAT_Main_Start()
{
    UINT auiPubIp[NAT_MAX_PUB_IP_NUM];
    NAT_HANDLE hNatHandle;
    int ret;

    if (g_bNatMainStartted == TRUE)
    {
        return BS_OK;
    }

    if (NAT_PHY_GetPubPcapIndex() == PCAP_AGENT_INDEX_INVALID)
    {
        EXEC_OutString(" Please config pcap first.\r\n");
        return BS_ERR;
    }

    nat_main_AutoConfig();

    Mem_Zero(auiPubIp, sizeof(auiPubIp));

    hNatHandle = NAT_Create(50000, 65535, NAT_MAIN_MS_IN_TICK, TRUE);
    if (NULL == hNatHandle)
    {
        return (BS_NO_MEMORY);
    }
    auiPubIp[0] = g_uiNatPubIpAddress;
    NAT_SetPubIp(hNatHandle, auiPubIp);

    g_hNatMainHandle = hNatHandle;

    ret = MTimer_Add(&g_stNatMTimer, NAT_MAIN_MS_IN_TICK,
            TIMER_FLAG_CYCLE, nat_cmd_TimeOut, NULL);
    if (ret < 0) {
        NAT_Destory(hNatHandle);
        g_hNatMainHandle = NULL;

        return BS_CAN_NOT_OPEN;
    }

    NAT_Phy_Start();

    g_bNatMainStartted = TRUE;

    return BS_OK;
}

VOID NAT_Main_SetGateWay(IN CHAR *pcIp)
{
    UINT uiNextHop;

    if (NULL == pcIp)
    {
        return;
    }

    TXT_Strlcpy(g_szNatGateWay, pcIp, sizeof(g_szNatGateWay));

    uiNextHop = Socket_NameToIpNet(g_szNatGateWay);
    if (0 == uiNextHop)
    {
        g_szNatGateWay[0] = '\0';
    }
    else
    {
        g_uiNatGateWay = htonl(uiNextHop);
    }

    return;
}

VOID NAT_Main_SetPubIp(IN CHAR *pcIp)
{
    UINT uiPubIp;
    UINT auiPubIp[NAT_MAX_PUB_IP_NUM];

    if (NULL == pcIp)
    {
        return;
    }

    TXT_Strlcpy(g_szNatPubIpAddress, pcIp, sizeof(g_szNatPubIpAddress));

    uiPubIp = Socket_NameToIpNet(g_szNatPubIpAddress);
    if (0 == uiPubIp)
    {
        g_szNatPubIpAddress[0] = '\0';
    }
    else
    {
        uiPubIp = htonl(uiPubIp);
        g_uiNatPubIpAddress = uiPubIp;

        if (g_hNatMainHandle != NULL)
        {
            Mem_Zero(auiPubIp, sizeof(auiPubIp));
            auiPubIp[0] = uiPubIp;        
            NAT_SetPubIp(g_hNatMainHandle, auiPubIp);
        }
    }

    return;
}

UINT NAT_Main_GetPubIp()
{
    return g_uiNatPubIpAddress;
}

VOID NAT_Main_SetPubMac(IN CHAR *pcMac)
{
    if (NULL == pcMac)
    {
        return;
    }

    TXT_Strlcpy(g_szNatPubMac, pcMac, sizeof(g_szNatPubMac));

    if (BS_OK != ETH_String2Mac(pcMac, g_stNatPubMac.aucMac))
    {
        g_szNatPubMac[0] = '\0';
    }

    return;
}

static int nat_main_ShowEach(IN NAT_NODE_S *pstNatNode, IN HANDLE hUserHandle)
{
    CHAR szPrivateAddress[24];
    CHAR szPubAddress[24];

    BS_Snprintf(szPrivateAddress, sizeof(szPrivateAddress), "%pI4:%d",
        &pstNatNode->uiPrivateIp, ntohs(pstNatNode->usPrivatePort));
    BS_Snprintf(szPubAddress, sizeof(szPubAddress), "%pI4:%d",
        &pstNatNode->uiPubIp, ntohs(pstNatNode->usPubPort));

    EXEC_OutInfo(" %-20s %-20s %-5s %-6d %s\r\n",
        szPrivateAddress, szPubAddress,
        IPProtocol_GetNameExt(pstNatNode->ucType),
        pstNatNode->uiDomainId,
        NAT_GetStatusString(pstNatNode->ucType, pstNatNode->ucStatus));

    return 0;
}

VOID NAT_Main_Show()
{
    if (NULL == g_hNatMainHandle)
    {
        return;
    }

    EXEC_OutString(" Private              Pub                  Type  Domain Status\r\n"
                              "------------------------------------------------------------------------\r\n");

    NAT_Walk(g_hNatMainHandle, nat_main_ShowEach, NULL);

    EXEC_OutString("\r\n");

    return;
}

VOID NAT_Main_Save(IN HANDLE hFile)
{
    if (g_szNatGateWay[0] != '\0')
    {
        CMD_EXP_OutputCmd(hFile, "gateway-ip %s", g_szNatGateWay);
    }

    if (g_szNatPubIpAddress[0] != '\0')
    {
        CMD_EXP_OutputCmd(hFile, "pub-ip %s", g_szNatPubIpAddress);
    }

    if (g_szNatPubMac[0] != '\0')
    {
        CMD_EXP_OutputCmd(hFile, "pub-mac %s", g_szNatPubMac);
    }

    if (g_bNatMainStartted == TRUE)
    {
        CMD_EXP_OutputCmd(hFile, "start");
    }

    return;
}

PLUG_API BS_STATUS NAT_CMD_DebugPacket(IN UINT ulArgc, IN UCHAR **argv)
{
    NAT_SetDbgFlag(g_hNatMainHandle, NAT_DBG_PACKET);

	return BS_OK;
}

PLUG_API BS_STATUS NAT_CMD_NoDebugPacket(IN UINT ulArgc, IN UCHAR **argv)
{
    NAT_ClrDbgFlag(g_hNatMainHandle, NAT_DBG_PACKET);

	return BS_OK;
}


