/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-7-14
* Description: 
* History:     
******************************************************************************/

#ifndef __WAN_BRIDGE_H_
#define __WAN_BRIDGE_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

BS_STATUS WAN_Bridge_Init();
MAC_ADDR_S * WAN_Bridge_GetMac();

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__WAN_BRIDGE_H_*/


