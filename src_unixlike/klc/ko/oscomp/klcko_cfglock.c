/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "klcko_lib.h"

static DEFINE_SPINLOCK(g_klcko_cfg_lock);

void KlcKoCfg_Lock(void)
{
    spin_lock(&g_klcko_cfg_lock);
}
EXPORT_SYMBOL(KlcKoCfg_Lock);

void KlcKoCfg_Unlock(void)
{
    spin_unlock(&g_klcko_cfg_lock);
}
EXPORT_SYMBOL(KlcKoCfg_Unlock);


