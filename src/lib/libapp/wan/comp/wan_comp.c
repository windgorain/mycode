/******************************************************************************
* Copyright (C) Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2013-3-26
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "comp/comp_wan.h"

#include "../h/wan_vrf.h"
#include "../h/wan_fib.h"
#include "../h/wan_arp.h"
#include "../h/wan_ip_addr.h"
#include "../h/wan_udp_service.h"

static COMP_WAN_S g_stWanComp;

VOID WAN_COMP_Init()
{
    g_stWanComp.comp.comp_name = COMP_WAN_NAME;

    /* vrf */
    g_stWanComp.pfCreateVrf = WanVrf_CreateVrf;
    g_stWanComp.pfDestoryVrf = WanVrf_DestoryVrf;
    g_stWanComp.pfVrfReg = WanVrf_RegEventListener;
    g_stWanComp.pfVrfAllocDataIndex = WanVrf_AllocDataIndex;
    g_stWanComp.pfSetVrfData = WanVrf_SetData;
    g_stWanComp.pfGetVrfData = WanVrf_GetData;

    /* udp service */
    g_stWanComp.pfUdpServiceReg = WanUdpService_RegService;

    /* fib */
    g_stWanComp.pfFibAdd = WanFib_Add;
    g_stWanComp.pfFibDel = WanFib_Del;
    g_stWanComp.pfFibAddRange = WanFib_AddRangeFib;
    g_stWanComp.pfFibDelRange = WanFib_DelRangeFib;

    /* ip address */
    g_stWanComp.pfIPAddrAddIp = WAN_IPAddr_AddIp;
    g_stWanComp.pfIPAddrSetMode = WAN_IPAddr_SetMode;

    COMP_Reg(&g_stWanComp.comp);
}

