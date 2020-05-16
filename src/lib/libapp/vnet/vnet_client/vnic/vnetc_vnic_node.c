/******************************************************************************
* Copyright (C),    LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2013-12-20
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "../../inc/vnet_conf.h"
#include "../../inc/vnet_ifnet.h"
#include "../../inc/vnet_node.h"

#include "../inc/vnetc_conf.h"
#include "../inc/vnetc_udp_phy.h"
#include "../inc/vnetc_node.h"
#include "../inc/vnetc_vnic_phy.h"

static UINT g_uiVnetcVnicNodeID = VNET_NID_MAKE(VNET_NID_TYPE_VNIC, 0);

BS_STATUS VNETC_VnicNode_Init()
{
    return VNETC_NODE_Set(g_uiVnetcVnicNodeID, VNETC_VNIC_PHY_GetVnicIfIndex(), 0, VNETC_NODE_FLAG_STATIC | VNETC_NODE_FLAG_INNER);
}

UINT VNETC_VnicNode_GetNID()
{
    return g_uiVnetcVnicNodeID;
}


