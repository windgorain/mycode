/******************************************************************************
* Copyright (C) Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2012-12-5
* Description: 
* History:     
******************************************************************************/

#ifndef __IN_IPADDR_H_
#define __IN_IPADDR_H_

#ifdef __cplusplus
    extern "C" {
#endif 

BS_STATUS IN_IPAddr_SelectSrcAddr
(
    IN UINT uiDstAddr,
    IN VRF_INDEX vrfIndex, 
    IN IF_INDEX ifIndexOut,
    IN UINT uiNextHop,
    OUT UINT *puiSrcAddr
);

BS_STATUS IN_IPAddr_GetAddrInVrf
(
    IN VRF_INDEX vrfIndex,
    IN UINT uiIPAddr, 
    IN UINT uiAddrType,
    OUT IPADDR_INFO_S *pstAddrInfo
);

BS_STATUS IN_IPAddr_MatchBestNetInVrf
(
    IN VRF_INDEX vrfIndex,
    IN UINT uiIPAddr, 
    IN UINT uiAddrType, 
    OUT IPADDR_INFO_S *pstAddrInfo
);

BS_STATUS IN_IPAddr_GetFwdAddrInVrf
(
    IN VRF_INDEX vrfIndex,
    OUT IPADDR_INFO_S *pstAddrInfo
);

BS_STATUS IN_IPAddr_GetPriorAddr
(
    IN IF_INDEX ifIndex,
    IN UINT uiAddrType, 
    OUT IPADDR_INFO_S *pstAddrInfo
);

#ifdef __cplusplus
    }
#endif 

#endif 


