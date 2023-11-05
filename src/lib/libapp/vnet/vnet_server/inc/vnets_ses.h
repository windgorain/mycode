/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2013-12-12
* Description: 
* History:     
******************************************************************************/

#ifndef __VNETS_SES_H_
#define __VNETS_SES_H_

#include "utl/mbuf_utl.h"
#include "utl/ses_utl.h"

#ifdef __cplusplus
    extern "C" {
#endif 

BS_STATUS VNETS_SES_Init();
BS_STATUS VNETS_SES_RegCloseNotifyEvent(IN PF_SES_CLOSE_NOTIFY_FUNC pfFunc, IN USER_HANDLE_S *pstUserHandle);
BS_STATUS VNETS_SES_PktInput(IN UINT ulIfIndex, IN MBUF_S *pstMbuf);
BS_STATUS VNETS_SES_SendPkt(IN UINT uiSesID, IN MBUF_S *pstMbuf);
VOID VNETS_SES_Close(IN UINT uiSesID);
BS_STATUS VNETS_SES_GetPhyInfo(IN UINT uiSesID, OUT VNETS_PHY_CONTEXT_S *pstPhyInfo);
UINT VNETS_SES_GetIfIndex(IN UINT uiSesID);
BS_STATUS VNETS_SES_SetProperty(IN UINT uiSesID, IN UINT uiIndex, IN HANDLE hValue);
HANDLE VNETS_SES_GetProperty(IN UINT uiSesID, IN UINT uiIndex);
BS_STATUS VNETS_SES_NoDebugAll();

#ifdef __cplusplus
    }
#endif 

#endif 


