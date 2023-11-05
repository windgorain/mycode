/******************************************************************************
* Copyright (C),    LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2014-1-14
* Description: 直连检测协议
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/ses_utl.h"

#include "../inc/vnetc_node.h"
#include "../inc/vnetc_phy.h"
#include "../inc/vnetc_ses.h"
#include "../inc/vnetc_udp_phy.h"



static VOID vnetc_c2c_direct_DetectSuccess(IN UINT uiPeerNodeID, IN UINT uiSesID, IN UINT uiIfIndex)
{
    VNETC_NODE_Learn(uiPeerNodeID, uiIfIndex, uiSesID);
}

static BS_STATUS vnetc_c2c_direct_SesEvent(IN UINT uiSesID, IN UINT uiEvent, IN USER_HANDLE_S *pstUserHandle)
{
    UINT uiPeerNodeID = HANDLE_UINT(pstUserHandle->ahUserHandle[0]);
    UINT uiIfIndex = HANDLE_UINT(pstUserHandle->ahUserHandle[1]);

    switch (uiEvent)
    {
        case SES_EVENT_CONNECT:
        {
            vnetc_c2c_direct_DetectSuccess(uiPeerNodeID, uiSesID, uiIfIndex);
            break;
        }

        case SES_EVENT_CONNECT_FAILED:
        case SES_EVENT_PEER_CLOSED:
        {
            VNETC_NODE_DirectDetectFailed(uiPeerNodeID);
            VNETC_SES_SetProperty(uiSesID, VNETC_SES_PROPERTY_NODE_ID, NULL);
            VNETC_SES_Close(uiSesID);
            break;
        }

        default:
        {
            break;
        }
    }

    return BS_OK;
}

BS_STATUS VNETC_C2C_Direct_StartDetect
(
    IN UINT uiPeerNodeID,
    IN UINT uiPeerIP,    
    IN USHORT usPeerPort 
)
{
    VNETC_PHY_CONTEXT_S stPhyContext;
    UINT uiSesID;
    USER_HANDLE_S stUserHandle;

    stPhyContext.uiIfIndex = VNETC_UDP_PHY_GetIfIndex();
    stPhyContext.unPhyContext.stUdpPhyContext.uiPeerIp = uiPeerIP;
    stPhyContext.unPhyContext.stUdpPhyContext.usPeerPort = usPeerPort;

    uiSesID = VNETC_SES_CreateClient(&stPhyContext);
    if (0 == uiSesID)
    {
        VNETC_NODE_DirectDetectFailed(uiPeerNodeID);
        return BS_ERR;
    }

    stUserHandle.ahUserHandle[0] = UINT_HANDLE(uiPeerNodeID);
    stUserHandle.ahUserHandle[1] = UINT_HANDLE(stPhyContext.uiIfIndex);
    VNETC_SES_SetEventNotify(uiSesID, vnetc_c2c_direct_SesEvent, &stUserHandle);
    VNETC_SES_SetProperty(uiSesID, VNETC_SES_PROPERTY_NODE_ID, UINT_HANDLE(uiPeerNodeID));

    if (BS_OK != VNETC_SES_Connect(uiSesID))
    {
        VNETC_SES_Close(uiSesID);
        return BS_ERR;
    }

    return BS_OK;
}

