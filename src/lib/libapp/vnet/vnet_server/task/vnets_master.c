/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2012-8-19
* Description: 
* History:     
******************************************************************************/

#include "bs.h"

#include "../inc/vnets_master.h"

static TASK_MASTER_HANDLE g_hVnetsTaskMaster = NULL;

BS_STATUS VNETS_Master_Init()
{
    g_hVnetsTaskMaster = TASK_Master_Create(1000);
    if (NULL == g_hVnetsTaskMaster)
    {
        return BS_NO_RESOURCE;
    }

    return BS_OK;
}

BS_STATUS VNETS_Master_SetEvent
(
    IN UINT uiEventOffset, /* 0-15 */
    IN PF_TASK_MASTER_FUNC pfFunc,
    IN USER_HANDLE_S *pstUserHandle
)
{
    return TASK_Master_SetEvent(g_hVnetsTaskMaster, uiEventOffset, pfFunc, pstUserHandle);
}

BS_STATUS VNETS_Master_EventInput(IN UINT uiEventOffset)
{
    return TASK_Master_EventInput(g_hVnetsTaskMaster, uiEventOffset);
}

BS_STATUS VNETS_Master_MsgInput(PF_TASK_MASTER_FUNC pfFunc, void *ud)
{
    return TASK_Master_MsgInput(g_hVnetsTaskMaster, pfFunc, ud);
}

VCLOCK_HANDLE VNETS_Master_AddTimer
(
    IN UINT uiTime,
    IN UINT flag,
    IN PF_TIME_OUT_FUNC pfFunc,
    IN USER_HANDLE_S *pstUserHandle
)
{
    return TASK_Master_AddTimer(g_hVnetsTaskMaster, uiTime, flag, pfFunc, pstUserHandle);
}

VOID VNETS_Master_DelTimer(IN VCLOCK_HANDLE hTimerHandle)
{
    TASK_Master_DelTimer(g_hVnetsTaskMaster, hTimerHandle);
}

