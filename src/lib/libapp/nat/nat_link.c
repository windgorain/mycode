/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2013-1-9
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/ip_utl.h"
#include "utl/eth_utl.h"
#include "utl/eth_mbuf.h"
#include "comp/comp_pcap.h"
#include "comp/comp_if.h"

#include "nat_main.h"
#include "nat_arp.h"

BS_STATUS NAT_Link_Input (IN UINT ulIfIndex, IN MBUF_S *pstMbuf)
{
    ETH_HEADER_S * pstEthHeader;
	USHORT usProto;

    if (BS_OK != MBUF_MakeContinue (pstMbuf, sizeof(ETH_HEADER_S)))
    {
        MBUF_Free (pstMbuf);
        return (BS_ERR);
    }

    pstEthHeader = MBUF_MTOD (pstMbuf);

    MBUF_SET_SOURCEMAC(pstMbuf, pstEthHeader->stSMac.aucMac);

    MBUF_CutHead (pstMbuf, sizeof(ETH_HEADER_S));

	usProto = ntohs(pstEthHeader->usProto);

	switch (usProto)
	{
		case ETH_P_IP:
		{
			NAT_Main_PktInput(pstMbuf);
			break;
		}

		case ETH_P_ARP:
		{
			NAT_ARP_PacketInput(pstMbuf);
			break;
		}

		default:
		{
			MBUF_Free(pstMbuf);
			break;
		}
	}

	return BS_OK;
}

static BS_STATUS nat_link_GetDstMac(IN UINT uiIfIndex, IN MBUF_S *pstMbuf, OUT MAC_ADDR_S *pstMac)
{
    UINT uiNextHop;

    uiNextHop = MBUF_GET_NEXT_HOP(pstMbuf);
    if (0 == uiNextHop)
    {
        return (BS_ERR);
    }

    return NAT_ARP_GetMacByIp(uiIfIndex, uiNextHop, pstMbuf, pstMac);
}

BS_STATUS NAT_Link_OutPut(IN UINT uiIfIndex, IN MBUF_S *pstMbuf, IN USHORT usProtoType)
{
    ETH_HEADER_S * pstEthHeader;
    USHORT usEthFlag;
    MAC_ADDR_S stDMacAddr;
    BS_STATUS eRet;
    MAC_ADDR_S stSMacAddr;
    UCHAR *pcHostMac;

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

    usEthFlag = MBUF_GET_ETH_MARKFLAG(pstMbuf);

    if (usEthFlag & MBUF_L2_FLAG_SRC_MAC)
    {
        pcHostMac = MBUF_GET_SOURCEMAC(pstMbuf);
    }
    else
    {
        IFNET_Ioctl(uiIfIndex, IFNET_CMD_GET_MAC, &stSMacAddr);
        pcHostMac = stSMacAddr.aucMac;
    }

    MAC_ADDR_COPY(pstEthHeader->stSMac.aucMac, pcHostMac);
    
    if (usEthFlag & MBUF_L2_FLAG_DST_MAC)
    {
        MAC_ADDR_COPY(pstEthHeader->stDMac.aucMac, MBUF_GET_DESTMAC(pstMbuf));
    }
    else
    {   
        eRet = nat_link_GetDstMac(uiIfIndex, pstMbuf, &stDMacAddr);
        if (BS_PROCESSED == eRet)
        {
            return BS_OK;
        }
        else if (BS_OK != eRet)
        {
            MBUF_Free(pstMbuf);
            return (BS_ERR);
        }

        MAC_ADDR_COPY(pstEthHeader->stDMac.aucMac, stDMacAddr.aucMac);
    }

    return IFNET_PhyOutput(uiIfIndex, pstMbuf);
}


