/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-7-12
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "comp/comp_if.h"

#include "app/wan_pub.h"
#include "app/if_pub.h"

#include "../h/wan_pcap.h"
#include "../h/wan_inloop.h"
#include "../h/wan_proto.h"
#include "../h/wan_eth_link.h"

static UINT g_uiWanIfTypeInloop = 0;

BS_STATUS WAN_IF_Init()
{
    IF_LINK_PARAM_S stLinkParam = {0};
    IF_PROTO_PARAM_S stProtoParam = {0};
    IF_TYPE_PARAM_S stTypeParam = {0};

    stLinkParam.pfLinkOutput = WAN_InLoop_LinkOutput;
    IFNET_SetLinkType(IF_INLOOP_LINK_TYPE_MAME, &stLinkParam);

    stLinkParam.pfLinkInput = WAN_ETH_LinkInput;
    stLinkParam.pfLinkOutput = WAN_ETH_LinkOutput;
    IFNET_SetLinkType(IF_ETH_LINK_TYPE_MAME, &stLinkParam);

    stProtoParam.pfProtoInput = WAN_Proto_Input;
    IFNET_SetProtoType(IF_PROTO_IP_TYPE_MAME, &stProtoParam);

    stTypeParam.pcProtoType = IF_PROTO_IP_TYPE_MAME;
    stTypeParam.pcLinkType = IF_INLOOP_LINK_TYPE_MAME;
    stTypeParam.uiFlag = IF_TYPE_FLAG_HIDE;
    g_uiWanIfTypeInloop = IFNET_AddIfType(IF_INLOOP_IF_TYPE_MAME, &stTypeParam);

    return BS_OK;
}

IF_INDEX WAN_IF_GetIfIndexByEnv(IN VOID *pEnv)
{
    return IFNET_GetIfIndexByCmdEnv(pEnv);
}

