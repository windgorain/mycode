/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2015-6-1
* Description: 
* History:     
******************************************************************************/

#ifndef __SVPN_LOCAL_USER_H_
#define __SVPN_LOCAL_USER_H_

#include "utl/string_utl.h"

#ifdef __cplusplus
    extern "C" {
#endif 

BS_STATUS SVPN_LocalUser_Init();

BS_STATUS SVPN_LocalUser_AddUser(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN CHAR *pcUserName, IN CHAR *pcPassword);
VOID SVPN_LocalUser_DelUser(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN CHAR *pcUserName);
BOOL_T SVPN_LocalUser_Check(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN CHAR *pcUserName, IN CHAR *pcPassword);
BOOL_T SVPN_LocalUser_IsExist(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN CHAR *pcUserName);
BS_STATUS SVPN_LocalUser_GetNext
(
    IN SVPN_CONTEXT_HANDLE hSvpnContext,
    IN CHAR *pcCurrentUserName,
    OUT CHAR szNextUserName[SVPN_MAX_USER_NAME_LEN + 1]
);
HSTRING SVPN_LocalUser_GetRoleAsHString(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN CHAR *pcUserName);
BS_STATUS SVPN_LocalUser_SetRole(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN CHAR *pcUserName, IN CHAR *pcRole);
BS_STATUS SVPN_LocalAdmin_AddUser(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN CHAR *pcUserName, IN CHAR *pcPassword);
VOID SVPN_LocalAdmin_DelUser(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN CHAR *pcUserName);
BS_STATUS SVPN_LocalAdmin_SetUser(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN CHAR *pcUserName, IN CHAR *pcPassword);
BOOL_T SVPN_LocalAdmin_Check(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN CHAR *pcUserName, IN CHAR *pcPassword);
BOOL_T SVPN_LocalAdmin_IsExist(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN CHAR *pcUserName);
BS_STATUS SVPN_LocalAdmin_GetNext
(
    IN SVPN_CONTEXT_HANDLE hSvpnContext,
    IN CHAR *pcCurrentUserName,
    OUT CHAR szNextUserName[SVPN_MAX_USER_NAME_LEN + 1]
);
 
#ifdef __cplusplus
    }
#endif 

#endif 


