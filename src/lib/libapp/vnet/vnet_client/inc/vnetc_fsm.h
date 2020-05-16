/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2014-5-25
* Description: 
* History:     
******************************************************************************/

#ifndef __VNETC_FSM_H_
#define __VNETC_FSM_H_

#include "utl/fsm_utl.h"

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

typedef enum
{
    VNETC_FSM_STATE_INIT = 0,
    VNETC_FSM_STATE_SES,
    VNETC_FSM_STATE_TP,
    VNETC_FSM_STATE_TP_FOREVER,
    VNETC_FSM_STATE_AUTH,
    VNETC_FSM_STATE_ENTER_DOMAIN,
    VNETC_FSM_STATE_OUT_DOMAIN,
    VNETC_FSM_STATE_RUNNING,

    VNETC_FSM_STATE_MAX
}VNETC_FSM_STATE_E;

typedef enum
{
    VNETC_FSM_EVENT_START = 0,
    VNETC_FSM_EVENT_SES_FAILED,
    VNETC_FSM_EVENT_SES_OK,
    VNETC_FSM_EVENT_TP_FAILED,
    VNETC_FSM_EVENT_TP_OK,
    VNETC_FSM_EVENT_AUTH_FAILED,
    VNETC_FSM_EVENT_AUTH_OK,
    VNETC_FSM_EVENT_ENTER_DOMAIN_FAILED,
    VNETC_FSM_EVENT_ENTER_DOMAIN_OK,
    VNETC_FSM_EVENT_KICK_OUT_DOMAIN,
    VNETC_FSM_EVENT_REBOOT_DOMAIN,
    VNETC_FSM_EVENT_REAUTH,

    VNETC_FSM_EVENT_MAX
}VNETC_FSM_EVENT_E;

BS_STATUS VNETC_FSM_Init();
VOID VNETC_FSM_EventHandle(IN UINT uiEvent);
VOID VNETC_FSM_ChangeState(IN UINT uiState);
BS_STATUS VNETC_FSM_RegStateListener(IN PF_FSM_STATE_LISTEN pfListenFunc, IN USER_HANDLE_S *pstUserHandle/* 可以为NULL */);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__VNETC_FSM_H_*/


