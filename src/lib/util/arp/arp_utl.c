/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2008-9-30
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/eth_utl.h"
#include "utl/eth_mbuf.h"
#include "utl/arp_utl.h"

BOOL_T ARP_IsArpPacket(IN MBUF_S *pstMbuf)
{
    ETH_HEADER_S *pstEthHead;

    if (BS_OK != MBUF_MakeContinue (pstMbuf, sizeof (ETH_HEADER_S)))
    {
        return FALSE;
    }

    pstEthHead = MBUF_MTOD (pstMbuf);

    if (pstEthHead->usProto != htons(ETH_P_ARP))
    {
        return FALSE;
    }

    return TRUE;
}

MBUF_S * ARP_BuildPacket
(
    IN UINT uiSrcIp,  /* 网络序 */
    IN UINT uiDstIp, /* 网络序 */
    IN UCHAR *pucSrcMac,
    IN UCHAR *pucDstMac,
    IN USHORT usArpType /* 主机序 */
)
{
    ARP_HEADER_S *pstArpHeader;
    MBUF_S *pstMbuf;
    static UCHAR aucBroadcastMac[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

    pstMbuf = MBUF_Create(MBUF_DATA_DATA, 200 + sizeof(ARP_HEADER_S));
    if (NULL == pstMbuf)
    {
        return NULL;
    }

    MBUF_Prepend(pstMbuf, sizeof(ARP_HEADER_S));
    MBUF_MakeContinue(pstMbuf, sizeof(ARP_HEADER_S));
    pstArpHeader = MBUF_MTOD(pstMbuf);

    Mem_Zero(pstArpHeader, sizeof(ARP_HEADER_S));
    
    pstArpHeader->usHardWareType = htons(ARP_HARDWARE_TYPE_ETH);
    pstArpHeader->usProtoType = htons(ETH_P_IP);
    pstArpHeader->ucHardAddrLen = 6;
    pstArpHeader->ucProtoAddrLen = 4;
    pstArpHeader->usOperation = htons(usArpType);
    MEM_Copy(pstArpHeader->aucSenderHA, pucSrcMac, 6);
    pstArpHeader->ulSenderIp = uiSrcIp;
    pstArpHeader->ulDstIp = uiDstIp;

    if (pucDstMac != NULL)
    {
        MAC_ADDR_COPY(pstArpHeader->aucDstHA, pucDstMac);
        MBUF_SET_DESTMAC(pstMbuf, pucDstMac);
    }
    else
    {
        MBUF_SET_DESTMAC(pstMbuf, aucBroadcastMac);
    }
    MBUF_SET_ETH_MARKFLAG(pstMbuf, MBUF_L2_FLAG_DST_MAC);

    MBUF_SET_SOURCEMAC(pstMbuf, pucSrcMac);
    MBUF_SET_ETH_MARKFLAG(pstMbuf, MBUF_L2_FLAG_SRC_MAC);

    MBUF_SET_ETH_L2TYPE(pstMbuf, ETHERTYPE_ARP);

    return pstMbuf;
}

MBUF_S * ARP_BuildPacketWithEthHeader
(
    IN UINT uiSrcIp,  /* 网络序 */
    IN UINT uiDstIp, /* 网络序 */
    IN UCHAR *pucSrcMac,
    IN UCHAR *pucDstMac,
    IN USHORT usArpType /* 主机序 */
)
{
    MBUF_S *pstMbuf;
    ETH_HEADER_S * pstEthHeader;

    pstMbuf = ARP_BuildPacket(uiSrcIp, uiDstIp, pucSrcMac, pucDstMac, usArpType);
    if (NULL == pstMbuf)
    {
        return NULL;
    }

    if (BS_OK != ETH_PadPacket(pstMbuf, FALSE))
    {
        MBUF_Free(pstMbuf);
        return NULL;
    }

    if (BS_OK != MBUF_Prepend (pstMbuf, sizeof (ETH_HEADER_S)))
    {
        MBUF_Free (pstMbuf);
        return NULL;
    }

    if (BS_OK != MBUF_MakeContinue (pstMbuf, sizeof (ETH_HEADER_S)))
    {
        MBUF_Free (pstMbuf);
        return NULL;
    }

    pstEthHeader = MBUF_MTOD (pstMbuf);

    pstEthHeader->usProto = htons(ETH_P_ARP);

    MAC_ADDR_COPY(pstEthHeader->stSMac.aucMac, pucSrcMac);
    MAC_ADDR_COPY(pstEthHeader->stDMac.aucMac, MBUF_GET_DESTMAC(pstMbuf));

    return pstMbuf;
}


