/******************************************************************************
* Copyright (C), 2000-2006,  LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2017-3-28
* Description: ICMP请求的应答处理
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/ip_utl.h"
#include "utl/ip_mbuf.h"
#include "utl/icmp_utl.h"
#include "utl/icmp_mbuf.h"
#include "utl/in_checksum.h"

#include "../h/wan_ipfwd.h"

BS_STATUS WAN_ICMP_Input(IN MBUF_S *pstMbuf)
{
    ICMP_HEAD_S *pstIcmpHeader;
    IP_HEAD_S *pstIpHeader;
    UINT uiPrevLen;  
    USHORT usRawSum;
    UINT uiDstIp;
    UINT uiSrcIp;

    pstIpHeader = IP_GetIPHeaderByMbuf(pstMbuf, NET_PKT_TYPE_IP);
    if (NULL == pstIpHeader)
    {
        MBUF_Free(pstMbuf);
        return BS_ERR;
    }

    uiDstIp = pstIpHeader->unSrcIp.uiIp;
    uiSrcIp = pstIpHeader->unDstIp.uiIp;
    
    pstIcmpHeader = ICMP_GetIcmpHeaderByMbuf(pstMbuf, NET_PKT_TYPE_IP);
    if (NULL == pstIcmpHeader)
    {
        MBUF_Free(pstMbuf);
        return BS_ERR;
    }

    if (pstIcmpHeader->ucType != ICMP_TYPE_ECHO_REQUEST)
    {
        MBUF_Free(pstMbuf);
        return BS_OK;
    }

    uiPrevLen = (UCHAR*)pstIcmpHeader - (UCHAR*)MBUF_MTOD(pstMbuf);

    MBUF_CutHead(pstMbuf, uiPrevLen);

    usRawSum = IN_CHKSUM_UnWrap(pstIcmpHeader->usCheckSum);
    usRawSum = IN_CHKSUM_DelRaw(usRawSum, &pstIcmpHeader->ucType, 1);

    pstIcmpHeader->ucType = 0;

    usRawSum = IN_CHKSUM_AddRaw(usRawSum, &pstIcmpHeader->ucType, 1);
    
    pstIcmpHeader->usCheckSum = IN_CHKSUM_Wrap(usRawSum);

    return WAN_IpFwd_Output(pstMbuf, uiDstIp, uiSrcIp, IPPROTO_ICMP);
}

