/******************************************************************************
* Copyright (C) Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2013-4-3
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/txt_utl.h"
#include "utl/exec_utl.h"

#include "../h/wan_vrf.h"

/* vrf %STRING */
PLUG_API BS_STATUS WAN_VFCmd_EnterView
(
    IN UINT ulArgc,
    IN CHAR **argv,
    IN VOID *pEnv
)
{
    UINT uiVrf;

    uiVrf = WanVrf_GetIdByName(argv[1]);
    if (uiVrf == 0)
    {
        /* 名字必须以字母或者数字开头, 否则是保留名字, 不允许配置, 至于徐进入 */
        if ((isalpha(argv[1][0])) || (isdigit(argv[1][0])))
        {
            uiVrf = WanVrf_CreateVrf(argv[1]);
        }
    }

    if (uiVrf == 0)
    {
        EXEC_OutString(" Can't create vrf.\r\n");
        return BS_ERR;
    }

    return BS_OK;
}


UINT WAN_VrfCmd_GetVrfByEnv(IN VOID *pEnv)
{
    CHAR *pcVFName;

    pcVFName = CMD_EXP_GetCurrentModeValue(pEnv);

    return WanVrf_GetIdByName(pcVFName);
}

BS_STATUS WAN_VFCmd_Init()
{
    return BS_OK;
}

VOID WAN_VrfCmd_Save(IN HANDLE hFile)
{
    UINT uiVrfID = 0;
    CHAR szName[WAN_VRF_MAX_NAME_LEN + 1];

    while ((uiVrfID = WanVrf_GetNext(uiVrfID)) != 0)
    {
        if (BS_OK != WanVrf_GetNameByID(uiVrfID, szName))
        {
            continue;
        }

        if (CMD_EXP_IsShowing(hFile)
            || (isalpha(szName[0]))
            || (isdigit(szName[0])))
        {
            CMD_EXP_OutputMode(hFile, "vrf %s", szName);
            CMD_EXP_OutputModeQuit(hFile);
        }
    }
}


