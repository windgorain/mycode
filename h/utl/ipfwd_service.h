/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2013-1-10
* Description: 
* History:     
******************************************************************************/

#ifndef __IPFWD_SERVICE_H_
#define __IPFWD_SERVICE_H_

#include "utl/ip_utl.h"

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */


typedef HANDLE IPFWD_SERVICE_HANDLE;

typedef enum
{
    IPFWD_SERVICE_PRE_ROUTING,               /* 接收报文预处理阶段 */
    IPFWD_SERVICE_LOCAL_IN,                  /* 到本机报文处理阶段 */
    IPFWD_SERVICE_NO_FIB,                    /* 没有找到对应的FIB */
    IPFWD_SERVICE_BEFORE_FORWARD,            /* 转发报文之前 */
    IPFWD_SERVICE_LOCAL_OUT,                 /* 本机发送报文处理阶段 */
    IPFWD_SERVICE_POST_ROUTING_BEFOREFRAG,   /* 出接口发送分片前预处理阶段 */
    IPFWD_SERVICE_POST_ROUTING_AFTERFRAG,    /* 出接口发送分片后预处理阶段 */
    IPFWD_SERVICE_BEFORE_DELIVER_UP,         /* deliver up之前 */

    IPFWD_SERVICE_PHASE_MAX
}IPFWD_SERVICE_PHASE_E;

typedef enum
{
    IPFWD_SERVICE_RET_CONTINUE = 0,   /* 继续进行后续处理 */
    IPFWD_SERVICE_RET_TAKE_OVER,      /* 业务已经将报文接管 */

    IPFWD_SERVICE_RET_MAX
}IPFWD_SERVICE_RET_E;

typedef IPFWD_SERVICE_RET_E (*PF_IPFWD_SERVICE_FUNC)(IN IP_HEAD_S *pstIpHead, IN MBUF_S *pstMbuf, IN USER_HANDLE_S *pstUserHandle);


IPFWD_SERVICE_HANDLE IPFWD_Service_Create();
VOID IPFWD_Service_Destory(IN IPFWD_SERVICE_HANDLE hIpFwdService);
IPFWD_SERVICE_RET_E IPFWD_Service_Handle
(
    IN IPFWD_SERVICE_HANDLE hIpFwdService,
    IN IPFWD_SERVICE_PHASE_E ePhase,
    IN IP_HEAD_S *pstIpHead,
    IN MBUF_S *pstMbuf
);

/*
所有的注册必须要在系统正式运行前注册完成.
如果某个系统不需要处理,到自己里面去判断,以免在注册过程中同时报文处理导致死机
*/
BS_STATUS IPFWD_Service_Reg
(
    IN IPFWD_SERVICE_HANDLE hIpFwdService,
    IN IPFWD_SERVICE_PHASE_E ePhase,
    IN UINT uiOrder, /* 优先级. 值越小优先级越高 */
    IN CHAR *pcName, /* 必须静态存在 */
    IN PF_IPFWD_SERVICE_FUNC pfFunc,
    IN USER_HANDLE_S *pstUserHandle
);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__IPFWD_SERVICE_H_*/


