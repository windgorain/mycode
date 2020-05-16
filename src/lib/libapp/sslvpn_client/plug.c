/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-6-22
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/local_info.h"

extern BS_STATUS SVPNC_Init ();

PLUG_API int Plug_Stage(int stage)
{
    switch (stage) {
        case PLUG_STAGE_PLUG_LOAD:
            return SVPNC_Init();
        default:
            break;
    }

    return 0;
}

PLUG_MAIN


