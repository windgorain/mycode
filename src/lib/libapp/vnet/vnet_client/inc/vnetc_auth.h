/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2009-3-24
* Description: 
* History:     
******************************************************************************/

#ifndef __VNETC_AUTH_H_
#define __VNETC_AUTH_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

BS_STATUS VNETC_AUTH_Init();
BS_STATUS VNETC_AUTH_StartAuth();
VOID VNETC_AUTH_Logout();

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__VNETC_AUTH_H_*/


