/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2015-12-15
* Description: 
* History:     
******************************************************************************/

#ifndef __WEBCENTER_INNER_H_
#define __WEBCENTER_INNER_H_

#include "comp/comp_kfapp.h"
#include "comp/comp_wsapp.h"

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

WS_DELIVER_RET_E WebCenter_RequestIn(IN WS_TRANS_HANDLE hTrans, IN UINT uiEvent);
WS_DELIVER_RET_E WebCenter_Login(IN WS_TRANS_HANDLE hTrans, IN UINT uiEvent);
WS_DELIVER_RET_E WebCenter_Logout(IN WS_TRANS_HANDLE hTrans, IN UINT uiEvent);
WS_DELIVER_RET_E WebCenter_CheckOnline(IN WS_TRANS_HANDLE hTrans, IN UINT uiEvent);
UINT WebCenter_GetOnlineUserID(IN WS_TRANS_HANDLE hWsTrans);
BOOL_T WebCenter_IsPermit(IN WS_TRANS_HANDLE hWsTrans);
BOOL_T WebCenter_IsLoopbackAddr(IN WS_TRANS_HANDLE hWsTrans);
BS_STATUS WebCenter_Deliver_Init();
BS_STATUS WebCenter_Cmd_Save(IN HANDLE hFile);
VOID WebCenter_Deliver_BindService(IN CHAR *pcWsService);
BS_STATUS WebCenter_ULM_Init();
BS_STATUS WebCenter_KF_Init();
BS_STATUS WebCenter_BindWsService(IN CHAR *pcWsService, IN BOOL_T bInner);
CHAR * WebCenter_GetBindedInnerWsService();
CHAR * WebCenter_GetBindedWsService();

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__WEBCENTER_INNER_H_*/


