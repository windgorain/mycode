/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2014-1-21
* Description: 
* History:     
******************************************************************************/

#ifndef __VNETC_SES_C2S_H_
#define __VNETC_SES_C2S_H_

#ifdef __cplusplus
    extern "C" {
#endif 

BS_STATUS VNETC_SesC2S_Init();
BS_STATUS VNETC_SesC2S_Connect();
VOID VNETC_SesC2S_Close();
UINT VNETC_SesC2S_GetSesId();
VOID VNETC_SesC2S_SetPhyContext(IN VNETC_PHY_CONTEXT_S *pstPhyContext);

#ifdef __cplusplus
    }
#endif 

#endif 


