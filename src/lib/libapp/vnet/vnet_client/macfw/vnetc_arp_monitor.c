/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2008-10-13
* Description: 监视过来的ARP报文,并记录IP和MAC的映射，以使pc机发送ARP请求时不需要广播.
*              被监视的表项在多次ARP请求探测后如果没有回应则删除.
* History:     
******************************************************************************/

#include "bs.h"

#include "utl/ipmac_tbl.h"
#include "utl/arp_utl.h"

#include "../inc/vnetc_ipmac.h"

static VOID vnetc_arpmonitor_DealArp(IN MBUF_S *pstMbuf)
{
    ETH_ARP_PACKET_S *pstArpHeader;

    if (BS_OK != MBUF_MakeContinue (pstMbuf, sizeof (ETH_ARP_PACKET_S)))
    {
        return;
    }

    pstArpHeader = MBUF_MTOD (pstMbuf);

    switch (ntohs(pstArpHeader->usOperation))
    {
        case ARP_REQUEST:
        case ARP_REPLY:
            VNETC_Ipmac_Add(pstArpHeader->ulSenderIp, (MAC_ADDR_S*)pstArpHeader->aucSenderHA);
            break;

        default:
            break;
    }

    return;
}

VOID VNETC_ArpMonitor_PacketMonitor(IN MBUF_S *pstMbuf)
{
    if (ARP_IsArpPacket(pstMbuf))
    {
        vnetc_arpmonitor_DealArp(pstMbuf);
    }
}

VOID VNETC_ArpMonitor_ProcArpRequest(IN UINT ulIfIndex, IN MBUF_S *pstMbuf)
{
	ETH_ARP_PACKET_S *pstArp;
    MAC_ADDR_S stMac;

    if (ARP_IsArpPacket(pstMbuf) == FALSE)
    {
        return;
    }

    if (BS_OK != MBUF_MakeContinue (pstMbuf, sizeof (ETH_ARP_PACKET_S)))
    {
        return;
    }

    pstArp = MBUF_MTOD (pstMbuf);
    if (pstArp->usOperation != htons(ARP_REQUEST))          
    {
        return;
    }

    if (pstArp->ulDstIp == pstArp->ulSenderIp)   
    {
        return;
    }

    if (NULL == VNETC_Ipmac_GetMacByIp(pstArp->ulDstIp, &stMac))
    {
        return;
    }

    MEM_Copy (pstArp->aucDMac, stMac.aucMac, 6);

    return;
}


