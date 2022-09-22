/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "klcko_impl.h"

static KLCKO_SYM_S g_klcko_os_helpers[KLC_OSHELPER_MAX];

void * KlcKoOsHelper_GetHelper(int helper_id)
{
    if (helper_id >= KLC_OSHELPER_MAX) {
        return NULL;
    }
    return g_klcko_os_helpers[helper_id].sym;
}

u64 KlcKoOsHelper_GetHelperErr(int helper_id)
{
    if (helper_id >= KLC_OSHELPER_MAX) {
        return -1;
    }
    return g_klcko_os_helpers[helper_id].err;
}

int KlcKoOsHelper_SetHelper(int helper_id, void *func, u64 err)
{
    if (helper_id >= KLC_OSHELPER_MAX) {
        return -1;
    }
    g_klcko_os_helpers[helper_id].sym = func;
    g_klcko_os_helpers[helper_id].err = err;
    return 0;
}

int KlcKoOsHelper_Init(void)
{
    return 0;
}

