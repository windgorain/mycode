/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/mybpf_loader.h"
#include "utl/mybpf_prog.h"
#include "../h/ulcapp_hookpoint.h"
#include "../h/ulcapp_runtime.h"

int ULCAPP_Init()
{
    int ret = 0;

    ret |= ULCAPP_RuntimeInit();

    return ret;
}



