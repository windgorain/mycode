/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      lixingang  Version: 1.0  Date: 2007-2-3
* Description:  用于小临界区的保护，所有的小临界区都可以使用，
*                     但是由于使用者太多，释放一定要快，所以不能给大临界区使用
* History:     
******************************************************************************/
#include "bs.h"
#include "utl/exec_utl.h"

typedef struct
{
#ifdef IN_UNIXLIKE
    pthread_mutex_t stSplx;
#endif
#ifdef IN_WINDOWS
    CRITICAL_SECTION stSplx;
#endif

    THREAD_ID iSplxOwnerTID;     
    CHAR *pcSplxOwnerFile; 
    UINT uiSplxOwnerLine;  
}_SPLX_CTRL_S;


static _SPLX_CTRL_S g_stSplx;

static void splx_init()
{
    Mem_Zero(&g_stSplx, sizeof(_SPLX_CTRL_S));

    g_stSplx.pcSplxOwnerFile = "NULL";

#ifdef IN_UNIXLIKE
    pthread_mutex_init(&g_stSplx.stSplx, 0);
#endif

#ifdef IN_WINDOWS
    InitializeCriticalSection(&g_stSplx.stSplx);
#endif

#ifdef IN_DEBUG
    g_stSplx.iSplxOwnerTID = 0;
#endif
}

CONSTRUCTOR(init) {
    splx_init();
}

VOID _SPLX_P(IN CHAR *pcFilename, IN UINT uiLine)
{
#ifdef IN_DEBUG
    THREAD_ID iTid = 0;

    iTid = THREAD_GetSelfID();
    if ((iTid == g_stSplx.iSplxOwnerTID) && (iTid != 0))
    {
        BS_DBGASSERT(0);
    }
#endif


#ifdef IN_UNIXLIKE
    pthread_mutex_lock(&g_stSplx.stSplx);
#endif

#ifdef IN_WINDOWS
    EnterCriticalSection(&g_stSplx.stSplx);
#endif

#ifdef IN_DEBUG
    g_stSplx.iSplxOwnerTID = iTid;
#endif

    g_stSplx.pcSplxOwnerFile = pcFilename;
    g_stSplx.uiSplxOwnerLine = uiLine;

}

VOID SPLX_V()
{

#ifdef IN_DEBUG
    g_stSplx.iSplxOwnerTID = 0;
#endif

#ifdef IN_UNIXLIKE
    pthread_mutex_unlock(&g_stSplx.stSplx);
#endif

#ifdef IN_WINDOWS
    LeaveCriticalSection(&g_stSplx.stSplx);
#endif

    g_stSplx.pcSplxOwnerFile = "NULL";
    g_stSplx.uiSplxOwnerLine = 0;
}

BS_STATUS SPLX_Display()
{
#ifdef IN_DEBUG
    EXEC_OutInfo(" Owner Tid=%d, Owner file=%s(%d)\r\n",
        g_stSplx.iSplxOwnerTID, g_stSplx.pcSplxOwnerFile, g_stSplx.uiSplxOwnerLine);
#else
    EXEC_OutInfo(" Owner file=%s(%d)\r\n",
        g_stSplx.pcSplxOwnerFile, g_stSplx.uiSplxOwnerLine);
#endif
    return BS_OK;
}

