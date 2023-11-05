/******************************************************************************
* Copyright (C) Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2012-11-25
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/mbuf_utl.h"
#include "utl/ip_utl.h"
#include "utl/in_checksum.h"
#include "inet/in_pub.h"

#include "protosw.h"
#include "domain.h"
#include "in.h"
#include "in_ipaddr.h"
#include "udp_func.h"
#include "ip_options.h"
#include "uipc_func.h"
#include "uipc_compat.h"

#include "../in_pcb.h"
#include "../in_mcast.h"
#include "../icmp.h"
#include "../inet_ip.h"
#include "../inet6_ip.h"


UINT imo_match_group(IN IP_MOPTIONS_S *imo, IN UINT ifindex, IN SOCKADDR_S *group)
{
    return (UINT)(-1);
}


IN_MSOURCE_S * imo_match_source(IN IP_MOPTIONS_S *imo, IN UINT gidx, IN SOCKADDR_S *src)
{
    return NULL;
}


void inp_freemoptions(IN IP_MOPTIONS_S *imo)
{
    return;
}


int inp_setmoptions(IN INPCB_S *inp, IN SOCKOPT_S *sopt)
{
    return 0;
}


int inp_getmoptions(struct inpcb *inp, struct sockopt *sopt)
{
    return ENOTSOCK;
}



