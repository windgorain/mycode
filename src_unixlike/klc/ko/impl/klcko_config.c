/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "klcko_impl.h"

static int klcko_config_process(int cmd, NETLINK_MSG_S *msg)
{
    int ret = 0;

    switch (cmd) {
        case KLC_NL_DECUSE_KO:
            module_put(THIS_MODULE);
            break;
        case KLC_NL_INCUSE_KO:
            __module_get(THIS_MODULE);
            break;
        case KLC_NL_KO_VER:
            ret = KLC_KO_VERSION;
            break;
        default:
            ret = -EINVAL;
            break;
    }

    return ret;
}

static int klcko_config_nlmsg(int cmd, void *msg)
{
    return klcko_config_process(cmd, msg);
}

int KlcKoConfig_Init(void)
{
    return KlcKoNl_Reg(KLC_NL_TYPE_CONFIG, klcko_config_nlmsg);
}

void KlcKoConfig_Fini(void)
{
    KlcKoNl_Reg(KLC_NL_TYPE_CONFIG, NULL);
}

