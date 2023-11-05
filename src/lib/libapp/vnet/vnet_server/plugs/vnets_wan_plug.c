/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2013-3-29
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "app/if_pub.h"
#include "comp/comp_wan.h"
#include "comp/comp_if.h"

#include "../../inc/vnet_conf.h"

#include "../inc/vnets_conf.h"
#include "../inc/vnets_domain.h"
#include "../inc/vnets_dc.h"
#include "../inc/vnets_context.h"
#include "../inc/vnets_mac_layer.h"
#include "../inc/vnets_node.h"
#include "../inc/vnets_wan_plug.h"


static UINT g_uiVnetsWanPlugVfUserIndex = 0;
static IF_INDEX g_ifVnetsWanPlugIfindex = 0;

static BS_STATUS vnets_wan_plug_PhyOutput(IN IF_INDEX uiIfIndex, IN MBUF_S *pstMbuf)
{
    UINT uiWanVrfID;
    UINT uiDomainID;
    
    if (NULL == VNETS_Context_AddContextToMbuf(pstMbuf))
    {
        MBUF_Free(pstMbuf);
        return BS_ERR;
    }

    uiWanVrfID = MBUF_GET_OUTVPNID(pstMbuf);

    uiDomainID = (UINT)(ULONG)WanVrf_GetData(uiWanVrfID, g_uiVnetsWanPlugVfUserIndex);

    if (uiDomainID == 0)
    {
        MBUF_Free(pstMbuf);
        return BS_ERR;
    }

    return VNETS_MacLayer_Output(uiDomainID, pstMbuf);
}

static BS_STATUS vnets_wan_plug_UdpServiceInput
(
    IN MBUF_S *pstMbuf,
    IN WAN_UDP_SERVICE_PARAM_S *pstParam,
    IN USER_HANDLE_S *pstUserHandle
)
{
    PF_VNETS_WAN_PLUG_UDP_SERVICE_FUNC pfServiceFunc = pstUserHandle->ahUserHandle[0];

    return pfServiceFunc(pstMbuf, pstParam);
}

BS_STATUS VNETS_WAN_PLUG_Init()
{
    IF_PHY_PARAM_S stParam;
    IF_TYPE_PARAM_S stTypeParam = {0};
    
    Mem_Zero (&stParam, sizeof (stParam));
    stParam.pfPhyOutput = vnets_wan_plug_PhyOutput;
    IFNET_SetPhyType("vnets.wan", &stParam);

    stTypeParam.pcProtoType = IF_PROTO_IP_TYPE_MAME;
    stTypeParam.pcLinkType = IF_ETH_LINK_TYPE_MAME;
    stTypeParam.pcPhyType = "vnets.wan";
    stTypeParam.uiFlag = IF_TYPE_FLAG_HIDE;
    
    IFNET_AddIfType("vnets.wan", &stTypeParam);

    g_ifVnetsWanPlugIfindex = IFNET_CreateIf("vnets.wan");

    g_uiVnetsWanPlugVfUserIndex = WanVrf_AllocDataIndex();
    
    return BS_OK;
}

BS_STATUS VNETS_WAN_PLUG_PktInput(IN MBUF_S *pstMbuf)
{
    UINT uiVrfID;
    UINT uiDomainID;
    HANDLE hValue;
    UINT uiSrcNodeID;

    uiSrcNodeID = VNETS_Context_GetSrcNodeID(pstMbuf);
    if (0 == uiSrcNodeID)
    {
        MBUF_Free(pstMbuf);
        return BS_ERR;
    }

    uiDomainID = VNETS_NODE_GetDomainID(uiSrcNodeID);
    if (uiDomainID == 0)
    {
        MBUF_Free(pstMbuf);
        return BS_ERR;
    }

    if (BS_OK != VNETS_Domain_GetProperty(uiDomainID, VNETS_DOMAIN_WAN_VF_ID, &hValue))
    {
        MBUF_Free(pstMbuf);
        return BS_ERR;
    }

    uiVrfID = HANDLE_UINT(hValue);

    if (uiVrfID == 0)
    {
        MBUF_Free(pstMbuf);
        return BS_ERR;
    }

    MBUF_SET_INVPNID(pstMbuf, uiVrfID);

    return IFNET_LinkInput(g_ifVnetsWanPlugIfindex, pstMbuf);
}

VOID VNETS_WAN_PLUG_RegUdpService
(
    IN USHORT usPort,
    IN UINT uiFlag, 
    IN PF_VNETS_WAN_PLUG_UDP_SERVICE_FUNC pfServiceFunc
)
{
    USER_HANDLE_S stUserHandle;

    stUserHandle.ahUserHandle[0] = pfServiceFunc;

    WanUdpService_RegService(usPort, uiFlag, vnets_wan_plug_UdpServiceInput, &stUserHandle);
}

static BS_STATUS vnets_wan_plug_DomainEventCreate(IN VNETS_DOMAIN_RECORD_S *pstParam)
{
    UINT uiWanVrfID;

    uiWanVrfID = WanVrf_CreateVrf(pstParam->szDomainName);
    if (uiWanVrfID == 0)
    {
        return BS_ERR;
    }

    pstParam->ahProperty[VNETS_DOMAIN_WAN_VF_ID] = UINT_HANDLE(uiWanVrfID);

    WanVrf_SetData(uiWanVrfID, g_uiVnetsWanPlugVfUserIndex, UINT_HANDLE(pstParam->uiDomainID));
    
    return BS_OK;
}

static VOID vnets_wan_plug_DomainEventDelete(IN VNETS_DOMAIN_RECORD_S *pstParam)
{
    UINT uiWanVrfID;


    uiWanVrfID = HANDLE_UINT(pstParam->ahProperty[VNETS_DOMAIN_WAN_VF_ID]);

    pstParam->ahProperty[VNETS_DOMAIN_WAN_VF_ID] = NULL;

    if (uiWanVrfID == 0)
    {
        return;
    }

    WanVrf_DestoryVrf(uiWanVrfID);
}

BS_STATUS VNETS_WAN_PLUG_DomainEvent
(
    IN VNETS_DOMAIN_RECORD_S *pstParam,
    IN UINT uiEvent
)
{
    BS_STATUS eRet = BS_OK;

    switch (uiEvent)
    {
        case VNET_DOMAIN_EVENT_AFTER_CREATE:
        {
            eRet = vnets_wan_plug_DomainEventCreate(pstParam);
            break;
        }

        case VNET_DOMAIN_EVENT_PRE_DESTROY:
        {
            vnets_wan_plug_DomainEventDelete(pstParam);
            break;
        }

        default:
        {
            break;
        }
    }

    return eRet;
}

