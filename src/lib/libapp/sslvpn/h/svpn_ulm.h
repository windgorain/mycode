/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2015-6-16
* Description: 
* History:     
******************************************************************************/

#ifndef __SVPN_ULM_H_
#define __SVPN_ULM_H_

#include "utl/ulm_utl.h"
#include "utl/string_utl.h"

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

BS_STATUS SVPN_ULM_Init();
UINT SVPN_ULM_AddUser(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN CHAR *pcUserName, IN UINT uiUserType);
BS_STATUS SVPN_ULM_SetUserRole(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN UINT uiUserID, IN CHAR *pcRoles);
BS_STATUS SVPN_ULM_GetUserCookie
(
    IN SVPN_CONTEXT_HANDLE hSvpnContext,
    IN UINT uiUserID,
    OUT CHAR szUserCookie[ULM_USER_COOKIE_LEN + 1]
);
UINT SVPN_ULM_GetUserIDByCookie(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN CHAR *pcCookie);
UINT SVPN_ULM_GetUserType(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN UINT uiOlUserID);
BS_STATUS SVPN_ULM_GetUserName
(
    IN SVPN_CONTEXT_HANDLE hSvpnContext,
    IN UINT uiUserID,
    OUT CHAR *pcUserName
);
HSTRING SVPN_ULM_GetUserRoles
(
    IN SVPN_CONTEXT_HANDLE hSvpnContext,
    IN UINT uiUserID
);

BOOL_T SVPN_ULM_CheckWebProxyPermit(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN UINT uiOnlineID, IN CHAR *pcUrl);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__SVPN_ULM_H_*/


