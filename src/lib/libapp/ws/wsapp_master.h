/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2015-12-2
* Description: 
* History:     
******************************************************************************/

#ifndef __WSAPP_MASTER_H_
#define __WSAPP_MASTER_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

BS_STATUS WSAPP_Master_Init();
BS_STATUS WSAPP_Master_AddGW(IN WSAPP_GW_S *pstGW, IN UINT uiIp, IN USHORT usPort);
VOID WSAPP_Master_DelGW(IN WSAPP_GW_S *pstGW);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__WSAPP_MASTER_H_*/


