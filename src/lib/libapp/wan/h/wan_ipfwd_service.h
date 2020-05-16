/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2013-3-26
* Description: 
* History:     
******************************************************************************/

#ifndef __WAN_IPFWD_SERVICE_H_
#define __WAN_IPFWD_SERVICE_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

BS_STATUS WAN_IpFwdService_Init();

IPFWD_SERVICE_RET_E WAN_IpFwdService_Process
(
    IN IPFWD_SERVICE_PHASE_E ePhase,
    IN IP_HEAD_S *pstIpHead,
    IN MBUF_S *pstMbuf
);

/* 所有的注册必须要在系统正式运行前注册完成.
 如果某个系统不需要处理,到自己里面去判断,
 以免在注册过程中同时报文处理导致死机 */
BS_STATUS WAN_IpFwdService_Reg
(
    IN IPFWD_SERVICE_PHASE_E ePhase,
    IN UINT uiOrder,
    IN CHAR *pcName,
    IN PF_IPFWD_SERVICE_FUNC pfFunc,
    IN USER_HANDLE_S *pstUserHandle
);


#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__WAN_IPFWD_SERVICE_H_*/


