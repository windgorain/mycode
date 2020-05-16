/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2008-9-21
* Description: 
* History:     
******************************************************************************/

#ifndef __VNETS_MAC_LAYER_H_
#define __VNETS_MAC_LAYER_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

VOID VNETS_MacLayer_NoDebugAll();
BS_STATUS VNETS_MacLayer_Input (IN MBUF_S *pstMbuf);
BS_STATUS VNETS_MacLayer_Output(IN UINT uiDomainID, IN MBUF_S *pstMbuf);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__VNETS_MAC_LAYER_H_*/


