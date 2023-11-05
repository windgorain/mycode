/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2012-3-30
* Description: 
* History:     
******************************************************************************/

#ifndef __VNDIS_DEF_H_
#define __VNDIS_DEF_H_

#ifdef __cplusplus
    extern "C" {
#endif 


#define VNDIS_BIT_ISSET(ulFlag, ulBits)  ((ulFlag) & (ulBits))
#define VNDIS_BIT_ISSETALL(ulFlag, ulBits)  ((ulFlag) & (ulBits) == (ulBits))
#define VNDIS_BIT_SET(ulFlag, ulBits)  ((ulFlag) |= (ulBits))
#define VNDIS_BIT_CLR(ulFlag, ulBits)  ((ulFlag) &= ~(ulBits))

#define VNDIS_MAJOR_VERSION       5
#define VNDIS_MINOR_VERSION       0

#if DBG
#define DEBUGP(Fmt) \
{ \
    DbgPrint("VNdis.SYS:"); \
    DbgPrint Fmt; \
}
#else 
#define DEBUGP(Fmt)
#endif

#define DEBUG_IN_FUNC() DEBUGP(("---> %s\n", __FUNCTION__))
#define DEBUG_OUT_FUNC(_result) DEBUGP(("<--- %s:0x%x\n", __FUNCTION__, _result))

#define VNDIS_MINIMUM_MTU 576
#define VNDIS_MAXIMUM_MTU 1500
#define VNDIS_DEF_MTU VNDIS_MAXIMUM_MTU
#define VNDIS_MAX_ETH_PACKET_SIZE (VNDIS_MAXIMUM_MTU + sizeof(VNDIS_ETH_HEADER_S))

typedef unsigned char MACADDR [6];

#define ENABLE_NONADMIN 1       

typedef struct {
  MACADDR stAddr;
}VNDIS_ETH_ADDR_S;


typedef struct
{
    MACADDR dest;
    MACADDR src;

    USHORT proto;
}VNDIS_ETH_HEADER_S;

#define ETH_SNAPORI_LEN         3

typedef struct tagETHSNAP
{
    MACADDR    dest;
    MACADDR    src;
    USHORT     usLen;                                   
    UCHAR      ucDSAP;                                  
    UCHAR      ucSSAP;                                  
    UCHAR      ucCtrl;                                  
    UCHAR      aucORI[ETH_SNAPORI_LEN];
    USHORT     usType;
} VNDIS_ETH_SNAPENCAP_S;


#define ETH_MAC_LEN     14
#define ETH_LLC_LEN     3
#define ETH_SNAP_LEN    5


#define ETH_MIN_TYPE     0x0600
#define ETH_MAX_MTU      0x05DC

#define ETH_IS_PKTTYPE(usPktLenOrType) ((usPktLenOrType) >= ETH_MIN_TYPE)
#define ETH_IS_PKTLEN(usPktLenOrType) ((usPktLenOrType) <= ETH_MAX_MTU)


#define ETHERTYPE_IP         0x0800   
#define ETHERTYPE_IP6        0x86DD   
#define ETHERTYPE_ARP        0x0806   
#define ETHERTYPE_ISIS       0x8000   
#define ETHERTYPE_ISIS2      0x8870   
#define ETHERTYPE_IP_MPLS    0x8847   
#define ETHERTYPE_OTHER      0xFFFF   


#define VNDIS_LITTLE_ENDIAN  
#ifdef VNDIS_LITTLE_ENDIAN
#define ntohs(x) RtlUshortByteSwap(x)
#define htons(x) RtlUshortByteSwap(x)
#define ntohl(x) RtlUlongByteSwap(x)
#define htonl(x) RtlUlongByteSwap(x)
#else
#define ntohs(x) ((USHORT)(x))
#define htons(x) ((USHORT)(x))
#define ntohl(x) ((ULONG)(x))
#define htonl(x) ((ULONG)(x))
#endif

#ifdef __cplusplus
    }
#endif 

#endif 


