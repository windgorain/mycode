/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2013-1-9
* Description: 
* History:     
******************************************************************************/
#include "bs.h"
#include "comp/comp_pcap.h"
#include "comp/comp_if.h"

#include "nat_if.h"
#include "nat_cmd.h"
#include "nat_link.h"
#include "nat_arp.h"
#include "nat_phy.h"
#include "nat_comp_inner.h"

BS_STATUS NAT_Init()
{
    COMP_PCAP_Init();
    CompIf_Init();
	NAT_Phy_Init();
    NAT_ARP_Init();
    NAT_CMD_Init();
    return BS_OK;
}


