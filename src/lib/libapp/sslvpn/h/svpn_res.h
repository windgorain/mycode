/******************************************************************************
* Copyright (C), 2000-2006,  Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2016-9-8
* Description: 
* History:     
******************************************************************************/

#ifndef __SVPN_RES_H_
#define __SVPN_RES_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */


BS_STATUS SVPN_WebRes_Init();
BS_STATUS SVPN_TcpRes_Init();
BS_STATUS SVPN_IPRes_Init();
BS_STATUS SVPN_IPPoolMf_Init();
BS_STATUS SVPN_InnerDNS_Init();

BS_STATUS SVPN_AclCmd_Save(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN HANDLE hFile);
BS_STATUS SVPN_WebResCmd_Save(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN HANDLE hFile);
BS_STATUS SVPN_TcpResCmd_Save(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN HANDLE hFile);
BS_STATUS SVPN_IpResCmd_Save(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN HANDLE hFile);
BS_STATUS SVPN_IpPoolCmd_Save(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN HANDLE hFile);
BS_STATUS SVPN_RoleCmd_Save(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN HANDLE hFile);
BS_STATUS SVPN_LocalUser_Save(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN HANDLE hFile);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__SVPN_RES_H_*/


