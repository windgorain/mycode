/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2012-11-15
* Description: 
* History:     
******************************************************************************/

#ifndef __IN_H_
#define __IN_H_

#include "uipc_socket.h"

#ifdef __cplusplus
    extern "C" {
#endif 

#define IPPROTO_IP          0
#define IPPROTO_ICMP        1
#define IPPROTO_TCP         6
#define IPPROTO_UDP         17

#define IPPROTO_HOPOPTS     0       
#define IPPROTO_IGMP        2       
#define IPPROTO_GGP         3       
#define IPPROTO_IPV4        4       
#define IPPROTO_IPIP        IPPROTO_IPV4    
#define IPPROTO_ST          7       
#define IPPROTO_EGP         8       
#define IPPROTO_PIGP        9       
#define IPPROTO_RCCMON      10      
#define IPPROTO_NVPII       11      
#define IPPROTO_PUP         12      
#define IPPROTO_ARGUS       13      
#define IPPROTO_EMCON       14      
#define IPPROTO_XNET        15      
#define IPPROTO_CHAOS       16      
#define IPPROTO_MUX         18      
#define IPPROTO_MEAS        19      
#define IPPROTO_HMP         20      
#define IPPROTO_PRM         21      
#define IPPROTO_IDP         22      
#define IPPROTO_TRUNK1      23      
#define IPPROTO_TRUNK2      24      
#define IPPROTO_LEAF1       25      
#define IPPROTO_LEAF2       26      
#define IPPROTO_RDP         27      
#define IPPROTO_IRTP        28      
#define IPPROTO_TP          29      
#define IPPROTO_BLT         30      
#define IPPROTO_NSP         31      
#define IPPROTO_INP         32      
#define IPPROTO_DCCP        33      
#define IPPROTO_3PC         34      
#define IPPROTO_IDPR        35      
#define IPPROTO_XTP         36      
#define IPPROTO_DDP         37      
#define IPPROTO_CMTP        38      
#define IPPROTO_TPXX        39      
#define IPPROTO_IL          40      
#define IPPROTO_IPV6        41      
#define IPPROTO_SDRP        42      
#define IPPROTO_ROUTING     43      
#define IPPROTO_FRAGMENT    44      
#define IPPROTO_IDRP        45      
#define IPPROTO_RSVP        46      
#define IPPROTO_GRE         47      
#define IPPROTO_MHRP        48      
#define IPPROTO_BHA         49      
#define IPPROTO_ESP         50      
#define IPPROTO_AH          51      
#define IPPROTO_INLSP       52      
#define IPPROTO_SWIPE       53      
#define IPPROTO_NHRP        54      
#define IPPROTO_MOBILE      55      
#define IPPROTO_TLSP        56      
#define IPPROTO_SKIP        57      
#define IPPROTO_ICMPV6      58      
#define IPPROTO_NONE        59      
#define IPPROTO_DSTOPTS     60      
#define IPPROTO_AHIP        61      
#define IPPROTO_CFTP        62      
#define IPPROTO_HELLO       63      
#define IPPROTO_SATEXPAK    64      
#define IPPROTO_KRYPTOLAN   65      
#define IPPROTO_RVD         66      
#define IPPROTO_IPPC        67      
#define IPPROTO_ADFS        68      
#define IPPROTO_SATMON      69      
#define IPPROTO_VISA        70      
#define IPPROTO_IPCV        71      
#define IPPROTO_CPNX        72      
#define IPPROTO_CPHB        73      
#define IPPROTO_WSN         74      
#define IPPROTO_PVP         75      
#define IPPROTO_BRSATMON    76      
#define IPPROTO_ND          77      
#define IPPROTO_WBMON       78      
#define IPPROTO_WBEXPAK     79      
#define IPPROTO_EON         80      
#define IPPROTO_VMTP        81      
#define IPPROTO_SVMTP       82      
#define IPPROTO_VINES       83      
#define IPPROTO_TTP         84      
#define IPPROTO_IGP         85      
#define IPPROTO_DGP         86      
#define IPPROTO_TCF         87      
#define IPPROTO_IGRP        88      
#define IPPROTO_OSPFIGP     89      
#define IPPROTO_SRPC        90      
#define IPPROTO_LARP        91      
#define IPPROTO_MTP         92      
#define IPPROTO_AX25        93      
#define IPPROTO_IPEIP       94      
#define IPPROTO_MICP        95      
#define IPPROTO_SCCSP       96      
#define IPPROTO_ETHERIP     97      
#define IPPROTO_ENCAP       98      
#define IPPROTO_APES        99      
#define IPPROTO_GMTP        100     
#define IPPROTO_IPCOMP      108     
#define IPPROTO_SCTP        132     
#define IPPROTO_UDPLITE     136

#define IPPROTO_PIM         103     
#define IPPROTO_VRRP        112     
#define IPPROTO_PGM         113     
#define IPPROTO_PFSYNC      240     


#define IPPROTO_OLD_DIVERT  254     

#define IPPROTO_RAW         255     
#define IPPROTO_MAX         256


#define IPPROTO_DONE        257


#define IPPROTO_DIVERT      258     


#define IPPROTO_SPACER      32767       

#define MAXTTL    255



#define IP_DEFAULT_MULTICAST_TTL        1
#define IP_DEFAULT_MULTICAST_LOOP       1

struct ipovly {
    UCHAR    ih_x1[9];        
    UCHAR    ih_pr;            
    USHORT    ih_len;            
    UINT ih_src;        
    UINT ih_dst;        
};


struct in_addr_4in6 {
    UINT    ia46_pad32[3];
    UINT    ia46_addr4;
};

typedef struct snd_if {
    IF_INDEX si_if;             
    UINT si_nxthop;   
    UINT si_lcladdr;  
}SND_IF_S;


#ifdef __cplusplus
    }
#endif 

#endif 


