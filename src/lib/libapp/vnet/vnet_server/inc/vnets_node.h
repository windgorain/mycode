/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2013-12-3
* Description: 
* History:     
******************************************************************************/
#ifndef __VNETS_NODE_H_
#define __VNETS_NODE_H_

#include "utl/net.h"

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

#define VNETS_NODE_INVALID_ID 0

#define VNETS_NODE_COOKIE_STRING_LEN 17

typedef struct
{
    UINT uiNodeID;
    UINT uiCookie;
    UINT uiDomainID;
    UINT uiSesID;
    UINT uiTpID;
    UINT uiIP;   /* 网络序 */
    UINT uiMask; /* 网络序 */
    MAC_ADDR_S stMACAddr;
    CHAR szUserName[VNET_CONF_MAX_USER_NAME_LEN + 1];
    CHAR szAlias[VNET_CONF_MAX_ALIAS_LEN + 1];
    CHAR szDescription[VNET_CONF_MAX_DES_LEN + 1];
}VNETS_NODE_S;

typedef struct
{
    UINT uiNodeID;

    UINT uiIP;   /* 网络序 */
    UINT uiMask; /* 网络序 */
    UINT uiSesID;
    UINT uiLoginTime; /* 登录时间. 格式:Time from init , 单位:s */
    MAC_ADDR_S stMACAddr;
    CHAR szUserName[VNET_CONF_MAX_USER_NAME_LEN + 1];
    CHAR szAlias[VNET_CONF_MAX_ALIAS_LEN + 1];
    CHAR szDescription[VNET_CONF_MAX_DES_LEN + 1];
}VNETS_NODE_INFO_S;

typedef BS_WALK_RET_E (*PF_VNETS_NODE_WALK_EACH)(IN UINT uiNodeID, IN HANDLE hUserHandle);

BS_STATUS VNETS_NODE_Init();
BS_STATUS VNETS_NODE_Init2();
UINT VNETS_NODE_AddNode(IN UINT uiSesID);
VOID VNETS_NODE_DelNode(IN UINT uiNodeID);
BS_STATUS VNETS_NODE_SetUserName(IN UINT uiNodeID, IN CHAR *pcUserName);
BS_STATUS VNETS_NODE_SetTPID(IN UINT uiNodeID, IN UINT uiTPID);
BS_STATUS VNETS_NODE_SetDomainID(IN UINT uiNodeID, IN UINT uiDomainID);
BS_STATUS VNETS_NODE_SetNodeInfo(IN UINT uiNodeID, IN VNETS_NODE_INFO_S *pstInfo);
VNETS_NODE_S * VNETS_NODE_GetNode(IN UINT uiNodeID);
UINT VNETS_NODE_GetCookie(IN UINT uiNodeID);
BS_STATUS VNETS_NODE_GetCookieString(IN UINT uiNodeID, OUT CHAR szCookieString[VNETS_NODE_COOKIE_STRING_LEN + 1]);
UINT VNETS_NODE_GetNodeIdByCookieString(IN CHAR *pcCookieString);
UINT VNETS_NODE_GetDomainID(IN UINT uiNodeID);
UINT VNETS_NODE_GetSesID(IN UINT uiNodeID);
BS_STATUS VNETS_NODE_GetNodeInfo(IN UINT uiNodeID, OUT VNETS_NODE_INFO_S *pstNodeInfo);
UINT VNETS_NODE_GetNext(IN UINT uiCurrentID);
VOID VNETS_NODE_Walk(IN PF_VNETS_NODE_WALK_EACH pfFunc, IN HANDLE hUserHandle);
UINT VNETS_NODE_Self();

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__VNETS_NODE_H_*/


