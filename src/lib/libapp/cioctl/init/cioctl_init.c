/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "../h/cioctl_cmd.h"

int CIOCTL_Init()
{
    int ret = 0;

    ret |= CIOCTL_CMD_Init();

    return ret;
}



