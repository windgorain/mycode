/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"

extern int TUN_PKT_Init();
extern BS_STATUS TUN_IF_Init();

int TUN_Init()
{
    TUN_PKT_Init();
    return TUN_IF_Init();
}
