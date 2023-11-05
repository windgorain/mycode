/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2013-4-27
* Description: 
* History:     
******************************************************************************/

#ifndef __WAN_NAT_H_
#define __WAN_NAT_H_

#ifdef __cplusplus
    extern "C" {
#endif 

BS_STATUS WAN_NAT_Init();
BS_STATUS WAN_NAT_SetOutBound(IN UINT uiIfIndex);
VOID WAN_NAT_DelOutBound(IN IF_INDEX ifIndex);
BS_STATUS WAN_NAT_KfInit();

#ifdef __cplusplus
    }
#endif 

#endif 


