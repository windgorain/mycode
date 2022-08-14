/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2013-12-16
* Description: 
* History:     
******************************************************************************/

#include "bs.h"

#include "utl/ses_utl.h"
#include "utl/mutex_utl.h"
#include "comp/comp_if.h"

#include "../../inc/vnet_conf.h"
#include "../../inc/vnet_ifnet.h"

#include "../inc/vnetc_conf.h"
#include "../inc/vnetc_context.h"
#include "../inc/vnetc_master.h"
#include "../inc/vnetc_udp_phy.h"
#include "../inc/vnetc_ses.h"
#include "../inc/vnetc_vpn_link.h"

#define _VNETC_SES_MAX_NUM (1024 * 64)

#define _VNETC_SES_TIMEOUT_TIME 1000  /* 1s */
#define _VNETC_SES_KEEP_ALIVE_IDLE   30
#define _VNETC_SES_KEEP_ALIVE_INTVAL 5
#define _VNETC_SES_KEEP_ALIVE_MAX_TRYS 3

static SES_HANDLE g_hVnetcSesHandle = NULL;
static MUTEX_S g_stVnetcSesMutex; /* 只用于互斥删除和显示.其他不存在问题 */

static BS_STATUS vnetc_ses_RecvPkt(IN UINT uiSesID, IN MBUF_S *pstMbuf)
{
    VNETC_Context_SetRecvSesID(pstMbuf, uiSesID);

    return VNETC_VPN_LINK_Input(pstMbuf);
}

static BS_STATUS vnetc_ses_SendPkt(IN MBUF_S *pstMbuf, IN VOID *pUserContext)
{
    VNETC_PHY_CONTEXT_S *pstContext = pUserContext;
    VNETC_PHY_CONTEXT_S *pstPhyContext;

    if (NULL == VNETC_Context_AddContextToMbuf(pstMbuf))
    {
        MBUF_Free(pstMbuf);
        return BS_ERR;
    }

    pstPhyContext = VNETC_Context_GetPhyContext(pstMbuf);
    *pstPhyContext = *pstContext;

    return IFNET_PhyOutput(pstContext->uiIfIndex, pstMbuf);
}

static BS_STATUS vnetc_ses_EventNotify(IN UINT uiSesID, IN UINT uiEvent)
{
    switch (uiEvent)
    {
        case SES_EVENT_CONNECT:
        {
            break;
        }

        case SES_EVENT_CONNECT_FAILED:
        case SES_EVENT_PEER_CLOSED:
        {
            SES_Close(g_hVnetcSesHandle, uiSesID);
            break;
        }

        default:
        {
            break;
        }
    }

    return BS_OK;
}

static VOID vnetc_ses_PktInput(IN USER_HANDLE_S *pstUserHandle)
{
    MBUF_S *pstMbuf = pstUserHandle->ahUserHandle[0];

    SES_PktInput(g_hVnetcSesHandle, pstMbuf, VNETC_Context_GetPhyContext(pstMbuf));
}

static VOID vnetc_ses_TimeOut(IN HANDLE hTimerHandle, IN USER_HANDLE_S *pstUserHandle)
{
    SES_TimerStep(g_hVnetcSesHandle);
}

BS_STATUS VNETC_SES_Init()
{
    SES_OPT_KEEP_ALIVE_TIME_S stKeepAliveSet;

    g_hVnetcSesHandle = SES_CreateInstance(_VNETC_SES_MAX_NUM,
            sizeof(VNETC_PHY_CONTEXT_S),
            VNETC_SES_PROPERTY_MAX,
            vnetc_ses_RecvPkt,
            vnetc_ses_SendPkt,
            vnetc_ses_EventNotify);
    if (g_hVnetcSesHandle == NULL)
    {
        return BS_ERR;
    }

    stKeepAliveSet.usIdle = _VNETC_SES_KEEP_ALIVE_IDLE;
    stKeepAliveSet.usIntval = _VNETC_SES_KEEP_ALIVE_INTVAL;
    stKeepAliveSet.usMaxProbeCount= _VNETC_SES_KEEP_ALIVE_MAX_TRYS;
    SES_SetDftKeepAlive(g_hVnetcSesHandle, &stKeepAliveSet);

    MUTEX_Init(&g_stVnetcSesMutex);

    VNETC_Master_AddTimer(_VNETC_SES_TIMEOUT_TIME, TIMER_FLAG_CYCLE, vnetc_ses_TimeOut, NULL);

    return BS_OK;
}

UINT VNETC_SES_CreateClient(IN VNETC_PHY_CONTEXT_S *pstPhyContext)
{
    return SES_CreateClient(g_hVnetcSesHandle, pstPhyContext);
}

SES_TYPE_E VNETC_SES_GetType(IN UINT uiSesID)
{
    return SES_GetType(g_hVnetcSesHandle, uiSesID);
}

BS_STATUS VNETC_SES_SetEventNotify(IN UINT uiSesID, IN PF_SES_EVENT_NOTIFY pfEventNotify, IN USER_HANDLE_S *pstUserHandle)
{
    return SES_SetEventNotify(g_hVnetcSesHandle, uiSesID, pfEventNotify, pstUserHandle);
}

BS_STATUS VNETC_SES_RegCloseNotify(IN PF_SES_CLOSE_NOTIFY_FUNC pfFunc,IN USER_HANDLE_S * pstUserHandle)
{
    return SES_RegCloseNotifyEvent(g_hVnetcSesHandle, pfFunc, pstUserHandle);
}

BS_STATUS VNETC_SES_SetProperty(IN UINT uiSesID, IN UINT uiPropertyIndex, IN HANDLE hValue)
{
    return SES_SetProperty(g_hVnetcSesHandle, uiSesID, uiPropertyIndex, hValue);
}

HANDLE VNETC_SES_GetProperty(IN UINT uiSesID, IN UINT uiPropertyIndex)
{
    return SES_GetProperty(g_hVnetcSesHandle, uiSesID, uiPropertyIndex);
}

BS_STATUS VNETC_SES_Connect(IN UINT uiSesID)
{
    if (BS_OK != SES_Connect(g_hVnetcSesHandle, uiSesID))
    {
        return BS_ERR;
    }

    return BS_OK;
}

BS_STATUS VNETC_SES_PktInput(IN UINT ulIfIndex, IN MBUF_S *pstMbuf)
{
    USER_HANDLE_S stUserHandle;

    stUserHandle.ahUserHandle[0] = pstMbuf;
    
    if (BS_OK != VNETC_Master_MsgInput(vnetc_ses_PktInput, &stUserHandle))
    {
        MBUF_Free(pstMbuf);
        return BS_ERR;
    }

    return BS_OK;
}

BS_STATUS VNETC_SES_SendPkt(IN UINT uiSesID, IN MBUF_S *pstMbuf)
{
    return SES_SendPkt(g_hVnetcSesHandle, uiSesID, pstMbuf);
}

VOID VNETC_SES_Close(IN UINT uiSesID)
{
    MUTEX_P(&g_stVnetcSesMutex);
    SES_Close(g_hVnetcSesHandle, uiSesID);
    MUTEX_V(&g_stVnetcSesMutex);
}

BS_STATUS VNETC_SES_SetOpt(IN UINT uiSesID, IN UINT uiOpt, IN VOID *pValue)
{
    return SES_SetOpt(g_hVnetcSesHandle, uiSesID, uiOpt, pValue);
}

VNETC_PHY_CONTEXT_S * VNETC_SES_GetPhyContext(IN UINT uiSesID)
{
    return SES_GetUsrContext(g_hVnetcSesHandle, uiSesID);
}

UINT VNETC_SES_GetIfIndex(IN UINT uiSesID)
{
    VNETC_PHY_CONTEXT_S *pstContext;
    UINT uiIfIndex;

    pstContext = SES_GetUsrContext(g_hVnetcSesHandle, uiSesID);
    if (NULL == pstContext)
    {
        return 0;
    }

    uiIfIndex = pstContext->uiIfIndex;

    return uiIfIndex;
}

static BS_WALK_RET_E vnetc_ses_show(IN UINT uiSesID, IN HANDLE hUserHandle)
{
    EXEC_OutInfo(" %-8x %-9x %s \r\n",
        uiSesID,
        SES_GetPeerSESID(g_hVnetcSesHandle, uiSesID),
        SES_GetStatusString(SES_GetStatus(g_hVnetcSesHandle, uiSesID)));

    return BS_WALK_CONTINUE;
}

/* show session */
PLUG_API BS_STATUS VNETC_SES_Show(IN UINT ulArgc, IN CHAR **argv)
{
    EXEC_OutInfo(" SESID    PeerSESID Status\r\n"
        "--------------------------------------------\r\n");

    MUTEX_P(&g_stVnetcSesMutex);
    SES_Walk(g_hVnetcSesHandle, vnetc_ses_show, NULL);
    MUTEX_V(&g_stVnetcSesMutex);

    EXEC_OutString("\r\n");

    return BS_OK;
}

/* debug session protocol packet*/
PLUG_API BS_STATUS VNETC_SES_DebugProtocolPacket(IN UINT ulArgc, IN CHAR **argv)
{
    SES_AddDbgFlag(g_hVnetcSesHandle, SES_DBG_FLAG_PROTOCOL_PKT);

    return BS_OK;
}

/* no debug session protocol packet*/
PLUG_API BS_STATUS VNETC_SES_NoDebugProtocolPacket(IN UINT ulArgc, IN CHAR **argv)
{
    SES_ClrDbgFlag(g_hVnetcSesHandle, SES_DBG_FLAG_PROTOCOL_PKT);

    return BS_OK;
}

/* debug session data packet*/
PLUG_API BS_STATUS VNETC_SES_DebugDataPacket(IN UINT ulArgc, IN CHAR **argv)
{
    SES_AddDbgFlag(g_hVnetcSesHandle, SES_DBG_FLAG_DATA_PKT);

    return BS_OK;
}

/* no debug session data packet*/
PLUG_API BS_STATUS VNETC_SES_NoDebugDataPacket(IN UINT ulArgc, IN CHAR **argv)
{
    SES_ClrDbgFlag(g_hVnetcSesHandle, SES_DBG_FLAG_DATA_PKT);

    return BS_OK;
}
