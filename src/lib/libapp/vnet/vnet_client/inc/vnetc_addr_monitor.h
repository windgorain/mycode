/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2012-11-3
* Description: 
* History:     
******************************************************************************/

#ifndef __VNETC_ADDR_MONITOR_H_
#define __VNETC_ADDR_MONITOR_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

typedef VOID (*PF_VNETC_AddrMonitor_Notify_Func)(IN USER_HANDLE_S *pstUserHandle);

BS_STATUS VNETC_AddrMonitor_Init();
UINT VNETC_AddrMonitor_GetIP();
UINT VNETC_AddrMonitor_GetMask();
BS_STATUS VNETC_AddrMonitor_RegNotify
(
    IN PF_VNETC_AddrMonitor_Notify_Func pfFunc,
    IN USER_HANDLE_S *pstUserHandle
);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__VNETC_ADDR_MONITOR_H_*/


