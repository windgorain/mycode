/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2015-12-12
* Description: 
* History:     
******************************************************************************/
#include "bs.h"
#include "utl/exec_utl.h"

#include "webcenter_inner.h"

/* bind inner-ws-service %STRING */
PLUG_API BS_STATUS WebCenter_Cmd_BindInnerWsService(IN UINT ulArgc, IN CHAR **argv)
{
    CHAR *pcWsService = argv[2];

    if (BS_OK != WebCenter_BindWsService(pcWsService, TRUE))
    {
        EXEC_OutString("bind ws service failed.\r\n");
        return BS_ERR;
    }

    return BS_OK;
}

/* bind ws-service %STRING */
PLUG_API BS_STATUS WebCenter_Cmd_BindWsService(IN UINT ulArgc, IN CHAR **argv)
{
    CHAR *pcWsService = argv[2];

    if (BS_OK != WebCenter_BindWsService(pcWsService, FALSE))
    {
        EXEC_OutString("bind ws service failed.\r\n");
        return BS_ERR;
    }

    return BS_OK;
}

BS_STATUS WebCenter_Cmd_Save(IN HANDLE hFile)
{
    CHAR *pcTmp;

    pcTmp = WebCenter_GetBindedInnerWsService();
    if (pcTmp[0] != '\0') {
        CMD_EXP_OutputCmd(hFile, "bind inner-ws-service %s", pcTmp);
    }

    pcTmp = WebCenter_GetBindedWsService();
    if (pcTmp[0] != '\0') {
        CmdExp_OutputCmd(hFile, "bind ws-service %s", pcTmp);
    }

    return BS_OK;
}

