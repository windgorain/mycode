/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2008-11-2
* Description: 
* History:     
******************************************************************************/
#include "bs.h"
#include "utl/sysrun_utl.h"

VOID _SysrunBs_Exit(INT lExitNum)
{
    _SysrunUtl_Exit(lExitNum);
}

BS_STATUS _SysrunBs_RegExitNotifyFunc(IN PF_SYSRUN_EXIT_NOTIFY_FUNC pfFunc, IN USER_HANDLE_S *ud)
{
    return _SysrunUtl_RegExitNotifyFunc(pfFunc, ud);
}

