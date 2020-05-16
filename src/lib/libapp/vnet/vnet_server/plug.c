/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2008-9-1
* Description: 
* History:     
******************************************************************************/

#include "bs.h"

#include "utl/local_info.h"

extern BS_STATUS VNETS_Init ();

PLUG_API int Plug_Stage(int stage)
{
    switch (stage) {
        case PLUG_STAGE_PLUG_LOAD:
            return VNETS_Init();
        default:
            break;
    }

    return 0;
}

PLUG_MAIN


