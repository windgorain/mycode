/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2014-7-5
* Description: 
* History:     
******************************************************************************/

#ifndef __MYPOLL_PROTO_H_
#define __MYPOLL_PROTO_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

 BS_STATUS _Mypoll_Proto_Init(IN _MYPOLL_CTRL_S *pstMyPoll);
VOID _MyPoll_Proto_Fini(IN _MYPOLL_CTRL_S *pstMyPoll);
BS_STATUS _Mypoll_Proto_Add
(
    IN _MYPOLL_CTRL_S *pstMyPoll,
    IN INT iSocketId,
    IN UINT uiEvent,
    IN PF_MYPOLL_EV_NOTIFY pfNotifyFunc
);
BS_STATUS _Mypoll_Proto_Set
(
    IN _MYPOLL_CTRL_S *pstMyPoll,
    IN INT iSocketId,
    IN UINT uiEvent,
    IN PF_MYPOLL_EV_NOTIFY pfNotifyFunc
);
VOID _Mypoll_Proto_Del
(
    IN _MYPOLL_CTRL_S *pstMyPoll,
    IN INT iSocketId
);

BS_WALK_RET_E _Mypoll_Proto_Run(IN _MYPOLL_CTRL_S *pstMyPoll);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__MYPOLL_PROTO_H_*/


