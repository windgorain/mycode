/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-1-23
* Description: 
* History:     
******************************************************************************/

#ifndef __VNETS_DC_H_
#define __VNETS_DC_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

#define VNET_DC_MAX_EMAIL_LEN 255

BS_STATUS VNETS_DC_Init();
BOOL_T VNETS_DC_IsDomainExist(IN CHAR *pcDomainName);
BOOL_T VNETS_DC_IsBroadcastPermit(IN CHAR *pcDomainName);
BS_STATUS VNETS_DC_AddUser(IN CHAR *pcUserName, IN CHAR *pcPassWord);
BS_STATUS VNETS_DC_SetPassword(IN CHAR *pcUserName, IN CHAR *pcPassword);
BOOL_T VNETS_DC_IsUserExist(IN CHAR *pcUserName);
BS_STATUS VNETS_DC_SetEmail(IN CHAR *pcUserName, IN CHAR *pcEmail);
BS_STATUS VNETS_DC_GetEmail(IN CHAR *pcUserName, OUT CHAR szEmail[VNET_DC_MAX_EMAIL_LEN + 1]);
BOOL_T VNETS_DC_CheckUserPassword
(
    IN CHAR *pcUserName,
    IN CHAR *pcPassWord
);
BS_STATUS VNETS_DC_SetDhcpConfig
(
    IN CHAR *pcDomain,
    IN CHAR *pcEnable,
    IN CHAR *pcStartIp,
    IN CHAR *pcEndIp,
    IN CHAR *pcMask,
    IN CHAR *pcGateway
);
BS_STATUS VNETS_DC_GetDhcpConfig
(
    IN CHAR *pcDomain,
    OUT CHAR *pcEnable,
    OUT CHAR *pcStartIp,
    OUT CHAR *pcEndIp,
    OUT CHAR *pcMask,
    OUT CHAR *pcGateway
);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__VNETS_DC_H_*/


