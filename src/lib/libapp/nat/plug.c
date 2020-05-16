/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2013-1-9
* Description: 
* History:     
******************************************************************************/

#include "bs.h"

#include "utl/local_info.h"

extern BS_STATUS NAT_Init();

PLUG_API int Plug_Stage(int stage)
{
    switch (stage) {
        case PLUG_STAGE_PLUG_LOAD:
            NAT_Init();
            break;
        default:
            break;
    }

    return 0;
}

PLUG_MAIN

