/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2013-4-25
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/eth_utl.h"
#include "utl/socket_utl.h"
#include "utl/txt_utl.h"
#include "utl/exec_utl.h"

#include "arpcheat_main.h"


PLUG_API BS_STATUS ARPCheat_CMD_SetPcapIndex(IN UINT ulArgc, IN CHAR **argv)
{
    UINT uiPcapIndex;

    TXT_Atoui(argv[1], &uiPcapIndex);

    ARPCheat_Main_SetPcapIndex(uiPcapIndex);

    return BS_OK;
}


PLUG_API BS_STATUS ARPCheat_CMD_SetCheatIP(IN UINT ulArgc, IN CHAR **argv)
{
    UINT uiIP;

    uiIP = Socket_NameToIpNet(argv[2]);
    if (0 == uiIP)
    {
        EXEC_OutString("Invalid IP.");
        return BS_ERR;
    }

    ARPCheat_Main_SetCheatIP(uiIP);

    return BS_OK;
}


PLUG_API BS_STATUS ARPCheat_CMD_SetCheatMAC(IN UINT ulArgc, IN CHAR **argv)
{
    MAC_ADDR_S stMacAddr;

    STRING_2_MAC_ADDR(argv[2], stMacAddr.aucMac);

    ARPCheat_Main_SetCheatMAC(&stMacAddr);

    return BS_OK;
}


PLUG_API BS_STATUS ARPCheat_CMD_Start(IN UINT ulArgc, IN CHAR **argv)
{
    return ARPCheat_Main_Start();
}


PLUG_API BS_STATUS ARPCheat_CMD_Save(IN HANDLE hFile)
{
    ARPCheat_Main_Save(hFile);

    return BS_OK;
}

VOID ARPCheat_CMD_Init()
{
	
}

