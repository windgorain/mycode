/*================================================================
*   Created：2018.10.04 LiXingang All rights reserved.
*   Description: 自动根据系统创建接口
*
================================================================*/
#include "bs.h"

#include "utl/local_info.h"

extern int POLLER_Init();

PLUG_API int Plug_Stage(int stage)
{
    switch (stage) {
        case PLUG_STAGE_PLUG_LOAD:
            POLLER_Init();
            break;
        default:
            break;
    }

    return 0;
}

PLUG_MAIN
