/******************************************************************************
* Copyright (C), 2000-2006,  Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2017-4-28
* Description: 
* History:     
******************************************************************************/

#ifndef __WAN_ARP_AGENT_H_
#define __WAN_ARP_AGENT_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

BS_STATUS WAN_ArpAgent_Init();
BOOL_T WAN_ArpAgent_IsAgentOn(IN IF_INDEX ifIndex, IN UINT uiIP/* 主机序 */);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__WAN_ARP_AGENT_H_*/


