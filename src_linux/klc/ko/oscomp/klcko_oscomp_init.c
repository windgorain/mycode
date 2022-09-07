/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "klcko_lib.h"
#include "ko/utl/kutl_modinit.h"

static KUTL_MODINIT_S g_klcko_oscomp_init[] = {
    {.name="oscomp", .init=KlcKoComp_Init, .fini=NULL},
    {.name="timer", .init=KlcKoTimer_Init, .fini=KlcKoTimer_Fini},
    {.name="netlink", .init=KlcKoNl_Init, .fini=KlcKoNl_Fini},
    {.name="netfilter", .init = KlcKoNf_Init, .fini = KlcKoNf_Fini},
    {.name="inline", .init=KlcKoInline_Init, .fini=NULL},
    {.name=NULL}
};

static int __init klcko_oscomp_init(void)
{
    if (KUtlModinit_Init(g_klcko_oscomp_init) < 0) {
        KO_Print("Load ko error \n");
        return -1;
    }

    return 0;
}

static void __exit klcko_oscomp_exit(void)
{
    KUtlModinit_Fini(g_klcko_oscomp_init);
}

module_init(klcko_oscomp_init)
module_exit(klcko_oscomp_exit)
MODULE_LICENSE("GPL");

