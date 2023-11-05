/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2008-10-9
* Description: 
* History:     
******************************************************************************/

#ifndef __VCLOCK_UTL_H_
#define __VCLOCK_UTL_H_

#include "utl/mutex_utl.h"
#include "utl/timer_common.h"

#ifdef __cplusplus
    extern "C" {
#endif 

#define _VCLOCK_TIMER_TOTLE_LEVEL 6
#define _VCLOCK_TIMER_SCALE_PER_LEVEL 64



typedef struct
{
    DLL_NODE_S stDllNode;
    UINT ulTick;        
    UINT uiTriggerTick; 
    UINT flag;
    PF_TIME_OUT_FUNC pfFunc;
    USER_HANDLE_S stUserHandle;
}VCLOCK_NODE_S;

typedef struct
{
    UINT create_lock:1;
    MUTEX_S lock;
    UINT ulNodeCount;
    UINT uiCurrentTick; 
    UINT ulCurrentLevelTick[_VCLOCK_TIMER_TOTLE_LEVEL];  
    DLL_HEAD_S stTimerLevel[_VCLOCK_TIMER_TOTLE_LEVEL][_VCLOCK_TIMER_SCALE_PER_LEVEL];
}VCLOCK_INSTANCE_S;

typedef VCLOCK_INSTANCE_S* VCLOCK_INSTANCE_HANDLE;
typedef VCLOCK_NODE_S* VCLOCK_HANDLE;

int VCLOCK_InitInstance(OUT VCLOCK_INSTANCE_S *pstVClockInstance, IN BOOL_T bCreateLock);
void VCLOCK_FiniInstance(IN VCLOCK_INSTANCE_S *pstVClockInstance);

VCLOCK_INSTANCE_HANDLE  VCLOCK_CreateInstance(BOOL_T bCreateLock);
void VCLOCK_DeleteInstance(VCLOCK_INSTANCE_HANDLE hVClock);

int VCLOCK_AddTimer
(
    IN VCLOCK_INSTANCE_S *pstVClockInstance,
    IN VCLOCK_NODE_S *vclock_node,
    IN UINT first_tick, 
    IN UINT tick,      
    IN UINT flag,
    IN PF_TIME_OUT_FUNC pfFunc,
    IN USER_HANDLE_S *pstUserHandle
);
BS_STATUS VCLOCK_DelTimer(IN VCLOCK_INSTANCE_HANDLE hVClockInstanceId, IN VCLOCK_NODE_S *vclock_node);

VCLOCK_HANDLE VCLOCK_CreateTimer
(
    IN VCLOCK_INSTANCE_HANDLE hVClockInstanceId,
    IN UINT first_tick, 
    IN UINT tick,      
    IN UINT flag,
    IN PF_TIME_OUT_FUNC pfFunc,
    IN USER_HANDLE_S *pstUserHandle
);
void VCLOCK_DestroyTimer(IN VCLOCK_INSTANCE_S *pstVClockInstance, IN VCLOCK_HANDLE hTimer);

BS_STATUS VCLOCK_Pause(IN VCLOCK_INSTANCE_HANDLE hVClockInstanceId, IN VCLOCK_HANDLE hTimerId);
BS_STATUS VCLOCK_Resume(IN VCLOCK_INSTANCE_HANDLE hVClockInstanceId, IN VCLOCK_HANDLE hTimerId, IN UINT first_tick);
BS_STATUS VCLOCK_GetInfo(IN VCLOCK_INSTANCE_HANDLE hVClockInstanceId, IN VCLOCK_HANDLE hTimerId, OUT TIMER_INFO_S *pstTimerInfo);
BS_STATUS VCLOCK_RestartWithTick(IN VCLOCK_INSTANCE_HANDLE hVClockInstanceId, IN VCLOCK_HANDLE hTimerId, IN UINT first_tick, IN UINT tick);
BS_STATUS VCLOCK_Refresh(IN VCLOCK_INSTANCE_HANDLE hVClockInstanceId, IN VCLOCK_HANDLE hTimerId);
UINT VCLOCK_GetTickLeft(IN VCLOCK_INSTANCE_HANDLE hVClockInstanceId, IN VCLOCK_HANDLE hTimerId);


BS_STATUS VCLOCK_Step(IN VCLOCK_INSTANCE_HANDLE hVClockInstanceId);


#ifdef __cplusplus
    }
#endif 

#endif 


