/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2008-9-20
* Description: 
* History:     
******************************************************************************/

#ifndef __MAC_TABLE_H_
#define __MAC_TABLE_H_

#include "utl/net.h"
#include "utl/eth_utl.h"

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

typedef struct MAC_TBL_STRUCT *MACTBL_HANDLE;

#define MAC_NODE_FLAG_STATIC          0x1             /* 静态/动态位 */
#define MAC_NODE_FLAG_SEND_UP         0x2             /* 上送位 */
#define MAC_NODE_FLAG_MONITOR_UP      0x4             /* 镜像上送 */

typedef struct
{
    MAC_ADDR_S stMac;
    UINT uiFlag;
    UINT uiPRI; /* 优先级, 数字越大优先级越高 */
}MAC_NODE_S;

typedef enum
{
    MAC_MODE_SET,               /* 覆盖,即便是高优先级也被覆盖 */
    MAC_MODE_LEARN,             /* 覆盖低优先级表项, 但不覆盖高优先级表项 */

    MAC_MODE_MAX
}MAC_NODE_MODE_E;

typedef VOID (*PF_MACTBL_WALK_FUNC)
   (IN MACTBL_HANDLE hMacTbl,
    IN MAC_NODE_S *pstNode,
    IN VOID *pUserData,
    IN VOID * pUserHandle);

/* 事件通知函数 */
#define MAC_TBL_EVENT_ADD 0x1
#define MAC_TBL_EVENT_DEL 0x2   /* 不包含老化 */
#define MAC_TBL_EVENT_OLD 0x4
typedef VOID (*PF_MACTBL_NOTIFY_FUNC)
(
    IN MACTBL_HANDLE hMacTbl,
    IN UINT uiEvent,
    IN MAC_NODE_S *pstNode,
    IN VOID *pUserData,
    IN USER_HANDLE_S *pstUserHandle
);

MACTBL_HANDLE MACTBL_CreateInstance(UINT uiUserDataSize/* 用户数据大小 */);
void MACTBL_DestoryInstance(IN MACTBL_HANDLE hMacTbl);
void MACTBL_SetOldTick(MACTBL_HANDLE mactbl, UINT old_tick);
void MACTBL_SetNotify(MACTBL_HANDLE mactbl, UINT uiEvent,
        PF_MACTBL_NOTIFY_FUNC pfNotifyFunc, USER_HANDLE_S *pstUserHandle);
BS_STATUS MACTBL_Add
(
    IN MACTBL_HANDLE hMacTbl,
    IN MAC_NODE_S *pstMacNode,
    IN VOID *pUserData,
    IN MAC_NODE_MODE_E eMode
);
BS_STATUS MACTBL_Del(IN MACTBL_HANDLE hMacTbl, IN MAC_NODE_S *pstMacNode);
BS_STATUS MACTBL_Find(IN MACTBL_HANDLE hMacTbl, INOUT MAC_NODE_S *pstMacNode, OUT VOID *pUserData);
VOID MACTBL_Walk(IN MACTBL_HANDLE hMacTbl, IN PF_MACTBL_WALK_FUNC pfFunc, IN VOID *pUserHandle);
/* 触发一次老化 */
VOID MACTBL_TickStep(IN MACTBL_HANDLE hMacTbl);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__MAC_TABLE_H_*/


