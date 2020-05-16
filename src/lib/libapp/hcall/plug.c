/******************************************************************************
* Copyright (C), 2000-2006,  LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2016-8-29
* Description: HTTP Call: 使用http协议, 发送Action
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/local_info.h"

extern BS_STATUS HCall_Init();

PLUG_API int Plug_Stage(int stage)
{
    switch (stage) {
        case PLUG_STAGE_PLUG_LOAD:
            HCall_Init();
            break;
        default:
            break;
    }

    return 0;
}

PLUG_MAIN

