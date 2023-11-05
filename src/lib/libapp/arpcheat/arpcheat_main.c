/******************************************************************************
* Copyright (C),    LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2013-4-25
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/arp_cheat_utl.h"
#include "comp/comp_pcap.h"

#include "arpcheat_main.h"

#define _ARPCHEAT_DFT_INTERVAL  10000  

static ARP_CHEAT_HANDLE g_hArpCheatHandle;
static MTIMER_S g_stArpCheatMTimer;
static BOOL_T g_hArpCheatStart;

static VOID arpcheat_main_TimeOut(IN HANDLE hTimerHandle, IN USER_HANDLE_S *pstUserHandle)
{
    ARPCheat_SendCheatPkt(g_hArpCheatHandle);
}

BS_STATUS ARPCheat_Main_Init()
{
    g_hArpCheatHandle = ARPCheat_Create();
    if (NULL == g_hArpCheatHandle)
    {
        return BS_NO_MEMORY;
    }

    return BS_OK;
}

VOID ARPCheat_Main_SetPcapIndex(IN UINT uiPcapIndex)
{
    ARPCheat_SetPcapIndex(g_hArpCheatHandle, uiPcapIndex);
}

VOID ARPCheat_Main_SetCheatIP(IN UINT uiIP)
{
    ARPCheat_SetCheatIP(g_hArpCheatHandle, uiIP);
}

VOID ARPCheat_Main_SetCheatMAC(IN MAC_ADDR_S *pstMAC)
{
    ARPCheat_SetCheatMAC(g_hArpCheatHandle, pstMAC);
}

BS_STATUS ARPCheat_Main_Start()
{
    int ret;

    ret = MTimer_Add(&g_stArpCheatMTimer, _ARPCHEAT_DFT_INTERVAL,
            TIMER_FLAG_CYCLE, arpcheat_main_TimeOut, NULL);
    if (ret < 0) {
        return BS_ERR;
    }

    ARPCheat_SendCheatPkt(g_hArpCheatHandle);

    g_hArpCheatStart = TRUE;

    return BS_OK;
}

VOID ARPCheat_Main_Save(IN HANDLE hFile)
{
    UINT uiPcapIndex;
    UINT uiIP;
    MAC_ADDR_S *pstMac;

    uiPcapIndex = ARPCheat_GetPcapIndex(g_hArpCheatHandle);
    if (uiPcapIndex != PCAP_INVALID_INDEX)
    {
        CMD_EXP_OutputCmd(hFile, "pcap %d", uiPcapIndex);
    }

    uiIP = ARPCheat_GetCheatIP(g_hArpCheatHandle);
    if (uiIP != 0)
    {
        CMD_EXP_OutputCmd(hFile, "cheat ip %pI4", &uiIP);
    }

    pstMac = ARPCheat_GetCheatMAC(g_hArpCheatHandle);
    if (NULL != pstMac)
    {
        CMD_EXP_OutputCmd(hFile, "cheat mac %pM", pstMac);
    }

    if (g_hArpCheatStart)
    {
        CMD_EXP_OutputCmd(hFile, "start");
    }
}

