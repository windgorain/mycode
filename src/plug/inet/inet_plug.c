/******************************************************************************
* Copyright (C) Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2013-1-7
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/local_info.h"
#include "inet/in_pub.h"

PLUG_API int Plug_Stage(int stage)
{
    switch (stage) {
        case PLUG_STAGE_PLUG_LOAD:
            return IN_Init();
        default:
            break;
    }

    return 0;
}

PLUG_MAIN

