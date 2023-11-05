/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-2-18
* Description: 
* History:     
******************************************************************************/

#ifndef __VNETS_WEB_ULM_H_
#define __VNETS_WEB_ULM_H_

#include "utl/ulm_utl.h"

#ifdef __cplusplus
    extern "C" {
#endif 

UINT VNETS_WebUlm_Add(IN CHAR *pcUserName);
BS_STATUS VNETS_WebUlm_Del(IN UINT uiUserID);
BS_STATUS VNETS_WebUlm_GetCookie(IN UINT uiUserID, OUT CHAR szUserCookie[ULM_USER_COOKIE_LEN + 1]);
UINT VNETS_WebUlm_GetUserIDByCookie(IN CHAR *pcCookie);
BS_STATUS VNETS_WebUlm_GetUserInfo(IN UINT uiUserID, OUT ULM_USER_INFO_S *pstUserInfo);

#ifdef __cplusplus
    }
#endif 

#endif 


