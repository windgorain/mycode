/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"

extern int TAP_PKT_Init();
extern BS_STATUS TAP_IF_Init();

int TAP_Init()
{
    TAP_PKT_Init();
    return TAP_IF_Init();
}
