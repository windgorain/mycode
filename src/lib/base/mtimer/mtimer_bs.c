/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2007-8-18
* Description: 相比于VTimer, 只创建一个Timer定时向一个线程发送消息
*              所有MTimer在这个线程中使用vclock调度
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/vclock_utl.h"
#include "utl/atomic_once.h"
#include "utl/msgque_utl.h"
#include "utl/event_utl.h"

/* defines */
#define _MTimer_DFT_TIME_PER_TICK 1000  /* ms */

#define _VTIMER_MTimer_EVENT 1
#define _MTimer_MSG_TYPE 1

/* structs */

typedef struct
{
    HANDLE  hClockId;
    HANDLE  timer;
    THREAD_ID ulThreadID;
    MSGQUE_HANDLE hQueID;
}_MTimer_HEAD_S;

/* var */
static _MTimer_HEAD_S g_stMTimerHead;
static EVENT_HANDLE g_hMtimerEventId = 0;

static UINT _mtimer_Ms2Tick(UINT ms)
{
    return (ms + _MTimer_DFT_TIME_PER_TICK - 1)/ _MTimer_DFT_TIME_PER_TICK;
}

static void _MTimer_Main(IN USER_HANDLE_S *pstUserHandle)
{
    UINT64 uiEvent = 0;
    MSGQUE_MSG_S stMsg;

    while (1) {
        Event_Read(g_hMtimerEventId, _VTIMER_MTimer_EVENT,
                &uiEvent, EVENT_FLAG_WAIT, BS_WAIT_FOREVER);

        if (uiEvent & _VTIMER_MTimer_EVENT) {
            while(BS_OK == MSGQUE_ReadMsg(g_stMTimerHead.hQueID, &stMsg)) {
                if (_MTimer_MSG_TYPE == HANDLE_UINT(stMsg.ahMsg[0])) {
                    VCLOCK_Step(g_stMTimerHead.hClockId);
                }
            }
        }
    }

    return;
}

static VOID _MTimer_OnTimer(IN HANDLE timer, IN USER_HANDLE_S *pstUserHandle)
{
    MSGQUE_MSG_S stMsg;

    stMsg.ahMsg[0] = UINT_HANDLE(_MTimer_MSG_TYPE);
    MSGQUE_WriteMsg(g_stMTimerHead.hQueID, &stMsg);
    Event_Write(g_hMtimerEventId, _VTIMER_MTimer_EVENT);
}

static UINT _MTimer_GetAdjustTick()
{
    UINT ulMsgNum = 0;

    /* 为了避免提前触发, 要加上现在消息队列中已有的消息个数,
       并且在其基础上再加一个Tick*/
    ulMsgNum = MSGQUE_Count(g_stMTimerHead.hQueID);

    return ulMsgNum + 1;
}

static BS_STATUS _mtimer_InitOnce(void *ud)
{
    Mem_Zero(&g_stMTimerHead, sizeof(_MTimer_HEAD_S));

    if (0 == (g_stMTimerHead.hClockId = VCLOCK_CreateInstance(TRUE)))
    {
        RETURN(BS_ERR);
    }

    if (NULL == (g_hMtimerEventId = Event_Create()))
    {
        VCLOCK_DeleteInstance(g_stMTimerHead.hClockId);
        g_stMTimerHead.hClockId = 0;
        BS_WARNNING(("Can't crete event!"));
        RETURN(BS_ERR);
    }

    if (NULL == (g_stMTimerHead.hQueID = MSGQUE_Create(128)))
    {
        Event_Delete(g_hMtimerEventId);
        g_hMtimerEventId = 0;
        VCLOCK_DeleteInstance(g_stMTimerHead.hClockId);
        g_stMTimerHead.hClockId = 0;
        BS_WARNNING(("Can't crete MTimer queue!"));
        RETURN(BS_ERR);
    }

    g_stMTimerHead.timer = Timer_Create(_MTimer_DFT_TIME_PER_TICK,
            TIMER_FLAG_CYCLE, _MTimer_OnTimer, NULL);
    if (NULL == g_stMTimerHead.timer)
    {
        Event_Delete(g_hMtimerEventId);
        g_hMtimerEventId = 0;
        MSGQUE_Delete(g_stMTimerHead.hQueID);
        g_stMTimerHead.hQueID = 0;
        VCLOCK_DeleteInstance(g_stMTimerHead.hClockId);
        g_stMTimerHead.hClockId = 0;
        RETURN(BS_ERR);
    }

    if (THREAD_ID_INVALID == (g_stMTimerHead.ulThreadID
                = THREAD_Create("MTimer", NULL, _MTimer_Main, NULL)))
    {
        Event_Delete(g_hMtimerEventId);
        g_hMtimerEventId = 0;
        Timer_Delete(g_stMTimerHead.timer);
        g_stMTimerHead.timer = NULL;
        MSGQUE_Delete(g_stMTimerHead.hQueID);
        g_stMTimerHead.hQueID = NULL;
        VCLOCK_DeleteInstance(g_stMTimerHead.hClockId);
        g_stMTimerHead.hClockId = 0;
        BS_WARNNING(("Can't crete MTimer thread!"));
        RETURN(BS_ERR);
    }

    return BS_OK;
}

static void _mtimer_Init()
{
    static ATOM_ONCE_S once = ATOM_ONCE_INIT_VALUE;
    AtomOnce_Do(&once, _mtimer_InitOnce, NULL);
}

int MTimer_Add(MTIMER_S *timer, UINT time_ms, UINT flag,
        PF_TIME_OUT_FUNC pfFunc, USER_HANDLE_S *pstUserHandle)
{
    UINT first_tick;
    UINT tick;

    _mtimer_Init();

    if (g_hMtimerEventId == NULL) {
        BS_WARNNING(("MTimer not init"));
        RETURN(BS_NOT_INIT);
    }

    tick = _mtimer_Ms2Tick(time_ms);
    first_tick = tick + _MTimer_GetAdjustTick();

    return VCLOCK_AddTimer(g_stMTimerHead.hClockId, &timer->vclock,
            first_tick, tick, flag, pfFunc, pstUserHandle);
}

int MTimer_Del(MTIMER_S *timer)
{
    return VCLOCK_DelTimer(g_stMTimerHead.hClockId, &timer->vclock);
}

BS_STATUS MTimer_Pause(IN HANDLE hMTimerID)
{
    return VCLOCK_Pause(g_stMTimerHead.hClockId, hMTimerID);
}

BS_STATUS MTimer_Resume(IN HANDLE hMTimerID)
{
    return VCLOCK_Resume(g_stMTimerHead.hClockId, hMTimerID, _MTimer_GetAdjustTick());
}

BS_STATUS MTimer_GetInfo(IN HANDLE hMTimerID, OUT TIMER_INFO_S *pstTimerInfo)
{
    BS_STATUS eRet;
    
    eRet = VCLOCK_GetInfo(g_stMTimerHead.hClockId, hMTimerID, pstTimerInfo);
    pstTimerInfo->ulTime *= _MTimer_DFT_TIME_PER_TICK;

    return eRet;
}

BS_STATUS MTimer_RestartWithTime(IN HANDLE hMTimerID, IN UINT ulTime/* ms */)
{
    UINT ulTick;

    ulTick = (ulTime + _MTimer_DFT_TIME_PER_TICK - 1) / _MTimer_DFT_TIME_PER_TICK;

    return VCLOCK_RestartWithTick(g_stMTimerHead.hClockId, hMTimerID,
            ulTick, _MTimer_GetAdjustTick());
}

/* 得到还有多少ms 就超时了 */
BS_STATUS MTimer_GetTimeLeft(IN HANDLE hMTimerID, OUT UINT *pulTimeLeft)
{
    UINT ulResTick;

    ulResTick = VCLOCK_GetTickLeft(g_stMTimerHead.hClockId, hMTimerID);

    *pulTimeLeft = ulResTick * _MTimer_DFT_TIME_PER_TICK;

    return BS_OK;
}


