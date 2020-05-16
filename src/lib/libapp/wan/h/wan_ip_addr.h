/******************************************************************************
* Copyright (C) Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2013-4-2
* Description: 
* History:     
******************************************************************************/

#ifndef __WAN_IP_ADDR_H_
#define __WAN_IP_ADDR_H_

#include "utl/if_utl.h"
#include "app/wan_pub.h"

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

#define WAN_IP_ADDR_MAX_IF_IP_NUM 32   /* 支持的最多SubIP个数 */

typedef struct
{
    WAN_IP_ADDR_MODE_E enIpMode;
    WAN_IP_ADDR_INFO_S astIP[WAN_IP_ADDR_MAX_IF_IP_NUM];
}WAN_IP_ADDR_S;

#define WAN_IPADDR_EVENT_ADD_ADDR 0x1
#define WAN_IPADDR_EVENT_DEL_ADDR 0x2
#define WAN_IPADDR_EVENT_MODIFY_ADDR 0x4

typedef VOID (*PF_WAN_IPAddr_EventNotify)
(
    IN UINT uiEvent,
    IN IF_INDEX ifIndex,
    IN WAN_IP_ADDR_INFO_S *pstOld,
    IN WAN_IP_ADDR_INFO_S *pstNew,
    IN USER_HANDLE_S *pstUserHandle
);

BS_STATUS WAN_IPAddr_Init();
BS_STATUS WAN_IPAddr_KfInit();
BS_STATUS WAN_IPAddr_DelInterfaceAllIp(IN IF_INDEX ifIndex);
BS_STATUS WAN_IPAddr_GetFirstIp(IN UINT uiIfIndex, OUT WAN_IP_ADDR_INFO_S *pstAddr);
/* 在接口上最长匹配地址节点 */
BS_STATUS WAN_IPAddr_MatchBestNet(IN UINT uiIfIndex, IN UINT uiIpAddr, OUT WAN_IP_ADDR_INFO_S *pstAddr);
BOOL_T WAN_IPAddr_IsInterfaceIp(IN UINT uiIfIndex, IN UINT uiIP/* 网络序 */);
BS_STATUS WAN_IPAddr_SetMode(IN IF_INDEX ifIndex, IN WAN_IP_ADDR_MODE_E enMode);
WAN_IP_ADDR_MODE_E WAN_IPAddr_GetMode(IN IF_INDEX ifIndex);
BS_STATUS WAN_IPAddr_AddIp(IN WAN_IP_ADDR_INFO_S *pstAddrInfo);
BS_STATUS WAN_IPAddr_DelIp(IN IF_INDEX ifIndex, IN UINT uiIP);
BS_STATUS WAN_IPAddr_GetInterfaceAllIp(IN IF_INDEX ifIndex, OUT WAN_IP_ADDR_S *pstIpAddrs);
BS_STATUS WAN_IPAddr_RegListener(IN PF_WAN_IPAddr_EventNotify pfFunc, IN USER_HANDLE_S *pstUserHandle);

BS_STATUS WAN_IpAddrCmd_Init();

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__WAN_IP_ADDR_H_*/


