/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2015-9-6
* Description: 
* History:     
******************************************************************************/

#ifndef __SVPN_WEB_PROXY_H_
#define __SVPN_WEB_PROXY_H_

#ifdef __cplusplus
    extern "C" {
#endif 

BS_STATUS SVPN_WebProxy_Init();

WS_DELIVER_RET_E SVPN_WebProxy_RequestIn(IN WS_TRANS_HANDLE hTrans, IN UINT uiEvent);

WS_DELIVER_RET_E SVPN_WebProxy_ReferIn(IN WS_TRANS_HANDLE hTrans, IN UINT uiEvent);

WS_DELIVER_RET_E SVPN_WebProxy_AuthIn(IN WS_TRANS_HANDLE hTrans, IN UINT uiEvent);


#ifdef __cplusplus
    }
#endif 

#endif 


