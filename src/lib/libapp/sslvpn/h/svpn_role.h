/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-6-2
* Description: 
* History:     
******************************************************************************/

#ifndef __SVPN_ROLE_H_
#define __SVPN_ROLE_H_

#ifdef __cplusplus
    extern "C" {
#endif 

BS_STATUS SVPN_Role_Init();
HSTRING SVPN_Role_GetACL(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN CHAR *pcRoleName);

#ifdef __cplusplus
    }
#endif 

#endif 


