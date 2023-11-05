/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2013-3-20
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/sif_utl.h"
#include "utl/fib_utl.h"
#include "utl/ipfwd_service.h"
#include "utl/vf_utl.h"
#include "comp/comp_if.h"

#include "../h/wan_vrf.h"
#include "../h/wan_ifnet.h"
#include "../h/wan_ipfwd_service.h"
#include "../h/wan_fib.h"
#include "../h/wan_ipfwd.h"
#include "../h/wan_blackhole.h"

static UINT g_uiWanBlackHoleIfIndex = 0;

BS_STATUS WAN_BlackHole_Init()
{
     
    if (g_uiWanBlackHoleIfIndex != 0)
    {
        return BS_OK;
    }

    g_uiWanBlackHoleIfIndex = IFNET_CreateIf(COMP_IF_BLACK_HOLE_TYPE);
    if (0 == g_uiWanBlackHoleIfIndex)
    {
        return (BS_ERR);
    }

    return BS_OK;
}

UINT WAN_BlackHole_GetIfIndex()
{
    return g_uiWanBlackHoleIfIndex;
}


