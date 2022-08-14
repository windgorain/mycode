/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2013-1-22
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/mbuf_utl.h"
#include "utl/sif_utl.h"
#include "utl/fib_utl.h"
#include "utl/ippool_utl.h"
#include "utl/ipfwd_service.h"
#include "comp/comp_wan.h"
#include "comp/comp_pcap.h"
#include "comp/comp_kfapp.h"
#include "comp/comp_if.h"
#include "comp/comp_acl.h"
#include "comp/comp_support.h"

#include "../h/wan_ifnet.h"
#include "../h/wan_vrf.h"
#include "../h/wan_nat.h"
#include "../h/wan_dhcp.h"
#include "../h/wan_pcap.h"
#include "../h/wan_arp_agent.h"
#include "../h/wan_arp.h"
#include "../h/wan_fib.h"
#include "../h/wan_kf.h"
#include "../h/wan_cmd.h"
#include "../h/wan_vrf_cmd.h"
#include "../h/wan_inloop.h"
#include "../h/wan_blackhole.h"
#include "../h/wan_eth_link.h"
#include "../h/wan_ip_addr.h"
#include "../h/wan_proto.h"
#include "../h/wan_bridge.h"
#include "../h/wan_ifnet.h"
#include "../h/wan_ipfwd_service.h"

BS_STATUS WAN_Init()
{
    WAN_KF_Init();
    WAN_Bridge_Init();
    WAN_IF_Init();
    WAN_IPAddr_Init();
    WAN_IpAddrCmd_Init();
    WAN_InLoop_Init();
    WanVrf_Init();
    WAN_BlackHole_Init();
    WAN_IpFwdService_Init();
    WAN_Proto_Init();
    WAN_ARP_Init();
    WanFib_Init();
    WAN_NAT_Init();
    WAN_DHCP_Init();
    WAN_PCAP_Init();
    WAN_ArpAgent_Init();

    WAN_CMD_Init();
    WAN_VFCmd_Init();
    
    WanVrf_Init2();

    return BS_OK;
}
