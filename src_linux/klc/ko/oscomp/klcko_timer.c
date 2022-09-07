/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "klcko_lib.h"

static struct timer_list g_klcko_timer;

static void klcko_time_out(void)
{
    static KLC_EVENT_S ev = {.event=KLC_SYS_EVENT_TIMER, .sub_event=1};

    KlcKo_SystemEventPublish(&ev, 0, 0);

    g_klcko_timer.expires = jiffies + HZ;
    add_timer(&g_klcko_timer);

    return;
}

int KlcKoTimer_Init(void)
{
    KO_SETUP_TIMER(&g_klcko_timer, (void*)klcko_time_out, 0);
    g_klcko_timer.expires = jiffies + HZ;
    add_timer(&g_klcko_timer);

    return 0;
}

void KlcKoTimer_Fini(void)
{
    del_timer_sync(&g_klcko_timer);
}

