/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2013-4-25
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "comp/comp_pcap.h"

#include "arpcheat_main.h"
#include "arpcheat_cmd.h"

BS_STATUS ARPCheat_Init()
{
    COMP_PCAP_Init();
	ARPCheat_Main_Init();
    ARPCheat_CMD_Init();

	return BS_OK;
}

