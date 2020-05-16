/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2012-4-22
* Description: 
* History:     
******************************************************************************/

#ifndef __VNET_MAC_ACL_H_
#define __VNET_MAC_ACL_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

BS_STATUS VNETS_MAC_ACL_Init();

VOID VNETS_MAC_ACL_PermitBroadcast(IN BOOL_T bIsPermit);

BOOL_T VNETS_MAC_ACL_IsPermitBroadcast();


#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__VNET_MAC_ACL_H_*/


