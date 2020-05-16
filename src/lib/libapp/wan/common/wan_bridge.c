/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-7-14
* Description: 
* History:     
******************************************************************************/
#include "bs.h"
#include "utl/eth_utl.h"

#include "comp/comp_support.h"

static MAC_ADDR_S g_stWanBridgeMac;

BS_STATUS WAN_Bridge_Init()
{
    CompSupport_Ioctl(SPT_IOCTL_GET_BRIDGE_MAC, &g_stWanBridgeMac);

    return BS_OK;
}

MAC_ADDR_S * WAN_Bridge_GetMac()
{
    return &g_stWanBridgeMac;
}

