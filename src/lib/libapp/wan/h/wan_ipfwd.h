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
#endif 

BS_STATUS WAN_IpFwd_Input (IN MBUF_S *pstMbuf);
BS_STATUS WAN_IpFwd_Output
(
    IN MBUF_S *pstMbuf,
    IN UINT uiDstIp,    
    IN UINT uiSrcIp,    
    IN UCHAR ucProto
);

BS_STATUS WAN_IpFwd_Send(IN MBUF_S *pstMbuf);

#ifdef __cplusplus
    }
#endif 

#endif 


