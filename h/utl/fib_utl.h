/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2007-9-30
* Description: 
* History:     
******************************************************************************/

#ifndef __FIB_UTL_H_
#define __FIB_UTL_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

#include "utl/net.h"

typedef HANDLE FIB_HANDLE;

#define FIB_MAX_NEXT_HOP_NUM 8 /* 等价路由最多的条数 */

#define FIB_FLAG_DELIVER_UP 0x1      /* inloopback, input报文命中此fib表示要deliver up, Flag字母:U */
#define FIB_FLAG_DIRECT     0x2      /* 直连路由, Flag字母:D */
#define FIB_FLAG_STATIC     0x4      /* 静态路由, Flag字母:S */
#define FIB_FLAG_AUTO_IF        0x8      /* 动态出接口fib, Flag字母:I */

typedef struct
{
    UINT uiDstOrStartIp; /* dst用于fib,网络序, start用于rangefib,主机序 */
    UINT uiMaskOrEndIp;  /* mask用于fib,网络序, endip用于rangefib,主机序 */
}FIB_KEY_S;

typedef struct
{
    FIB_KEY_S stFibKey;
    UINT uiNextHop;      /* 网络序 */
    UINT uiOutIfIndex;
    UINT uiFlag;
}FIB_NODE_S;

typedef BS_WALK_RET_E (*PF_FIB_WALK_FUNC)(IN FIB_NODE_S *pstFibNode, IN HANDLE hUserHandle);

#define FIB_INSTANCE_FLAG_CREATE_LOCK 0x1

FIB_HANDLE FIB_Create(IN UINT uiInstanceFlag /* FIB_INSTANCE_FLAG_XXX */);
VOID FIB_Destory(IN FIB_HANDLE hFibHandle);
BS_STATUS FIB_Add(IN FIB_HANDLE hFibHandle, IN FIB_NODE_S *pstFibNode);
/* 关键字: dst,mask,flag的Static位
           当flag为static时, 如果nexthop为0,表示删除所有nexthop.否则只删除对应的nexthop
           当flag为非static时,忽略nexthop */
VOID FIB_Del(IN FIB_HANDLE hFibHandle, IN FIB_NODE_S *pstFibNode);
VOID FIB_DelAll(IN FIB_HANDLE hFibHandle);
BS_STATUS FIB_Find(IN FIB_HANDLE hFibHandle, IN UINT ip/* net order */, IN UINT mask/*net order*/);
BS_STATUS FIB_PrefixMatch(IN FIB_HANDLE hFibHandle, IN UINT uiDstIp /* 网络序 */, OUT FIB_NODE_S *pstFibNode);
VOID FIB_Walk(IN FIB_HANDLE hFibHandle, IN PF_FIB_WALK_FUNC pfWalkFunc, IN HANDLE hUserHandle);
BS_STATUS FIB_GetNext
(
    IN FIB_HANDLE hFibHandle,
    IN FIB_NODE_S *pstFibCurrent/* 如果为NULL表示获取第一个 */,
    OUT FIB_NODE_S *pstFibNext
);
BS_STATUS FIB_Show (IN FIB_HANDLE hFibHandle);
CHAR * FIB_GetFlagString(IN UINT uiFlag, OUT CHAR *pcFlagString);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__FIB_UTL_H_*/


