/*================================================================
*   Created by LiXingang, Copyright LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"

static MUTEX_S g_ulcapp_cfg_lock;

CONSTRUCTOR(init) {
    MUTEX_Init(&g_ulcapp_cfg_lock);
}

void ULCAPP_CfgLock()
{
    MUTEX_P(&g_ulcapp_cfg_lock);
}

void ULCAPP_CfgUnlock()
{
    MUTEX_V(&g_ulcapp_cfg_lock);
}

