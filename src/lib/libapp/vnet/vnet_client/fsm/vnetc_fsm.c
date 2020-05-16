/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2014-5-22
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/fsm_utl.h"

#include "../inc/vnetc_tp.h"
#include "../inc/vnetc_auth.h"
#include "../inc/vnetc_enter_domain.h"
#include "../inc/vnetc_fsm.h"
#include "../inc/vnetc_phy.h"
#include "../inc/vnetc_ses_c2s.h"

static BS_STATUS vnetc_fsm_StartSesC2S(IN FSM_S *pstFsm, IN UINT uiEvent);
static BS_STATUS vnetc_fsm_StartAuth(IN FSM_S *pstFsm, IN UINT uiEvent);
static BS_STATUS vnetc_fsm_StartEnterDomain(IN FSM_S *pstFsm, IN UINT uiEvent);
static BS_STATUS vnetc_fsm_StartTP(IN FSM_S *pstFsm, IN UINT uiEvent);
static BS_STATUS vnetc_fsm_TriggerTPKeepAlive(IN FSM_S *pstFsm, IN UINT uiEvent);


static FSM_STATE_MAP_S g_astVnetcFsmStateMap[VNETC_FSM_STATE_MAX] = 
{
    {"S.Init", VNETC_FSM_STATE_INIT},
    {"S.Ses", VNETC_FSM_STATE_SES},
    {"S.TP", VNETC_FSM_STATE_TP},
    {"S.TpForEver", VNETC_FSM_STATE_TP_FOREVER},
    {"S.Auth", VNETC_FSM_STATE_AUTH},
    {"S.EnterDomain", VNETC_FSM_STATE_ENTER_DOMAIN},
    {"S.OutDomain", VNETC_FSM_STATE_OUT_DOMAIN},
    {"S.Running", VNETC_FSM_STATE_RUNNING}
};

static FSM_EVENT_MAP_S g_astVnetcFsmEventMap[VNETC_FSM_EVENT_MAX] = 
{
    {"E.Start", VNETC_FSM_EVENT_START},
    {"E.SesFailed", VNETC_FSM_EVENT_SES_FAILED},
    {"E.SesOK", VNETC_FSM_EVENT_SES_OK},
    {"E.TpFailed", VNETC_FSM_EVENT_TP_FAILED},
    {"E.TpOK", VNETC_FSM_EVENT_TP_OK},
    {"E.AuthFailed", VNETC_FSM_EVENT_AUTH_FAILED},
    {"E.AuthOK", VNETC_FSM_EVENT_AUTH_OK},
    {"E.EnterDomainFailed", VNETC_FSM_EVENT_ENTER_DOMAIN_FAILED},
    {"E.EnterDomainOK", VNETC_FSM_EVENT_ENTER_DOMAIN_OK},
    {"E.KickOutDomain", VNETC_FSM_EVENT_KICK_OUT_DOMAIN},
    {"E.RebootDomain", VNETC_FSM_EVENT_REBOOT_DOMAIN},
    {"E.ReAuth", VNETC_FSM_EVENT_REAUTH}
};


static FSM_SWITCH_MAP_S g_astVnetcFsmSwichMap[] =
{
    {"S.Init", "E.Start", "S.Ses", vnetc_fsm_StartSesC2S},

    {"S.Ses", "E.SesFailed", "S.Init", NULL},
    {"S.Ses", "E.SesOK", "S.TP", vnetc_fsm_StartTP},

    {"S.TP", "E.TpFailed", "S.Init", NULL},
    {"S.TP,S.TpForEver", "E.TpOK", "S.Auth", vnetc_fsm_StartAuth},

    {"S.Auth", "E.AuthFailed", "S.Init", NULL},
    {"S.Auth", "E.AuthOK", "S.EnterDomain", vnetc_fsm_StartEnterDomain},

    {"S.EnterDomain", "E.EnterDomainFailed", "S.OutDomain", NULL},
    {"S.EnterDomain", "E.EnterDomainOK", "S.Running", NULL},

    {"S.EnterDomain,S.Running", "E.ReAuth", "S.Auth", vnetc_fsm_StartAuth},
    {"S.EnterDomain,S.Running", "E.KickOutDomain", "S.OutDomain", NULL},
    {"S.EnterDomain,S.Running", "E.RebootDomain", "S.EnterDomain", vnetc_fsm_StartEnterDomain},

    {"S.Auth,S.EnterDomain,S.Running", "E.TpFailed", "S.TpForEver", vnetc_fsm_StartTP},
    {"S.TpForEver", "E.TpFailed", FSM_STATE_NO_CHANGE_STRING, vnetc_fsm_StartTP},

    {"S.Running", "E.SesOK", FSM_STATE_NO_CHANGE_STRING, vnetc_fsm_TriggerTPKeepAlive},
};

static FSM_SWITCH_TBL g_hVnetcFsmSwitchTbl;
static FSM_S g_stVnetcFsm;

static BS_STATUS vnetc_fsm_StartSesC2S(IN FSM_S *pstFsm, IN UINT uiEvent)
{
    if(BS_OK != VNETC_SesC2S_Connect())
    {
        VNETC_FSM_EventHandle(VNETC_FSM_EVENT_SES_FAILED);
    }

    return BS_OK;
}

static BS_STATUS vnetc_fsm_StartTP(IN FSM_S *pstFsm, IN UINT uiEvent)
{
    if (BS_OK != VNETC_TP_ConnectServer())
    {
        VNETC_FSM_EventHandle(VNETC_FSM_EVENT_TP_FAILED);
    }

    return BS_OK;
}

static BS_STATUS vnetc_fsm_TriggerTPKeepAlive(IN FSM_S *pstFsm, IN UINT uiEvent)
{
    VNETC_TP_TriggerKeepAlive();

    return BS_OK;
}

static BS_STATUS vnetc_fsm_StartAuth(IN FSM_S *pstFsm, IN UINT uiEvent)
{
    if (BS_OK != VNETC_AUTH_StartAuth())
    {
        VNETC_FSM_EventHandle(VNETC_FSM_EVENT_AUTH_FAILED);
    }

    return BS_OK;
}

static BS_STATUS vnetc_fsm_StartEnterDomain(IN FSM_S *pstFsm, IN UINT uiEvent)
{
    if (BS_OK != VNETC_EnterDomain_Start())
    {
        VNETC_FSM_EventHandle(VNETC_FSM_EVENT_ENTER_DOMAIN_FAILED);
    }

    return BS_OK;
}


VOID VNETC_FSM_ChangeState(IN UINT uiState)
{
    FSM_SetState(&g_stVnetcFsm, uiState);
}

VOID VNETC_FSM_EventHandle(IN UINT uiEvent)
{
    FSM_EventHandle(&g_stVnetcFsm, uiEvent);
}

BS_STATUS VNETC_FSM_RegStateListener(IN PF_FSM_STATE_LISTEN pfListenFunc, IN USER_HANDLE_S *pstUserHandle/* 可以为NULL */)
{
    return FSM_RegStateListener(g_hVnetcFsmSwitchTbl, pfListenFunc, pstUserHandle);
}

BS_STATUS VNETC_FSM_Init()
{
    g_hVnetcFsmSwitchTbl = FSM_CreateSwitchTbl(g_astVnetcFsmStateMap, VNETC_FSM_STATE_MAX,
                                   g_astVnetcFsmEventMap, VNETC_FSM_EVENT_MAX,
                                   g_astVnetcFsmSwichMap, sizeof(g_astVnetcFsmSwichMap)/sizeof(FSM_SWITCH_MAP_S));
    if (NULL == g_hVnetcFsmSwitchTbl)
    {
        return BS_NO_MEMORY;
    }

    FSM_Init(&g_stVnetcFsm, g_hVnetcFsmSwitchTbl);
    FSM_InitState(&g_stVnetcFsm, VNETC_FSM_STATE_INIT);

    return BS_OK;
}

/* debug fsm */
PLUG_API BS_STATUS VNETC_FSM_Debug(IN UINT ulArgc, IN CHAR **argv)
{
    FSM_SetDbgFlag(&g_stVnetcFsm, FSM_DBG_FLAG_ALL);

    return BS_OK;
}

/* no debug fsm */
PLUG_API BS_STATUS VNETC_FSM_NoDebug(IN UINT ulArgc, IN CHAR **argv)
{
    FSM_ClrDbgFlag(&g_stVnetcFsm, FSM_DBG_FLAG_ALL);

    return BS_OK;
}

