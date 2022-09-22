/*********************************************************
*   Copyright (C) LiXingang
*   Description: 
*
*************************************************/
#include "klcko_impl.h"

static int _klcko_sys_nlget(NETLINK_MSG_S *msg)
{
    int i;
    void *fn;
    KLC_OSHELPER_LIST_S __user *out = msg->reply_ptr;

    if (! out) {
        return KO_ERR_BAD_PARAM;
    }

    for (i=0; i<KLC_OSHELPER_MAX; i++) {
        if (i == KLC_HELPER_DO) {
            fn = KlcKo_Do;
        } else {
            fn = KlcKoOsHelper_GetHelper(i);
        }
        if (fn) {
            if (copy_to_user(&out->helpers[i], &fn, sizeof(fn))) {
                return -EFAULT;
            }
        }
    }

    return 0;
}

static int _klcko_sys_nlmsg(int cmd, void *msg)
{
    int ret;

    switch (cmd) {
        case KLC_NL_SYS_GET_OS_HELPER:
            ret = _klcko_sys_nlget(msg);
            break;
        default:
            ret = KO_ERR_BAD_PARAM;
            break;
    }

    return ret;
}

int KlcKoSys_Init(void)
{
    KlcKoNl_Reg(KLC_NL_TYPE_SYS, _klcko_sys_nlmsg);
    return 0;
}

void KlcKoSys_Fini(void)
{
    KlcKoNl_Reg(KLC_NL_TYPE_SYS, NULL);
}

