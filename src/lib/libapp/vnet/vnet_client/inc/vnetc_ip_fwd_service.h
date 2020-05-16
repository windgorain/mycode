

#ifndef __VNETC_IP_FWD_SERVICE_H_
#define __VNETC_IP_FWD_SERVICE_H_

#include "utl/mbuf_utl.h"
#include "utl/ip_utl.h"

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

typedef enum
{
    VNETC_IPFWD_SERVICE_PRE_ROUTING,               /* 接收报文预处理阶段 */
    VNETC_IPFWD_SERVICE_LOCAL_IN,                  /* 到本机报文处理阶段 */
    VNETC_IPFWD_SERVICE_FORWARD,                   /* 转发报文处理阶段 */
    VNETC_IPFWD_SERVICE_LOCAL_OUT,                 /* 本机发送报文处理阶段 */
    VNETC_IPFWD_SERVICE_POST_ROUTING_BEFOREFRAG,   /* 出接口发送分片前预处理阶段 */
    VNETC_IPFWD_SERVICE_POST_ROUTING_AFTERFRAG,    /* 出接口发送分片后预处理阶段 */
    VNETC_IPFWD_SERVICE_PRE_RELAY,                 /* 报文透传前 */
    VNETC_IPFWD_SERVICE_RELAY_RECEIVE,             /* 报文透传接收 */

    VNETC_IPFWD_SERVICE_PHASE_MAX
}VNETC_IPFWD_SERVICE_PHASE_E;

typedef enum
{
    VNETC_IPFWD_SERVICE_RET_CONTINUE = 0,   /* 继续进行后续处理 */
    VNETC_IPFWD_SERVICE_RET_TAKE_OVER,      /* 业务已经将报文接管 */
    VNETC_IPFWD_SERVICE_RET_ERR,            /* 产生错误 */

    VNETC_IPFWD_SERVICE_RET_MAX
}VNETC_IPFWD_SERVICE_RET_E;

typedef VNETC_IPFWD_SERVICE_RET_E (*PF_VNETC_IPFWD_SERVICE_FUNC)(IN IP_HEAD_S *pstIpHead, IN MBUF_S *pstMbuf);

/* 所有的注册必须要在系统正式运行前注册完成. 如果某个系统不需要处理,到自己里面去判断,以免在注册过程中同时报文处理导致死机 */
BS_STATUS VNETC_IpFwdService_Reg
(
    IN VNETC_IPFWD_SERVICE_PHASE_E ePhase,
    IN PF_VNETC_IPFWD_SERVICE_FUNC pfFunc,
    IN USER_HANDLE_S *pstUserHandle
);

VNETC_IPFWD_SERVICE_RET_E VNETC_IpFwdService_Handle
(
    IN VNETC_IPFWD_SERVICE_PHASE_E ePhase,
    IN IP_HEAD_S *pstIpHead,
    IN MBUF_S *pstMbuf
);

BS_STATUS VNETC_IpFwdService_Init();

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__VNETC_IP_FWD_SERVICE_H_*/




