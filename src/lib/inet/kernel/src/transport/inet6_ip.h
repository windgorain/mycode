/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2012-11-26
* Description: 
* History:     
******************************************************************************/

#ifndef __INET6_IP_H_
#define __INET6_IP_H_

#ifdef __cplusplus
    extern "C" {
#endif 

struct ip6_hdr {
    union {
        struct ip6_hdrctl {
            UINT ip6_un1_flow;    
            USHORT ip6_un1_plen;    
            UCHAR ip6_un1_nxt;    
            UCHAR ip6_un1_hlim;    
        } ip6_un1;
        UCHAR ip6_un2_vfc;    
    } ip6_ctlun;
    struct in6_addr ip6_src;    
    struct in6_addr ip6_dst;    
} ;

void ip6_savecontrol(IN INPCB_S *in6p, IN MBUF_S *m, OUT MBUF_S **mp);
void in6_sin_2_v4mapsin6(IN SOCKADDR_IN_S *sin, OUT SOCKADDR_IN6_S *sin6);


#ifdef __cplusplus
    }
#endif 

#endif 


