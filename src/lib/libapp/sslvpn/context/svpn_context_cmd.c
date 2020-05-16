/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2014-11-2
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/ws_utl.h"
#include "utl/exec_utl.h"

#include "../h/svpn_def.h"
#include "../h/svpn_context.h"
#include "../h/svpn_res.h"
#include "../h/svpn_cfglock.h"

#include "svpn_context_inner.h"

/* context %STRING */
PLUG_API BS_STATUS SVPN_ContextCmd_EnterView(int argc, char **argv, VOID *pEnv)
{
    if (argc < 2)
    {
        return BS_ERR;
    }

    SVPN_CfgLock_WLock();
    if (BS_OK != SVPN_Context_AddContext(argv[1]))
    {
        EXEC_OutString("Create context failed.\r\n");
    }
    SVPN_CfgLock_WUnLock();

    return BS_OK;
}

/* no context %STRING */
PLUG_API BS_STATUS SVPN_ContextCmd_NoContext(int argc, char **argv, VOID *pEnv)
{
    if (argc < 2)
    {
        return BS_ERR;
    }

    SVPN_CfgLock_WLock();
    SVPN_Context_DelContext(argv[2]);
    SVPN_CfgLock_WUnLock();

    return BS_OK;
}

/* bind ws-service %STRING */
PLUG_API BS_STATUS SVPN_ContextCmd_BindService(int argc, char **argv, VOID *pEnv)
{
    SVPN_CfgLock_WLock();
    if (BS_OK != SVPN_Context_BindService(CMD_EXP_GetCurrentModeValue(pEnv), argv[2]))
    {
        EXEC_OutString("Bind service failed.\r\n");
    }
    SVPN_CfgLock_WUnLock();

    return BS_OK;
}

/* description %STRING */
PLUG_API BS_STATUS SVPN_ContextCmd_Description(int argc, char **argv, VOID *pEnv)
{
    SVPN_CfgLock_WLock();
    if (BS_OK != SVPN_Context_SetDescription(CMD_EXP_GetCurrentModeValue(pEnv), argv[1]))
    {
        EXEC_OutString("Operation failed.\r\n");
    }
    SVPN_CfgLock_WUnLock();

    return BS_OK;
}

BS_STATUS SVPN_ContextCmd_Save(IN HANDLE hFile)
{
    UINT64 uiContextID = 0;
    SVPN_CONTEXT_HANDLE hSvpnContext;
    CHAR *pcTmp;

    SVPN_CfgLock_RLock();
    
    while ((uiContextID = SVPN_Context_GetNextID(uiContextID)) != 0)
    {
        hSvpnContext = SVPN_Context_GetByID(uiContextID);

        CMD_EXP_OutputMode(hFile, "context %s", SVPN_Context_GetNameByID(uiContextID));

        if (CMD_EXP_IsShowing(hFile))
        {
            SVPN_AclCmd_Save(hSvpnContext, hFile);
            SVPN_WebResCmd_Save(hSvpnContext, hFile);
            SVPN_TcpResCmd_Save(hSvpnContext, hFile);
            SVPN_IpResCmd_Save(hSvpnContext, hFile);
            SVPN_IpPoolCmd_Save(hSvpnContext, hFile);
            SVPN_RoleCmd_Save(hSvpnContext, hFile);
            SVPN_LocalUser_Save(hSvpnContext, hFile);
        }

        pcTmp = SVPN_Context_GetWsService(hSvpnContext);
        if ((pcTmp != NULL) && (pcTmp[0] != '\0'))
        {
            CMD_EXP_OutputCmd(hFile, "bind ws-service %s", pcTmp);
        }

        pcTmp = SVPN_Context_GetDescription(hSvpnContext);
        if ((pcTmp != NULL) && (pcTmp[0] != '\0'))
        {
            CMD_EXP_OutputCmd(hFile, "description %s ", pcTmp);
        }

        CMD_EXP_OutputModeQuit(hFile);
    }

    SVPN_CfgLock_RUnLock();

    return BS_OK;
}


