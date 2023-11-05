/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-7-9
* Description: 
* History:     
******************************************************************************/

#ifndef __SVPN_IPTUNNEL_H_
#define __SVPN_IPTUNNEL_H_

#include "comp/comp_if.h"

#ifdef __cplusplus
    extern "C" {
#endif 

BS_STATUS SVPN_IpTunNode_Init();
BS_STATUS SVPN_IpTunnel_Init();
WS_DELIVER_RET_E SVPN_IpTunnel_RequestIn(IN WS_TRANS_HANDLE hTrans, IN UINT uiEvent);
IF_INDEX SVPN_IpTunnel_GetInterface();

#ifdef __cplusplus
    }
#endif 

#endif 


