/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2007-9-23
* Description: 
* History:     
******************************************************************************/
/* retcode所需要的宏 */
#define RETCODE_FILE_NUM VNET_RETCODE_FILE_NUM_VNICPHY

#include "bs.h"
    
#include "utl/vnic_lib.h"
#include "utl/vnic_agent.h"
#include "utl/msgque_utl.h"
#include "utl/txt_utl.h"
#include "utl/eth_utl.h"
#include "utl/sys_utl.h"
#include "utl/vnic_tap.h"
#include "utl/local_info.h"

#include "comp/comp_if.h"

#include "../../inc/vnet_conf.h"
#include "../../inc/vnet_ifnet.h"

#include "../inc/vnetc_vnic_link.h"
#include "../inc/vnetc_conf.h"
#include "../inc/vnetc_context.h"
#include "../inc/vnetc_vnic_phy.h"

/* VNET VNIC PHY的事件 */
#define _VNET_VNIC_PHY_SEND_DATA_EVENT 0x1

/* VNET VNIC PHY的消息类型 */
#define _VNET_VNIC_PHY_SEND_DATA_MSG   1


static UINT g_ulVnetVnicIfIndex = 0;
static VNIC_AGENT_HANDLE g_hVnicAgentHandle = 0;
static MSGQUE_HANDLE g_hVnetVnicPhyQueId = NULL;
static EVENT_HANDLE g_hVnetVnicEventId = 0;
static BOOL_T g_bVnetVnicInited = FALSE;

static MAC_ADDR_S g_stVnicMac;

static BS_STATUS _VNET_VNIC_PHY_Output (IN UINT ulIfIndex, IN MBUF_S *pstMbuf)
{
    return VNIC_Agent_Write(g_hVnicAgentHandle, pstMbuf);
}

static BS_STATUS _VNET_VNIC_PHY_Input(IN HANDLE hVnicAgentId, IN MBUF_S *pstMbuf, IN HANDLE hUserHandle)
{
    VNETC_CONTEXT_S *pstContext;
    UINT ulSesstionId = HANDLE_UINT(hUserHandle);

    pstContext = VNETC_Context_AddContextToMbuf(pstMbuf);
    if (NULL == pstContext)
    {
        MBUF_Free(pstMbuf);
        RETURN(BS_NO_MEMORY);
    }

    if (BS_OK != ETH_PadPacket(pstMbuf, TRUE))
    {
        MBUF_Free(pstMbuf);
        return BS_ERR;
    }

    VNETC_Context_SetRecvSesID (pstMbuf, ulSesstionId);

    MBUF_SET_RECV_IF_INDEX(pstMbuf,g_ulVnetVnicIfIndex);
    return VNET_VNIC_LinkInput(g_ulVnetVnicIfIndex, pstMbuf);
}

BS_STATUS VNET_VNIC_CreateVnic()
{
    VNIC_HANDLE hVnic;
    UINT ulOutLen;

    if (g_bVnetVnicInited == TRUE)
    {
        return BS_OK;
    }

    hVnic = VNIC_Dev_Open();
    if (hVnic == NULL)
    {
        BS_WARNNING (("Can't open vnic device!"));
        return BS_ERR;
    }

    g_hVnicAgentHandle = VNIC_Agent_Create();
    if (NULL == g_hVnicAgentHandle)
    {
        VNIC_Delete(hVnic);
        return BS_ERR;
    }
    VNIC_Agent_SetVnic(g_hVnicAgentHandle, hVnic);

    if (BS_OK != VNIC_Ioctl (hVnic, TAP_WIN_IOCTL_GET_MAC, (UCHAR*)&g_stVnicMac,
        6, (UCHAR*)&g_stVnicMac, 6, &ulOutLen))
    {
        BS_WARNNING (("Can't get vnic mac!"));
    }

    VNETC_SetHostMac(&g_stVnicMac);

    g_bVnetVnicInited = TRUE;

    return BS_OK;
}

VOID VNET_VNIC_DelVnic ()
{
    VNIC_HANDLE hVnic;

    hVnic = VNIC_Agent_GetVnic(g_hVnicAgentHandle);    
    VNIC_Agent_Close(g_hVnicAgentHandle);
    VNIC_Delete(hVnic);

    g_hVnicAgentHandle = 0;
    g_bVnetVnicInited = FALSE;
}

VOID VNET_VNIC_PHY_SetMediaStatus(IN UINT uiStatus)
{
    HANDLE hVnicId;
    UINT ulLen;

    hVnicId = VNIC_Agent_GetVnic (g_hVnicAgentHandle);
    if (hVnicId == 0)
    {
        return;
    }

    if (uiStatus == 0)
    {
        VNIC_Agent_Stop(g_hVnicAgentHandle);
    }

    VNIC_Ioctl (hVnicId, TAP_WIN_IOCTL_SET_MEDIA_STATUS,
        (UCHAR*)&uiStatus, 4, (UCHAR*)&uiStatus, 4, &ulLen);

    if (uiStatus == 1)
    {
        VNIC_Agent_Start(g_hVnicAgentHandle, _VNET_VNIC_PHY_Input, NULL);
    }
}

MAC_ADDR_S * VNET_VNIC_PHY_GetVnicMac ()
{
    return &g_stVnicMac;
}

UINT VNETC_VNIC_PHY_GetVnicIfIndex ()
{
    return g_ulVnetVnicIfIndex;
}

BS_STATUS VNETC_VNIC_PHY_GetIp(OUT UINT *puiIp, OUT UINT *puiMask)
{
    HANDLE hVnicId;

    hVnicId = VNIC_Agent_GetVnic (g_hVnicAgentHandle);
    if (hVnicId == 0)
    {
        return BS_NOT_READY;
    }

    return VNIC_GetIP(hVnicId, puiIp, puiMask);
}

BS_STATUS VNET_VNIC_PHY_Init ()
{
    IF_PHY_PARAM_S stPhyParam;
    IF_LINK_PARAM_S stLinkParam;
    IF_TYPE_PARAM_S stTypeParam = {0};

    stPhyParam.pfPhyOutput = _VNET_VNIC_PHY_Output;
    CompIf_SetPhyType("vnets.l2.vnic", &stPhyParam);

    stLinkParam.pfLinkInput = VNET_VNIC_LinkInput;
    stLinkParam.pfLinkOutput = VNET_VNIC_LinkOutput;
    CompIf_SetLinkType("vnets.l2.vnic", &stLinkParam);

    stTypeParam.pcLinkType = "vnets.l2.vnic";
    stTypeParam.pcPhyType = "vnets.l2.vnic";
    stTypeParam.uiFlag = IF_TYPE_FLAG_HIDE;

    CompIf_AddIfType("vnets.l2.vnic", &stTypeParam);

    g_ulVnetVnicIfIndex = CompIf_CreateIf("vnets.l2.vnic");
    if (0 == g_ulVnetVnicIfIndex)
    {
        RETURN(BS_ERR);
    }

    if (NULL == (g_hVnetVnicEventId = Event_Create()))
    {
        RETURN(BS_ERR);
    }

    if (NULL == (g_hVnetVnicPhyQueId = MSGQUE_Create(128)))
    {
        RETURN(BS_ERR);
    }

    return BS_OK;
}


