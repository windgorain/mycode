/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2012-6-3
* Description: 
* History:     
******************************************************************************/

#ifndef __VNETC_LIB_H_
#define __VNETC_LIB_H_

#include "../vnet_client/inc/vnetc_auth.h"
#include "../vnet_client/inc/vnetc_addr_monitor.h"
#include "../vnet_client/inc/vnetc_api.h"

#ifdef __cplusplus
    extern "C" {
#endif 

typedef VOID (*PF_VNETC_LIB_DESC_NOTIFY)(IN USER_HANDLE_S *pstUserHandle);

BS_STATUS VNETC_LIB_Init();
CHAR * VNETC_LIB_GetVersion();
BS_STATUS VNETC_LIB_SetServerAddress(IN CHAR *pcServer);
BS_STATUS VNETC_LIB_SetDomain(IN CHAR *pcDomain);
CHAR * VNETC_LIB_GetDomain();
BS_STATUS VNETC_LIB_SetUser(IN CHAR *pcUserName, IN CHAR *pcPassword);
CHAR * VNETC_LIB_GetUserName();
CHAR * VNETC_LIB_GetPassword();
BS_STATUS VNETC_LIB_SetDescription(IN CHAR *pcDescription);
CHAR * VNETC_LIB_GetDescription();
BS_STATUS VNETC_LIB_Login();
VOID VNETC_LIB_Logout();
VNET_USER_STATUS_E VNETC_LIB_GetUserStatus();
CHAR * VNETC_LIB_GetUserStatusString();
VNET_USER_STATUS_E VNETC_LIB_GetUserReason();
CHAR * VNETC_LIB_GetUserReasonString();
BS_STATUS VNETC_LIB_SetIfSavePassword(IN BOOL_T bSave);
BOOL_T VNETC_LIB_GetIfSavePassword();
BS_STATUS VNETC_LIB_SetIfAutoLogin(IN BOOL_T bAuto);
BOOL_T VNETC_LIB_GetIfAutoLogin();
BS_STATUS VNETC_LIB_SetIfSelfStart(IN BOOL_T bAuto);
BOOL_T VNETC_LIB_GetIfSelfStart();
VOID VNETC_LIB_SaveConfig();
BS_STATUS VNETC_LIB_RegAddrMonitorNotify
(
    IN PF_VNETC_AddrMonitor_Notify_Func pfFunc,
    IN USER_HANDLE_S *pstUserHandle
);
UINT VNETC_LIB_GetAddrMonitorIP();
UINT VNETC_LIB_GetAddrMonitorMask();

#ifdef __cplusplus
    }
#endif 

#endif 


