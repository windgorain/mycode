/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "klcko_lib.h"

typedef int (*PF_KlcKoEvent_SystemPublish)(void *ev, u64 p1, u64 p2);

static PF_KlcKoEvent_SystemPublish g_klcko_system_event_publish = NULL;

void KlcKo_SetSystemEventPublish(void *func)
{
    g_klcko_system_event_publish = func;
    if (! func) {
        synchronize_rcu();
    }
}
EXPORT_SYMBOL(KlcKo_SetSystemEventPublish);

int KlcKo_SystemEventPublish(void *ev, u64 p1, u64 p2)
{
    PF_KlcKoEvent_SystemPublish func;
    int ret = -1;

    rcu_read_lock();

    func = g_klcko_system_event_publish;
    mb();
    if (func) {
        ret = func(ev, p1, p2);
    }

    rcu_read_unlock();

    return ret;
}
EXPORT_SYMBOL(KlcKo_SystemEventPublish);

