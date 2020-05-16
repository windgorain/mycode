/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2013-3-29
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/mbuf_utl.h"
#include "utl/ip_utl.h"
#include "utl/ipfwd_service.h"

#include "../h/wan_udp_service.h"
#include "../h/wan_icmp.h"
#include "../h/wan_deliver_up.h"
#include "../h/wan_ipfwd_service.h"


BS_STATUS WAN_DeliverUp(IN IP_HEAD_S *pstIpHead, IN MBUF_S *pstMbuf)
{
    IPFWD_SERVICE_RET_E eServiceRet;

    eServiceRet = WAN_IpFwdService_Process(IPFWD_SERVICE_BEFORE_DELIVER_UP, pstIpHead, pstMbuf);
    if (eServiceRet == IPFWD_SERVICE_RET_TAKE_OVER)
    {
        return BS_OK;
    }

    switch(pstIpHead->ucProto)
    {
        case IP_PROTO_UDP:
        {
            WanUdpService_Input(pstMbuf);
            break;
        }

        case IP_PROTO_ICMP:
        {
            WAN_ICMP_Input(pstMbuf);
            break;
        }

        default:
        {
            MBUF_Free (pstMbuf);
            break;
        }
    }

    return BS_OK;
}

