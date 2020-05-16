/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2014-8-22
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/sys_utl.h"    
#include "utl/rwlock_utl.h"

#ifdef IN_WINDOWS

VOID RWLOCK_Init(IN RWLOCK_S *pstLock)
{
    Mem_Zero(pstLock, sizeof(RWLOCK_S));
    SpinLock_Init(&pstLock->stSpinLock);
    MUTEX_Init(&pstLock->stWriterMutex);
}

VOID RWLOCK_Fini(IN RWLOCK_S *pstLock)
{
    MUTEX_Final(&pstLock->stWriterMutex);
}

VOID RWLOCK_ReadLock(IN RWLOCK_S *pstLock)
{
    BOOL_T bNeedWait = TRUE;

    while (1)
    {
        SpinLock_Lock(&pstLock->stSpinLock);
        if (pstLock->uiWriterCount == 0)
        {
            pstLock->uiReaderCount ++;
            bNeedWait = FALSE;
        }
        SpinLock_UnLock(&pstLock->stSpinLock);

        if (bNeedWait == FALSE)
        {
            break;
        }

        Sleep(1);
    }
}

VOID RWLOCK_ReadUnLock(IN RWLOCK_S *pstLock)
{
    SpinLock_Lock(&pstLock->stSpinLock);
    pstLock->uiReaderCount --;
    SpinLock_UnLock(&pstLock->stSpinLock);
}

VOID RWLOCK_WriteLock(IN RWLOCK_S *pstLock)
{
    BOOL_T bNeedWait = TRUE;
    BOOL_T bInced = FALSE;

    while (1)
    {
        SpinLock_Lock(&pstLock->stSpinLock);

        if (bInced == FALSE)
        {
            pstLock->uiWriterCount ++;
            bInced = TRUE;
        }

        if (pstLock->uiReaderCount == 0)
        {
            bNeedWait = FALSE;
        }
        SpinLock_UnLock(&pstLock->stSpinLock);

        if (bNeedWait == FALSE)
        {
            break;
        }

        Sleep(1);
    }

    MUTEX_P(&pstLock->stWriterMutex);
}

VOID RWLOCK_WriteUnLock(IN RWLOCK_S *pstLock)
{
    MUTEX_V(&pstLock->stWriterMutex);

    SpinLock_Lock(&pstLock->stSpinLock);
    pstLock->uiWriterCount --;
    SpinLock_UnLock(&pstLock->stSpinLock);
}

#endif


