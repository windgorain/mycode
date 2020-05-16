/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"

#include "utl/ip_utl.h"
#include "utl/ip_mbuf.h"
#include "utl/data2hex_utl.h"
#include "utl/tcp_utl.h"
#include "utl/tcp_mbuf.h"
#include "utl/in_checksum.h"

TCP_HEAD_S * TCP_GetTcpHeaderByMbuf(IN MBUF_S *pstMbuf, IN NET_PKT_TYPE_E enPktType)
{
    IP_HEAD_S *pstIpHeader;
    TCP_HEAD_S *pstTcpHeader = NULL;
    NET_PKT_TYPE_E enPktTypeTmp = enPktType;
    UINT uiHeadLen = 0;
    UCHAR *pucData;

    if (enPktTypeTmp != NET_PKT_TYPE_TCP)
    {
        pstIpHeader = IP_GetIPHeaderByMbuf(pstMbuf, enPktType);
        if (NULL == pstIpHeader)
        {
            return NULL;
        }

        if (pstIpHeader->ucProto != IP_PROTO_TCP)
        {
            return NULL;
        }

        enPktTypeTmp = NET_PKT_TYPE_TCP;
        uiHeadLen = (((UCHAR*)pstIpHeader) - (UCHAR*)MBUF_MTOD(pstMbuf)) + IP_HEAD_LEN(pstIpHeader);
    }

    if (enPktTypeTmp == NET_PKT_TYPE_TCP)
    {
        if (BS_OK != MBUF_MakeContinue(pstMbuf, uiHeadLen + sizeof(TCP_HEAD_S)))
        {
            return NULL;
        }

        pucData = MBUF_MTOD(pstMbuf);
        pstTcpHeader = (TCP_HEAD_S*)(pucData + uiHeadLen);
    }

    return pstTcpHeader;
}


