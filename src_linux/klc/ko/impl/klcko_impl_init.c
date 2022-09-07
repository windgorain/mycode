/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "klcko_impl.h"
#include "ko/utl/kutl_modinit.h"

static KUTL_MODINIT_S g_klcko_impl_init[] = {
    {.name="config", .init=KlcKoConfig_Init, .fini=KlcKoConfig_Fini},
    {.name="module", .init=KlcKoModule_Init, .fini=KlcKoModule_Fini},
    {.name="oshelper", .init=KlcKoOsHelper_Init, .fini=NULL},
    {.name="id_func", .init=KlcKoIDFunc_Init, .fini=KlcKoIDFunc_Fini},
    {.name="name_func", .init=KlcKoNameFunc_Init, .fini=KlcKoNameFunc_Fini},
    {.name="name_data", .init=KlcKoNameData_Init, .fini=KlcKoNameData_Fini},
    {.name="name_map", .init=KlcKoNameMap_Init, .fini=KlcKoNameMap_Fini},
    {.name="event", .init=KlcKoEvent_Init, .fini=KlcKoEvent_Fini},
    {.name="impl", .init=KlcKo_Init, .fini=KlcKo_Fini},
    {.name=NULL}
};

static int __init klcko_impl_init(void)
{
    if (KUtlModinit_Init(g_klcko_impl_init) < 0) {
        KO_PrintString("Load klc error");
        return -1;
    }

    return 0;
}

static void __exit klcko_impl_exit(void)
{
    KUtlModinit_Fini(g_klcko_impl_init);
}

module_init(klcko_impl_init)
module_exit(klcko_impl_exit)
MODULE_LICENSE("GPL");

