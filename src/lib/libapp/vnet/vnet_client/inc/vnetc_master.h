/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2012-8-19
* Description: 
* History:     
******************************************************************************/

#ifndef __VNETC_MASTER_H_
#define __VNETC_MASTER_H_

#include "utl/task_master.h"

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

#define VNETC_MASTER_EVENT_STOP     0
#define VNETC_MASTER_EVENT_RESTART  1

BS_STATUS VNETC_Master_Init();

BS_STATUS VNETC_Master_SetEvent
(
    IN UINT uiEventOffset, /* 0-15 */
    IN PF_TASK_MASTER_FUNC pfFunc,
    IN USER_HANDLE_S *pstUserHandle
);

BS_STATUS VNETC_Master_EventInput(IN UINT uiEventOffset);

BS_STATUS VNETC_Master_MsgInput
(
    IN PF_TASK_MASTER_FUNC pfFunc,
    IN USER_HANDLE_S *pstUserHandle
);

VCLOCK_HANDLE VNETC_Master_AddTimer
(
    IN UINT uiTime,  /* ms */
    IN UINT flag,
    IN PF_TIME_OUT_FUNC pfFunc,
    IN USER_HANDLE_S *pstUserHandle
);

VOID VNETC_Master_DelTimer(IN VCLOCK_HANDLE hTimerHandle);

VOID VNETC_Master_RefreshTimer(IN VCLOCK_HANDLE hTimerHandle);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__VNETC_CLOCK_H_*/


