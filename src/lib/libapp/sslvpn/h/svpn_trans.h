/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2015-6-10
* Description: 
* History:     
******************************************************************************/

#ifndef __SVPN_TRANS_H_
#define __SVPN_TRANS_H_

#include "utl/cjson.h"

#ifdef __cplusplus
    extern "C" {
#endif 

#define SVPN_ONLINE_USER_LEVEL_USER  1
#define SVPN_ONLINE_USER_LEVEL_ADMIN 2

typedef struct
{
    WS_TRANS_HANDLE hWsTrans;
    SVPN_CONTEXT_HANDLE hSvpnContext;
    UINT uiOnlineUserID;
}SVPN_TRANS_S;

SVPN_TRANS_S * SVPN_Trans_Create(IN WS_TRANS_HANDLE hWsTrans);
VOID SVPN_Trans_Destory(IN WS_TRANS_HANDLE hWsTrans);

#ifdef __cplusplus
    }
#endif 

#endif 


