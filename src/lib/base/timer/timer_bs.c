/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2007-8-16
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/timer_single.h"

HANDLE Timer_Create(UINT ulTime, UINT flag, PF_TIME_OUT_FUNC pfFunc, USER_HANDLE_S *pstUserHandle)
{
    return TimerSingle_Create(ulTime, flag, pfFunc, pstUserHandle);
}

BS_STATUS Timer_Delete(HANDLE timer)
{
    return TimerSingle_Delete(timer);
}

BS_STATUS Timer_GetInfo(HANDLE timer, OUT TIMER_INFO_S *pstTimerInfo)
{
    return TimerSingle_GetInfo(timer, pstTimerInfo);
}

BS_STATUS Timer_Pause(HANDLE timer)
{
    return TimerSingle_Pause(timer);
}

BS_STATUS Timer_Resume(HANDLE timer)
{
    return TimerSingle_Resume(timer);
}

BS_STATUS Timer_ReSetTime(HANDLE timer, UINT ulTime)
{
    return TimerSingle_ReSetTime(timer, ulTime);
}
