/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2015-12-3
* Description: 
* History:     
******************************************************************************/

#ifndef __WSAPP_CFGLOCK_H_
#define __WSAPP_CFGLOCK_H_

#ifdef __cplusplus
    extern "C" {
#endif 

VOID WSAPP_CfgLock_Init();
VOID WSAPP_CfgLock_RLock();
VOID WSAPP_CfgLock_RUnLock();
VOID WSAPP_CfgLock_WLock();
VOID WSAPP_CfgLock_WUnLock();

#ifdef __cplusplus
    }
#endif 

#endif 


