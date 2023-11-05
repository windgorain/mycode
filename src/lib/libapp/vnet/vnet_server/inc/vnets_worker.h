/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2012-8-17
* Description: 
* History:     
******************************************************************************/

#ifndef __VNETS_WORKER_H_
#define __VNETS_WORKER_H_

#include "utl/task_worker.h"

#ifdef __cplusplus
    extern "C" {
#endif 

#define VNETS_WORKER_CLOCK_TIME_OF_TICK 5000

BS_STATUS VNETS_Worker_Init();
BS_STATUS VNETS_Worker_SetEvent(UINT uiEventOffset, PF_TASK_WORKER_FUNC pfFunc,
    IN USER_HANDLE_S * pstUserHandle);
BS_STATUS VNETS_Worker_EventInput(IN UINT uiDomainId, IN UINT uiEventOffset);
BS_STATUS VNETS_Worker_MsgInput(UINT uiDomainId, PF_TASK_WORKER_FUNC pfFunc,
     void *ud);
VCLOCK_HANDLE VNETS_Worker_AddTimer(UINT uiDomainId, UINT uiTime,
        PF_TIME_OUT_FUNC pfFunc, USER_HANDLE_S * pstUserHandle);
VOID VNETS_Worker_DelTimer(IN UINT uiDomainId, IN VCLOCK_HANDLE hTimerHandle);

#ifdef __cplusplus
    }
#endif 

#endif 



