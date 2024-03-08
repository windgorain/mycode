/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2012-8-19
* Description: 
* History:     
******************************************************************************/

#include "bs.h"

#include "utl/task_worker.h"

#include "../inc/vnets_worker.h"

#define _VNETS_WORKER_NUM 4

static TASK_WORKER_HANDLE g_hVnetsTaskWorker = NULL;

static UINT vnets_worker_GetWorkerIdByDomainId(IN UINT uiDomainId)
{
    return uiDomainId % _VNETS_WORKER_NUM;
}

BS_STATUS VNETS_Worker_Init()
{
    g_hVnetsTaskWorker = TASK_Worker_Create("vnets_worker", _VNETS_WORKER_NUM, VNETS_WORKER_CLOCK_TIME_OF_TICK);

    if (g_hVnetsTaskWorker == NULL)
    {
        return BS_NO_RESOURCE;
    }

    return BS_OK;
}

BS_STATUS VNETS_Worker_SetEvent
(
    IN UINT uiEventOffset,
    IN PF_TASK_WORKER_FUNC pfFunc,
    IN USER_HANDLE_S * pstUserHandle
)
{
    return TASK_Worker_SetEvent(g_hVnetsTaskWorker, uiEventOffset, pfFunc, pstUserHandle);
}

BS_STATUS VNETS_Worker_EventInput(IN UINT uiDomainId, IN UINT uiEventOffset)
{
    UINT uiWorkerId = vnets_worker_GetWorkerIdByDomainId(uiDomainId);

    return TASK_Worker_EventInput(g_hVnetsTaskWorker, uiWorkerId, uiEventOffset);
}

BS_STATUS VNETS_Worker_MsgInput(UINT uiDomainId, PF_TASK_WORKER_FUNC pfFunc,
    void *ud)
{
    UINT uiWorkerId = vnets_worker_GetWorkerIdByDomainId(uiDomainId);

    return TASK_Worker_MsgInput(g_hVnetsTaskWorker, uiWorkerId, pfFunc, ud);
}

VCLOCK_HANDLE VNETS_Worker_AddTimer
(
    IN UINT uiDomainId,
    IN UINT uiTime,
    IN PF_TIME_OUT_FUNC pfFunc,
    IN USER_HANDLE_S * pstUserHandle
)
{
    UINT uiWorkerId = vnets_worker_GetWorkerIdByDomainId(uiDomainId);
    
    return TASK_Worker_AddTimer(g_hVnetsTaskWorker, uiWorkerId, uiTime, pfFunc, pstUserHandle);
}

VOID VNETS_Worker_DelTimer(IN UINT uiDomainId, IN VCLOCK_HANDLE hTimerHandle)
{
    UINT uiWorkerId = vnets_worker_GetWorkerIdByDomainId(uiDomainId);

    TASK_Worker_DelTimer(g_hVnetsTaskWorker, uiWorkerId, hTimerHandle);
}



