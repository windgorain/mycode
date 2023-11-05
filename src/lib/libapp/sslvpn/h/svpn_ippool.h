/******************************************************************************
* Copyright (C), 2000-2006,  Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2016-7-17
* Description: 
* History:     
******************************************************************************/

#ifndef __SVPN_IPPOOL_H_
#define __SVPN_IPPOOL_H_

#ifdef __cplusplus
    extern "C" {
#endif 

BS_STATUS SVPN_IPPOOL_Init();
BS_STATUS SVPN_IPPOOL_AddRange
(
    IN SVPN_CONTEXT_HANDLE hSvpnContext,
    IN UINT uiStartIP,
    IN UINT uiEndIP
);
VOID SVPN_IPPOOL_DelRange
(
    IN SVPN_CONTEXT_HANDLE hSvpnContext,
    IN UINT uiStartIP,
    IN UINT uiEndIP
);

BS_STATUS SVPN_IPPOOL_ModifyRange
(
    IN SVPN_CONTEXT_HANDLE hSvpnContext,
    IN UINT uiOldStartIP,
    IN UINT uiOldEndIP,
    IN UINT uiStartIP,
    IN UINT uiEndIP
);


UINT SVPN_IPPOOL_AllocIP(IN SVPN_CONTEXT_HANDLE hSvpnContext);

VOID SVPN_IPPOOL_FreeIP(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN UINT uiIP);


#ifdef __cplusplus
    }
#endif 

#endif 


