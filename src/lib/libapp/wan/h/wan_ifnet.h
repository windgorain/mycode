/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2013-3-19
* Description: 
* History:     
******************************************************************************/

#ifndef __WAN_IFNET_H_
#define __WAN_IFNET_H_

#ifdef __cplusplus
    extern "C" {
#endif 

BS_STATUS WAN_IF_Init();

IF_INDEX WAN_IF_GetIfIndexByEnv(IN VOID *pEnv);

#ifdef __cplusplus
    }
#endif 

#endif 


