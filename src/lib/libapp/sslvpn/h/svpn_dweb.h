/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2014-9-5
* Description: 
* History:     
******************************************************************************/

#ifndef __SVPN_DWEB_H_
#define __SVPN_DWEB_H_

#include "utl/cjson.h"
#include "utl/ws_utl.h"

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

typedef struct
{
    WS_TRANS_HANDLE hWsTrans;
    SVPN_CONTEXT_HANDLE hSvpnContext;
    UINT uiOnlineUserID;
    cJSON * pstJson;
}SVPN_DWEB_S;

BS_STATUS SVPN_DWEB_Init();
WS_DELIVER_RET_E SVPN_DWeb_RequestIn(IN WS_TRANS_HANDLE hTrans, IN UINT uiEvent);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__SVPN_DWEB_H_*/


