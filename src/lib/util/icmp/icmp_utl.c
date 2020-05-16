
#include "bs.h"

#include "utl/ip_utl.h"
#include "utl/eth_utl.h"
#include "utl/icmp_utl.h"
#include "utl/in_checksum.h"

/* 返回网络序的校验和 */
USHORT ICMP_CheckSum (IN UCHAR *pucBuf, IN UINT uiLen)
{
    return IN_CHKSUM_CheckSum(pucBuf, uiLen);
}

ICMP_HEAD_S * ICMP_GetIcmpHeader(IN VOID *pucData, IN UINT uiDataLen, IN NET_PKT_TYPE_E enPktType)
{
    IP46_HEAD_S pstIpHeader ={0};
    ICMP_HEAD_S *pstIcmpHeader = NULL;
    NET_PKT_TYPE_E enPktTypeTmp = enPktType;
    UINT uiHeadLen = 0;

    if (enPktTypeTmp != NET_PKT_TYPE_ICMP) {
        if(0 != IP46_GetIPHeader(&pstIpHeader, pucData, uiDataLen, enPktType)) {
            return NULL;
        }

        if (pstIpHeader.family == ETH_P_IP) {
            if ((pstIpHeader.iph.ip4->ucProto != IP_PROTO_ICMP)) {
                /*not icmp packet or packet is a fragment one*/
                return NULL;
            }
            uiHeadLen = ((UCHAR*)(pstIpHeader.iph.ip4) - (UCHAR*)pucData) + IP_HEAD_LEN(pstIpHeader.iph.ip4);
        }else if (pstIpHeader.family == ETH_P_IP6) {
            if (pstIpHeader.iph.ip6->next != IP_PROTO_UDP) {
                return NULL;
            }
            uiHeadLen = (((UCHAR*)pstIpHeader.iph.ip6) - (UCHAR*)pucData) + IP6_HDR_LEN;
        }

        enPktTypeTmp = NET_PKT_TYPE_ICMP;
    }

    if (enPktTypeTmp == NET_PKT_TYPE_ICMP) {
        /* 长度不够ICMP头的长度则返回NULL */
        if (uiHeadLen + sizeof(ICMP_HEAD_S) > uiDataLen) {
            return NULL;
        }

        pstIcmpHeader = (ICMP_HEAD_S*)((UCHAR*)pucData + uiHeadLen);
    }

    return pstIcmpHeader;
}

ICMP_ECHO_HEAD_S * ICMP_GetEchoHeader(void *data, UINT datalen, NET_PKT_TYPE_E enPktType)
{
    UINT icmp_len;
    ICMP_HEAD_S *icmphdr = ICMP_GetIcmpHeader(data, datalen, enPktType);
    if (icmphdr == NULL) {
        return NULL;
    }

    if ((icmphdr->ucType != ICMP_TYPE_ECHO_REQUEST)
            && (icmphdr->ucType != ICMP_TYPE_ECHO_REPLY)) {
        return NULL;
    }

    icmp_len = datalen - ((char*)icmphdr - (char*)data);
    if (icmp_len < sizeof(ICMP_ECHO_HEAD_S)) {
        return NULL;
    }

    return (void*)icmphdr;
}

