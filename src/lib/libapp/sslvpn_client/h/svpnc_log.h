/******************************************************************************
* Copyright (C), 2000-2006,  Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2017-4-27
* Description: 
* History:     
******************************************************************************/

#ifndef __SVPNC_LOG_H_
#define __SVPNC_LOG_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

BS_STATUS SVPNC_Log_Init();
VOID SVPNC_Log(IN CHAR *pszLogFmt, ...);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__SVPNC_LOG_H_*/


