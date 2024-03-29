/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "klcko_impl.h"
#include "ko/utl/kmodinit_utl.h"
#include "ko/ko_utl.h"

static KUTL_MODINIT_S g_klcko_impl_init[] = {
    {.name="var", .init=KlcKoVar_Init, .fini=NULL},
    {.name="func", .init=KlcKoFunc_Init, .fini=NULL},
    {.name="impl_fin2", .init=NULL, .fini=KlcKo_Fini2},
    {.name="module", .init=KlcKoMod_Init, .fini=KlcKoMod_Fini2},
    {.name="config", .init=KlcKoConfig_Init, .fini=KlcKoConfig_Fini},
    {.name="oshelper", .init=KlcKoOsHelper_Init, .fini=NULL},
    {.name="module_prefini", .init=NULL, .fini=KlcKoMod_Fini1},
    {.name="impl", .init=KlcKo_Init, .fini=KlcKo_Fini1},
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

