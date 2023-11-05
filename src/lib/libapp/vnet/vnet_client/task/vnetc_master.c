/******************************************************************************
* Copyright (C),    LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2012-8-19
* Description: 
* History:     
******************************************************************************/

#include "bs.h"

#include "../inc/vnetc_master.h"


#define VNETC_MASTER_CLOCK_TIME_OF_TICK 1000 


static TASK_MASTER_HANDLE g_hVnetcTaskMaster = NULL;

BS_STATUS VNETC_Master_Init()
{
    g_hVnetcTaskMaster = TASK_Master_Create(VNETC_MASTER_CLOCK_TIME_OF_TICK);
    if (NULL == g_hVnetcTaskMaster)
    {
        return BS_NO_RESOURCE;
    }

    return BS_OK;
}

BS_STATUS VNETC_Master_SetEvent
(
    IN UINT uiEventOffset, 
    IN PF_TASK_MASTER_FUNC pfFunc,
    IN USER_HANDLE_S *pstUserHandle
)
{
    return TASK_Master_SetEvent(g_hVnetcTaskMaster, uiEventOffset, pfFunc, pstUserHandle);
}

BS_STATUS VNETC_Master_EventInput(IN UINT uiEventOffset)
{
    return TASK_Master_EventInput(g_hVnetcTaskMaster, uiEventOffset);
}

BS_STATUS VNETC_Master_MsgInput ( PF_TASK_MASTER_FUNC pfFunc, void *ud)
{
    return TASK_Master_MsgInput(g_hVnetcTaskMaster, pfFunc, ud);
}

VCLOCK_HANDLE VNETC_Master_AddTimer
(
    IN UINT uiTime,  
    IN UINT flag,
    IN PF_TIME_OUT_FUNC pfFunc,
    IN USER_HANDLE_S *pstUserHandle
)
{
    return TASK_Master_AddTimer(g_hVnetcTaskMaster, uiTime, flag, pfFunc, pstUserHandle);
}

VOID VNETC_Master_DelTimer(IN VCLOCK_HANDLE hTimerHandle)
{
    TASK_Master_DelTimer(g_hVnetcTaskMaster, hTimerHandle);
}

VOID VNETC_Master_RefreshTimer(IN VCLOCK_HANDLE hTimerHandle)
{
    TASK_Master_RefreshTimer(g_hVnetcTaskMaster, hTimerHandle);
}

