

#include "bs.h"

#include "utl/tp_utl.h"
#include "utl/txt_utl.h"
#include "utl/exec_utl.h"

#include "../../inc/vnet_conf.h"
#include "../../inc/vnet_tp.h"
#include "../../inc/vnet_node.h"

#include "../inc/vnets_protocol.h"
#include "../inc/vnets_domain.h"
#include "../inc/vnets_ses.h"
#include "../inc/vnets_logout.h"
#include "../inc/vnets_mac_tbl.h"
#include "../inc/vnets_enter_domain.h"
#include "../inc/vnets_dns_quest_list.h"
#include "../inc/vnets_vpn_link.h"
#include "../inc/vnets_master.h"
#include "../inc/vnets_tp.h"
#include "../inc/vnets_node_fwd.h"

#define _VNETS_TP_TIME_OUT_TIME  1000
#define _VNETS_TP_KEEP_ALIVE_IDLE   600   
#define _VNETS_TP_KEEP_ALIVE_INTVAL 10
#define _VNETS_TP_KEEP_ALIVE_MAX_COUNT 6

static TP_HANDLE g_hVnetsTp;
static TP_ID g_hVnetsServerTpId;

static BS_STATUS vnets_tp_Send
(
    IN TP_HANDLE hTpHandle,
    IN TP_CHANNEL_S *pstChannel,
    IN MBUF_S *pstMbuf,
    IN USER_HANDLE_S *pstUserHandle
)
{
    UINT uiSesId;
    UINT uiNID;

    if (pstChannel == NULL)
    {
        MBUF_Free(pstMbuf);
        return BS_BAD_PARA;
    }

    if (NULL == VNETS_Context_AddContextToMbuf(pstMbuf))
    {
        MBUF_Free(pstMbuf);
        return BS_NO_MEMORY;
    }

    uiNID = VNETS_Context_GetDstNodeID(pstMbuf);
    if (uiNID == 0)
    {
        uiSesId = HANDLE_UINT(pstChannel->ahChannelDesc[0]);
        return VNETS_NodeFwd_OutputBySes(uiSesId, pstMbuf, VNET_NODE_PKT_PROTO_TP);
    }
    else
    {
        return VNETS_NodeFwd_Output(uiNID, pstMbuf, VNET_NODE_PKT_PROTO_TP);
    }
}

static BS_STATUS vnets_tp_Accept
(
    IN TP_HANDLE hTpHandle,
    IN TP_ID uiTpId
)
{
    TP_ID uiAcceptTpId;

    if (BS_OK != TP_Accept(hTpHandle, uiTpId, &uiAcceptTpId))
    {
        return BS_ERR;
    }

    return BS_OK;
}

static BS_STATUS vnets_tp_Err
(
    IN TP_HANDLE hTpHandle,
    IN TP_ID uiTpId
)
{
    TP_Close(hTpHandle, uiTpId);

    return BS_OK;
}

static BS_STATUS vnets_tp_Event
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
        case TP_EVENT_ACCEPT:
        {
            vnets_tp_Accept(hTpHandle, uiTpId);
            break;
        }

        case TP_EVENT_READ:
        {
            pstMbuf = TP_RecvMbuf(hTpHandle, uiTpId);
            if (NULL != pstMbuf)
            {
                VNETS_Protocol_Input(uiTpId, pstMbuf);
            }
            break;
        }

        case TP_EVENT_ERR:
        {
            vnets_tp_Err(hTpHandle, uiTpId);
            break;
        }

        default:
        {
            break;
        }
    }

    return BS_OK;
}

static VOID vnets_tp_TimeOut(IN HANDLE hTimerHandle, IN USER_HANDLE_S *pstUserHandle)
{
    TP_TimerStep(g_hVnetsTp);
}

BS_STATUS VNETS_TP_Input(IN MBUF_S *pstMbuf)
{
    UINT uiSesId;
    BS_STATUS eRet;
    TP_CHANNEL_S stChannel;

    uiSesId = VNETS_Context_GetRecvSesID(pstMbuf);

    stChannel.ahChannelDesc[0] = UINT_HANDLE(uiSesId);

    eRet = TP_PktInput(g_hVnetsTp, pstMbuf, &stChannel);

    return eRet;
}

BS_STATUS VNETS_TP_Output(IN UINT uiTpId, IN MBUF_S *pstMbuf)
{
    return TP_SendMbuf(g_hVnetsTp, uiTpId, pstMbuf);
}

BS_STATUS VNETS_TP_Init()
{
    TP_HANDLE hTpHandle;
    TP_ID uiTpId;
    
    hTpHandle = TP_Create(vnets_tp_Send, vnets_tp_Event, NULL, VNETS_TP_PROPERTY_MAX);
    if (NULL == hTpHandle)
    {
        return BS_NO_MEMORY;
    }

    TP_SetDftKeepAlive(hTpHandle, _VNETS_TP_KEEP_ALIVE_IDLE, _VNETS_TP_KEEP_ALIVE_INTVAL, _VNETS_TP_KEEP_ALIVE_MAX_COUNT);

    uiTpId = TP_Socket(hTpHandle, TP_TYPE_PROTOCOL);
    if (0 == uiTpId)
    {
        TP_Destory(hTpHandle);
        return BS_NO_MEMORY;
    }

    if (BS_OK != TP_Bind(hTpHandle, uiTpId, VNET_TP_SERVER_PROTOCOL_ID))
    {
        TP_Close(hTpHandle, uiTpId);
        TP_Destory(hTpHandle);
        return BS_ERR;
    }

    TP_Listen(hTpHandle, uiTpId);

    g_hVnetsTp = hTpHandle;
    g_hVnetsServerTpId = uiTpId;

    return BS_OK;
}

BS_STATUS VNETS_TP_Init2()
{
    if (NULL == VNETS_Master_AddTimer(_VNETS_TP_TIME_OUT_TIME, TRUE, vnets_tp_TimeOut, NULL))
    {
        return BS_ERR;
    }

    return BS_OK;
}

BS_STATUS VNETS_TP_RegCloseEvent(IN PF_TP_CLOSE_NOTIFY_FUNC pfFunc, IN USER_HANDLE_S * pstUserHandle)
{
    return TP_RegCloseNotifyEvent(g_hVnetsTp, pfFunc, pstUserHandle);
}

BS_STATUS VNETS_TP_SetProperty(IN UINT uiTPID, IN UINT uiIndex, IN HANDLE hValue)
{
    return TP_SetProperty(g_hVnetsTp, uiTPID, uiIndex, hValue);
}

HANDLE VNETS_TP_GetProperty(IN UINT uiTPID, IN UINT uiIndex)
{
    return TP_GetProperty(g_hVnetsTp, uiTPID, uiIndex);
}

PLUG_API BS_STATUS VNETS_TP_DebugPkt(IN UINT ulArgc, IN CHAR ** argv)
{
    TP_SetDbgFlag(g_hVnetsTp, TP_DBG_FLAG_PKT);

    return BS_OK;
}

PLUG_API BS_STATUS VNETS_TP_NoDebugPkt(IN UINT ulArgc, IN CHAR ** argv)
{
    TP_ClrDbgFlag(g_hVnetsTp, TP_DBG_FLAG_PKT);

    return BS_OK;
}

static VOID vnets_tp_Show(IN TP_HANDLE hTpHandle, IN TP_ID uiTpId, IN HANDLE hUserHandle)
{
    TP_STATE_S stState;

    TP_GetState(hTpHandle, uiTpId, &stState);
    
    EXEC_OutInfo(
        " %-5s %-8x %-8x %-9s\r\n",
        stState.eType == TP_TYPE_PROTOCOL ? "Proto" : "Link",
        stState.uiLocalTpId,
        stState.uiPeerTpId,
        TP_GetStatusString(stState.uiStatus)
        );
}


PLUG_API BS_STATUS VNETS_TP_Show(IN UINT ulArgc, IN CHAR ** argv)
{
    EXEC_OutString(" Type  LID      FID      Status\r\n"
                              "----------------------------------\r\n");
    
    TP_Walk(g_hVnetsTp, vnets_tp_Show, NULL);

    EXEC_OutString("\r\n");

    return BS_OK;
}

