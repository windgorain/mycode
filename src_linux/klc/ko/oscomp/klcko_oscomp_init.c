/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "klcko_lib.h"
#include "ko/utl/kmodinit_utl.h"
#include "ko/ko_utl.h"

static KUTL_MODINIT_S g_klcko_oscomp_init[] = {
    {.name="netlink", .init=KlcKoNl_Init, .fini=KlcKoNl_Fini},
    {.name="inline", .init=KlcKoInline_Init, .fini=NULL},
    {.name=NULL}
};

static int __init klcko_oscomp_init(void)
{
    if (KModinit_Init(g_klcko_oscomp_init) < 0) {
        KO_Print("Load ko error \n");
        return -1;
    }

    return 0;
}

static void __exit klcko_oscomp_exit(void)
{
    KModinit_Fini(g_klcko_oscomp_init);
    synchronize_rcu();
    rcu_barrier(); 
}

module_init(klcko_oscomp_init)
module_exit(klcko_oscomp_exit)
MODULE_LICENSE("GPL");

