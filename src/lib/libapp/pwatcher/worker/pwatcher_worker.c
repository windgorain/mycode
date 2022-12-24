/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/txt_utl.h"
#include "utl/msgque_utl.h"
#include "utl/event_utl.h"
#include "utl/array_bit.h"
#include "../h/pwatcher_conf.h"
#include "../h/pwatcher_recver.h"
#include "../h/pwatcher_worker.h"

static THREAD_LOCAL int g_pwatcher_worker_id = -1;
static int g_pwatcher_worker_max = 0;
static THREAD_NAMED_OB_S g_pwatcher_thread_ob;
static UINT g_pwatcher_worker_bitmp[(PWATCHER_WORKER_MAX + 31)/32];

static int _pwatcher_woker_get()
{
    INT64 id;

    id = ArrayBit_GetFree(g_pwatcher_worker_bitmp, PWATCHER_WORKER_MAX);
    if (id < 0) {
        return -1;
    }

    ArrayBit_Set(g_pwatcher_worker_bitmp, id);

    if (g_pwatcher_worker_max <= id) {
        g_pwatcher_worker_max = id + 1;
    }

    return id;
}

static void _pwatcher_woker_free(int id)
{
    SPLX_P();
    if (id >= 0) {
        ArrayBit_Clr(g_pwatcher_worker_bitmp, id);
    }
    SPLX_V();
}

static void _pwatcher_worker_thread_event(UINT event, THREAD_NAMED_INFO_S *info)
{
    switch (event) {
        case THREAD_NAMED_EVENT_QUIT:
            _pwatcher_woker_free(g_pwatcher_worker_id);
            break;
    }
}

int PWatcherWorker_SelfID()
{
    if (g_pwatcher_worker_id < 0) {
        SPLX_P();
        g_pwatcher_worker_id = _pwatcher_woker_get();
        SPLX_V();
    }

    return g_pwatcher_worker_id;
}

int PWatcherWorker_Max()
{
    return g_pwatcher_worker_max;
}

int PWatcherWorker_Init()
{
    g_pwatcher_thread_ob.ob_func = _pwatcher_worker_thread_event;
    THREAD_RegOb(&g_pwatcher_thread_ob);
    return 0;
}

