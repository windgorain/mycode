/******************************************************************************
* Copyright (C), 2000-2006,  Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2017-5-25
* Description: 
* History:     
******************************************************************************/

#ifndef __SUPPORT_BRIDGE_H_
#define __SUPPORT_BRIDGE_H_

#include "utl/eth_utl.h"

#ifdef __cplusplus
    extern "C" {
#endif 

VOID SupportBridge_Init();
MAC_ADDR_S * SupportBridge_GetMac();
VOID SupportBridge_IoctlGetMac(OUT MAC_ADDR_S *pstMac);

#ifdef __cplusplus
    }
#endif 

#endif 


