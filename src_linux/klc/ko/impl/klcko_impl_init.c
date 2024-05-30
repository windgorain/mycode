/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "klcko_impl.h"
#include "ko/utl/kmodinit_utl.h"
#include "ko/ko_utl.h"
#include "klcko_kv.h"
#include "klcko_loader.h"
#include "klcko_bpf_helper.h"

static KUTL_MODINIT_S g_klcko_impl_init[] = {
    {.name="func", .init=KlcKoFunc_Init, .fini=NULL},

    {.name="kv", .init=KlcKoKv_Init, .fini=KlcKoKv_Fini},
    {.name="loader", .init=KlcKoLoad_Init, .fini=KlcKoLoad_Fini},
    {.name="helper", .init=KlcKoHelper_Init, .fini=KlcKoHelper_Fini},

    {.name="config", .init=KlcKoConfig_Init, .fini=KlcKoConfig_Fini},
    {.name="impl", .init=KlcKo_Init, .fini=KlcKo_Fini},
    {.name=NULL}
};

static int __init klcko_impl_init(void)
{
    if (KModinit_Init(g_klcko_impl_init) < 0) {
        KO_Print("Klc load error");
        return -1;
    }

    return 0;
}

static void __exit klcko_impl_exit(void)
{
    KModinit_Fini(g_klcko_impl_init);
    synchronize_rcu();
    rcu_barrier(); 
}

module_init(klcko_impl_init)
module_exit(klcko_impl_exit)
MODULE_LICENSE("GPL");

