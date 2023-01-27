/******************************************************************************
* Copyright (C),  LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2011-3-24
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/darray_utl.h"
#include "utl/mypoll_utl.h"
#include "utl/socket_utl.h"
#include "utl/bit_opt.h"
#include "utl/signal_utl.h"

#include "mypoll_inner.h"
#include "mypoll_proto.h"

static _MYPOLL_CTRL_S * g_mypoll_signal = NULL;

static BS_WALK_RET_E mypoll_ProcessUserEvent(_MYPOLL_CTRL_S *pstCtrl)
{
    UINT uiUserEvent;

    if (NULL != pstCtrl->pfUserEventFunc) {
        MUTEX_P(&pstCtrl->lock);
        uiUserEvent = pstCtrl->uiUserEvent;
        pstCtrl->uiUserEvent = 0;
        MUTEX_V(&pstCtrl->lock);

        if (uiUserEvent != 0) {
            return pstCtrl->pfUserEventFunc(uiUserEvent, &(pstCtrl->stUserEventUserHandle));
        }
    }

    return BS_WALK_CONTINUE;
}

static BS_WALK_RET_E mypoll_ProcessSignal(_MYPOLL_CTRL_S *pstCtrl)
{
    int i;

    for (i=0; i<_MYPOLL_MAX_SINGAL_NUM; i++) {
        if (pstCtrl->singal_processers[i].pfFunc != NULL) {
            if (pstCtrl->singal_processers[i].uiCount != 0) {
                pstCtrl->singal_processers[i].uiCount = 0;
                if (BS_WALK_STOP == pstCtrl->singal_processers[i].pfFunc(i)) {
                    return BS_WALK_STOP;
                }
            }
        }
    }

    return BS_WALK_CONTINUE;
}

STATIC BS_WALK_RET_E mypoll_NotifyTrigger
(
    IN INT iSocketId,
    IN UINT uiEvent,
    IN USER_HANDLE_S *pstUserHandle
)
{
    _MYPOLL_CTRL_S *pstCtrl = pstUserHandle->ahUserHandle[0];
    UCHAR aucData[256];
	UINT uiReadLen;

    (VOID) Socket_Read2(pstCtrl->iSocketDst, aucData, sizeof(aucData), &uiReadLen, 0);

    if (BS_WALK_STOP == mypoll_ProcessUserEvent(pstCtrl)) {
        return BS_WALK_STOP;
    }

    if (BS_WALK_STOP == mypoll_ProcessSignal(pstCtrl)) {
        return BS_WALK_STOP;
    }

    return BS_WALK_CONTINUE;
}

STATIC BS_STATUS mypoll_SetFdInfo
(
    IN _MYPOLL_CTRL_S *pstCtrl,
    IN INT iSocketId,
    IN UINT uiEvent,
    IN PF_MYPOLL_EV_NOTIFY pfNotifyFunc,
    IN USER_HANDLE_S *pstUserHandle
)
{
    MYPOLL_FDINFO_S *pstFdInfo;

    pstFdInfo = DARRAY_Get(pstCtrl->hFdInfoTbl, iSocketId);
    if (NULL == pstFdInfo)
    {
        pstFdInfo = MEM_ZMalloc(sizeof(MYPOLL_FDINFO_S));
        if (NULL == pstFdInfo)
        {
            return BS_NO_MEMORY;
        }
        if (BS_OK != DARRAY_Set(pstCtrl->hFdInfoTbl, iSocketId, pstFdInfo))
        {
            MEM_Free(pstFdInfo);
            return BS_ERR;
        }
    }

    pstFdInfo->pfNotifyFunc = pfNotifyFunc;
    pstFdInfo->uiEvent = uiEvent;
    if (BIT_ISSET(pstCtrl->uiFlag, _MYPOLL_FLAG_LOOP_ODD)) {
        BIT_SET(pstFdInfo->flag, _MYPOLL_FDINFO_LOOP_ODD);
    } else {
        BIT_CLR(pstFdInfo->flag, _MYPOLL_FDINFO_LOOP_ODD);
    }
    if (NULL != pstUserHandle)
    {
        pstFdInfo->stUserHandle = *pstUserHandle;
    }

    return BS_OK;
}

static inline MYPOLL_FDINFO_S *mypoll_GetFdInfo(IN _MYPOLL_CTRL_S *pstCtrl, IN INT iSocketId)
{
    return DARRAY_Get(pstCtrl->hFdInfoTbl, iSocketId);
}

STATIC VOID mypoll_FreeFdInfo
(
    IN _MYPOLL_CTRL_S *pstCtrl,
    IN INT iSocketId
)
{
    MYPOLL_FDINFO_S *pstFdInfo;

    pstFdInfo = DARRAY_Clear(pstCtrl->hFdInfoTbl, iSocketId);
    if (NULL != pstFdInfo)
    {
        MEM_Free(pstFdInfo);
    }
}

static void mypoll_Signal(IN int iSigno)
{
    if (g_mypoll_signal == NULL) {
        return;
    }

    g_mypoll_signal->singal_processers[iSigno].uiCount ++;
    Socket_Write((UINT)g_mypoll_signal->iSocketSrc, (char*)"0", 1, 0);
}

MYPOLL_HANDLE MyPoll_Create(void)
{
    _MYPOLL_CTRL_S *pstCtrl;
    INT aiFd[2];
    USER_HANDLE_S stUserHandle;

    pstCtrl = MEM_ZMalloc(sizeof(_MYPOLL_CTRL_S));
    if (NULL == pstCtrl)
    {
        return NULL;
    }
    pstCtrl->iSocketDst = -1;
    pstCtrl->iSocketSrc = -1;

    if (BS_OK != Socket_Pair(SOCK_STREAM, aiFd))
    {
        MyPoll_Destory(pstCtrl);
        return NULL;
    }
    pstCtrl->iSocketSrc = aiFd[0];
    pstCtrl->iSocketDst = aiFd[1];

    Socket_SetNoBlock(aiFd[0], 1);
    Socket_SetNoBlock(aiFd[1], 1);
    Socket_SetNoDelay(aiFd[0], 1);
    Socket_SetNoDelay(aiFd[1], 1);

    pstCtrl->hFdInfoTbl = DARRAY_Create(1024, 128);
    if (NULL == pstCtrl->hFdInfoTbl)
    {
        MyPoll_Destory(pstCtrl);
        return NULL;
    }

    if (BS_OK != _Mypoll_Proto_Init(pstCtrl))
    {
        MyPoll_Destory(pstCtrl);
        return NULL;
    }

    stUserHandle.ahUserHandle[0] = pstCtrl;
    if (BS_OK != MyPoll_SetEvent(pstCtrl, aiFd[1], MYPOLL_EVENT_IN,
                    mypoll_NotifyTrigger, &stUserHandle))
    {
        MyPoll_Destory(pstCtrl);
        return NULL;
    }

    MUTEX_Init(&pstCtrl->lock);

    return pstCtrl;
}

VOID MyPoll_Destory(IN MYPOLL_HANDLE hMypoll)
{
    _MYPOLL_CTRL_S *pstCtrl = (_MYPOLL_CTRL_S*)hMypoll;

    if (g_mypoll_signal == pstCtrl) {
        g_mypoll_signal = NULL;
    }

    _MyPoll_Proto_Fini(pstCtrl);

    if (pstCtrl->iSocketSrc >= 0)
    {
        Socket_Close(pstCtrl->iSocketSrc);
        Socket_Close(pstCtrl->iSocketDst);
    }

    if (pstCtrl->hFdInfoTbl != NULL)
    {
        DARRAY_Destory(pstCtrl->hFdInfoTbl);
    }

    MUTEX_Final(&pstCtrl->lock);

    MEM_Free(pstCtrl);
    
    return;
}

/* 设置事件位,会覆盖掉已有的事件位 */
BS_STATUS MyPoll_SetEvent
(
    IN MYPOLL_HANDLE hMypoll,
    IN INT iSocketId,
    IN UINT uiEvent,
    IN PF_MYPOLL_EV_NOTIFY pfNotifyFunc,
    IN USER_HANDLE_S *pstUserHandle
)
{
    _MYPOLL_CTRL_S *pstCtrl = (_MYPOLL_CTRL_S*)hMypoll;
    int add = 0;

    if (NULL == mypoll_GetFdInfo(pstCtrl, iSocketId)) {
        add = 1;
    }

    if (BS_OK != mypoll_SetFdInfo(pstCtrl, iSocketId, uiEvent, pfNotifyFunc, pstUserHandle))
    {
        return BS_ERR;
    }

    if (add) {
        return _Mypoll_Proto_Add(pstCtrl, iSocketId, uiEvent, pfNotifyFunc);
    } else {
        return _Mypoll_Proto_Set(pstCtrl, iSocketId, uiEvent, pfNotifyFunc);
    }
}

BS_STATUS MyPoll_ModifyUserHandle(MYPOLL_HANDLE hMypoll,
        INT iSocketId, USER_HANDLE_S *pstUserHandle)
{
    _MYPOLL_CTRL_S *pstCtrl = (_MYPOLL_CTRL_S*)hMypoll;
    MYPOLL_FDINFO_S *info;

    info = mypoll_GetFdInfo(pstCtrl, iSocketId);
    if (NULL == info) {
        RETURN(BS_NOT_FOUND);
    }

    if (pstUserHandle) {
        info->stUserHandle = *pstUserHandle;
    }

    return BS_OK;
}

USER_HANDLE_S * MyPoll_GetUserHandle(MYPOLL_HANDLE hMypoll, INT iSocketId)
{
    _MYPOLL_CTRL_S *pstCtrl = (_MYPOLL_CTRL_S*)hMypoll;
    MYPOLL_FDINFO_S *info;

    info = mypoll_GetFdInfo(pstCtrl, iSocketId);
    if (NULL == info) {
        return NULL;
    }

    return &info->stUserHandle;
}

/* 在原有事件位的基础上, 增加新的事件位 */
BS_STATUS MyPoll_AddEvent(IN MYPOLL_HANDLE hMypoll, IN INT iSocketId, IN UINT uiEvent)
{
    _MYPOLL_CTRL_S *pstCtrl = (_MYPOLL_CTRL_S*)hMypoll;
    MYPOLL_FDINFO_S *pstFdInfo;

    pstFdInfo = mypoll_GetFdInfo(pstCtrl, iSocketId);
    if (NULL == pstFdInfo)
    {
        return BS_ERR;
    }

    if ((pstFdInfo->uiEvent & uiEvent) == uiEvent)
    {
        return BS_OK;
    }

    pstFdInfo->uiEvent |= uiEvent;

    return MyPoll_SetEvent(hMypoll, iSocketId, pstFdInfo->uiEvent, pstFdInfo->pfNotifyFunc, &pstFdInfo->stUserHandle);
}

/* 在原有事件位的基础上, 删除一些事件位 */
BS_STATUS MyPoll_DelEvent(IN MYPOLL_HANDLE hMypoll, IN INT iSocketId, IN UINT uiEvent)
{
    _MYPOLL_CTRL_S *pstCtrl = (_MYPOLL_CTRL_S*)hMypoll;
    MYPOLL_FDINFO_S *pstFdInfo;

    pstFdInfo = mypoll_GetFdInfo(pstCtrl, iSocketId);
    if (NULL == pstFdInfo)
    {
        return BS_ERR;
    }

    if ((pstFdInfo->uiEvent & uiEvent) == 0)
    {
        return BS_OK;
    }

    pstFdInfo->uiEvent &= (~uiEvent);

    return MyPoll_SetEvent(hMypoll, iSocketId, pstFdInfo->uiEvent, pstFdInfo->pfNotifyFunc, &pstFdInfo->stUserHandle);
}

/* 清除所有事件位 */
BS_STATUS MyPoll_ClearEvent(IN MYPOLL_HANDLE hMypoll, IN INT iSocketId)
{
    return MyPoll_DelEvent(hMypoll, iSocketId, MYPOLL_EVENT_ALL);
}

/* 修改事件位, 覆盖掉原有事件位 */
BS_STATUS MyPoll_ModifyEvent(IN MYPOLL_HANDLE hMypoll, IN INT iSocketId, IN UINT uiEvent)
{
    _MYPOLL_CTRL_S *pstCtrl = (_MYPOLL_CTRL_S*)hMypoll;
    MYPOLL_FDINFO_S *pstFdInfo;

    pstFdInfo = mypoll_GetFdInfo(pstCtrl, iSocketId);
    if (NULL == pstFdInfo)
    {
        return BS_ERR;
    }

    if (pstFdInfo->uiEvent == uiEvent)
    {
        return BS_OK;
    }

    pstFdInfo->uiEvent = uiEvent;

    return MyPoll_SetEvent(hMypoll, iSocketId, pstFdInfo->uiEvent, pstFdInfo->pfNotifyFunc, &pstFdInfo->stUserHandle);
}

/* 不再关注fd */
VOID MyPoll_Del
(
    IN MYPOLL_HANDLE hMypoll,
    IN INT iSocketId
)
{
    _MYPOLL_CTRL_S *pstCtrl = (_MYPOLL_CTRL_S*)hMypoll;

    _Mypoll_Proto_Del(pstCtrl, iSocketId);

    mypoll_FreeFdInfo(pstCtrl, iSocketId);
}

BS_WALK_RET_E MyPoll_Run(IN MYPOLL_HANDLE hMypoll)
{
    _MYPOLL_CTRL_S *pstCtrl = (_MYPOLL_CTRL_S*)hMypoll;

    return _Mypoll_Proto_Run(pstCtrl);
}

/* 可能因为某些原因,当前poll出来的信息可能已经无效,需要重新poll获取数据 */
/* 此函数为pller回调函数中调用 */
VOID MyPoll_Restart(IN MYPOLL_HANDLE hMypoll)
{
    _MYPOLL_CTRL_S *pstCtrl = (_MYPOLL_CTRL_S*)hMypoll;

    pstCtrl->uiFlag |= _MYPOLL_FLAG_RESTART;
}

/* 设置遇到Signal处理函数 */
BS_STATUS MyPoll_SetSignalProcessor(IN MYPOLL_HANDLE hMypoll, IN INT signo, IN PF_MYPOLL_SIGNAL_FUNC pfFunc)
{
    _MYPOLL_CTRL_S *pstCtrl = (_MYPOLL_CTRL_S*)hMypoll;

    if (signo >= _MYPOLL_MAX_SINGAL_NUM) {
        RETURN(BS_OUT_OF_RANGE);
    }

    g_mypoll_signal = pstCtrl;
    pstCtrl->singal_processers[signo].pfFunc = pfFunc;

    SIGNAL_Set(signo, 0, (void*)mypoll_Signal);

    return BS_OK;
}

BS_STATUS MyPoll_SetUserEventProcessor(IN MYPOLL_HANDLE hMypoll,
        IN PF_MYPOLL_USER_EVENT_FUNC pfFunc,
        IN USER_HANDLE_S *pstUserHandle  /* 可以为NULL */)
{
    _MYPOLL_CTRL_S *pstCtrl = (_MYPOLL_CTRL_S*)hMypoll;

    pstCtrl->pfUserEventFunc = pfFunc;

    if (NULL != pstUserHandle) {
        pstCtrl->stUserEventUserHandle = *pstUserHandle;
    }

    return BS_OK;
}

/* 触发事件 */
BS_STATUS MyPoll_PostUserEvent(IN MYPOLL_HANDLE hMypoll, IN UINT uiEvent)
{
    _MYPOLL_CTRL_S *pstCtrl = (_MYPOLL_CTRL_S*)hMypoll;
    int need_trigger = 0;

    if (NULL != pstCtrl) {
        MUTEX_P(&pstCtrl->lock);
        if (pstCtrl->uiUserEvent == 0) {
            need_trigger = 1;
        }
        pstCtrl->uiUserEvent |= uiEvent;
        MUTEX_V(&pstCtrl->lock);

        if (need_trigger) {
            MyPoll_Trigger(hMypoll);
        }
    }

    return BS_OK;
}

/* 触发mypoller一次 */
BS_STATUS MyPoll_Trigger(MYPOLL_HANDLE hMyPoll)
{
    _MYPOLL_CTRL_S *pstCtrl = (_MYPOLL_CTRL_S*)hMyPoll;
    Socket_Write((UINT)pstCtrl->iSocketSrc, (char*)"0", 1, 0);

    return 0;
}

