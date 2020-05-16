/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2012-10-19
* Description: 
* History:     
******************************************************************************/

#ifndef __VNETS_CONF_H_
#define __VNETS_CONF_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */


VOID VNETS_SetHostMac(IN MAC_ADDR_S *pstMac);
MAC_ADDR_S * VNETS_GetHostMac();
BOOL_T VNETS_IsHostMac(IN MAC_ADDR_S *pstMac);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__VNETS_CONF_H_*/


