/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      lixingang  Version: 1.0  Date: 2007-2-8
* Description: 
* History:     
******************************************************************************/

#include "bs.h"

#ifdef IN_WINDOWS

#include <process.h> 

BS_STATUS _OSTHREAD_DisplayCallStack(IN UINT ulOSTID)
{
    HANDLE hHandle;
    CONTEXT context; 
    BOOL_T bSuccess; 
    context.ContextFlags = (CONTEXT_FULL | CONTEXT_CONTROL);
    
    hHandle = OpenThread(THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT, FALSE, ulOSTID);

    BS_DBGASSERT(hHandle != NULL);

    bSuccess = GetThreadContext(hHandle, &context);
    if (bSuccess != TRUE)
    {
        return BS_ERR;
    }

    EXEC_OutInfo(" %x\r\n", context.Ebp);

    CloseHandle (hHandle);

    return BS_OK;
}

UINT _OSTHREAD_GetRunTime (IN ULONG ulOsTID)
{
    FILETIME stCreateTime;
    FILETIME stExitTime;
    FILETIME stKernelTime;
    FILETIME stUserTime;
    HANDLE   hHandle;

    hHandle = OpenThread(THREAD_QUERY_INFORMATION, FALSE, ulOsTID);

    if (hHandle == NULL)
    {
        BS_WARNNING(("Can't open thread!"));
        return 0;
    }

    if (0 == GetThreadTimes (hHandle, &stCreateTime, &stExitTime, &stKernelTime, &stUserTime))
    {
        CloseHandle (hHandle);
        return 0;
    }
    
    CloseHandle (hHandle);

    return (stKernelTime.dwLowDateTime + stUserTime.dwLowDateTime) / 10000;  
}

#endif

