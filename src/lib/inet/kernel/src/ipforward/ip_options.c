/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2012-11-25
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/mbuf_utl.h"
#include "utl/ip_utl.h"
#include "utl/udp_utl.h"

#include "ip_options.h"

VOID IP_SaveSrcOption ( IN MBUF_S *pstMBuf, OUT UCHAR *ucOldIPHeader )
{
    IP_HEAD_S *pstIp;
    UINT uiIpHeadLen;

    if ((NULL == pstMBuf) || (NULL == ucOldIPHeader))
    {
        return;
    }
    
    pstIp = MBUF_MTOD(pstMBuf);

    uiIpHeadLen = (UINT)pstIp->ucHLen << 2;
    if (uiIpHeadLen > sizeof(IP_HEAD_S))
    {
        /* 源路由选项报文 */
        if (0 != (MBUF_GET_IP_PKTTYPE(pstMBuf) & IP_PKT_SRCROUTE))
        {    
            (VOID) memcpy(ucOldIPHeader, (void *) pstIp, uiIpHeadLen);
        }
    }

    return;
}

VOID IP_StrIpOptions ( INOUT MBUF_S *pstMBuf )
{
    IP_HEAD_S *pstIp;
    UINT uiOptLen;

    if (NULL != pstMBuf)
    {
        pstIp = MBUF_MTOD(pstMBuf);
        uiOptLen = ((UINT) pstIp->ucHLen << 2) - (UINT) sizeof(IP_HEAD_S);

        (VOID) MBUF_CutPart(pstMBuf, (UINT)sizeof(IP_HEAD_S), uiOptLen);

        pstIp = MBUF_MTOD(pstMBuf);
        pstIp->ucHLen = sizeof(IP_HEAD_S) >> 2;
    }

    return;
}
