/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2011-3-24
* Description: 
* History:     
******************************************************************************/

#ifndef __MYPOLL_UTL_H_
#define __MYPOLL_UTL_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

typedef HANDLE MYPOLL_HANDLE;

/* mypoll 事件 */
#ifdef IN_LINUX
#include <sys/epoll.h>
#define MYPOLL_EVENT_IN  EPOLLIN
#define MYPOLL_EVENT_OUT EPOLLOUT
#define MYPOLL_EVENT_ERR EPOLLERR
#define MYPOLL_EVENT_HUP EPOLLHUP
#else
#define MYPOLL_EVENT_IN  0x1
#define MYPOLL_EVENT_OUT 0x2
#define MYPOLL_EVENT_ERR 0x4
#define MYPOLL_EVENT_HUP 0x10
#endif
#define MYPOLL_EVENT_ALL (MYPOLL_EVENT_IN | MYPOLL_EVENT_OUT | MYPOLL_EVENT_ERR)


typedef BS_WALK_RET_E (*PF_MYPOLL_EV_NOTIFY)(IN INT iSocketId, IN UINT uiEvent, IN USER_HANDLE_S *pstUserHandle);
typedef BS_WALK_RET_E (*PF_MYPOLL_USER_EVENT_FUNC)(IN UINT uiEvent, IN USER_HANDLE_S *pstUserHandle);
typedef BS_WALK_RET_E (*PF_MYPOLL_SIGNAL_FUNC)(IN int signum);

MYPOLL_HANDLE MyPoll_Create();
VOID MyPoll_Destory(IN MYPOLL_HANDLE hMypoll);
BS_STATUS MyPoll_SetEvent
(
    IN MYPOLL_HANDLE hMypoll,
    IN INT iSocketId,
    IN UINT uiEvent,
    IN PF_MYPOLL_EV_NOTIFY pfNotifyFunc,
    IN USER_HANDLE_S *pstUserHandle
);
BS_STATUS MyPoll_AddEvent(IN MYPOLL_HANDLE hMypoll, IN INT iSocketId, IN UINT uiEvent);
BS_STATUS MyPoll_DelEvent(IN MYPOLL_HANDLE hMypoll, IN INT iSocketId, IN UINT uiEvent);
BS_STATUS MyPoll_ClearEvent(IN MYPOLL_HANDLE hMypoll, IN INT iSocketId);
BS_STATUS MyPoll_ModifyEvent(IN MYPOLL_HANDLE hMypoll, IN INT iSocketId, IN UINT uiEvent);
BS_STATUS MyPoll_ModifyUserHandle(MYPOLL_HANDLE hMypoll,
        INT iSocketId, USER_HANDLE_S *pstUserHandle);
USER_HANDLE_S * MyPoll_GetUserHandle(MYPOLL_HANDLE hMypoll, INT iSocketId);
VOID MyPoll_Del(IN MYPOLL_HANDLE hMypoll, IN INT iSocketId);
BS_STATUS MyPoll_SetSignalProcessor(IN MYPOLL_HANDLE hMypoll, IN INT signo, IN PF_MYPOLL_SIGNAL_FUNC pfFunc);
BS_STATUS MyPoll_SetUserEventProcessor(IN MYPOLL_HANDLE hMypoll, IN PF_MYPOLL_USER_EVENT_FUNC pfFunc, IN USER_HANDLE_S *pstUserHandle);
BS_STATUS MyPoll_PostUserEvent(IN MYPOLL_HANDLE hMypoll, IN UINT uiEvent);
BS_STATUS MyPoll_Trigger(MYPOLL_HANDLE hMyPoll);
BS_WALK_RET_E MyPoll_Run(IN MYPOLL_HANDLE hMypoll);
VOID MyPoll_Restart(IN MYPOLL_HANDLE hMypoll);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__MYPOLL_UTL_H_*/


