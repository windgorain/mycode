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
#endif 

BS_STATUS WAN_IpFwdService_Init();

IPFWD_SERVICE_RET_E WAN_IpFwdService_Process
(
    IN IPFWD_SERVICE_PHASE_E ePhase,
    IN IP_HEAD_S *pstIpHead,
    IN MBUF_S *pstMbuf
);


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
#endif 

#endif 


