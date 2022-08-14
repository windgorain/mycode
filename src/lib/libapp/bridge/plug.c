/******************************************************************************
* Copyright (C), 2000-2006,  LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2016-10-18
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/local_info.h"
#include "h/bridge_init.h"

PLUG_API int Plug_Stage(int stage, void *env)
{
    switch (stage) {
        case PLUG_STAGE_PLUG_LOAD:
            Bridge_Init();
            break;
        default:
            break;
    }

    return 0;
}

PLUG_MAIN


