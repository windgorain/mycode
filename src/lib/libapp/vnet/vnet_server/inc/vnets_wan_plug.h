/******************************************************************************
* Copyright (C) Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2013-3-29
* Description: 
* History:     
******************************************************************************/

#ifndef __VNETS_WAN_PLUG_H_
#define __VNETS_WAN_PLUG_H_

#include "comp/comp_wan.h"

#ifdef __cplusplus
    extern "C" {
#endif 

typedef BS_STATUS (*PF_VNETS_WAN_PLUG_UDP_SERVICE_FUNC)(IN MBUF_S *pstMbuf, IN WAN_UDP_SERVICE_PARAM_S *pstParam);

BS_STATUS VNETS_WAN_PLUG_Init();

BS_STATUS VNETS_WAN_PLUG_DomainEvent
(
    IN VNETS_DOMAIN_RECORD_S *pstParam,
    IN UINT uiEvent
);

BS_STATUS VNETS_WAN_PLUG_PktInput(IN MBUF_S *pstMbuf);

#ifdef __cplusplus
    }
#endif 

#endif 


