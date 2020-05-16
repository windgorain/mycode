/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2013-3-29
* Description: 
* History:     
******************************************************************************/

#ifndef __WAN_UDP_SERVICE_H_
#define __WAN_UDP_SERVICE_H_

#include "comp/comp_wan.h"

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

VOID WanUdpService_Init();
BS_STATUS WanUdpService_Input(IN MBUF_S *pstMbuf);
BS_STATUS WanUdpService_Output(IN MBUF_S *pstMbuf, IN WAN_UDP_SERVICE_PARAM_S *pstParam);
BS_STATUS WanUdpService_RegService
(
    IN USHORT usPort/* 网络序 */,
    IN UINT uiFlag, /* WAN_UDP_SERVICE_FLAG_XXX */
    IN PF_WAN_SERVICE_FUNC pfServiceFunc,
    IN USER_HANDLE_S *pstUserHandle
);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__WAN_UDP_SERVICE_H_*/


