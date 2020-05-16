/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2008-9-5
* Description: 
* History:     
******************************************************************************/

#ifndef __VNETC_IPMAC_H_
#define __VNETC_IPMAC_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

BS_STATUS VNETC_Ipmac_Init();
VOID VNETC_Ipmac_Fin();
BS_STATUS VNETC_Ipmac_Add(IN UINT uiIp/* 利大會 */, IN MAC_ADDR_S *pstMac);
VOID VNETC_Ipmac_Del(IN UINT uiIp/* 利大會 */);
MAC_ADDR_S * VNETC_Ipmac_GetMacByIp(IN UINT uiIp/* 利大會 */, OUT MAC_ADDR_S *pstMac);


#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__VNETC_IPMAC_H_*/


