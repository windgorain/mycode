/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2009-10-18
* Description: 
* History:     
******************************************************************************/

#include "bs.h"

#include "utl/local_info.h"

#include "scall.h"

PLUG_API int Plug_Stage(int stage)
{
    switch (stage) {
        case PLUG_STAGE_PLUG_LOAD:
            return SCALL_Start();
        default:
            break;
    }

    return 0;
}

PLUG_MAIN


