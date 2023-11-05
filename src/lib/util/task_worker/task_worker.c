/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2012-8-19
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/event_utl.h"
#include "utl/msgque_utl.h"
#include "utl/task_worker.h"

#define TASK_WORKER_GET_TICK_BY_TIME(uiTime, uiTimeOfTick) (((uiTime) + (uiTimeOfTick) - 1) / (uiTimeOfTick))


#define _TASK_WORKER_EVENT_DATA  0x80000000
#define _TASK_WORKER_EVENT_CLOCK 0x40000000
#define _TASK_WORKER_EVENT_QUIT  0x20000000


#define _TASK_WORKER_MSG_DATA   1


#define _TASK_WORKER_MAX_USER_EVENT 16
#define _TASK_WORKER_USER_EVENT_BITS 0x0000ffff

typedef struct
{
    THREAD_ID uiTid;         
    VCLOCK_INSTANCE_HANDLE hClockInstance;
    EVENT_HANDLE hEvent;    
    MSGQUE_HANDLE hMsgQue;  
}TASK_WORKER_S;

typedef struct
{
    PF_TASK_WORKER_FUNC pfFunc;
    USER_HANDLE_S stUserHandle;
}_TASK_WORKER_USER_EVENT_S;

typedef struct
{
    UINT uiWorkerNum;
    UINT uiTimeOfTick;
    MTIMER_S timer;
    UINT uiCurrentWorkerId;   
    _TASK_WORKER_USER_EVENT_S astUserEvent[_TASK_WORKER_MAX_USER_EVENT];
    TASK_WORKER_S astWorker[0];
}TASK_WORKER_CTRL_S;

static inline BS_STATUS task_worker_DealDataMsg (IN MSGQUE_MSG_S *pstMsg)
{
    PF_TASK_WORKER_FUNC pfFunc = pstMsg->ahMsg[1];

    pfFunc(pstMsg->ahMsg[2]);

    return BS_OK;
}

static inline BS_STATUS task_worker_DealMsg (IN MSGQUE_MSG_S *pstMsg)
{
    UINT ulMsgType;
    BS_STATUS eRet = BS_OK;

    ulMsgType = HANDLE_UINT(pstMsg->ahMsg[0]);

    switch (ulMsgType)
    {
        case _TASK_WORKER_MSG_DATA:
            eRet = task_worker_DealDataMsg(pstMsg);
            break;

        default:
            BS_WARNNING(("Not support yet!"));
            break;
    }

    return eRet;
}

static VOID task_worker_DealUserEvent(IN TASK_WORKER_CTRL_S *pstCtrl, IN UINT uiEvents)
{
    UINT i;
    UINT uiEvent;
    PF_TASK_WORKER_FUNC pfFunc;

    for (i=0; i<_TASK_WORKER_MAX_USER_EVENT; i++)
    {
        uiEvent = 0x1 << i;

        if ((uiEvents & uiEvent) == 0)
        {
            continue;
        }

        pfFunc = pstCtrl->astUserEvent[i].pfFunc;
        if (pfFunc == NULL)
        {
            continue;
        }

        pfFunc(&pstCtrl->astUserEvent[i].stUserHandle);
    }
}

static void task_worker_Clean(TASK_WORKER_S *pstWorker)
{
    if (pstWorker->hMsgQue) {
        MSGQUE_Delete(pstWorker->hMsgQue);
        pstWorker->hMsgQue = NULL;
    }
    if (pstWorker->hEvent) {
        Event_Delete(pstWorker->hEvent);
        pstWorker->hEvent = NULL;
    }

    if (pstWorker->hClockInstance) {
        VCLOCK_DeleteInstance(pstWorker->hClockInstance);
        pstWorker->hClockInstance = NULL;
    }

    pstWorker->uiTid = 0;
}

static void task_worker_Main (IN USER_HANDLE_S *pstUserHandle)
{
    UINT64 uiEvent;
    MSGQUE_MSG_S stMsg;
    TASK_WORKER_CTRL_S *pstCtrl = pstUserHandle->ahUserHandle[0];
    TASK_WORKER_S *pstWorker = pstUserHandle->ahUserHandle[1];

    for (;;)
    {
        Event_Read (pstWorker->hEvent, 0xffffffff, &uiEvent,
                EVENT_FLAG_WAIT, BS_WAIT_FOREVER);

        if (uiEvent & _TASK_WORKER_EVENT_CLOCK) {
            VCLOCK_Step(pstWorker->hClockInstance);
        }

        if (uiEvent & _TASK_WORKER_EVENT_DATA)
        {
            while (BS_OK == MSGQUE_ReadMsg (pstWorker->hMsgQue, &stMsg))
            {
                task_worker_DealMsg(&stMsg);
            }
        }

        if (uiEvent & _TASK_WORKER_USER_EVENT_BITS)
        {
            task_worker_DealUserEvent(pstCtrl, uiEvent);
        }

        if (uiEvent & _TASK_WORKER_EVENT_QUIT)
        {
            break;
        }
    }

    task_worker_Clean(pstWorker);

    return;
}

static BS_STATUS task_worker_InitOne
(
    IN TASK_WORKER_CTRL_S *pstCtrl,
    IN TASK_WORKER_S *pstWorker,
    IN UINT uiTime
)
{
    USER_HANDLE_S stUserHandle;

    if (uiTime != 0) {
        pstWorker->hClockInstance = VCLOCK_CreateInstance(TRUE);
        if (NULL == pstWorker->hClockInstance) {
            task_worker_Clean(pstWorker);
            RETURN(BS_ERR);
        }
    }

    if (NULL == (pstWorker->hEvent = Event_Create())) {
        task_worker_Clean(pstWorker);
        RETURN(BS_ERR);
    }

    if (NULL == (pstWorker->hMsgQue = MSGQUE_Create(512)))
    {
        task_worker_Clean(pstWorker);
        RETURN(BS_ERR);
    }

    stUserHandle.ahUserHandle[0] = pstCtrl;
    stUserHandle.ahUserHandle[1] = pstWorker;

    if (0 == (pstWorker->uiTid = THREAD_Create("taskworker", NULL,
                    task_worker_Main, &stUserHandle))) {
        task_worker_Clean(pstWorker);
        RETURN(BS_ERR);
    }

    return BS_OK;
}

static VOID task_worker_TriggerQuit(IN TASK_WORKER_S *pstWorker)
{
    Event_Write(pstWorker->hEvent, _TASK_WORKER_EVENT_QUIT);
}

static VOID task_worker_TimeOut(IN HANDLE hTimerId, IN USER_HANDLE_S *pstUserHandle)
{
    TASK_WORKER_CTRL_S *pstCtrl = pstUserHandle->ahUserHandle[0];
    UINT i;

    for (i=0; i<pstCtrl->uiWorkerNum; i++) {
        Event_Write(pstCtrl->astWorker[i].hEvent, _TASK_WORKER_EVENT_CLOCK);
    }
}

TASK_WORKER_HANDLE TASK_Worker_Create(IN UINT uiWorkerNum,
        IN UINT uiTime)
{
    UINT i, j;
    int ret;
    TASK_WORKER_CTRL_S *pstCtrl;
    USER_HANDLE_S stUserHandle;

    pstCtrl = MEM_ZMalloc(sizeof(TASK_WORKER_CTRL_S)
            + sizeof(TASK_WORKER_S) * uiWorkerNum);
    if (NULL == pstCtrl) {
        return NULL;
    }

    pstCtrl->uiWorkerNum = uiWorkerNum;

    for (i=0; i<uiWorkerNum; i++) {
        if (BS_OK != task_worker_InitOne(pstCtrl,
                    &pstCtrl->astWorker[i], uiTime)) {
            for (j=0; j<i; j++) {
                task_worker_TriggerQuit(&pstCtrl->astWorker[j]);
            }

            MEM_Free(pstCtrl);
            return NULL;
        }
    }

    stUserHandle.ahUserHandle[0] = pstCtrl;
    ret = MTimer_Add(&pstCtrl->timer, uiTime, TIMER_FLAG_CYCLE,
            task_worker_TimeOut, &stUserHandle);
    if (ret < 0) {
        for (i=0; i<uiWorkerNum; i++) {
            task_worker_TriggerQuit(&pstCtrl->astWorker[i]);
        }
        
        return NULL;
    }

    return pstCtrl;
}

BS_STATUS TASK_Worker_SetEvent
(
    IN TASK_WORKER_HANDLE hTaskWorker,
    IN UINT uiEventOffset, 
    IN PF_TASK_WORKER_FUNC pfFunc,
    IN USER_HANDLE_S *pstUserHandle
)
{
    TASK_WORKER_CTRL_S *pstCtrl = hTaskWorker;

    if (uiEventOffset >= _TASK_WORKER_MAX_USER_EVENT)
    {
        return BS_OUT_OF_RANGE;
    }

    pstCtrl->astUserEvent[uiEventOffset].pfFunc = pfFunc;
    if (pstUserHandle)
    {
        pstCtrl->astUserEvent[uiEventOffset].stUserHandle = *pstUserHandle;
    }

    return BS_OK;
}

BS_STATUS TASK_Worker_EventInput
(
    IN TASK_WORKER_HANDLE hTaskWorker,
    IN UINT uiWokerId,  
    IN UINT uiEventOffset 
)
{
    TASK_WORKER_CTRL_S *pstCtrl = hTaskWorker;
    UINT uiEvent;
    UINT uiDstWorkerId = uiWokerId;

    if (uiDstWorkerId >= pstCtrl->uiWorkerNum)
    {
        return BS_OUT_OF_RANGE;
    }

    if (uiEventOffset >= _TASK_WORKER_MAX_USER_EVENT)
    {
        return BS_OUT_OF_RANGE;
    }

    if (uiDstWorkerId == TASK_WORKER_ID_AUTO)
    {
        uiDstWorkerId = pstCtrl->uiCurrentWorkerId;
        pstCtrl->uiCurrentWorkerId ++;
        if (pstCtrl->uiCurrentWorkerId >= pstCtrl->uiWorkerNum)
        {
            pstCtrl->uiCurrentWorkerId = 0;
        }
    }

    uiEvent = 0x1 << uiEventOffset;

    return Event_Write(pstCtrl->astWorker[uiDstWorkerId].hEvent, uiEvent);
}

BS_STATUS TASK_Worker_MsgInput
(
    TASK_WORKER_HANDLE hTaskWorker,
    UINT uiWokerId,  
    PF_TASK_WORKER_FUNC pfFunc,
    void *ud
)
{
    TASK_WORKER_CTRL_S *pstCtrl = hTaskWorker;
    UINT to = uiWokerId;
    MSGQUE_MSG_S stMsg;

    if (to >= pstCtrl->uiWorkerNum) {
        return BS_OUT_OF_RANGE;
    }
    
    if (to == TASK_WORKER_ID_AUTO) {
        to = pstCtrl->uiCurrentWorkerId;
        pstCtrl->uiCurrentWorkerId ++;
        if (pstCtrl->uiCurrentWorkerId >= pstCtrl->uiWorkerNum) {
            pstCtrl->uiCurrentWorkerId = 0;
        }
    }

    stMsg.ahMsg[0] = UINT_HANDLE(_TASK_WORKER_MSG_DATA);
    stMsg.ahMsg[1] = pfFunc;
    stMsg.ahMsg[2] = ud;

    if (BS_OK != MSGQUE_WriteMsg(pstCtrl->astWorker[to].hMsgQue, &stMsg))
    {
        return(BS_FULL);
    }

    Event_Write(pstCtrl->astWorker[to].hEvent, _TASK_WORKER_EVENT_DATA);

    return BS_OK;
}

VCLOCK_HANDLE TASK_Worker_AddTimer
(
    IN TASK_WORKER_HANDLE hTaskWorker,
    IN UINT uiWokerId,
    IN UINT uiTime,    
    IN PF_TIME_OUT_FUNC pfFunc,
    IN USER_HANDLE_S *pstUserHandle
)
{
    TASK_WORKER_CTRL_S *pstCtrl = hTaskWorker;
    UINT tick = TASK_WORKER_GET_TICK_BY_TIME(uiTime, pstCtrl->uiTimeOfTick);

    if (uiWokerId >= pstCtrl->uiWorkerNum) {
        return NULL;
    }

    return VCLOCK_CreateTimer(pstCtrl->astWorker[uiWokerId].hClockInstance,
            tick, tick, TIMER_FLAG_CYCLE, pfFunc, pstUserHandle);
}

VOID TASK_Worker_DelTimer
(
    IN TASK_WORKER_HANDLE hTaskWorker,
    IN UINT uiWokerId,
    IN VCLOCK_HANDLE hTimerHandle
)
{
    TASK_WORKER_CTRL_S *pstCtrl = hTaskWorker;

    if (uiWokerId >= pstCtrl->uiWorkerNum)
    {
        return;
    }
    
    VCLOCK_DestroyTimer(pstCtrl->astWorker[uiWokerId].hClockInstance, hTimerHandle);
}
