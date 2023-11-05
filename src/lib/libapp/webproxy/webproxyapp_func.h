/******************************************************************************
* Copyright (C), 2000-2006,  Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2016-2-21
* Description: 
* History:     
******************************************************************************/

#ifndef __WEBPROXYAPP_FUNC_H_
#define __WEBPROXYAPP_FUNC_H_

#ifdef __cplusplus
    extern "C" {
#endif 

BS_STATUS WebProxyApp_Deliver_Init();
BS_STATUS WebProxyApp_Main_Init();
BS_STATUS WebProxyApp_Cmd_Save(IN HANDLE hFile);
VOID WebProxyApp_Deliver_BindService(IN CHAR *pcWsService);


#ifdef __cplusplus
    }
#endif 

#endif 


