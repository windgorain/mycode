/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2008-11-2
* Description: 
* History:     
******************************************************************************/
#include "bs.h"
#include "utl/sysrun_utl.h"

typedef struct
{
	DLL_NODE_S stLinkList;
    PF_SYSRUN_EXIT_NOTIFY_FUNC pfFunc;
    BOOL_T bIsUserHandleExit;
    USER_HANDLE_S stUserHandle;
}_SYSRUN_EXIT_NOTIFY_S;

static DLL_HEAD_S g_stSysrunExitNotifyListHead = 
    DLL_HEAD_INIT_VALUE(&g_stSysrunExitNotifyListHead);

VOID _SysrunUtl_Exit(INT lExitNum)
{
    _SYSRUN_EXIT_NOTIFY_S *pstNotify, *pstNotifyTmp;
    USER_HANDLE_S *pstUserHandle;

    DLL_SAFE_SCAN(&g_stSysrunExitNotifyListHead, pstNotify, pstNotifyTmp)
    {
        if (pstNotify->bIsUserHandleExit == TRUE)
        {
            pstUserHandle = &(pstNotify->stUserHandle);
        }
        else
        {
            pstUserHandle = NULL;
        }

        pstNotify->pfFunc(lExitNum, pstUserHandle);
    }

    Sleep(10);  

    exit(lExitNum);
}

BS_STATUS _SysrunUtl_RegExitNotifyFunc(IN PF_SYSRUN_EXIT_NOTIFY_FUNC pfFunc,
        IN USER_HANDLE_S *pstUserHandle)
{
    _SYSRUN_EXIT_NOTIFY_S *pstNotify;

    pstNotify = malloc(sizeof(_SYSRUN_EXIT_NOTIFY_S));
    if (NULL == pstNotify)
    {
        RETURN(BS_NO_MEMORY);
    }
    memset(pstNotify, 0, sizeof(_SYSRUN_EXIT_NOTIFY_S));

    pstNotify->pfFunc = pfFunc;
    if (pstUserHandle != NULL)
    {
        pstNotify->stUserHandle = *pstUserHandle;
        pstNotify->bIsUserHandleExit = TRUE;
    }
    else
    {
        pstNotify->bIsUserHandleExit = FALSE;
    }

    DLL_ADD(&g_stSysrunExitNotifyListHead, pstNotify);

    return BS_OK;
}

