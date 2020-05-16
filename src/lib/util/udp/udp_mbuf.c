/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"

#include "utl/ip_utl.h"
#include "utl/ip_mbuf.h"
#include "utl/udp_utl.h"
#include "utl/data2hex_utl.h"
#include "utl/in_checksum.h"

UDP_HEAD_S * UDP_GetUDPHeaderByMbuf(IN MBUF_S *pstMbuf, IN NET_PKT_TYPE_E enPktType)
{
    IP_HEAD_S *pstIpHeader;
    UDP_HEAD_S *pstUdpHeader = NULL;
    NET_PKT_TYPE_E enPktTypeTmp = enPktType;
    UINT uiHeadLen = 0;
    UCHAR *pucData;

    if (enPktTypeTmp != NET_PKT_TYPE_UDP)
    {
        pstIpHeader = IP_GetIPHeaderByMbuf(pstMbuf, enPktType);
        if (NULL == pstIpHeader)
        {
            return NULL;
        }

        if (pstIpHeader->ucProto != IP_PROTO_UDP)
        {
            return NULL;
        }

        enPktTypeTmp = NET_PKT_TYPE_UDP;
        uiHeadLen = (((UCHAR*)pstIpHeader) - (UCHAR*)MBUF_MTOD(pstMbuf)) + IP_HEAD_LEN(pstIpHeader);
    }

    if (enPktTypeTmp == NET_PKT_TYPE_UDP)
    {
        if (BS_OK != MBUF_MakeContinue(pstMbuf, uiHeadLen + sizeof(UDP_HEAD_S)))
        {
            return NULL;
        }

        pucData = MBUF_MTOD(pstMbuf);
        pstUdpHeader = (UDP_HEAD_S*)(pucData + uiHeadLen);
    }

    return pstUdpHeader;
}

/* 给MBUF加上UDP头 */
BS_STATUS UDP_BuilderHeader
(
    IN MBUF_S *pstMbuf,
    IN USHORT usSrcPort/* 网络序 */,
    IN USHORT usDstPort/* 网络序 */,
    IN UINT uiSrcIp/* 网络序 */,
    IN UINT uiDstIp/* 网络序 */
)
{
    UDP_HEAD_S *pstUdpHeader;
    USHORT usDataLen;
    
    if (BS_OK != MBUF_Prepend(pstMbuf, sizeof(UDP_HEAD_S)))
    {
        return BS_NO_MEMORY;
    }

    if (BS_OK != MBUF_MakeContinue(pstMbuf, MBUF_TOTAL_DATA_LEN(pstMbuf)))
    {
        return BS_NO_MEMORY;
    }

    pstUdpHeader = MBUF_MTOD(pstMbuf);
    pstUdpHeader->usDstPort = usDstPort;
    pstUdpHeader->usSrcPort = usSrcPort;
    usDataLen = MBUF_TOTAL_DATA_LEN(pstMbuf);
    pstUdpHeader->usDataLength = htons(usDataLen);
    pstUdpHeader->usCrc = 0;
    pstUdpHeader->usCrc = UDP_CheckSum((UCHAR*)pstUdpHeader, usDataLen, (UCHAR*)&uiSrcIp, (UCHAR*)&uiDstIp);

    return BS_OK;
}

