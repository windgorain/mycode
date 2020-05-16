/******************************************************************************
* Copyright (C), 2000-2006,  LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2017-5-25
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "../h/wan_ip_addr.h"
#include "../h/wan_fib.h"
#include "../h/wan_nat.h"

BS_STATUS WAN_KF_Init()
{
    WAN_IPAddr_KfInit();
    WanFib_KfInit();
    WAN_NAT_KfInit();

    return BS_OK;
}

