/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2008-9-20
* Description: 
* History:     
******************************************************************************/

#define RETCODE_FILE_NUM VNET_RETCODE_FILE_NUM_MACFW
        
#include "bs.h"

#include "utl/mac_table.h"
#include "utl/msgque_utl.h"
#include "utl/arp_utl.h"

#include "../../inc/vnet_conf.h"
#include "../../inc/vnet_node.h"

#include "../../vnet/inc/vnet_mac_acl.h"

#include "../inc/vnets_conf.h"
#include "../inc/vnets_mac_tbl.h"
#include "../inc/vnets_mac_layer.h"
#include "../inc/vnets_phy.h"
#include "../inc/vnets_ses.h"
#include "../inc/vnets_node.h"
#include "../inc/vnets_node_fwd.h"
#include "../inc/vnets_context.h"
#include "../inc/vnets_wan_plug.h"


#define _VNETS_MAC_LAYER_DBG_PACKET 0x1

static UINT g_ulVnetSMacLayerDebugFlag = 0;

static inline BS_STATUS vnets_maclayer_DeliveUp(IN MBUF_S *pstMbuf)
{
    return VNETS_WAN_PLUG_PktInput(pstMbuf);
}

static VOID vnets_maclayer_LearnMAC(IN UINT uiDomainID, IN UINT uiNodeID, IN MAC_ADDR_S *pstMAC)
{
    VNETS_MAC_NODE_S stMacNode;

    memset(&stMacNode, 0, sizeof(VNETS_MAC_NODE_S));
    stMacNode.stMacNode.stMac = *pstMAC;
    stMacNode.stUserData.uiNodeID = uiNodeID;

    VNETS_MACTBL_Add(uiDomainID, &stMacNode, MAC_MODE_LEARN);
}

static BOOL_T vnets_maclayer_IsPermit(IN MAC_ADDR_S *pstMAC)
{
    if (MAC_ADDR_IS_MULTICAST(pstMAC->aucMac))
    {
        return TRUE;
    }

    if (VNETS_IsHostMac(pstMAC))
    {
        return TRUE;
    }

    return FALSE;
}

BS_STATUS VNETS_MacLayer_Input (IN MBUF_S *pstMbuf)
{
    ETH_HEADER_S *pstEthHead;
    UINT uiDomainId;
    UINT uiSrcNodeID;

    if (BS_OK != MBUF_MakeContinue(pstMbuf, sizeof(ETH_HEADER_S)))
    {
        MBUF_Free(pstMbuf);
        RETURN(BS_BAD_PARA);
    }

    pstEthHead = MBUF_MTOD(pstMbuf);

    BS_DBG_OUTPUT(g_ulVnetSMacLayerDebugFlag, _VNETS_MAC_LAYER_DBG_PACKET,
        ("VNETS-MacLayer:Receive packet: destMAC:%pM, sourceMAC:%pM, Protocol:0x%04x\r\n",
        &pstEthHead->stDMac, &pstEthHead->stSMac,
        ntohs(pstEthHead->usProto)));

    
    if (TRUE != vnets_maclayer_IsPermit(&pstEthHead->stDMac))
    {
        MBUF_Free(pstMbuf);
        return BS_OK;
    }

    uiSrcNodeID = VNETS_Context_GetSrcNodeID(pstMbuf);
    if (0 == uiSrcNodeID)
    {
        MBUF_Free (pstMbuf);
        RETURN(BS_ERR);
    }

    uiDomainId = VNETS_NODE_GetDomainID(uiSrcNodeID);
    if (uiDomainId == 0)
    {
        MBUF_Free (pstMbuf);
        RETURN(BS_ERR);
    }

    vnets_maclayer_LearnMAC(uiDomainId, uiSrcNodeID, &pstEthHead->stSMac);

    return vnets_maclayer_DeliveUp(pstMbuf);
}

BS_STATUS VNETS_MacLayer_Output(IN UINT uiDomainID, IN MBUF_S *pstMbuf)
{
    ETH_HEADER_S *pstEthHead;
    UINT uiNodeID = 0;
    VNETS_MAC_NODE_S stMacNode;

    if (BS_OK != MBUF_MakeContinue(pstMbuf, sizeof(ETH_HEADER_S)))
    {
        MBUF_Free(pstMbuf);
        RETURN(BS_BAD_PARA);
    }

    pstEthHead = MBUF_MTOD(pstMbuf);

    BS_DBG_OUTPUT(g_ulVnetSMacLayerDebugFlag, _VNETS_MAC_LAYER_DBG_PACKET,
        ("VNETS-MacLayer:Send packet: destMAC:%pM, sourceMAC:%pM, Protocol:0x%04x\r\n",
        &pstEthHead->stDMac, &pstEthHead->stSMac,
        ntohs(pstEthHead->usProto)));

    if (! MAC_ADDR_IS_MULTICAST(pstEthHead->stDMac.aucMac))
    {
        stMacNode.stMacNode.stMac = pstEthHead->stDMac;
        if (BS_OK == VNETS_MACTBL_Find(uiDomainID, &stMacNode))
        {
            uiNodeID = stMacNode.stUserData.uiNodeID;
        }
    }

    return VNETS_NodeFwd_Output(uiNodeID, pstMbuf, VNET_NODE_PKT_PROTO_DATA);
}



PLUG_API VOID VNETS_MacLayer_DebugPacket(IN UINT ulArgc, IN CHAR **argv)
{
    g_ulVnetSMacLayerDebugFlag |= _VNETS_MAC_LAYER_DBG_PACKET;
}


PLUG_API VOID VNETS_MacLayer_NoDebugPacket(IN UINT ulArgc, IN CHAR **argv)
{
    g_ulVnetSMacLayerDebugFlag &= ~_VNETS_MAC_LAYER_DBG_PACKET;
}


VOID VNETS_MacLayer_NoDebugAll()
{
    g_ulVnetSMacLayerDebugFlag = 0;
}


