/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2013-4-2
* Description: 
* History:     
******************************************************************************/

#ifndef __WAN_DHCP_H_
#define __WAN_DHCP_H_

#ifdef __cplusplus
    extern "C" {
#endif 


UINT WAN_DHCP_GetServerIP(IN UINT uiVFID);


UINT WAN_DHCP_GetMask(IN UINT uiVFID);

BS_STATUS WAN_DHCP_Init();

#ifdef __cplusplus
    }
#endif 

#endif 


