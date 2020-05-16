/******************************************************************************
* Copyright (C) Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2013-1-15
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "comp/comp_pcap.h"

#include "pcap_main.h"
#include "pcap_comp_inner.h"

static COMP_PCAP_S g_stPcapComp;

VOID PCAP_COMP_Init()
{
    g_stPcapComp.pfRegService = PCAP_Main_RegService;
    g_stPcapComp.pfUnregService = PCAP_Main_UnregService;
    g_stPcapComp.pfSendPkt = PCAP_Main_SendPkt;
    g_stPcapComp.pfIoctl = PCAP_Main_Ioctl;
    g_stPcapComp.comp.comp_name = COMP_PCAP_NAME;

    COMP_Reg(&g_stPcapComp.comp);
}

