/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2013-4-27
* Description: 
* History:     
******************************************************************************/

#ifndef __WAN_PCAP_H_
#define __WAN_PCAP_H_

#include "utl/mbuf_utl.h"

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

BS_STATUS WAN_PCAP_Init();
BS_STATUS WAN_PCAP_AutoCreateInterface();

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__WAN_PCAP_H_*/


