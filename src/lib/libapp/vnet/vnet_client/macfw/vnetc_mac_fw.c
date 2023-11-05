/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2008-10-13
* Description: 
* History:     
******************************************************************************/

#define RETCODE_FILE_NUM VNET_RETCODE_FILE_NUM_CMACFW

#include "bs.h"

#include "utl/mac_table.h"
#include "utl/msgque_utl.h"
#include "utl/arp_utl.h"
#include "utl/dhcp_utl.h"

#include "../../inc/vnet_conf.h"
#include "../../inc/vnet_node.h"

#include "../inc/vnetc_mac_tbl.h"
#include "../inc/vnetc_peer_info.h"
#include "../inc/vnetc_phy.h"
#include "../inc/vnetc_context.h"
#include "../inc/vnetc_vnic_phy.h"
#include "../inc/vnetc_conf.h"
#include "../inc/vnetc_node.h"


#define _VNETC_MAC_FW_DBG_PACKET 0x1

typedef struct
{
    MBUF_S *pstMbuf;
    UINT ulExportSesId;
}_VNETC_MAC_BROADCAST_S;


static UINT g_ulVnetCMacFwDebugFlag = 0;
static BOOL_T g_bVnetcMacPermitBroadcast = FALSE;

static inline VOID vnetc_mac_fw_LearnSourceMAC(IN MBUF_S *pstMbuf)
{
    ETH_HEADER_S *pstEthHead;
    MAC_NODE_S stMacNode = {0};
    VNETC_MAC_USER_DATA_S stUserData;
    UINT uiSrcNID;

    uiSrcNID = VNETC_Context_GetSrcNID(pstMbuf);

    pstEthHead = MBUF_MTOD(pstMbuf);

    stMacNode.stMac = pstEthHead->stSMac;
    stMacNode.uiPRI = VNETC_MAC_PRI_LOW;

    stUserData.uiNodeID = uiSrcNID;

    VNETC_MACTBL_Add(&stMacNode, &stUserData, MAC_MODE_LEARN);
}

static inline BS_STATUS vnetc_mac_fw_ForwardPacket (IN MBUF_S *pstMbuf)
{
    ETH_HEADER_S *pstEthHead;
    MAC_NODE_S stMacNode;
    VNETC_MAC_USER_DATA_S stUserData;

    pstEthHead = MBUF_MTOD(pstMbuf);

    if (MAC_ADDR_IS_MULTICAST(pstEthHead->stDMac.aucMac))
    {
        BS_DBG_OUTPUT(g_ulVnetCMacFwDebugFlag, _VNETC_MAC_FW_DBG_PACKET,
            ("VNET-MAC-FW:Broadcast packet, destMAC:%pM, sourceMAC:%pM, Protocol:0x%04x\r\n",
            &pstEthHead->stDMac, &pstEthHead->stSMac,
            ntohs(pstEthHead->usProto)));

        return VNETC_NODE_BroadCast(pstMbuf, VNET_NODE_PKT_PROTO_DATA);
    }

    stMacNode.stMac = pstEthHead->stDMac;

    if (BS_OK != VNETC_MACTBL_Find(&stMacNode, &stUserData))
    {
        BS_DBG_OUTPUT(g_ulVnetCMacFwDebugFlag, _VNETC_MAC_FW_DBG_PACKET,
            ("VNET-MAC-FW:Broadcast packet, destMAC:%pM, sourceMAC:%pM, Protocol:0x%04x\r\n",
            &pstEthHead->stDMac, &pstEthHead->stSMac,
            ntohs(pstEthHead->usProto)));

        
        return VNETC_NODE_BroadCast(pstMbuf, VNET_NODE_PKT_PROTO_DATA);
    }

    BS_DBG_OUTPUT(g_ulVnetCMacFwDebugFlag, _VNETC_MAC_FW_DBG_PACKET,
        ("VNET-MAC-FW:Send packet to node %s%d, destMAC:%pM, sourceMAC:%pM, Protocol:0x%04x\r\n",
        VNET_NODE_GetTypeStringByNID(stUserData.uiNodeID),
        VNET_NID_INDEX(stUserData.uiNodeID),        
        &pstEthHead->stDMac, &pstEthHead->stSMac,
        ntohs(pstEthHead->usProto)));

    return VNETC_NODE_PktOutput(stUserData.uiNodeID, pstMbuf, VNET_NODE_PKT_PROTO_DATA);
}

static BOOL_T vnetc_mac_fw_IsPermitPkt(IN MBUF_S *pstMbuf)
{
    if (g_bVnetcMacPermitBroadcast == TRUE)
    {
        return TRUE;
    }
    
    if (MBUF_GET_RECV_IF_INDEX(pstMbuf) != VNETC_VNIC_PHY_GetVnicIfIndex())
    {
        return TRUE;
    }

    if ((TRUE == ARP_IsArpPacket(pstMbuf))
        || (TRUE == DHCP_IsDhcpRequestEthPacketByMbuf(pstMbuf, NET_PKT_TYPE_ETH)))
    {
        return TRUE;
    }

    if (FALSE == ETH_IsMulticastPktByMbuf(pstMbuf))
    {
        return TRUE;
    }

    return FALSE;
}

BS_STATUS VNETC_MAC_FW_Input(IN MBUF_S *pstMbuf)
{
    ETH_HEADER_S *pstEthHead;
    UINT uiNodeID;

    if (TRUE != vnetc_mac_fw_IsPermitPkt(pstMbuf))
    {
        MBUF_Free(pstMbuf);
        return BS_OK;
    }

    if (BS_OK != MBUF_MakeContinue(pstMbuf, sizeof(ETH_HEADER_S)))
    {
        MBUF_Free(pstMbuf);
        RETURN(BS_BAD_PARA);
    }

    uiNodeID = VNETC_Context_GetSrcNID(pstMbuf);

    pstEthHead = MBUF_MTOD(pstMbuf);

    BS_DBG_OUTPUT(g_ulVnetCMacFwDebugFlag, _VNETC_MAC_FW_DBG_PACKET,
        ("VNET-MAC-FW:Receive packet from node %s%d: destMAC:%pM, sourceMAC:%pM, Protocol:0x%04x, size:%d\r\n",
        VNET_NODE_GetTypeStringByNID(uiNodeID), VNET_NID_INDEX(uiNodeID), 
        &pstEthHead->stDMac, &pstEthHead->stSMac,
        ntohs(pstEthHead->usProto), MBUF_TOTAL_DATA_LEN(pstMbuf)));

    
    vnetc_mac_fw_LearnSourceMAC(pstMbuf);

    return vnetc_mac_fw_ForwardPacket(pstMbuf);
}

VOID VNETC_MAC_FW_SetPermitBoradcast(IN BOOL_T bPermit)
{
    g_bVnetcMacPermitBroadcast = bPermit;
}


PLUG_API VOID VNETC_MAC_FW_DebugPacket(IN UINT ulArgc, IN CHAR **argv)
{
    g_ulVnetCMacFwDebugFlag |= _VNETC_MAC_FW_DBG_PACKET;
}


PLUG_API VOID VNETC_MAC_FW_NoDebugPacket(IN UINT ulArgc, IN CHAR **argv)
{
    g_ulVnetCMacFwDebugFlag &= ~_VNETC_MAC_FW_DBG_PACKET;
}

VOID VNETC_MAC_FW_NoDebugAll()
{
    g_ulVnetCMacFwDebugFlag = 0;
}



