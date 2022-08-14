/******************************************************************************
* Copyright (C), 2000-2006,  LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2016-10-18
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/local_info.h"

extern BS_STATUS MUC_Init(void *env);
extern BS_STATUS MUC_Init2();

PLUG_API int Plug_Stage(int stage, void *env)
{
    switch (stage) {
        case PLUG_STAGE_PLUG_LOAD:
            MUC_Init(env);
            break;
        case PLUG_STAGE_PLUG_LOADED:
            MUC_Init2();
            break;
        default:
            break;
    }

    return 0;
}

PLUG_MAIN


