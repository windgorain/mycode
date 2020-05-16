/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      lixingang  Version: 1.0  Date: 2007-2-8
* Description: 
* History:     
******************************************************************************/

#include "bs.h"

#include "utl/mutex_utl.h"

VOID MUTEX_Init(IN MUTEX_S *pstMutex)
{
#ifdef IN_UNIXLIKE
    pthread_mutex_init(&pstMutex->stMutex, 0);
    pstMutex->uiCurrentThreadId = 0;
#endif

#ifdef IN_WINDOWS
    InitializeCriticalSection(&pstMutex->stMutex);
#endif

    pstMutex->uiCount = 0;
}

VOID MUTEX_Final(IN MUTEX_S *pstMutex)
{
#ifdef IN_WINDOWS
    DeleteCriticalSection(&pstMutex->stMutex);
#endif
}

VOID _MUTEX_P(IN MUTEX_S *pstMutex, IN CHAR *pcFile, IN UINT uiLine)
{
#ifdef IN_UNIXLIKE
    pthread_t uiSelfID;

    uiSelfID = pthread_self();
    
    if (pstMutex->uiCurrentThreadId != uiSelfID)
    {
        while (0 != pthread_mutex_lock(&pstMutex->stMutex))
        {
        }
        
        pstMutex->uiCurrentThreadId = uiSelfID;
    }

#endif

#ifdef IN_WINDOWS
    EnterCriticalSection(&pstMutex->stMutex);
#endif

    pstMutex->uiCount ++;
    pstMutex->pcFile = pcFile;
    pstMutex->uiLine = uiLine;

    return;
}

BOOL_T _MUTEX_TryP(IN MUTEX_S *pstMutex, IN CHAR *pcFile, IN UINT uiLine)
{
    BOOL_T bSuccess = FALSE;
    
#ifdef IN_UNIXLIKE
{
    pthread_t uiSelfID;

    uiSelfID = pthread_self();
    if (pstMutex->uiCurrentThreadId != uiSelfID)
    {
        if (0 == pthread_mutex_trylock(&pstMutex->stMutex))
        {
            pstMutex->uiCurrentThreadId = uiSelfID;
            bSuccess = TRUE;
        }
    }
}
#endif

#ifdef IN_WINDOWS
    if (0 != TryEnterCriticalSection(&pstMutex->stMutex))
    {
        bSuccess = TRUE;
    }
#endif

    if (bSuccess == TRUE)
    {
        pstMutex->uiCount ++;
        pstMutex->pcFile = pcFile;
        pstMutex->uiLine = uiLine;
    }

    return bSuccess;
}

VOID MUTEX_V(IN MUTEX_S *pstMutex)
{
    pstMutex->pcFile = NULL;
    pstMutex->uiLine = 0;

    BS_DBGASSERT(pstMutex->uiCount > 0);
    
    pstMutex->uiCount --;

#ifdef IN_UNIXLIKE
    if (pstMutex->uiCount == 0)
    {
        pstMutex->uiCurrentThreadId = 0;
        pthread_mutex_unlock(&pstMutex->stMutex);
    }
#endif

#ifdef IN_WINDOWS
    LeaveCriticalSection(&pstMutex->stMutex);
#endif
}

UINT MUTEX_GetCount(IN MUTEX_S *pstMutex)
{
    return pstMutex->uiCount;
}

