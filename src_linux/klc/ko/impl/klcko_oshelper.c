/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "klcko_impl.h"

static void * g_klcko_base_helpers[KLC_BASE_HELPER_MAX];
static void * g_klcko_sys_helpers[KLC_SYS_HELPER_MAX];

static u64 _klc_ko_oshelper_return0(void)
{
    return 0;
}

static u64 _klc_ko_oshelper_return1(void)
{
    return 1;
}

static u64 _klc_ko_oshelper_return2(void)
{
    return 2;
}

static u64 _klc_ko_oshelper_return_1(void)
{
    return -1LL;
}

static inline void * _klc_ko_oshelper_get_helper(void **tbl, int tbl_size, unsigned int helper_id)
{
    if (helper_id >= tbl_size) {
        return NULL;
    }
    return tbl[helper_id];
}

static inline int _klc_ko_oshelper_set_helper(void **tbl, int tbl_size, unsigned int helper_id, void *func, u64 err)
{
    if (helper_id >= tbl_size) {
        return -1;
    }

    if (! func) {
        if (err == 0) {
            func = _klc_ko_oshelper_return0;
        } else if (err == 1) {
            func = _klc_ko_oshelper_return1;
        } else if (err == 2) {
            func = _klc_ko_oshelper_return2;
        } else if (err == -1) {
            func = _klc_ko_oshelper_return_1;
        } else {
            func = _klc_ko_oshelper_return_1;
        }
    }

    tbl[helper_id] = func;

    return 0;
}

void * klc_get_helper(unsigned int helper_id)
{
    if (helper_id < KLC_BASE_HELPER_END) {
        return _klc_ko_oshelper_get_helper(g_klcko_base_helpers, ARRAY_SIZE(g_klcko_base_helpers), helper_id);
    }

    if (helper_id < KLC_SYS_HELPER_END) {
        return _klc_ko_oshelper_get_helper(g_klcko_sys_helpers, ARRAY_SIZE(g_klcko_sys_helpers), helper_id - KLC_SYS_HELPER_START);
    }

    return NULL;
}

int klc_set_helper(unsigned int helper_id, void *func, u64 err)
{
    if (helper_id < KLC_BASE_HELPER_END) {
        return _klc_ko_oshelper_set_helper(g_klcko_base_helpers, ARRAY_SIZE(g_klcko_base_helpers), helper_id, func, err);
    }

    if (helper_id < KLC_SYS_HELPER_END) {
        return _klc_ko_oshelper_set_helper(g_klcko_sys_helpers, ARRAY_SIZE(g_klcko_sys_helpers), helper_id - KLC_SYS_HELPER_START, func, err);
    }

    return -1;
}

int KlcKoOsHelper_Init(void)
{
    return 0;
}

