/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      lixingang  Version: 1.0  Date: 2007-2-8
* Description: 
* History:     
******************************************************************************/

#include "bs.h"

#include "thread_os.h"

#ifdef IN_UNIXLIKE

pthread_key_t g_tThreadLinuxKey;


BS_STATUS _OSTHREAD_DisplayCallStack(IN UINT ulOSTID)
{
    return BS_OK;
}

UINT _OSTHREAD_GetRunTime (IN ULONG ulOsTID)
{
    return 0;
}

#endif


