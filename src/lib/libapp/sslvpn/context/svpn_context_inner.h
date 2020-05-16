/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2014-11-20
* Description: 
* History:     
******************************************************************************/

#ifndef __SVPN_CONTEXT_INNER_H_
#define __SVPN_CONTEXT_INNER_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

BS_STATUS SVPN_Context_AddContext(IN CHAR *pcContextName);
BS_STATUS SVPN_Context_DelContext(IN CHAR *pcContextName);
BS_STATUS SVPN_Context_BindService(IN CHAR *pcContextName, IN CHAR *pcService);
BS_STATUS SVPN_Context_SetDescription(IN CHAR *pcContextName, IN CHAR *pcDesc);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__SVPN_CONTEXT_INNER_H_*/


