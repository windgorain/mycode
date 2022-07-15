/******************************************************************************
* Copyright (C) Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2012-12-16
* Description: 
* History:     
******************************************************************************/

#ifndef __IP_FWD_H_
#define __IP_FWD_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

/* 
 * IP Options Macro
 */

#define IPOPT_OPTVAL        0       /* option ID */
#define IPOPT_OLEN          1       /* option length */
#define IPOPT_OFFSET        2       /* offset within option */
#define IPOPT_MINOFF        4       /* min value of above */
#define IPOPT_MAXOFF        40      /* max value of above */

#define MAX_IPOPTLEN        40

#define IPOPT_EOL           0       /* end of option list */
#define IPOPT_NOP           1       /* no operation */

#define IPOPT_RR            7       /* record packet route */
#define IPOPT_TS            68      /* timestamp */
#define IPOPT_SECURITY      130     /* provide s,c,h,tcc */
#define IPOPT_LSRR          131     /* loose source route */
#define IPOPT_SATID         136     /* satnet id */
#define IPOPT_SSRR          137     /* strict source route */
#define IPOPT_ROUTE_ALERT   148     /* route alert, if value is 0, then delivertoup */

/* ip_output flag displacement bits */
typedef enum tagIP_OutputBit
{
    IP_OUTPUTBIT_PAD0, 
    IP_OUTPUTBIT_PAD1, 
    IP_OUTPUTBIT_PAD2, 
    IP_OUTPUTBIT_LSPV, 
    IP_OUTPUTBIT_SENDDATAIF, 
    IP_OUTPUTBIT_ROUTETOIF, 
    IP_OUTPUTBIT_PAD5, 
    IP_OUTPUTBIT_NATPT, 
    IP_OUTPUTBIT_MAX
} IP_OUTPUT_BIT;

/* flags passed to ip_output as last parameter */
#define IP_FORWARDING                ( 1 << IP_OUTPUTBIT_PAD0 ) /* 0x1, most of ip header exists */
#define IP_RAWOUTPUT                 ( 1 << IP_OUTPUTBIT_PAD1 ) /* 0x2, raw ip header exists */
#define IP_SENDONES                  ( 1 << IP_OUTPUTBIT_PAD2 ) /* 0x4, send all-ones broadcast, not used */
#define IP_SENDBY_LSPV               ( 1 << IP_OUTPUTBIT_LSPV ) /* 0x8, mpls ping */
#define IP_SENDDATAIF                ( 1 << IP_OUTPUTBIT_SENDDATAIF ) /* 0x10, special output interface and/or nexthop */
#define IP_ROUTETOIF                 ( 1 << IP_OUTPUTBIT_ROUTETOIF ) /* 0x20, SO_DONTROUTE bypass routing tables */
#define IP_ALLOWBROADCAST            ( 1 << IP_OUTPUTBIT_PAD5 ) /* 0x40, SO_BROADCAST can send broadcast packets */
#define IP_NATPT_NEXTHOP             ( 1 << IP_OUTPUTBIT_NATPT ) /* 0x80, NATPT */
#define IP_OUTPUTFLAG_MASK           ( IP_SENDBY_LSPV | \
                                       IP_SENDDATAIF | \
                                       IP_ROUTETOIF | \
                                       IP_NATPT_NEXTHOP ) /* mask for separating flow */


UCHAR IP_GetDefTTL();


#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__IP_FWD_H_*/


