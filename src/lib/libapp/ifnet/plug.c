/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2014-7-29
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/local_info.h"

extern BS_STATUS IFNET_Init();
extern int AIF_Loaded();
extern int AIF_CfgLoaded();
extern int AIF_CmdReged0();

PLUG_API int Plug_Stage(int stage)
{
    switch (stage) {
        case PLUG_STAGE_PLUG_LOAD:
            IFNET_Init();
            AIF_Loaded();
            break;
        case PLUG_STAGE_CMD_REGED0:
            AIF_CmdReged0();
            break;
        case PLUG_STAGE_CFG_LOADED:
            AIF_CfgLoaded();
            break;
        default:
            break;
    }

    return 0;
}

PLUG_MAIN



