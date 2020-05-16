/******************************************************************************
* Copyright (C), 2000-2006,  LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2016-9-23
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "comp/comp_pcap.h"

#include "pcap_dbg.h"


/* debug pcap packet */
PLUG_API VOID PCAP_Debug_Cmd(IN UINT ulArgc, IN CHAR **argv)
{
    PCAP_AGENT_DbgCmd(argv[1], argv[2]);
}

/* no debug pcap packet */
PLUG_API VOID PCAP_NoDebug_Cmd(IN UINT ulArgc, IN CHAR **argv)
{
    PCAP_AGENT_NoDbgCmd(argv[2], argv[3]);
}

VOID PCAP_DBG_Init()
{
    
}

