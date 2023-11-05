/*================================================================
*   Created by LiXingang: 2018.11.20
*   Description: timer utl 的单实例
*
================================================================*/
#include "bs.h"

#include "utl/timer_utl.h"
#include "utl/atomic_once.h"
#include "utl/timer_single.h"

static TIMER_UTL_CTRL_S g_timer_single;

static int timersingel_InitOnce(void *ud)
{
    return TimerUtl_Init(&g_timer_single, 100);
}

static inline void timersingel_init()
{
    static ATOM_ONCE_S once = ATOM_ONCE_INIT_VALUE;
    AtomOnce_WaitDo(&once, timersingel_InitOnce, NULL);
}

HANDLE TimerSingle_Create(UINT time, UINT flag, PF_TIME_OUT_FUNC func, USER_HANDLE_S *user_data)
{
    timersingel_init();
    return TimerUtl_Create(&g_timer_single, time, flag, func, user_data);
}

BS_STATUS TimerSingle_Delete(HANDLE timer)
{
    return TimerUtl_Delete(&g_timer_single, timer);
}

BS_STATUS TimerSingle_GetInfo(HANDLE timer, OUT TIMER_INFO_S *info)
{
    return TimerUtl_GetInfo(&g_timer_single, timer, info);
}

BS_STATUS TimerSingle_Pause(HANDLE timer)
{
    return TimerUtl_Pause(&g_timer_single, timer);
}

BS_STATUS TimerSingle_Resume(HANDLE timer)
{
    return TimerUtl_Resume(&g_timer_single, timer);
}

BS_STATUS TimerSingle_ReSetTime(HANDLE timer, UINT ulTime)
{
    return TimerUtl_ReSetTime(&g_timer_single, timer, ulTime);
}

