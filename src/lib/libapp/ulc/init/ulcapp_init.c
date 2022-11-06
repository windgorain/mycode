/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/mybpf_prog.h"
#include "utl/mybpf_loader.h"
#include "../h/ulcapp_hookpoint.h"
#include "../h/ulcapp_ioctl.h"

int ULCAPP_Init()
{
    int ret = 0;

    ret |= MYBPF_Loader_Init();
    ret |= ULCAPP_IOCTL_Init();
    ret |= ULCAPP_HookPointXdp_Init();

    return ret;
}



