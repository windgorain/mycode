/*================================================================
* Author：LiXingang. Data: 2019.07.30
* Description: 
*
================================================================*/
#include "bs.h"

#include "utl/ip_utl.h"
#include "utl/ip_mbuf.h"
#include "utl/icmp_utl.h"
#include "utl/in_checksum.h"

ICMP_HEAD_S * ICMP_GetIcmpHeaderByMbuf(IN MBUF_S *pstMbuf,
        IN NET_PKT_TYPE_E enPktType)
{
    IP_HEAD_S *pstIpHeader;
    ICMP_HEAD_S *pstIcmpHeader = NULL;
    NET_PKT_TYPE_E enPktTypeTmp = enPktType;
    UINT uiHeadLen = 0;
    UCHAR *pucData;

    if (enPktTypeTmp != NET_PKT_TYPE_ICMP)
    {
        pstIpHeader = IP_GetIPHeaderByMbuf(pstMbuf, enPktType);
        if (NULL == pstIpHeader)
        {
            return NULL;
        }

        if (pstIpHeader->ucProto != IP_PROTO_ICMP)
        {
            return NULL;
        }

        enPktTypeTmp = IP_PROTO_ICMP;
        uiHeadLen = (((UCHAR*)pstIpHeader) - (UCHAR*)MBUF_MTOD(pstMbuf)) +
            IP_HEAD_LEN(pstIpHeader);
    }

    if (enPktTypeTmp == IP_PROTO_ICMP)
    {
        if (BS_OK !=
                MBUF_MakeContinue(pstMbuf, uiHeadLen + sizeof(ICMP_HEAD_S)))
        {
            return NULL;
        }

        pucData = MBUF_MTOD(pstMbuf);
        pstIcmpHeader = (ICMP_HEAD_S*)(pucData + uiHeadLen);
    }

    return pstIcmpHeader;
}

