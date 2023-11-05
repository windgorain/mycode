/*================================================================
*   Created by LiXingang, Copyright LiXingang
*   Description: 
*
================================================================*/
#include "klcko_impl.h"

static int klcko_config_process(int cmd)
{
    int ret = 0;

    switch (cmd) {
        case KLC_NL_KO_VER:
            ret = KLC_KO_VERSION;
            break;
        case KLC_NL_KO_DECUSE:
            module_put(THIS_MODULE);
            break;
        case KLC_NL_KO_INCUSE:
            __module_get(THIS_MODULE);
            break;
        case KLC_NL_BASE_DECUSE:
            klc_base_module_use(0);
            break;
        case KLC_NL_BASE_INCUSE:
            klc_base_module_use(1);
            break;

        default:
            ret = -EINVAL;
            break;
    }

    return ret;
}

static int klcko_config_nlmsg(int cmd, void *data, int data_len, void *reply)
{
    return klcko_config_process(cmd);
}

int KlcKoConfig_Init(void)
{
    return KlcKoNl_Reg(KLC_NL_TYPE_CONFIG, klcko_config_nlmsg);
}

void KlcKoConfig_Fini(void)
{
    KlcKoNl_Reg(KLC_NL_TYPE_CONFIG, NULL);
}

