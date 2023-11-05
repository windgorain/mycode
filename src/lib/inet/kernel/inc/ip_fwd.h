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
#endif 



#define IPOPT_OPTVAL        0       
#define IPOPT_OLEN          1       
#define IPOPT_OFFSET        2       
#define IPOPT_MINOFF        4       
#define IPOPT_MAXOFF        40      

#define MAX_IPOPTLEN        40

#define IPOPT_EOL           0       
#define IPOPT_NOP           1       

#define IPOPT_RR            7       
#define IPOPT_TS            68      
#define IPOPT_SECURITY      130     
#define IPOPT_LSRR          131     
#define IPOPT_SATID         136     
#define IPOPT_SSRR          137     
#define IPOPT_ROUTE_ALERT   148     


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


#define IP_FORWARDING                ( 1 << IP_OUTPUTBIT_PAD0 ) 
#define IP_RAWOUTPUT                 ( 1 << IP_OUTPUTBIT_PAD1 ) 
#define IP_SENDONES                  ( 1 << IP_OUTPUTBIT_PAD2 ) 
#define IP_SENDBY_LSPV               ( 1 << IP_OUTPUTBIT_LSPV ) 
#define IP_SENDDATAIF                ( 1 << IP_OUTPUTBIT_SENDDATAIF ) 
#define IP_ROUTETOIF                 ( 1 << IP_OUTPUTBIT_ROUTETOIF ) 
#define IP_ALLOWBROADCAST            ( 1 << IP_OUTPUTBIT_PAD5 ) 
#define IP_NATPT_NEXTHOP             ( 1 << IP_OUTPUTBIT_NATPT ) 
#define IP_OUTPUTFLAG_MASK           ( IP_SENDBY_LSPV | \
                                       IP_SENDDATAIF | \
                                       IP_ROUTETOIF | \
                                       IP_NATPT_NEXTHOP ) 


UCHAR IP_GetDefTTL();


#ifdef __cplusplus
    }
#endif 

#endif 


