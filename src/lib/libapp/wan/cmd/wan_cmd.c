/******************************************************************************
* Copyright (C) Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2013-4-15
* Description: 
* History:     
******************************************************************************/
#include "bs.h"
#include "../h/wan_pcap.h"
#include "../h/wan_vrf_cmd.h"
#include "../h/wan_fib.h"

VOID WAN_CMD_Init()
{
    
}

PLUG_API BS_STATUS WAN_CMD_Save(IN HANDLE hFile)
{
    WAN_VrfCmd_Save(hFile);
    WAN_FIB_Save(hFile);

    return BS_OK;
}

