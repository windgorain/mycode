/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2013-4-3
* Description: 
* History:     
******************************************************************************/

#ifndef __WAN_VRF_CMD_H_
#define __WAN_VRF_CMD_H_

#ifdef __cplusplus
    extern "C" {
#endif 

BS_STATUS WAN_VFCmd_Init();
UINT WAN_VrfCmd_GetVrfByEnv(IN VOID *pEnv);
VOID WAN_VrfCmd_Save(IN HANDLE hFile);

#ifdef __cplusplus
    }
#endif 

#endif 


