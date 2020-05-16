/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2014-7-29
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/local_info.h"

extern BS_STATUS IFNET_Init ();

PLUG_API int Plug_Stage(int stage)
{
    switch (stage) {
        case PLUG_STAGE_PLUG_LOAD:
            IFNET_Init();
            break;
        default:
            break;
    }

    return 0;
}

PLUG_MAIN



