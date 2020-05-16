/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2014-1-7
* Description: 
* History:     
******************************************************************************/

#ifndef __VNETC_CALLER_H_
#define __VNETC_CALLER_H_

#include "utl/caller_utl.h"

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

typedef enum
{
    _VNETC_CALLER_PRI_SES = 0,
    _VNETC_CALLER_PRI_TP,
    _VNETC_CALLER_PRI_VER,
    _VNETC_CALLER_PRI_AUTH,
    _VNETC_CALLER_PRI_ENTER_DOMAIN,
    _VNETC_CALLER_PRI_ADDR_CHANGE,
    _VNETC_CALLER_PRI_NEIGHBOR,

    _VNETC_CALLER_PRI_MAX
}_VNETC_CALLER_PRI_E;

BS_STATUS VNETC_Caller_Init();
BS_STATUS VNETC_Caller_Reg(IN CHAR *pcName, IN UINT uiPRI, IN PF_CALLER_FUNC pfFunc, IN USER_HANDLE_S *pstUserHandle);
BS_STATUS VNETC_Caller_Call();
BS_STATUS VNETC_Caller_Finish();
BS_STATUS VNETC_Caller_SetByName(IN CHAR *pcName);
VOID VNETC_Caller_Reset();

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__VNETC_CALLER_H_*/


