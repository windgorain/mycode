/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2007-8-16
* Description: 基础定时器,回调函数应尽快返回
* History:  2018.11.20 moved from bs to utl   
******************************************************************************/
#include "bs.h"

#include "utl/bit_opt.h"
#include "utl/timer_utl.h"

typedef struct
{
    UINT flag;
    UINT  ulTime;
    VCLOCK_HANDLE vclock_id;
    PF_TIME_OUT_FUNC pfFunc;
    USER_HANDLE_S stUserHandle;
}_TIMER_UTL_NODE_S;

static void timerutl_Lock(TIMER_UTL_CTRL_S *ctrl)
{
    MUTEX_P(&ctrl->lock);
}

static void timerutl_UnLock(TIMER_UTL_CTRL_S *ctrl)
{
    MUTEX_V(&ctrl->lock);
}

static void timerutl_os_Main(IN USER_HANDLE_S *pstUserHandle)
{
    TIMER_UTL_CTRL_S *ctrl = pstUserHandle->ahUserHandle[0];

    for (;;)
    {
        Sleep(ctrl->precision);
        VCLOCK_Step(ctrl->vclock);
    }
}

static void timerutl_os_init(TIMER_UTL_CTRL_S *ctrl)
{
    USER_HANDLE_S user_data;
    user_data.ahUserHandle[0] = ctrl;
    THREAD_Create("Timer", NULL, timerutl_os_Main, &user_data);
}

static BS_STATUS timerutl_CallBackFunc(TIMER_UTL_CTRL_S *ctrl, UINT timer_id)
{
    _TIMER_UTL_NODE_S *pstTimer, stTimer;
    UINT nap_id = timer_id;

    timerutl_Lock(ctrl);
    pstTimer = NAP_GetNodeByID(ctrl->node_pool, nap_id);
    if (NULL != pstTimer) {
        stTimer = *pstTimer;
        if (0 == (pstTimer->flag & TIMER_FLAG_CYCLE)) {
            pstTimer->flag |= TIMER_FLAG_PAUSE;
        }
    }
    timerutl_UnLock(ctrl);

    if (NULL != pstTimer) {
        stTimer.pfFunc(UINT_HANDLE(timer_id), &stTimer.stUserHandle);
    }

    return BS_OK;
}

static VOID timerutl_TimeOut(IN HANDLE hOsTimerId, IN USER_HANDLE_S *pstUserHandle)
{
    (VOID)timerutl_CallBackFunc(pstUserHandle->ahUserHandle[0], HANDLE_UINT(pstUserHandle->ahUserHandle[1]));
}

BS_STATUS timerutl_SetTimer(TIMER_UTL_CTRL_S *ctrl, IN UINT ulTime, IN UINT flag, IN UINT ulTimerID, OUT VCLOCK_HANDLE *phOsTimerID)
{
    USER_HANDLE_S stUserHandle;
    UINT ulTick;

    ulTick = (ulTime + (ctrl->precision - 1)) / ctrl->precision;

    stUserHandle.ahUserHandle[0] = ctrl;
    stUserHandle.ahUserHandle[1] = UINT_HANDLE(ulTimerID);

    *phOsTimerID = (void*)VCLOCK_CreateTimer(ctrl->vclock, ulTick, ulTick, flag, timerutl_TimeOut, &stUserHandle);

    if (NULL == *phOsTimerID) {
        return BS_ERR;
    }

    return BS_OK;
}

static void timerutl_DeleteTimer(TIMER_UTL_CTRL_S *ctrl, VCLOCK_HANDLE vclock_id)
{
    VCLOCK_DestroyTimer(ctrl->vclock, vclock_id);
}

BS_STATUS TimerUtl_Init(TIMER_UTL_CTRL_S *ctrl, int precision)
{
    NAP_PARAM_S param = {0};

    memset(ctrl, 0, sizeof(TIMER_UTL_CTRL_S));
    MUTEX_Init(&ctrl->lock);

    param.enType = NAP_TYPE_HASH;
    param.uiMaxNum = 0xffff;
    param.uiNodeSize = sizeof(_TIMER_UTL_NODE_S);
    ctrl->node_pool = NAP_Create(&param);
    NAP_EnableSeq(ctrl->node_pool, 0, 0xffff);
    ctrl->vclock = VCLOCK_CreateInstance(TRUE);
    ctrl->precision = precision;

    timerutl_os_init(ctrl);

    return 0;
}

HANDLE TimerUtl_Create(TIMER_UTL_CTRL_S *ctrl, UINT ulTime, UINT flag, PF_TIME_OUT_FUNC pfFunc, USER_HANDLE_S *pstUserHandle)
{
    _TIMER_UTL_NODE_S *pstTimer;

    BS_DBGASSERT(NULL != pfFunc);

    timerutl_Lock(ctrl);
    pstTimer = NAP_Alloc(ctrl->node_pool);
    timerutl_UnLock(ctrl);

    if (NULL == pstTimer) {
        ERROR_SET(BS_NO_MEMORY);
        return NULL;
    }

    pstTimer->ulTime = ulTime;
    pstTimer->flag = flag;
    pstTimer->pfFunc = pfFunc;
    if (NULL != pstUserHandle) {
        pstTimer->stUserHandle = *pstUserHandle;
    }

    UINT nap_id = NAP_GetIDByNode(ctrl->node_pool, pstTimer);
    UINT timer_id = nap_id;

    if (BS_OK != timerutl_SetTimer(ctrl, ulTime, flag,
                timer_id, &pstTimer->vclock_id)) {
        timerutl_Lock(ctrl);
        NAP_Free(ctrl->node_pool, pstTimer);
        timerutl_UnLock(ctrl);

        ERROR_SET(BS_ERR);

        return NULL;
    }

    return UINT_HANDLE(timer_id);
}

BS_STATUS TimerUtl_Delete(TIMER_UTL_CTRL_S *ctrl, HANDLE timer)
{
    UINT timer_id = HANDLE_UINT(timer);
    UINT nap_id;
    _TIMER_UTL_NODE_S * pstTimer;
 
    if (NULL == timer) {
        RETURN(BS_NO_SUCH);
    }

    nap_id = timer_id;

    timerutl_Lock(ctrl);
    pstTimer = NAP_GetNodeByID(ctrl->node_pool, nap_id);
    if (pstTimer) {
        timerutl_DeleteTimer(ctrl, pstTimer->vclock_id);
        NAP_Free(ctrl->node_pool, pstTimer);
    }
    timerutl_UnLock(ctrl);

    return BS_OK;
}

BS_STATUS TimerUtl_GetInfo(TIMER_UTL_CTRL_S *ctrl, HANDLE timer, OUT TIMER_INFO_S *pstTimerInfo)
{
    UINT timer_id = HANDLE_UINT(timer);
    UINT nap_id;
    _TIMER_UTL_NODE_S *pstTimer;

    BS_DBGASSERT(NULL != pstTimerInfo);
 
    if (timer == NULL) {
        RETURN(BS_BAD_PARA);
    }

    nap_id = timer_id;

    timerutl_Lock(ctrl);
    pstTimer = NAP_GetNodeByID(ctrl->node_pool, nap_id);
    if (pstTimer) {
        pstTimerInfo->ulTime = pstTimer->ulTime;
        pstTimerInfo->flag = pstTimer->flag;
        pstTimerInfo->pfFunc = pstTimer->pfFunc;
    }
    timerutl_UnLock(ctrl);

    if (! pstTimer) {
        RETURN(BS_NOT_FOUND);
    }

    return BS_OK;
}

BS_STATUS TimerUtl_Pause(TIMER_UTL_CTRL_S *ctrl, HANDLE timer)
{
    UINT timer_id = HANDLE_UINT(timer);
    UINT nap_id;
    _TIMER_UTL_NODE_S * pstTimer;

    if (timer == NULL) {
        RETURN(BS_NO_SUCH);
    }

    nap_id = timer_id;

    pstTimer = NAP_GetNodeByID(ctrl->node_pool, nap_id);
    if (NULL == pstTimer) {
        RETURN(BS_NO_SUCH);
    }


    if (pstTimer->flag & TIMER_FLAG_PAUSE) {
        return BS_OK;
    }

    timerutl_DeleteTimer(ctrl, pstTimer->vclock_id);
    pstTimer->flag |= TIMER_FLAG_PAUSE;
    pstTimer->vclock_id = 0;

    return BS_OK;
}

BS_STATUS TimerUtl_Resume(TIMER_UTL_CTRL_S *ctrl, HANDLE timer)
{
    UINT timer_id = HANDLE_UINT(timer);
    _TIMER_UTL_NODE_S * pstTimer;
    
    if (timer == NULL) {
        RETURN(BS_NO_SUCH);
    }

    UINT nap_id = timer_id;

    pstTimer = NAP_GetNodeByID(ctrl->node_pool, nap_id);
    if (NULL == pstTimer) {
        RETURN(BS_NO_SUCH);
    }

    if (! (pstTimer->flag & TIMER_FLAG_PAUSE)) {
        return BS_OK;
    }

    if (BS_OK != timerutl_SetTimer(ctrl, pstTimer->ulTime, pstTimer->flag, timer_id, &pstTimer->vclock_id)) {
        RETURN(BS_ERR);
    }

    BIT_CLR(pstTimer->flag, TIMER_FLAG_PAUSE);

    return BS_OK;
}

BS_STATUS TimerUtl_ReSetTime(TIMER_UTL_CTRL_S *ctrl, HANDLE timer, UINT ulTime)
{
    UINT timer_id = HANDLE_UINT(timer);
    _TIMER_UTL_NODE_S * pstTimer;
    VCLOCK_HANDLE new_vclock_id;

    if (timer == NULL) {
        RETURN(BS_NO_SUCH);
    }

    UINT nap_id = timer_id;

    pstTimer = NAP_GetNodeByID(ctrl->node_pool, nap_id);
    if (NULL == pstTimer) {
        RETURN(BS_NO_SUCH);
    }

    if (BS_OK != timerutl_SetTimer(ctrl, ulTime, pstTimer->flag, timer_id, &new_vclock_id)) {
        RETURN(BS_ERR);
    }

    timerutl_DeleteTimer(ctrl, pstTimer->vclock_id);

    pstTimer->vclock_id = new_vclock_id;
    pstTimer->ulTime = ulTime;

    return BS_OK;
}

