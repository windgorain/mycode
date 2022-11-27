/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/local_info.h"

extern void LogCenter_Init();

PLUG_API int Plug_Stage(int stage)
{
    switch (stage) {
        case PLUG_STAGE_PLUG_LOAD:
            LogCenter_Init();
            break;
        default:
            break;
    }

    return 0;
}

PLUG_MAIN


