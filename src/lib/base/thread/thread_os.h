/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      lixingang  Version: 1.0  Date: 2007-2-9
* Description: 
* History:     
******************************************************************************/

#ifndef __THREAD_OS_H_
#define __THREAD_OS_H_

#ifdef __cplusplus
    extern "C" {
#endif 

BS_STATUS _OSTHREAD_DisplayCallStack(IN UINT ulOSTID);
UINT _OSTHREAD_GetRunTime (IN ULONG ulOsTID);

#ifdef __cplusplus
    }
#endif 

#endif 

