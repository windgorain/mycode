/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2012-11-24
* Description: 
* History:     
******************************************************************************/

#ifndef __ICMP_DEF_H_
#define __ICMP_DEF_H_

#ifdef __cplusplus
    extern "C" {
#endif 

#define BANDLIM_UNLIMITED -1
#define BANDLIM_ICMP_UNREACH 0
#define BANDLIM_ICMP_ECHO 1
#define BANDLIM_ICMP_TSTAMP 2
#define BANDLIM_RST_CLOSEDPORT 3 
#define BANDLIM_RST_OPENPORT 4   
#define BANDLIM_ICMP6_UNREACH 5
#define BANDLIM_MAX 5
int badport_bandlim(int);



#define    ICMP_ECHOREPLY        0        
#define    ICMP_UNREACH        3        
#define        ICMP_UNREACH_NET    0        
#define        ICMP_UNREACH_HOST    1        
#define        ICMP_UNREACH_PROTOCOL    2        
#define        ICMP_UNREACH_PORT    3        
#define        ICMP_UNREACH_NEEDFRAG    4        
#define        ICMP_UNREACH_SRCFAIL    5        
#define        ICMP_UNREACH_NET_UNKNOWN 6        
#define        ICMP_UNREACH_HOST_UNKNOWN 7        
#define        ICMP_UNREACH_ISOLATED    8        
#define        ICMP_UNREACH_NET_PROHIB    9        
#define        ICMP_UNREACH_HOST_PROHIB 10        
#define        ICMP_UNREACH_TOSNET    11        
#define        ICMP_UNREACH_TOSHOST    12        
#define        ICMP_UNREACH_FILTER_PROHIB 13        
#define        ICMP_UNREACH_HOST_PRECEDENCE 14        
#define        ICMP_UNREACH_PRECEDENCE_CUTOFF 15    
#define    ICMP_SOURCEQUENCH    4        
#define    ICMP_REDIRECT        5        
#define        ICMP_REDIRECT_NET    0        
#define        ICMP_REDIRECT_HOST    1        
#define        ICMP_REDIRECT_TOSNET    2        
#define        ICMP_REDIRECT_TOSHOST    3        
#define    ICMP_ALTHOSTADDR    6        
#define    ICMP_ECHO        8        
#define    ICMP_ROUTERADVERT    9        
#define        ICMP_ROUTERADVERT_NORMAL        0    
#define        ICMP_ROUTERADVERT_NOROUTE_COMMON    16    
#define    ICMP_ROUTERSOLICIT    10        
#define    ICMP_TIMXCEED        11        
#define        ICMP_TIMXCEED_INTRANS    0        
#define        ICMP_TIMXCEED_REASS    1        
#define    ICMP_PARAMPROB        12        
#define        ICMP_PARAMPROB_ERRATPTR 0        
#define        ICMP_PARAMPROB_OPTABSENT 1        
#define        ICMP_PARAMPROB_LENGTH 2            
#define    ICMP_TSTAMP        13        
#define    ICMP_TSTAMPREPLY    14        
#define    ICMP_IREQ        15        
#define    ICMP_IREQREPLY        16        
#define    ICMP_MASKREQ        17        
#define    ICMP_MASKREPLY        18        
#define    ICMP_TRACEROUTE        30        
#define    ICMP_DATACONVERR    31        
#define    ICMP_MOBILE_REDIRECT    32        
#define    ICMP_IPV6_WHEREAREYOU    33        
#define    ICMP_IPV6_IAMHERE    34        
#define    ICMP_MOBILE_REGREQUEST    35        
#define    ICMP_MOBILE_REGREPLY    36        
#define    ICMP_SKIP        39        
#define    ICMP_PHOTURIS        40        

void icmp_error(MBUF_S *n, int type, int code, UINT dest, int mtu);


#ifdef __cplusplus
    }
#endif 

#endif 

