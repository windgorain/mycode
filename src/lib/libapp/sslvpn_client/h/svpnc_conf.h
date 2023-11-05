/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-6-30
* Description: 
* History:     
******************************************************************************/

#ifndef __SVPNC_CONF_H_
#define __SVPNC_CONF_H_

#ifdef __cplusplus
    extern "C" {
#endif 

typedef enum
{
    SVPNC_CONN_TYPE_SSL = 0,
    SVPNC_CONN_TYPE_TCP
}SVPNC_CONN_TYPE_E;

BS_STATUS SVPNC_SslCtx_Init();

BS_STATUS SVPNC_SetServer(IN CHAR *pcServer);
BS_STATUS SVPNC_SetPort(IN CHAR *pcPort);
BS_STATUS SVPNC_SetConnType(IN CHAR *pcType);
SVPNC_CONN_TYPE_E SVPNC_GetConnType();
CHAR * SVPNC_GetServer();
UINT SVPNC_GetServerIP();
USHORT SVPNC_GetServerPort();
BS_STATUS SVPNC_SetUserName(IN CHAR *pcUserName);
CHAR * SVPNC_GetUserName();
BS_STATUS SVPNC_SetUserPasswdSimple(IN CHAR *pcPassword);
CHAR * SVPNC_GetUserPasswd();
VOID SVPNC_SetCookie(IN CHAR *pcCookie);
CHAR * SVPNC_GetCookie();
VOID * SVPNC_GetSslCtx();

#ifdef __cplusplus
    }
#endif 

#endif 


