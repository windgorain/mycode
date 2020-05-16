/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2014-7-22
* Description: 
* History:     
******************************************************************************/

#ifndef __WS_CONTEXT_H_
#define __WS_CONTEXT_H_

#include "utl/file_utl.h"

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */


BS_STATUS _WS_Context_InitContainer(INOUT _WS_VHOST_S *pstVHost);
WS_CONTEXT_HANDLE _WS_Context_Match(IN WS_VHOST_HANDLE hVHost, IN CHAR *pcContext);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__WS_CONTEXT_H_*/


