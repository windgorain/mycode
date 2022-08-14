/******************************************************************************
* Copyright (C), 2000-2006,  LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2016-10-18
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/local_info.h"

extern int ICLOG_Init ();

PLUG_API int Plug_Stage(int stage)
{
    switch (stage) {
        case PLUG_STAGE_PLUG_LOAD:
            ICLOG_Init();
            break;
        default:
            break;
    }

    return 0;
}

PLUG_MAIN


