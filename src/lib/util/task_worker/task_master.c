/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2012-8-19
* Description: 
* History:     
******************************************************************************/

#include "bs.h"

#include "utl/event_utl.h"
#include "utl/msgque_utl.h"
#include "utl/mbuf_utl.h"
#include "utl/task_master.h"


#define TASK_MASTER_GET_TICK_BY_TIME(uiTime/* ms */, uiTimeOfTick) (((uiTime) + (uiTimeOfTick) - 1) / (uiTimeOfTick))

/* 事件 */
#define _TASK_MASTER_EVENT_DATA  0x80000000
#define _TASK_MASTER_EVENT_CLOCK 0x40000000
#define _TASK_MASTER_EVENT_QUIT  0x20000000


/* 消息类型 */
#define _TASK_MASTER_MSG_DATA   1

/* 用户自定义事件 */
#define _TASK_MASTER_MAX_USER_EVENT 16
#define _TASK_MASTER_USER_EVENT_BITS 0x0000ffff

typedef struct
{
    PF_TASK_MASTER_FUNC pfFunc;
    USER_HANDLE_S stUserHandle;
}_TASK_MASTER_EVENT_S;

typedef struct
{
    THREAD_ID thread_id;
    UINT uiTimeOfTick;
    VCLOCK_INSTANCE_HANDLE hClockInstance;
    MTIMER_S timer;
    EVENT_HANDLE hEvent;
    MSGQUE_HANDLE hMsgQue;
    _TASK_MASTER_EVENT_S astEvent[_TASK_MASTER_MAX_USER_EVENT];
}_TASK_MASTER_S;

static inline BS_STATUS task_master_DealDataMsg (IN MSGQUE_MSG_S *pstMsg)
{
    PF_TASK_MASTER_FUNC pfFunc = pstMsg->ahMsg[1];

    pfFunc(pstMsg->ahMsg[2]);

    return BS_OK;
}

static inline BS_STATUS task_master_DealMsg (IN MSGQUE_MSG_S *pstMsg)
{
    UINT ulMsgType;
    BS_STATUS eRet = BS_OK;

    ulMsgType = HANDLE_UINT(pstMsg->ahMsg[0]);

    switch (ulMsgType)
    {
        case _TASK_MASTER_MSG_DATA:
            eRet = task_master_DealDataMsg(pstMsg);
            break;

        default:
            BS_WARNNING(("Not support yet!"));
            break;
    }

    return eRet;
}

static VOID task_master_DealUserEvent(IN _TASK_MASTER_S *pstCtrl, IN UINT uiEvents)
{
    UINT i;
    UINT uiEvent;
    PF_TASK_MASTER_FUNC pfFunc;

    for (i=0; i<_TASK_MASTER_MAX_USER_EVENT; i++)
    {
        uiEvent = 0x1 << i;

        if ((uiEvents & uiEvent) == 0)
        {
            continue;
        }

        pfFunc = pstCtrl->astEvent[i].pfFunc;
        if (pfFunc == NULL)
        {
            continue;
        }

        pfFunc(&pstCtrl->astEvent[i].stUserHandle);
    }
}

static void task_master_Main (IN USER_HANDLE_S *pstUserHandle)
{
    UINT64 uiEvent;
    MSGQUE_MSG_S stMsg;
    _TASK_MASTER_S *pstCtrl = pstUserHandle->ahUserHandle[0];

    for (;;) {
        Event_Read (pstCtrl->hEvent, 0xffffffff, &uiEvent,
                EVENT_FLAG_WAIT, BS_WAIT_FOREVER);

        if (uiEvent & _TASK_MASTER_EVENT_CLOCK) {
            VCLOCK_Step(pstCtrl->hClockInstance);
        }

        if (uiEvent & _TASK_MASTER_EVENT_DATA) {
            while (BS_OK == MSGQUE_ReadMsg(pstCtrl->hMsgQue, &stMsg)) {
                task_master_DealMsg(&stMsg);
            }
        }

        if (uiEvent & _TASK_MASTER_USER_EVENT_BITS) {
            task_master_DealUserEvent(pstCtrl, uiEvent);
        }

        if (uiEvent & _TASK_MASTER_EVENT_QUIT) {
            break;
        }
    }

    MSGQUE_Delete(pstCtrl->hMsgQue);
    Event_Delete(pstCtrl->hEvent);
    if (pstCtrl->hClockInstance) {
        VCLOCK_DeleteInstance(pstCtrl->hClockInstance);
    }

    pstCtrl->hEvent = NULL;
    pstCtrl->hMsgQue = NULL;
    pstCtrl->hClockInstance = NULL;
    pstCtrl->thread_id = 0;

    return;
}

static VOID task_master_TimeOut(IN HANDLE hTimerId, IN USER_HANDLE_S *pstUserHandle)
{
    _TASK_MASTER_S *pstCtrl = pstUserHandle->ahUserHandle[0];

    Event_Write(pstCtrl->hEvent, _TASK_MASTER_EVENT_CLOCK);
}

TASK_MASTER_HANDLE TASK_Master_Create(IN UINT uiTime/* ms. 如果为0表示不创建定时器 */)
{
    _TASK_MASTER_S *pstCtrl;
    EVENT_HANDLE hEvent;
    MSGQUE_HANDLE hMsgQue;
    THREAD_ID thread_id;
    USER_HANDLE_S stThreadUserHandle;
    VCLOCK_INSTANCE_HANDLE hClockInstance = NULL;
    USER_HANDLE_S stUserHandle;
    int ret;

    if (uiTime != 0)
    {
        hClockInstance = VCLOCK_CreateInstance(TRUE);
        if (NULL == hClockInstance)
        {
            return NULL;
        }
    }

    pstCtrl = MEM_ZMalloc(sizeof(_TASK_MASTER_S));
    if (NULL == pstCtrl)
    {
        if (hClockInstance)
        {
            VCLOCK_DeleteInstance(hClockInstance);
        }
        return NULL;
    }

    if (NULL == (hEvent = Event_Create()))
    {
        if (hClockInstance)
        {
            VCLOCK_DeleteInstance(hClockInstance);
        }
        return NULL;
    }

    if (NULL == (hMsgQue = MSGQUE_Create(512)))
    {
        if (hClockInstance)
        {
            VCLOCK_DeleteInstance(hClockInstance);
        }
        Event_Delete(hEvent);
        return NULL;
    }

    pstCtrl->uiTimeOfTick = uiTime;
    pstCtrl->hEvent = hEvent;
    pstCtrl->hMsgQue = hMsgQue;
    pstCtrl->hClockInstance = hClockInstance;

    stThreadUserHandle.ahUserHandle[0] = pstCtrl;

    if (0 == (thread_id = THREAD_Create("TaskMaster", NULL,
        task_master_Main, &stThreadUserHandle)))
    {
        if (hClockInstance)
        {
            VCLOCK_DeleteInstance(hClockInstance);
        }
        Event_Delete(hEvent);
        MSGQUE_Delete(hMsgQue);
        MEM_Free(pstCtrl);
        return NULL;
    }

    pstCtrl->thread_id = thread_id;

    stUserHandle.ahUserHandle[0] = pstCtrl;
    ret = MTimer_Add(&pstCtrl->timer, uiTime, TIMER_FLAG_CYCLE,
            task_master_TimeOut, &stUserHandle);
    if (ret < 0) {
        Event_Write(hEvent, _TASK_MASTER_EVENT_QUIT);
        return NULL;
    }

    return pstCtrl;
}

BS_STATUS TASK_Master_SetEvent
(
    IN TASK_MASTER_HANDLE hTaskMaster,
    IN UINT uiEventOffset, /* 0-15 */
    IN PF_TASK_MASTER_FUNC pfFunc,
    IN USER_HANDLE_S *pstUserHandle
)
{
    _TASK_MASTER_S *pstCtrl = hTaskMaster;

    if (uiEventOffset >= _TASK_MASTER_MAX_USER_EVENT)
    {
        return BS_OUT_OF_RANGE;
    }

    pstCtrl->astEvent[uiEventOffset].pfFunc = pfFunc;
    if (pstUserHandle)
    {
        pstCtrl->astEvent[uiEventOffset].stUserHandle = *pstUserHandle;
    }

    return BS_OK;
}

BS_STATUS TASK_Master_EventInput
(
    IN TASK_MASTER_HANDLE hTaskMaster,
    IN UINT uiEventOffset /* 0-15 */
)
{
    _TASK_MASTER_S *pstCtrl = hTaskMaster;
    UINT uiEvent;

    if (uiEventOffset >= _TASK_MASTER_MAX_USER_EVENT)
    {
        return BS_OUT_OF_RANGE;
    }

    uiEvent = 0x1 << uiEventOffset;

    return Event_Write(pstCtrl->hEvent, uiEvent);
}

BS_STATUS TASK_Master_MsgInput(TASK_MASTER_HANDLE hTaskMaster,
    PF_TASK_MASTER_FUNC pfFunc, void *ud)
{
    _TASK_MASTER_S *pstCtrl = hTaskMaster;
    MSGQUE_MSG_S stMsg;

    stMsg.ahMsg[0] = UINT_HANDLE(_TASK_MASTER_MSG_DATA);
    stMsg.ahMsg[1] = pfFunc;
    stMsg.ahMsg[2] = ud;

    if (BS_OK != MSGQUE_WriteMsg(pstCtrl->hMsgQue, &stMsg)) {
        return(BS_FULL);
    }

    Event_Write(pstCtrl->hEvent, _TASK_MASTER_EVENT_DATA);

    return BS_OK;
}

VCLOCK_HANDLE TASK_Master_AddTimer
(
    IN TASK_MASTER_HANDLE hTaskMaster,
    IN UINT uiTime,    /* 多少个ms之后触发. */
    IN UINT flag,
    IN PF_TIME_OUT_FUNC pfFunc,
    IN USER_HANDLE_S *pstUserHandle
)
{
    _TASK_MASTER_S *pstCtrl = hTaskMaster;
    UINT tick = TASK_MASTER_GET_TICK_BY_TIME(uiTime, pstCtrl->uiTimeOfTick);

    return VCLOCK_CreateTimer(pstCtrl->hClockInstance,
            tick, tick, flag, pfFunc, pstUserHandle);
}

VOID TASK_Master_DelTimer(IN TASK_MASTER_HANDLE hTaskMaster, IN VCLOCK_HANDLE hTimerHandle)
{
    _TASK_MASTER_S *pstCtrl = hTaskMaster;

    VCLOCK_DestroyTimer(pstCtrl->hClockInstance, hTimerHandle);
}

VOID TASK_Master_RefreshTimer(IN TASK_MASTER_HANDLE hTaskMaster, IN VCLOCK_HANDLE hTimerHandle)
{
    _TASK_MASTER_S *pstCtrl = hTaskMaster;

    VCLOCK_Refresh(pstCtrl->hClockInstance, hTimerHandle);
}

