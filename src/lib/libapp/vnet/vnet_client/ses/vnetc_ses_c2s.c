/******************************************************************************
* Copyright (C),    LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2013-12-26
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/mbuf_utl.h"

#include "../inc/vnetc_phy.h"
#include "../inc/vnetc_ses.h"
#include "../inc/vnetc_caller.h"
#include "../inc/vnetc_user_status.h"
#include "../inc/vnetc_fsm.h"
#include "../inc/vnetc_ses_c2s.h"


#define VNETC_SES_KEEPALIVE_IDLE 300
#define VNETC_SES_KEEPALIVE_INTVAL 3
#define VNETC_SES_KEEPALIVE_MAX_TRYS 5

static UINT g_uiVnetcSesC2s = 0;
static VNETC_PHY_CONTEXT_S g_stVnetcSesC2SPhyContext;
static BOOL_T g_bVnetcSesC2sTryConnFroever = FALSE;

static BS_STATUS vnetc_sesc2s_EventNotify(IN UINT uiSesID, IN UINT uiEvent, IN USER_HANDLE_S *pstUserHandle)
{
    switch (uiEvent)
    {
        case SES_EVENT_CONNECT:
        {
            g_bVnetcSesC2sTryConnFroever = TRUE;
            VNETC_FSM_EventHandle(VNETC_FSM_EVENT_SES_OK);
            break;
        }

        case SES_EVENT_CONNECT_FAILED:
        case SES_EVENT_PEER_CLOSED:
        {
            VNETC_SES_Close(uiSesID);

            g_uiVnetcSesC2s = 0;

            if (g_bVnetcSesC2sTryConnFroever == TRUE)
            {
                if (BS_OK != VNETC_SesC2S_Connect())
                {
                    VNETC_FSM_EventHandle(VNETC_FSM_EVENT_SES_FAILED);
                }
            }
            else
            {
                VNETC_User_SetStatus(VNET_USER_STATUS_OFFLINE, VNET_USER_REASON_CONNECT_FAILED);
                VNETC_FSM_EventHandle(VNETC_FSM_EVENT_SES_FAILED);
            }

            break;
        }

        default:
        {
            break;
        }
    }

    return BS_OK;
}

BS_STATUS VNETC_SesC2S_Connect()
{
    SES_OPT_KEEP_ALIVE_TIME_S stTime;

    if (g_uiVnetcSesC2s != 0)
    {
        VNETC_FSM_EventHandle(VNETC_FSM_EVENT_SES_OK);
        return BS_OK;
    }

    VNETC_User_SetStatus(VNET_USER_STATUS_CONNECTING, VNET_USER_REASON_NONE);
    
    g_uiVnetcSesC2s = VNETC_SES_CreateClient(&g_stVnetcSesC2SPhyContext);
    if (0 == g_uiVnetcSesC2s)
    {
        VNETC_User_SetStatus(VNET_USER_STATUS_OFFLINE, VNET_USER_REASON_NO_RESOURCE);
        return BS_ERR;
    }

    stTime.usIdle = VNETC_SES_KEEPALIVE_IDLE;
    stTime.usIntval = VNETC_SES_KEEPALIVE_INTVAL;
    stTime.usMaxProbeCount = VNETC_SES_KEEPALIVE_MAX_TRYS;
    
    VNETC_SES_SetOpt(g_uiVnetcSesC2s, SES_OPT_KEEP_ALIVE_TIME, &stTime);

    VNETC_SES_SetEventNotify(g_uiVnetcSesC2s, vnetc_sesc2s_EventNotify, NULL);

    if (BS_OK != VNETC_SES_Connect(g_uiVnetcSesC2s))
    {
        VNETC_User_SetStatus(VNET_USER_STATUS_OFFLINE, VNET_USER_REASON_CONNECT_FAILED);
        VNETC_SES_Close(g_uiVnetcSesC2s);
        g_uiVnetcSesC2s = 0;
        return BS_ERR;
    }

    return BS_OK;
}

BS_STATUS VNETC_SesC2S_Init()
{
    return BS_OK;
}

VOID VNETC_SesC2S_SetPhyContext(IN VNETC_PHY_CONTEXT_S *pstPhyContext)
{
    g_stVnetcSesC2SPhyContext = *pstPhyContext;
}

UINT VNETC_SesC2S_GetSesId()
{
    return g_uiVnetcSesC2s;
}

VOID VNETC_SesC2S_Close()
{
    if (g_uiVnetcSesC2s != 0)
    {
        VNETC_SES_Close(g_uiVnetcSesC2s);
        g_uiVnetcSesC2s = 0;
    }
}

