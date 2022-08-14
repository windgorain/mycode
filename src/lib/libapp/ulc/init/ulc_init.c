/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "../h/ulc_fd.h"
#include "../h/ulc_map.h"
#include "../h/ulc_hookpoint.h"

int ULC_Init()
{
    int ret = 0;

    ret |= ULC_FD_Init();
    ret |= ULC_MapArray_Init();
    ret |= ULC_MapHash_Init();
    ret |= ULC_HookPoint_Init();

    return ret;
}



