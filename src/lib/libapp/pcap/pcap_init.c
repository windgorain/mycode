/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2013-1-15
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "pcap_cmd.h"
#include "pcap_comp_inner.h"
#include "pcap_main.h"
#include "pcap_dbg.h"

BS_STATUS PCAP_Init()
{
    PCAP_Main_Init();
    PCAP_CMD_Init();
    PCAP_COMP_Init();
    PCAP_DBG_Init();

    return BS_OK;
}

BS_STATUS PCAP_CfgLoaded()
{
    PCAP_InitPcapList();

    return BS_OK;
}

