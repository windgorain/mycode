/*================================================================
*   Created by LiXingang
*   Description: 秒级定时器
*
================================================================*/
#include "bs.h"
#include "utl/cache_def.h"
#include "utl/dll_utl.h"
#include "utl/time_utl.h"
#include "utl/rdtsc_utl.h"

#include "../h/pwatcher_def.h"
#include "../h/pwatcher_worker.h"
#include "../h/pwatcher_timer.h"
#include "../h/pwatcher_event.h"
#include "../h/pwatcher_rcu.h"

typedef struct {
    UINT64 ts CACHE_ALIGNED;
}PWATCHER_TIME_S;

static PWATCHER_TIME_S g_pwatcher_time_old[PWATCHER_WORKER_MAX];
static UCHAR g_pwatcher_timer_min[PWATCHER_WORKER_MAX];
static UCHAR g_pwatcher_global_timer_min;

static void pwatchertimer_global_step(HANDLE timer, USER_HANDLE_S *pstUserHandle)
{
    
    PWatcherEvent_Notify(PWATCHER_EV_GLOBAL_TIMER, NULL, NULL);

    
    g_pwatcher_global_timer_min ++;
    if (g_pwatcher_global_timer_min >= 60) {
        PWatcherEvent_Notify(PWATCHER_EV_GLOBAL_MINUTE_TIMER, NULL, NULL);
        g_pwatcher_global_timer_min = 0;
    }
}

static void pwatchertimer_Wake(int worker_id)
{
    PWATCHER_TIMER_EV_S ev;
    ev.worker_id = worker_id;

    
    PWatcherEvent_Notify(PWATCHER_EV_WORKER_TIMER, NULL, &ev);

    
    g_pwatcher_timer_min[worker_id] ++;
    if (g_pwatcher_timer_min[worker_id] >= 60) {
        PWatcherEvent_Notify(PWATCHER_EV_WORKER_MINUTE_TIMER, NULL, &ev);
        g_pwatcher_timer_min[worker_id] = 0;
    }
}

void PWatcherTimer_Step()
{
    UINT64 ts;
    int worker_id = PWatcherWorker_SelfID();

    ts = RDTSC_Get();

    if (ts - g_pwatcher_time_old[worker_id].ts > RDTSC_HZ) {
        g_pwatcher_time_old[worker_id].ts = ts;
        pwatchertimer_Wake(worker_id);
    }
}

int PWatcherTimer_Init()
{
    static MTIMER_S timer;
    MTimer_Add(&timer, 1000, TIMER_FLAG_CYCLE, pwatchertimer_global_step, NULL);

    return 0;
}

