/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2013-4-12
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/txt_utl.h"
#include "utl/sif_utl.h"
#include "utl/fib_utl.h"
#include "utl/exec_utl.h"
#include "utl/ipfwd_service.h"
#include "utl/vf_utl.h"
#include "utl/arp_utl.h"
#include "utl/bit_opt.h"
#include "utl/msgque_utl.h"
#include "comp/comp_pcap.h"
#include "comp/comp_if.h"
#include "comp/comp_wan.h"

#include "../h/wan_vrf.h"
#include "../h/wan_ifnet.h"
#include "../h/wan_ipfwd_service.h"
#include "../h/wan_fib.h"
#include "../h/wan_eth_link.h"
#include "../h/wan_ipfwd.h"
#include "../h/wan_ip_addr.h"
#include "../h/wan_blackhole.h"
#include "../h/wan_vrf_cmd.h"
#include "../h/wan_bridge.h"

static VOID _wan_pcap_InterfaceSave(IN IF_INDEX ifIndex, IN HANDLE hFile)
{
    if (WAN_IP_ADDR_MODE_PCAP_HOST == WAN_IPAddr_GetMode(ifIndex))
    {
        CMD_EXP_OutputCmd(hFile, "ip address host");
    }

    return;
}

BS_STATUS WAN_PCAP_Init()
{
    IFNET_RegSave(_wan_pcap_InterfaceSave);

    return BS_OK;
}


PLUG_API BS_STATUS WAN_PCAP_SetIpHost
(
    IN UINT ulArgc,
    IN UCHAR **argv,
    IN VOID *pEnv
)
{
	NETINFO_ADAPTER_S stInfo;
    UINT uiVrf;
    FIB_NODE_S stFibNode;
    WAN_IP_ADDR_INFO_S stAddrInfo = {0};
    CHAR *pcIfName;
    IF_INDEX ifIndex;
    UINT pcap_index;

    pcIfName = CMD_EXP_GetCurrentModeName(pEnv);
    if (NULL == pcIfName) {
        EXEC_OutString(" Interface is not exist.\r\n");
        return BS_ERR;
    }

    pcap_index = IF_GetPhyIndexByIfName(pcIfName);
    if (IF_INVALID_PHY_INDEX == pcap_index) {
        EXEC_OutString(" Interface is not exist.\r\n");
        return BS_ERR;
    }

    ifIndex = IFNET_GetIfIndex(pcIfName);
    if (IF_INVALID_INDEX == ifIndex) {
        EXEC_OutString(" Interface is not exist.\r\n");
        return BS_ERR;
    }

	if (BS_OK != PCAP_Ioctl(pcap_index, PCAP_IOCTL_CMD_GET_NET_INFO, &stInfo))
    {
        EXEC_OutString(" Can't get the pcap netinfo.\r\n");
        return BS_ERR;
    }

    IFNET_Ioctl(ifIndex, IFNET_CMD_GET_VRF, &uiVrf);

    memset(&stFibNode, 0, sizeof(stFibNode));
    stFibNode.uiOutIfIndex = ifIndex;
    stFibNode.uiNextHop = stInfo.uiGateWay;

    WanFib_Add(uiVrf, &stFibNode);

    WanIPAddr_SetMode(ifIndex, WAN_IP_ADDR_MODE_PCAP_HOST);

    stAddrInfo.uiIfIndex = ifIndex;
    stAddrInfo.uiIP = stInfo.auiIpAddr[0];
    stAddrInfo.uiMask = stInfo.auiIpMask[0];
    WanIPAddr_AddIp(&stAddrInfo);

    return BS_OK;
}

