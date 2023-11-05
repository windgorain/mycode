
#include "bs.h"

#include "utl/tp_utl.h"
#include "utl/txt_utl.h"
#include "utl/caller_utl.h"

#include "../../inc/vnet_conf.h"
#include "../../inc/vnet_tp.h"
#include "../../inc/vnet_node.h"

#include "../inc/vnetc_conf.h"
#include "../inc/vnetc_tp.h"
#include "../inc/vnetc_node.h"
#include "../inc/vnetc_phy.h"
#include "../inc/vnetc_ses.h"
#include "../inc/vnetc_fsm.h"
#include "../inc/vnetc_caller.h"
#include "../inc/vnetc_user_status.h"
#include "../inc/vnetc_vnic_phy.h"
#include "../inc/vnetc_context.h"
#include "../inc/vnetc_protocol.h"
#include "../inc/vnetc_master.h"

#define _VNETC_TP_TIME_OUT_TIME   1000    
#define _VNETC_TP_KEEP_ALIVE_IDLE   600
#define _VNETC_TP_KEEP_ALIVE_INTVAL 10
#define _VNETC_TP_KEEP_ALIVE_MAX_COUNT 6

static TP_HANDLE g_hVnetcTp;
static TP_ID g_uiVnetcC2STpId = 0;

static BS_STATUS vnetc_tp_Send
(
    IN TP_HANDLE hTpHandle,
    IN TP_CHANNEL_S *pstChannel,
    IN MBUF_S *pstMbuf,
    IN USER_HANDLE_S *pstUserHandle
)
{
    UINT uiNID;

    if (NULL == VNETC_Context_AddContextToMbuf(pstMbuf))
    {
        MBUF_Free(pstMbuf);
        return BS_NO_MEMORY;
    }

    uiNID = HANDLE_UINT(pstChannel->ahChannelDesc[0]);

    return VNETC_NODE_PktOutput(uiNID, pstMbuf, VNET_NODE_PKT_PROTO_TP);
}

static BS_STATUS vnetc_tp_Connected
(
    IN TP_HANDLE hTpHandle,
    IN TP_ID uiTpId
)
{
    VNETC_FSM_EventHandle(VNETC_FSM_EVENT_TP_OK);

    return BS_OK;
}

static BS_STATUS vnetc_tp_Err
(
    IN TP_HANDLE hTpHandle,
    IN TP_ID uiTpId
)
{
    TP_Close(hTpHandle, uiTpId);

    VNETC_User_SetStatus(VNET_USER_STATUS_OFFLINE, VNET_USER_REASON_SERVER_NO_ACK);

    VNETC_FSM_EventHandle(VNETC_FSM_EVENT_TP_FAILED);

    return BS_OK;
}

static BS_STATUS vnetc_tp_Event
(
    IN TP_HANDLE hTpHandle,
    IN TP_ID uiTpId,
    IN UINT uiEvent,
    IN USER_HANDLE_S *pstUserHandle
)
{
    MBUF_S *pstMbuf;
    
    switch (uiEvent)
    {
        case TP_EVENT_CONNECT:
        {
            vnetc_tp_Connected(hTpHandle, uiTpId);
            break;
        }

        case TP_EVENT_READ:
        {
            pstMbuf = TP_RecvMbuf(hTpHandle, uiTpId);
            if (NULL != pstMbuf)
            {
                VNETC_Protocol_Input(uiTpId, pstMbuf);
            }
            break;
        }

        case TP_EVENT_ERR:
        {
            vnetc_tp_Err(hTpHandle, uiTpId);
            break;
        }

        default:
        {
            break;
        }
    }

    return BS_OK;
}

static TP_ID vnetc_tp_ConnectServer(IN UINT uiProtocolId)
{
    TP_ID uiTpId;
    UINT uiValue;
    TP_CHANNEL_S stChannel;
    TP_OPT_KEEP_ALIVE_S stKeepAlive;
    
    uiTpId = TP_Socket(g_hVnetcTp, TP_TYPE_CONN);
    if (uiTpId == 0)
    {
        return 0;
    }

    uiValue = 1;
    TP_SetOpt(g_hVnetcTp, uiTpId, TP_OPT_NBIO, &uiValue);

    stChannel.ahChannelDesc[0] = UINT_HANDLE(VNET_NID_SERVER);

    if (BS_OK != TP_Connect(g_hVnetcTp, uiTpId, uiProtocolId, &stChannel))
    {
        TP_Close(g_hVnetcTp, uiTpId);
        return 0;
    }

    stKeepAlive.usIdle = _VNETC_TP_KEEP_ALIVE_IDLE;
    stKeepAlive.usIntval = _VNETC_TP_KEEP_ALIVE_INTVAL;
    stKeepAlive.usMaxProbeCount = _VNETC_TP_KEEP_ALIVE_MAX_COUNT;

    TP_SetOpt(g_hVnetcTp, uiTpId, TP_OPT_KEEPALIVE_TIME, &stKeepAlive);

    return uiTpId;
}

static VOID vnetc_tp_TimeOut(IN HANDLE hTimerHandle, IN USER_HANDLE_S *pstUserHandle)
{
    TP_TimerStep(g_hVnetcTp);
}

BS_STATUS VNETC_TP_ConnectServer()
{
    TP_ID uiTpId;

    VNETC_User_SetStatus(VNET_USER_STATUS_CONNECTING, VNET_USER_REASON_NONE);

    uiTpId = vnetc_tp_ConnectServer(VNET_TP_SERVER_PROTOCOL_ID);
    if (uiTpId == 0)
    {
        VNETC_User_SetStatus(VNET_USER_STATUS_OFFLINE, VNET_USER_REASON_SERVER_NO_ACK);
        return BS_ERR;
    }

    g_uiVnetcC2STpId = uiTpId;

    return BS_OK;
}

VOID VNETC_TP_TriggerKeepAlive()
{
    if (g_uiVnetcC2STpId == 0)
    {
        return;
    }

    TP_TiggerKeepAlive(g_hVnetcTp, g_uiVnetcC2STpId);
}

BS_STATUS VNETC_TP_Init()
{
    TP_HANDLE hTpHandle;

    hTpHandle = TP_Create(vnetc_tp_Send, vnetc_tp_Event, NULL, 0);
    if (NULL == hTpHandle)
    {
        return BS_NO_MEMORY;
    }

    g_hVnetcTp = hTpHandle;

    return BS_OK;
}

BS_STATUS VNETC_TP_Init2()
{
    if (NULL == VNETC_Master_AddTimer(_VNETC_TP_TIME_OUT_TIME, TIMER_FLAG_CYCLE, vnetc_tp_TimeOut, NULL))
    {
        return BS_ERR;
    }

    return BS_OK;
}

BS_STATUS VNETC_TP_Stop()
{
    if (g_uiVnetcC2STpId != 0)
    {
        TP_Close(g_hVnetcTp, g_uiVnetcC2STpId);
        g_uiVnetcC2STpId = 0;
    }

    return BS_OK;
}

BS_STATUS VNETC_TP_PktInput(IN MBUF_S *pstMbuf)
{
    UINT uiSrcNID;
    TP_CHANNEL_S stTpChannel;

    uiSrcNID = VNETC_Context_GetSrcNID(pstMbuf);
    if (uiSrcNID == 0)
    {
        MBUF_Free(pstMbuf);
        return BS_ERR;
    }

    stTpChannel.ahChannelDesc[0] = UINT_HANDLE(uiSrcNID);

    return TP_PktInput(g_hVnetcTp, pstMbuf, &stTpChannel);
}

BS_STATUS VNETC_TP_PktOutput(IN UINT uiTpId, IN MBUF_S *pstMbuf)
{
    return TP_SendMbuf(g_hVnetcTp, uiTpId, pstMbuf);
}

PLUG_API BS_STATUS VNETC_TP_DebugPkt(IN UINT ulArgc, IN CHAR ** argv)
{
    TP_SetDbgFlag(g_hVnetcTp, TP_DBG_FLAG_PKT);

    return BS_OK;
}

UINT VNETC_TP_GetC2STP()
{
    return g_uiVnetcC2STpId;
}

PLUG_API BS_STATUS VNETC_TP_NoDebugPkt(IN UINT ulArgc, IN CHAR ** argv)
{
    TP_ClrDbgFlag(g_hVnetcTp, TP_DBG_FLAG_PKT);

    return BS_OK;
}

static VOID vnetc_tp_Show(IN TP_HANDLE hTpHandle, IN TP_ID uiTpId, IN HANDLE hUserHandle)
{
    TP_STATE_S stState;

    TP_GetState(hTpHandle, uiTpId, &stState);
    
    EXEC_OutInfo(" %-5s %-8x %-8x %-9s\r\n",
        stState.eType == TP_TYPE_PROTOCOL ? "Proto" : "Link",
        stState.uiLocalTpId,
        stState.uiPeerTpId,
        TP_GetStatusString(stState.uiStatus)
        );
}


PLUG_API BS_STATUS VNETC_TP_Show(IN UINT ulArgc, IN CHAR ** argv)
{
    EXEC_OutString(" Type  LID      FID      Status\r\n"
                              "--------------------------------\r\n");
    
    TP_Walk(g_hVnetcTp, vnetc_tp_Show, NULL);

    EXEC_OutString("\r\n");

    return BS_OK;
}


