/******************************************************************************
* Copyright (C) Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2013-1-15
* Description: 
* History:     
******************************************************************************/

#include "bs.h"

#include "utl/local_info.h"

extern BS_STATUS PCAP_Init();
extern BS_STATUS PCAP_CfgLoaded();

PLUG_API int Plug_Stage(int stage)
{
    switch(stage) {
        case PLUG_STAGE_PLUG_LOAD:
            return PCAP_Init();
        case PLUG_STAGE_CFG_LOADED:
            return PCAP_CfgLoaded();
        default:
            break;
    }

    return 0;
}

PLUG_MAIN

