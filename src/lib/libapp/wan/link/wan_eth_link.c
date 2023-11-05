/******************************************************************************
* Copyright (C) Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2013-1-22
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/bit_opt.h"
#include "utl/sif_utl.h"
#include "utl/ip_utl.h"
#include "utl/eth_utl.h"
#include "utl/eth_mbuf.h"
#include "utl/msgque_utl.h"
#include "comp/comp_if.h"

#include "../h/wan_eth_link.h"
#include "../h/wan_ipfwd.h"
#include "../h/wan_arp.h"


#define _WAN_ETH_LINK_DBG_PACKET 0x1

static UINT g_uiWanEthLinkDebugFlag = 0;

static BS_STATUS wan_eth_GetDstMac(IN UINT ulIfIndex, IN MBUF_S *pstMbuf, OUT MAC_ADDR_S *pstMac)
{
    UINT uiNextHop;

    uiNextHop = MBUF_GET_NEXT_HOP(pstMbuf);
    if (0 == uiNextHop)
    {
        return (BS_ERR);
    }

    return WAN_ARP_GetMacByIp(ulIfIndex, uiNextHop, pstMbuf, pstMac);
}

BS_STATUS WAN_ETH_LinkInput (IN UINT ulIfIndex, IN MBUF_S *pstMbuf)
{
    ETH_HEADER_S * pstEthHeader;
    CHAR szIfName[IF_MAX_NAME_LEN + 1];
    USHORT usProto;
    BOOL_T is_l3 = FALSE;

    if (BS_OK != MBUF_MakeContinue (pstMbuf, sizeof(ETH_HEADER_S)))
    {
        MBUF_Free (pstMbuf);
        return (BS_ERR);
    }

    pstEthHeader = MBUF_MTOD (pstMbuf);

    BS_DBG_OUTPUT(g_uiWanEthLinkDebugFlag, _WAN_ETH_LINK_DBG_PACKET,
             ("EthLink: Receive packet from %s,DestMAC:%pM,SourceMAC:%pM.\r\n",
             IFNET_GetIfName(ulIfIndex, szIfName),
             &pstEthHeader->stDMac,
             &pstEthHeader->stSMac));

    MBUF_SET_SOURCEMAC(pstMbuf, pstEthHeader->stSMac.aucMac);
    usProto = pstEthHeader->usProto;


    
    IFNET_Ioctl(ulIfIndex, IFNET_CMD_IS_L3, &is_l3);

    if (is_l3) {
        MBUF_CutHead (pstMbuf, sizeof(ETH_HEADER_S));
        return IFNET_ProtoInput(ulIfIndex, pstMbuf, usProto);
    } else { 
        
    }

    return 0;
}

BS_STATUS WAN_ETH_LinkOutput (IN UINT ulIfIndex, IN MBUF_S *pstMbuf, IN USHORT usProtoType)
{
    ETH_HEADER_S * pstEthHeader;
    MAC_ADDR_S stDMacAddr;
    MAC_ADDR_S stSMacAddr;
    UCHAR *pcHostMac;
    USHORT usFlag;
    UCHAR *pucDstMac;
    BS_STATUS eRet;

    if (BS_OK != ETH_PadPacket(pstMbuf, FALSE))
    {
        MBUF_Free(pstMbuf);
        return BS_ERR;
    }

    if (BS_OK != MBUF_Prepend (pstMbuf, sizeof (ETH_HEADER_S)))
    {
        MBUF_Free (pstMbuf);
        return (BS_ERR);
    }

    if (BS_OK != MBUF_MakeContinue (pstMbuf, sizeof (ETH_HEADER_S)))
    {
        MBUF_Free (pstMbuf);
        return (BS_ERR);
    }

    pstEthHeader = MBUF_MTOD (pstMbuf);

    pstEthHeader->usProto = usProtoType;

    usFlag = MBUF_GET_ETH_MARKFLAG(pstMbuf);

    if (usFlag & MBUF_L2_FLAG_DST_MAC)
    {
        pucDstMac = MBUF_GET_DESTMAC(pstMbuf);
    }
    else 
    {
        eRet = wan_eth_GetDstMac(ulIfIndex, pstMbuf, &stDMacAddr);
        if (BS_PROCESSED == eRet)
        {
            BS_DBG_OUTPUT(g_uiWanEthLinkDebugFlag, _WAN_ETH_LINK_DBG_PACKET,
                 ("EthLink: waitting for arp.\r\n"));

            return BS_OK;
        }
        else if (BS_OK != eRet)
        {
            BS_DBG_OUTPUT(g_uiWanEthLinkDebugFlag, _WAN_ETH_LINK_DBG_PACKET,
                 ("EthLink: arp error.\r\n"));
            MBUF_Free(pstMbuf);
            return (BS_ERR);
        }

        pucDstMac = stDMacAddr.aucMac;
    }

    if (usFlag & MBUF_L2_FLAG_SRC_MAC)
    {
        pcHostMac = MBUF_GET_SOURCEMAC(pstMbuf);
    }
    else
    {
        IFNET_Ioctl(ulIfIndex, IFNET_CMD_GET_MAC, &stSMacAddr);
        pcHostMac = stSMacAddr.aucMac;
    }

    MAC_ADDR_COPY(pstEthHeader->stSMac.aucMac, pcHostMac);
    MAC_ADDR_COPY(pstEthHeader->stDMac.aucMac, pucDstMac);

    BS_DBG_OUTPUT(g_uiWanEthLinkDebugFlag, _WAN_ETH_LINK_DBG_PACKET,
         ("EthLink: Send packet to %pM.\r\n", pucDstMac));

    return IFNET_PhyOutput (ulIfIndex, pstMbuf);
}


PLUG_API VOID WAN_EthLink_DebugPacket
(
    IN UINT ulArgc,
    IN CHAR **argv
)
{
    BIT_SET(g_uiWanEthLinkDebugFlag, _WAN_ETH_LINK_DBG_PACKET);
}


PLUG_API VOID WAN_EthLink_NoDebugPacket
(
    IN UINT ulArgc,
    IN CHAR **argv
)
{
    BIT_CLR(g_uiWanEthLinkDebugFlag, _WAN_ETH_LINK_DBG_PACKET);
}


