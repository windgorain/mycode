/******************************************************************************
* Copyright (C), 2000-2006,  Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2016-9-8
* Description: 
* History:     
******************************************************************************/

#ifndef __SVPNC_FUNC_H_
#define __SVPNC_FUNC_H_

#ifdef __cplusplus
    extern "C" {
#endif 

BS_STATUS SVPNC_CMD_Init();
BS_STATUS SVPNC_Login();
BS_STATUS SVPNC_TcpRelay_Start();
BS_STATUS SVPNC_IpTunnel_Start();
BS_STATUS SVPNC_TR_MainInit();
BS_STATUS SVPNC_TRService_Init();
BS_STATUS SVPNC_KF_Init();

#ifdef __cplusplus
    }
#endif 

#endif 


