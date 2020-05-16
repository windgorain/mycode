/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2007-7-20
* Description: 
* History:     
******************************************************************************/
#include "bs.h"
#include "utl/local_info.h"

extern BS_STATUS TELSVR_Init();

PLUG_API int Plug_Stage(int stage)
{
    switch (stage) {
        case PLUG_STAGE_PLUG_LOAD:
            return TELSVR_Init();
        default:
            break;
    }

    return 0;
}

PLUG_MAIN


