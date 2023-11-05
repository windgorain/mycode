/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/net.h"
#include "utl/eth_utl.h"
#include "utl/ip_utl.h"
#include "utl/udp_utl.h"
#include "utl/data2hex_utl.h"
#include "utl/in_checksum.h"


USHORT UDP_CheckSum
(
    IN UCHAR *pucBuf,
	IN UINT ulLen,
	IN UCHAR *pucSrcIp, 
	IN UCHAR *pucDstIp  
)
{
    USHORT usCheckSum = 0;

    usCheckSum = IN_CHKSUM_AddRaw(usCheckSum, pucBuf, ulLen);
    usCheckSum = IN_CHKSUM_AddRaw(usCheckSum, pucSrcIp, sizeof(UINT));
    usCheckSum = IN_CHKSUM_AddRaw(usCheckSum, pucDstIp, sizeof(UINT));
    usCheckSum = IN_CHKSUM_AddRawWord(usCheckSum, IP_PROTO_UDP);
    usCheckSum = IN_CHKSUM_AddRawWord(usCheckSum, ulLen);

    usCheckSum = IN_CHKSUM_Wrap(usCheckSum);

    if (usCheckSum == 0)
    {
        usCheckSum = 0xffff;
    }

    return usCheckSum;
}

UDP_HEAD_S * UDP_GetUDPHeader(IN VOID *pucData, IN UINT uiDataLen, IN NET_PKT_TYPE_E enPktType)
{
    IP46_HEAD_S  pstIpHeader = {0};
    UDP_HEAD_S *pstUdpHeader = NULL;
    NET_PKT_TYPE_E enPktTypeTmp = enPktType;
    UINT uiHeadLen = 0;

    if (enPktTypeTmp != NET_PKT_TYPE_UDP) {
        if(0 != IP46_GetIPHeader(&pstIpHeader, pucData, uiDataLen, enPktType)) {
            return NULL;
        }

        if (pstIpHeader.family == ETH_P_IP) {
            if (pstIpHeader.iph.ip4->ucProto != IP_PROTO_UDP)
            {
                return NULL;
            }
            uiHeadLen = (((UCHAR*)pstIpHeader.iph.ip4) -(UCHAR*) pucData) + IP_HEAD_LEN(pstIpHeader.iph.ip4);
        }else if (pstIpHeader.family == ETH_P_IP6) {
            if (pstIpHeader.iph.ip6->next != IP_PROTO_UDP)
            {
                return NULL;
            }
            uiHeadLen = (((UCHAR*)pstIpHeader.iph.ip6) - (UCHAR*)pucData) + IP6_HDR_LEN;
        }

        enPktTypeTmp = NET_PKT_TYPE_UDP;
    }

    if (enPktTypeTmp == NET_PKT_TYPE_UDP)
    {
        
        if (uiHeadLen + sizeof(UDP_HEAD_S) > uiDataLen)
        {
            return NULL;
        }

        pstUdpHeader = (UDP_HEAD_S*)(pucData + uiHeadLen);
    }

    return pstUdpHeader;
}

CHAR * UDP_Header2String(IN VOID *udp, OUT CHAR *info, IN UINT infosize)
{
    UDP_HEAD_S *udph = udp;

    snprintf(info, infosize,
            "\"sport\":%u,\"dport\":%u,"
            "\"udp_total_len\":%u,\"udp_chksum\":\"%02x\"",
            ntohs(udph->usSrcPort), ntohs(udph->usDstPort),
            ntohs(udph->usDataLength), ntohs(udph->usCrc));

    return info;
}

CHAR * UDP_Header2Hex(IN VOID *udp, OUT CHAR *hex)
{
    DH_Data2HexString(udp, 8, hex);

    return hex;
}

