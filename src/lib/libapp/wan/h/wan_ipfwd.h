/******************************************************************************
* Copyright (C) Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2013-1-24
* Description: 
* History:     
******************************************************************************/

#ifndef __WAN_IPFWD_H_
#define __WAN_IPFWD_H_

#include "utl/mbuf_utl.h"

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

BS_STATUS WAN_IpFwd_Input (IN MBUF_S *pstMbuf);
BS_STATUS WAN_IpFwd_Output
(
    IN MBUF_S *pstMbuf,
    IN UINT uiDstIp,    /* 网络序 */
    IN UINT uiSrcIp,    /* 网络序 */
    IN UCHAR ucProto
);
/* 相比于OutPut, 不再填写IP头的东西,认为上面已经填写好了 */
BS_STATUS WAN_IpFwd_Send(IN MBUF_S *pstMbuf);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__WAN_IPFWD_H_*/


