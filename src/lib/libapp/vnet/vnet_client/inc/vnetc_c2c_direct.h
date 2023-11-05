/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2015-6-1
* Description: 
* History:     
******************************************************************************/

#ifndef __VNETC_C2C_DIRECT_H_
#define __VNETC_C2C_DIRECT_H_

#ifdef __cplusplus
    extern "C" {
#endif 

BS_STATUS VNETC_C2C_Direct_StartDetect
(
    IN UINT uiPeerNodeID,
    IN UINT uiPeerIP,    
    IN USHORT usPeerPort 
);

#ifdef __cplusplus
    }
#endif 

#endif 


