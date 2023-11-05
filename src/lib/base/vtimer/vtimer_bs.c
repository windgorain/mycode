/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2007-8-17
* Description: 相比于Timer, VTimer自带一个任务, 所有的定时触发都是在任务中进行,
*              依赖于Timer,Timer回调函数发完消息后立即返回, 以免影响其他使用Timer的人
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/atomic_once.h"
#include "utl/msgque_utl.h"
#include "utl/event_utl.h"

#define _VTIMER_DFT_NUM 256
#define _VTIMER_MAX_NUM 65535

#define _VTIMER_TIMER_EVENT 1

#define _VTIMER_MSG_TYPE 1

typedef struct
{
    BOOL_T bIsUsed;
    UINT  flag;
    UINT  ulVTimerID;
    UINT  ulTime;
    HANDLE  hTimer;
    PF_TIME_OUT_FUNC pfFunc;
    USER_HANDLE_S stUserHandle;
}_VTIMER_CTRL_S;

static UINT  g_ulMaxVTimerNum = 0;
static USHORT *g_pusIncNum = NULL;
static _VTIMER_CTRL_S *g_pstVTimerCtrl = NULL;
static MSGQUE_HANDLE g_hVTimerQueId = NULL;
static THREAD_ID  g_ulVTimerThreadID = 0;
static EVENT_HANDLE g_hVTimerEventID = 0;

static inline _VTIMER_CTRL_S * _VTimer_GetVTimerByID(IN UINT ulVTimerID)
{
    _VTIMER_CTRL_S *pstVTimer;
    UINT ulIndex;

    ulIndex = (ulVTimerID & 0xffff) -1;

    if (ulIndex >= g_ulMaxVTimerNum)
    {
        return NULL;
    }

    pstVTimer = &g_pstVTimerCtrl[ulIndex];
    
    if (pstVTimer->ulVTimerID == ulVTimerID)
    {
        return pstVTimer;
    }

    return NULL;
}

static BS_STATUS _VTimer_ProcMsg(IN UINT ulVTimerID)
{
    _VTIMER_CTRL_S stVTimer, *pstVTimer;

    pstVTimer = _VTimer_GetVTimerByID(ulVTimerID);
    if (NULL == pstVTimer)
    {
        return BS_OK;
    }

    SPLX_P();
    stVTimer = *pstVTimer;
    if (!(pstVTimer->flag & TIMER_FLAG_CYCLE))
    {
        pstVTimer->flag |= TIMER_FLAG_PAUSE;
    }
    SPLX_V();

    if ((TRUE == stVTimer.bIsUsed) && (! TIMER_IS_PAUSE(stVTimer.flag))) {
        stVTimer.pfFunc(UINT_HANDLE(ulVTimerID), &(stVTimer.stUserHandle));
    }

    return BS_OK;
}

static VOID _VTimer_ProcTimerEvent(HANDLE hTimer, USER_HANDLE_S *ud)
{
    MSGQUE_MSG_S stMsg;

    stMsg.ahMsg[0] = UINT_HANDLE(_VTIMER_MSG_TYPE);
    stMsg.ahMsg[1] = hTimer; 
    stMsg.ahMsg[2] = ud->ahUserHandle[0];

    if (BS_OK != MSGQUE_WriteMsg(g_hVTimerQueId, &stMsg)) {
        return;
    }

    Event_Write(g_hVTimerEventID, _VTIMER_TIMER_EVENT);

    return;
}

static void _VTimer_Main(IN USER_HANDLE_S *pstUserHandle)
{
    UINT64 uiEvent = 0;
    MSGQUE_MSG_S stMsg;

    while (1)
    {
        Event_Read(g_hVTimerEventID, _VTIMER_TIMER_EVENT, &uiEvent,
                EVENT_FLAG_WAIT, BS_WAIT_FOREVER);

        if (uiEvent & _VTIMER_TIMER_EVENT)
        {
            while(BS_OK == MSGQUE_ReadMsg(g_hVTimerQueId, &stMsg))
            {
                if (_VTIMER_MSG_TYPE == HANDLE_UINT(stMsg.ahMsg[0]))
                {
                    _VTimer_ProcMsg(HANDLE_UINT(stMsg.ahMsg[2]));
                }
            }
        }
    }
}

static BS_STATUS _vtimer_InitOnce(void *ud)
{
    UINT ulLen;

    if (BS_OK != SYSCFG_GetKeyValueAsUint("vtimer", "capacity",
                &g_ulMaxVTimerNum)) {
        g_ulMaxVTimerNum = _VTIMER_DFT_NUM;
    }

    if (g_ulMaxVTimerNum > _VTIMER_MAX_NUM) {
        g_ulMaxVTimerNum = _VTIMER_MAX_NUM;
    }

    ulLen = g_ulMaxVTimerNum * sizeof(USHORT);
    g_pusIncNum = MEM_Malloc(ulLen);
    if (NULL == g_pusIncNum)
    {
        goto Err;
    }
    

    ulLen = g_ulMaxVTimerNum * sizeof(_VTIMER_CTRL_S);
    g_pstVTimerCtrl = MEM_Malloc(ulLen);
    if (NULL == g_pstVTimerCtrl)
    {
        goto Err;
    }
    Mem_Zero(g_pstVTimerCtrl, ulLen);

    if (NULL == (g_hVTimerEventID = Event_Create()))
    {
        goto Err;
    }

    if (NULL == (g_hVTimerQueId = MSGQUE_Create(128)))
    {
        goto Err;
    }

    if (THREAD_ID_INVALID == (g_ulVTimerThreadID = 
                THREAD_Create("VTimer", NULL, _VTimer_Main, NULL)))
    {
        goto Err;
    }

    return BS_OK;

Err:
    if (g_pstVTimerCtrl)
    {
        MEM_Free(g_pstVTimerCtrl);
        g_pstVTimerCtrl = NULL;
    }

    if (g_pusIncNum)
    {
        MEM_Free(g_pusIncNum);
        g_pusIncNum = NULL;
    }

    if (g_hVTimerQueId)
    {
        MSGQUE_Delete(g_hVTimerQueId);
        g_hVTimerQueId = NULL;
    }

    if (g_hVTimerEventID)
    {
        Event_Delete(g_hVTimerEventID);
        g_hVTimerEventID = 0;
    }

    RETURN(BS_ERR);
}

static void _vtimer_Init()
{
    static ATOM_ONCE_S once = ATOM_ONCE_INIT_VALUE;
    AtomOnce_WaitDo(&once, _vtimer_InitOnce, NULL);
}

BS_STATUS VTimer_Create
(
    IN UINT ulTime, 
    IN UINT flag,
    IN PF_TIME_OUT_FUNC pfFunc,
    IN USER_HANDLE_S *pstUserHandle,
    OUT HANDLE *phVTimerID
)
{
    UINT i;
    UINT ulVTimerID;
    _VTIMER_CTRL_S *pstVTimer = NULL;
    USER_HANDLE_S stlUserHandle;

    BS_DBGASSERT(NULL != pfFunc);
    BS_DBGASSERT(NULL != phVTimerID);

    _vtimer_Init();

    if (NULL == g_hVTimerQueId)
    {
        BS_WARNNING(("VTimer not init"));
        RETURN(BS_NOT_INIT);
    }

    SPLX_P();
    for (i=0; i<g_ulMaxVTimerNum; i++)
    {
        if (g_pstVTimerCtrl[i].bIsUsed == FALSE)
        {
            pstVTimer = &g_pstVTimerCtrl[i];
            pstVTimer->ulTime = ulTime;
            pstVTimer->pfFunc = pfFunc;
            pstVTimer->flag = flag;

            if (NULL != pstUserHandle)
            {
                pstVTimer->stUserHandle = *pstUserHandle;
            }

            pstVTimer->bIsUsed = TRUE;

            break;
        }
    }
    SPLX_V();

    if (NULL == pstVTimer) {
        RETURN(BS_NO_RESOURCE);
    }

    ulVTimerID = (i+1) | ((UINT)g_pusIncNum[i] << 16);
    g_pusIncNum[i]++;
    
    pstVTimer->ulVTimerID = ulVTimerID;

    Mem_Zero(&stlUserHandle, sizeof(USER_HANDLE_S));
    stlUserHandle.ahUserHandle[0] = UINT_HANDLE(ulVTimerID);

    pstVTimer->hTimer = Timer_Create(ulTime, flag, _VTimer_ProcTimerEvent,
            &stlUserHandle);
    if (pstVTimer->hTimer == NULL)
    {
        BS_WARNNING(("Can't create timer"));
        SPLX_P();
        g_pstVTimerCtrl[ulVTimerID-1].bIsUsed = FALSE;
        SPLX_V();
        
        return BS_ERR;
    }

    *phVTimerID = UINT_HANDLE(ulVTimerID);

    return BS_OK;
}

BS_STATUS VTimer_Delete(IN HANDLE hVTimerID)
{
    _VTIMER_CTRL_S *pstVTimer;
    UINT ulVTimerID = HANDLE_UINT(hVTimerID);

    pstVTimer = _VTimer_GetVTimerByID(ulVTimerID);
    if (NULL == pstVTimer)
    {
        return BS_OK;
    }

    Timer_Delete(pstVTimer->hTimer);

    SPLX_P();
    pstVTimer->bIsUsed = FALSE;
    SPLX_V();

    return BS_OK;
}

BS_STATUS VTimer_Pause(IN HANDLE hVTimerID)
{
    _VTIMER_CTRL_S *pstVTimer;
    UINT ulVTimerID = HANDLE_UINT(hVTimerID);

    pstVTimer = _VTimer_GetVTimerByID(ulVTimerID);
    if (NULL == pstVTimer)
    {
        return BS_OK;
    }

    Timer_Pause(pstVTimer->hTimer);
    pstVTimer->flag |= TIMER_FLAG_PAUSE;

    return BS_OK;
}

BS_STATUS VTimer_Resume(IN HANDLE hVTimerID)
{
    _VTIMER_CTRL_S *pstVTimer;
    UINT ulVTimerID = HANDLE_UINT(hVTimerID);

    pstVTimer = _VTimer_GetVTimerByID(ulVTimerID);
    if (NULL == pstVTimer)
    {
        return BS_OK;
    }

    if (! TIMER_IS_PAUSE(pstVTimer->flag))
    {
        RETURN(BS_NO_PERMIT);
    }

    Timer_Resume(pstVTimer->hTimer);
    TIMER_CLR_PAUSE(pstVTimer->flag);

    return BS_OK;
}

BS_STATUS VTimer_GetInfo(IN HANDLE hVTimerID, OUT TIMER_INFO_S *pstTimerInfo)
{
    _VTIMER_CTRL_S *pstVTimer;
    UINT ulVTimerID = HANDLE_UINT(hVTimerID);

    BS_DBGASSERT(NULL != pstTimerInfo);

    pstVTimer = _VTimer_GetVTimerByID(ulVTimerID);
    if (NULL == pstVTimer)
    {
        return BS_OK;
    }

    pstTimerInfo->ulTime = pstVTimer->ulTime;
    pstTimerInfo->flag = pstVTimer->flag;
    pstTimerInfo->pfFunc = pstVTimer->pfFunc;

    return BS_OK;    
}

BS_STATUS VTimer_ReSetTime(IN HANDLE hVTimerID, IN UINT ulTime)
{
    _VTIMER_CTRL_S *pstVTimer;
    UINT ulVTimerID = HANDLE_UINT(hVTimerID);

    pstVTimer = _VTimer_GetVTimerByID(ulVTimerID);
    if (NULL == pstVTimer)
    {
        return BS_OK;
    }

    if (BS_OK != Timer_ReSetTime(pstVTimer->hTimer, ulTime))
    {
        RETURN(BS_ERR);
    }

    pstVTimer->ulTime = ulTime;

    return BS_OK;
}


