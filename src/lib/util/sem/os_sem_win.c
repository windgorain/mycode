/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2012-11-20
* Description: 
* History:     
******************************************************************************/

#include "bs.h"

#include "os_sem.h"


#ifdef IN_WINDOWS

BS_STATUS _OSSEM_Create(CHAR *pcName, UINT ulInitNum, OUT OS_SEM *pOsSem)
{
    OS_SEM osSem;

    osSem = (OS_SEM)CreateSemaphore(NULL, ulInitNum, 0x7FFFFFFF, NULL);
    if (0 == osSem)
    {
        return (BS_ERR);
    }
    if (ERROR_ALREADY_EXISTS == GetLastError())
    {
        return (BS_ALREADY_EXIST);
    }

    *pOsSem = osSem;

    return BS_OK;
}

BS_STATUS _OSSEM_Delete(OS_SEM *pOsSem)
{
    CloseHandle((HANDLE)(*pOsSem));
    return BS_OK;
}

BS_STATUS _OSSEM_P(OS_SEM *pOsSem, BS_WAIT_E eWaitMode, UINT ulMilliseconds)
{
    INT lRet;

    if (BS_NO_WAIT == eWaitMode)
    {
        lRet = WaitForSingleObject((HANDLE)(*pOsSem), 0);
    }
    else
    {
        if (ulMilliseconds == BS_WAIT_FOREVER)
        {
            ulMilliseconds = INFINITE;
        }
        lRet = WaitForSingleObject((HANDLE)(*pOsSem), ulMilliseconds);
    }

    if (WAIT_OBJECT_0 == lRet)
    {
        return BS_OK;
    }
    else if (WAIT_TIMEOUT == lRet)
    {
        return BS_TIME_OUT;
    }
    else
    {
        return (BS_ERR);
    }

}

BS_STATUS _OSSEM_V(OS_SEM *pOsSem)
{
    ReleaseSemaphore((HANDLE)(*pOsSem), 1, NULL);
    return BS_OK;

}

#endif

