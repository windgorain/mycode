/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2013-1-15
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/txt_utl.h"
#include "utl/exec_utl.h"
#include "utl/eth_utl.h"
#include "utl/pktcap_utl.h"
#include "comp/comp_pcap.h"

#include "pcap_cmd.h"
#include "pcap_main.h"


/* pcap %INT<0-31> */
PLUG_API BS_STATUS PCAP_CMD_EnterAgent(IN UINT ulArgc, IN CHAR **argv, IN VOID *pEnv)
{
    return BS_OK;
}

/* show device */
PLUG_API BS_STATUS PCAP_CMD_ShowDev(IN UINT ulArgc, IN CHAR ** argv)
{
    PKTCAP_NIDS_INFO_HANDLE hInfo;

    hInfo = PKTCAP_GetNdisInfoList();
    if (NULL == hInfo) {
        return BS_ERR;
    }

    PKTCAP_PrintfNdisInfo(hInfo);
    PKTCAP_FreeNdisInfoList(hInfo);

    return BS_OK;
}

/* device %STRING */
PLUG_API BS_STATUS PCAP_CMD_SetDev(IN UINT ulArgc, IN CHAR **argv, IN VOID *pEnv)
{
    UINT uiIndex;
    CHAR *pcModeValue;

    if (ulArgc < 2) {
        return BS_ERR;
    }

    pcModeValue = CMD_EXP_GetCurrentModeValue(pEnv);

    TXT_Atoui(pcModeValue, &uiIndex);

    return PCAP_Main_SetNdis(uiIndex, argv[1]);
}

/* start */
PLUG_API BS_STATUS PCAP_CMD_Start(IN UINT ulArgc, IN CHAR **argv, IN VOID *pEnv)
{
    UINT uiIndex;
    CHAR *pcModeValue;

    pcModeValue = CMD_EXP_GetCurrentModeValue(pEnv);

    TXT_Atoui(pcModeValue, &uiIndex);

    return PCAP_Main_Start(uiIndex);
}

/* show this */
PLUG_API BS_STATUS PCAP_CMD_ShowThis(IN UINT ulArgc, IN CHAR **argv, IN VOID *pEnv)
{
    CHAR *pcNdisName;
    UINT uiIndex;
    CHAR *pcModeValue;

    pcModeValue = CMD_EXP_GetCurrentModeValue(pEnv);

    TXT_Atoui(pcModeValue, &uiIndex);

    pcNdisName = PCAP_Main_GetNdisName(uiIndex);

    if (pcNdisName != NULL) {
        EXEC_OutInfo(" device %s\r\n", pcNdisName);
    }

    if (PCAP_Main_IsStart(uiIndex)) {
        EXEC_OutString(" start\r\n");
    }

    return BS_OK;
}

PLUG_API BS_STATUS PCAP_CMD_Save(IN HANDLE hFile)
{
    ULONG i;
    CHAR *pcNdisName;

    for (i=0; i<PCAP_MAX_NUM; i++) {
        pcNdisName = PCAP_Main_GetNdisName(i);

        if (NULL ==  pcNdisName) {
            continue;
        }

        CMD_EXP_OutputMode(hFile, "pcap %d", i);

        CMD_EXP_OutputCmd(hFile, "device %s", pcNdisName);

        if (PCAP_Main_IsStart(i)) {
            CMD_EXP_OutputCmd(hFile, "start");
        }

        CMD_EXP_OutputModeQuit(hFile);
    }

    return BS_OK;
}

VOID PCAP_CMD_Init()
{
}

