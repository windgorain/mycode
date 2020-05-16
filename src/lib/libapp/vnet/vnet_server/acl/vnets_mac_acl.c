/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2008-10-5
* Description: 
* History:     
******************************************************************************/

#include "bs.h"
    
#include "utl/mbuf_utl.h"
#include "utl/msgque_utl.h"

#include "../../inc/vnet_conf.h"
#include "../../inc/vnet_ifnet.h"

static BOOL_T g_bVnetMacAclPermitBroadcast = FALSE;

BS_STATUS VNETS_MAC_ACL_Init()
{
    g_bVnetMacAclPermitBroadcast = FALSE;

    return BS_OK;
}

VOID VNETS_MAC_ACL_PermitBroadcast(IN BOOL_T bIsPermit)
{
    g_bVnetMacAclPermitBroadcast = bIsPermit;
}

BOOL_T VNETS_MAC_ACL_IsPermitBroadcast()
{
    return g_bVnetMacAclPermitBroadcast;
}


