/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "../h/poller_cmd.h"
#include "../h/poller_comp.h"
#include "../h/poller_core.h"

int POLLER_Init()
{
    POLLER_INS_Init();
    PollerComp_Init();
    POLLER_CMD_Init();

    return 0;
}
