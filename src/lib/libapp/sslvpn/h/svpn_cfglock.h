/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2014-11-20
* Description: 
* History:     
******************************************************************************/

#ifndef __SVPN_CFGLOCK_H_
#define __SVPN_CFGLOCK_H_

#ifdef __cplusplus
    extern "C" {
#endif 

VOID SVPN_CfgLock_Init();
VOID SVPN_CfgLock_RLock();
VOID SVPN_CfgLock_RUnLock();
VOID SVPN_CfgLock_WLock();
VOID SVPN_CfgLock_WUnLock();


#ifdef __cplusplus
    }
#endif 

#endif 


