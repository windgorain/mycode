/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2012-8-17
* Description: 
* History:     
******************************************************************************/

#ifndef __VNETS_MASTER_H_
#define __VNETS_MASTER_H_

#include "utl/task_master.h"

#ifdef __cplusplus
    extern "C" {
#endif 

BS_STATUS VNETS_Master_Init();
BS_STATUS VNETS_Master_SetEvent
(
    IN UINT uiEventOffset, 
    IN PF_TASK_MASTER_FUNC pfFunc,
    IN USER_HANDLE_S *pstUserHandle
);
BS_STATUS VNETS_Master_EventInput(IN UINT uiEventOffset);

BS_STATUS VNETS_Master_MsgInput(PF_TASK_MASTER_FUNC pfFunc, void *ud);

VCLOCK_HANDLE VNETS_Master_AddTimer
(
    IN UINT uiTime,
    IN UINT flag,
    IN PF_TIME_OUT_FUNC pfFunc,
    IN USER_HANDLE_S *pstUserHandle
);

VOID VNETS_Master_DelTimer(IN VCLOCK_HANDLE hTimerHandle);

#ifdef __cplusplus
    }
#endif 

#endif 


