/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2013-1-15
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "../h/pcap_cmd.h"
#include "../h/pcap_main.h"
#include "../h/pcap_dbg.h"

BS_STATUS PCAP_Init()
{
    PCAP_Main_Init();
    PCAP_CMD_Init();
    PCAP_DBG_Init();
    PCAP_InitPcapList();

    return BS_OK;
}

BS_STATUS PCAP_CfgLoaded()
{
    return BS_OK;
}

