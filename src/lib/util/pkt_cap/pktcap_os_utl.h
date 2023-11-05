/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2012-10-19
* Description: 
* History:     
******************************************************************************/

#ifndef __PKTCAP_OS_UTL_H_
#define __PKTCAP_OS_UTL_H_

#ifdef IN_WINDOWS
#include "winpcap/pcap.h"
#endif
#ifdef IN_UNIXLIKE
#include "pcap.h"
#endif

#ifdef __cplusplus
    extern "C" {
#endif 

typedef int	(*PF_pcap_findalldevs)(pcap_if_t **, char *);
typedef void (*PF_pcap_freealldevs)(pcap_if_t *);
typedef pcap_t* (*PF_pcap_open_live)(const char *, int, int, int, char *);
typedef int	(*PF_pcap_datalink)(pcap_t *);
typedef void (*PF_pcap_close)(pcap_t *);
typedef int	(*PF_pcap_sendpacket)(pcap_t *, const u_char *, int);
typedef int	(*PF_pcap_compile)(pcap_t *, struct bpf_program *, const char *, int, bpf_u_int32);
typedef int	(*PF_pcap_setfilter)(pcap_t *, struct bpf_program *);
typedef int	(*PF_pcap_dispatch)(pcap_t *, int, pcap_handler, u_char *);
typedef int	(*PF_pcap_loop)(pcap_t *, int, pcap_handler, u_char *);
typedef void (*PF_pcap_breakloop)(pcap_t *);

typedef struct
{
	PF_pcap_findalldevs pfFindAllDevs;
	PF_pcap_freealldevs pfFreeAllDevs;
	PF_pcap_open_live pfOpenLive;
	PF_pcap_datalink pfDataLink;
	PF_pcap_close pfClose;
	PF_pcap_sendpacket pfSendPacket;
	PF_pcap_compile pfCompile;
	PF_pcap_setfilter pfSetFilter;
	PF_pcap_dispatch pfDispatch;
	PF_pcap_loop pfLoop;
	PF_pcap_breakloop pfBreakLoop;
}_PKTCAP_FUNC_TBL_S;


#ifdef __cplusplus
    }
#endif 

#endif 


