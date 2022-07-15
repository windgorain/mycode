/******************************************************************************
* Copyright (C),    LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2012-11-26
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
#include "udp_func.h"
#include "ip_options.h"

#include "in_pcb.h"
#include "in_mcast.h"
#include "icmp.h"
#include "inet_ip.h"
#include "inet6_ip.h"

void ip6_savecontrol(IN INPCB_S *in6p, IN MBUF_S *m, OUT MBUF_S **mp)
{
    return;
}

void in6_sin_2_v4mapsin6(IN SOCKADDR_IN_S *sin, OUT SOCKADDR_IN6_S *sin6)
{
    Mem_Zero(sin6, sizeof(*sin6));
    sin6->sin6_len = sizeof(struct sockaddr_in6);
    sin6->sin6_family = AF_INET6;
    sin6->sin6_port = sin->sin_port;
    sin6->sin6_addr.si6_addr32[0] = 0;
    sin6->sin6_addr.si6_addr32[1] = 0;
    sin6->sin6_addr.si6_addr32[2] = htonl(0x0000ffff);
    sin6->sin6_addr.si6_addr32[3] = sin->sin_addr;
    sin6->sin_vrf = sin->sin_vrf;
}


