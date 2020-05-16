#include "bs.h"

#include "utl/bit_opt.h"
#include "utl/txt_utl.h"
#include "utl/mbuf_utl.h"
#include "utl/eth_utl.h"
#include "utl/pcap_agent.h"
#include "utl/msgque_utl.h"
#include "comp/comp_if.h"
#include "comp/comp_pcap.h"

#include "nat_if.h"
#include "nat_link.h"
#include "nat_phy.h"

#define _NAT_PHY_INVALID_INDEX 0xffffffff

#define _NAT_PHY_PCAP_FLAG_PRIVATE 0x1
#define _NAT_PHY_PCAP_FLAG_PUB     0x2

/* VNET ETH PHY的事件 */
#define _NAT_PHY_INPUT_DATA_EVENT 0x1

/* VNET ETH PHY的消息类型 */
#define _NAT_PHY_INPUT_DATA_MSG   1

typedef struct
{
    UINT uiFlag;
    UINT uiPcapIndex;
    UINT uiIfIndex;
    UINT uiHostIP;  /* 网络序 */
    UINT uiMask;    /* 网络序 */
    UINT uiGateWayIP;  /* 网络序 */
    MAC_ADDR_S stMacAddr;
}_NAT_PHY_CTRL_S;

static _NAT_PHY_CTRL_S g_astNatPrivatePhy[PCAP_MAX_NUM];
static UINT g_uiNatPhyPubPcapIndex = PCAP_INVALID_INDEX;
static MSGQUE_HANDLE g_hNatPhyQueId = NULL;
static EVENT_HANDLE g_hNatPhyEventId = 0;

static VOID nat_phy_PktIn
(
    IN UCHAR *pucPktData,
    IN PKTCAP_PKT_INFO_S *pstPktInfo,
    IN USER_HANDLE_S *pstUserHandle
)
{
    MBUF_S *pstMbuf;
    _NAT_PHY_CTRL_S *pstPhyPcap;
    MSGQUE_MSG_S stMsg;

    pstPhyPcap = pstUserHandle->ahUserHandle[0];
    
    if (pstPktInfo->uiPktCaptureLen != pstPktInfo->uiPktRawLen)
    {
        return;
    }

    pstMbuf = MBUF_CreateByCopyBuf(200, pucPktData, pstPktInfo->uiPktCaptureLen, MBUF_DATA_DATA);
    if (NULL == pstMbuf)
    {
        return;
    }

    MBUF_SET_RECV_IF_INDEX(pstMbuf, pstPhyPcap->uiIfIndex);

	stMsg.ahMsg[0] = UINT_HANDLE(_NAT_PHY_INPUT_DATA_MSG);
    stMsg.ahMsg[1] = pstMbuf;

    if (BS_OK != MSGQUE_WriteMsg(g_hNatPhyQueId, &stMsg)) {
        MBUF_Free(pstMbuf);
        return;
    }

    Event_Write(g_hNatPhyEventId, _NAT_PHY_INPUT_DATA_EVENT);

    return;
}

static BS_STATUS nat_phy_CreatePcap(IN UINT uiPcapIndex)
{
    UINT uiIfIndex;
    NETINFO_ADAPTER_S stInfo;

    if (g_astNatPrivatePhy[uiPcapIndex].uiIfIndex == 0)
    {
        uiIfIndex = CompIf_CreateIf("nat.pcap");
        if (uiIfIndex == 0)
        {
            return BS_ERR;
        }

        COMP_PCAP_Ioctl(uiPcapIndex, PCAP_IOCTL_CMD_GET_NET_INFO, &stInfo);

        g_astNatPrivatePhy[uiPcapIndex].uiIfIndex = uiIfIndex;
        g_astNatPrivatePhy[uiPcapIndex].uiPcapIndex = uiPcapIndex;
        g_astNatPrivatePhy[uiPcapIndex].uiHostIP = stInfo.auiIpAddr[0];
        g_astNatPrivatePhy[uiPcapIndex].uiMask = stInfo.auiIpMask[0];
        g_astNatPrivatePhy[uiPcapIndex].uiGateWayIP = stInfo.uiGateWay;
        MAC_ADDR_COPY(g_astNatPrivatePhy[uiPcapIndex].stMacAddr.aucMac, stInfo.stMacAddr.aucMac);

        CompIf_Ioctl(uiIfIndex, IFNET_CMD_SET_MAC, &stInfo.stMacAddr);
    }

    return BS_OK;
}

static BS_STATUS nat_phy_Output(IN UINT uiIfIndex, IN MBUF_S *pstMbuf)
{
    UINT uiPcapIndex;

    uiPcapIndex = NAT_PHY_GetPcapIndexByIfIndex(uiIfIndex);

    if (uiPcapIndex == PCAP_INVALID_INDEX)
    {
        MBUF_Free(pstMbuf);
        return BS_ERR;
    }
    
    return COMP_PCAP_SendPkt(uiPcapIndex, pstMbuf);
}

UINT nat_phy_GetIndexByIfIndex(IN UINT uiIfIndex)
{
    UINT i;

    for (i=0; i<PCAP_MAX_NUM; i++)
    {
        if (g_astNatPrivatePhy[i].uiIfIndex == uiIfIndex)
        {
            return i;
        }
    }

    return _NAT_PHY_INVALID_INDEX;
}

static inline BS_STATUS nat_pcap_DealInputDataMsg (IN MSGQUE_MSG_S *pstMsg)
{
    MBUF_S *pstMbuf;

    pstMbuf = (MBUF_S*)pstMsg->ahMsg[1];

    return NAT_Link_Input(MBUF_GET_RECV_IF_INDEX(pstMbuf), pstMbuf);
}

static inline BS_STATUS nat_pcap_DealMsg (IN MSGQUE_MSG_S *pstMsg)
{
    UINT ulMsgType;
    BS_STATUS eRet;

    ulMsgType = HANDLE_UINT(pstMsg->ahMsg[0]);

    switch (ulMsgType)
    {
        case _NAT_PHY_INPUT_DATA_MSG:
            eRet = nat_pcap_DealInputDataMsg(pstMsg);
            break;

        default:
            eRet = BS_NOT_SUPPORT;
            BS_WARNNING(("Not support yet!"));
            break;
    }

    return eRet;
}

static void nat_pcap_Main(IN USER_HANDLE_S *pstUserHandle)
{
    UINT64 uiEvent;
    MSGQUE_MSG_S stMsg;

    for (;;)
    {
        Event_Read (g_hNatPhyEventId, _NAT_PHY_INPUT_DATA_EVENT, &uiEvent,
                EVENT_FLAG_WAIT, BS_WAIT_FOREVER);

        if (uiEvent & _NAT_PHY_INPUT_DATA_EVENT) {
            while (BS_OK == MSGQUE_ReadMsg(g_hNatPhyQueId, &stMsg)) {
                nat_pcap_DealMsg (&stMsg);
            }
        }
    }
}

BS_STATUS NAT_Phy_Init()
{
    IF_PHY_PARAM_S stPhyParam;
    IF_LINK_PARAM_S stLinkParam;
    IF_TYPE_PARAM_S stTypeParam = {0};

    stLinkParam.pfLinkInput = NAT_Link_Input;
    stLinkParam.pfLinkOutput = NAT_Link_OutPut;
    CompIf_SetLinkType("nat.eth", &stLinkParam);

    stPhyParam.pfPhyOutput = nat_phy_Output;
    CompIf_SetPhyType("nat.pcap", &stPhyParam);

    stTypeParam.pcProtoType = NULL;
    stTypeParam.pcLinkType = "nat.eth";
    stTypeParam.pcPhyType = "nat.pcap";
    stTypeParam.uiFlag = IF_TYPE_FLAG_HIDE;

    CompIf_AddIfType("nat.pcap", &stTypeParam);

    if (NULL == (g_hNatPhyEventId = Event_Create()))
    {
        return (BS_ERR);
    }

    if (NULL == (g_hNatPhyQueId = MSGQUE_Create(512)))
    {
        return (BS_ERR);
    }

    THREAD_Create("nat_phy", NULL, nat_pcap_Main, NULL);

    return BS_OK;
}

BS_STATUS NAT_Phy_PrivatePcap(IN CHAR *pcPcap)
{
    UINT uiIndex = 0;

    TXT_Atoui(pcPcap, &uiIndex);

    if (uiIndex >= PCAP_MAX_NUM)
    {
        return BS_ERR;
    }

	if (BS_OK != nat_phy_CreatePcap(uiIndex))
	{
		return BS_ERR;
	}

	g_astNatPrivatePhy[uiIndex].uiFlag |= _NAT_PHY_PCAP_FLAG_PRIVATE;

	return BS_OK;
}

BS_STATUS NAT_Phy_PubPcap(IN CHAR *pcPcap)
{
    UINT uiIndex = 0;

    TXT_Atoui(pcPcap, &uiIndex);

    if (uiIndex >= PCAP_MAX_NUM)
    {
        return BS_ERR;
    }

	if (BS_OK != nat_phy_CreatePcap(uiIndex))
	{
		return BS_ERR;
	}

	if (g_uiNatPhyPubPcapIndex != PCAP_INVALID_INDEX)
	{
		BIT_CLR(g_astNatPrivatePhy[g_uiNatPhyPubPcapIndex].uiFlag,
				_NAT_PHY_PCAP_FLAG_PUB);
	}

    g_astNatPrivatePhy[uiIndex].uiFlag |= _NAT_PHY_PCAP_FLAG_PUB;
    g_uiNatPhyPubPcapIndex = uiIndex;

	return BS_OK;
}

UINT NAT_PHY_GetPubIfIndex()
{
    if (g_uiNatPhyPubPcapIndex == PCAP_INVALID_INDEX)
    {
        return 0;
    }

    return g_astNatPrivatePhy[g_uiNatPhyPubPcapIndex].uiIfIndex;
}

UINT NAT_PHY_GetPubPcapIndex()
{
    if (g_uiNatPhyPubPcapIndex == PCAP_INVALID_INDEX)
    {
        return PCAP_INVALID_INDEX;
    }

    return g_astNatPrivatePhy[g_uiNatPhyPubPcapIndex].uiPcapIndex;
}

UINT NAT_PHY_GetPubPcapIP()
{
    if (g_uiNatPhyPubPcapIndex == PCAP_INVALID_INDEX)
    {
        return 0;
    }

    return g_astNatPrivatePhy[g_uiNatPhyPubPcapIndex].uiHostIP;
}

MAC_ADDR_S * NAT_PHY_GetPubPcapMAC()
{
    if (g_uiNatPhyPubPcapIndex == PCAP_INVALID_INDEX)
    {
        return NULL;
    }

    return &g_astNatPrivatePhy[g_uiNatPhyPubPcapIndex].stMacAddr;
}

UINT NAT_PHY_GetPubPcapGateWayIP()
{
    if (g_uiNatPhyPubPcapIndex == PCAP_INVALID_INDEX)
    {
        return 0;
    }

    return g_astNatPrivatePhy[g_uiNatPhyPubPcapIndex].uiGateWayIP;
}


UINT NAT_PHY_GetPcapIndexByIfIndex(IN UINT uiIfIndex)
{
    UINT i;

    i = nat_phy_GetIndexByIfIndex(uiIfIndex);

    if (i == _NAT_PHY_INVALID_INDEX)
    {
        return PCAP_INVALID_INDEX;
    }

    return g_astNatPrivatePhy[i].uiPcapIndex;
}

UINT NAT_PHY_GetIP(IN UINT uiIfIndex)
{
    UINT uiIndex;

    uiIndex = nat_phy_GetIndexByIfIndex(uiIfIndex);
    if (uiIndex == _NAT_PHY_INVALID_INDEX)
    {
        return 0;
    }

    return g_astNatPrivatePhy[uiIndex].uiHostIP;
}

UINT NAT_PHY_GetMask(IN UINT uiIfIndex)
{
    UINT uiIndex;

    uiIndex = nat_phy_GetIndexByIfIndex(uiIfIndex);
    if (uiIndex == _NAT_PHY_INVALID_INDEX)
    {
        return 0;
    }

    return g_astNatPrivatePhy[uiIndex].uiMask;
}

/* 判断pub pcap是否同时是private pcap */
BOOL_T NAT_PHY_PubPcapIsPrivate()
{
    if (g_uiNatPhyPubPcapIndex == PCAP_INVALID_INDEX)
    {
        return FALSE;
    }

    if (g_astNatPrivatePhy[g_uiNatPhyPubPcapIndex].uiFlag & _NAT_PHY_PCAP_FLAG_PRIVATE)
    {
		return TRUE;
    }

    return FALSE;
}

BOOL_T NAT_PHY_IsPrivateIfIndex(IN UINT uiIfIndex)
{
	UINT uiIndex;

	uiIndex = NAT_PHY_GetPcapIndexByIfIndex(uiIfIndex);
	if (PCAP_INVALID_INDEX == uiIndex)
	{
		return FALSE;
	}

	if (g_astNatPrivatePhy[uiIndex].uiFlag & _NAT_PHY_PCAP_FLAG_PRIVATE)
	{
		return TRUE;
	}

	return FALSE;
}

BS_STATUS NAT_Phy_Start()
{
    UINT i;
    USER_HANDLE_S stUserHandle;

    for (i=0; i<PCAP_MAX_NUM; i++)
    {
        if (g_astNatPrivatePhy[i].uiFlag == 0)
        {
            continue;
        }

        stUserHandle.ahUserHandle[0] = &g_astNatPrivatePhy[i];

        COMP_PCAP_RegService(g_astNatPrivatePhy[i].uiPcapIndex, ETH_P_ARP, nat_phy_PktIn, &stUserHandle);
        COMP_PCAP_RegService(g_astNatPrivatePhy[i].uiPcapIndex, ETH_P_IP, nat_phy_PktIn, &stUserHandle);
    }

    return BS_OK;
}

VOID NAT_Phy_Save(IN HANDLE hFile)
{
    UINT i;

    if (g_uiNatPhyPubPcapIndex != PCAP_INVALID_INDEX)
    {
        CMD_EXP_OutputCmd(hFile, "pcap pub %d", g_uiNatPhyPubPcapIndex);
    }

    for (i=0; i<PCAP_MAX_NUM; i++)
    {
        if (g_astNatPrivatePhy[i].uiFlag & _NAT_PHY_PCAP_FLAG_PRIVATE)
        {
            CMD_EXP_OutputCmd(hFile, "pcap private %d",
				g_astNatPrivatePhy[i].uiPcapIndex);
        }
    }

    return;
}

