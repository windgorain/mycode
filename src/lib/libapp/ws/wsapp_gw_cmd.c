/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2015-11-30
* Description: 
* History:     
******************************************************************************/
#include "bs.h"
    
#include "utl/txt_utl.h"
#include "utl/exec_utl.h"
#include "utl/ws_utl.h"
#include "utl/nap_utl.h"
#include "utl/file_utl.h"
#include "utl/local_info.h"

#include "wsapp_def.h"
#include "wsapp_worker.h"
#include "wsapp_gw.h"
#include "wsapp_cfglock.h"

/* gateway %STRING */
PLUG_API BS_STATUS WSAPP_GwCmd_EnterView(IN UINT ulArgc, IN CHAR **argv, IN VOID *pEnv)
{
    CHAR *pcGwName;

    if (ulArgc < 2)
    {
        return BS_ERR;
    }

    pcGwName = argv[1];

    WSAPP_CfgLock_WLock();

    if (! WSAPP_GW_IsExist(pcGwName))
    {
        if (NULL == WSAPP_GW_Add(pcGwName))
        {
            EXEC_OutString(" Operation failed.\r\n");
        }
    }

    WSAPP_CfgLock_WUnLock();

    return BS_OK;
}

/* [no] webcenter {hide|readonly} */
PLUG_API BS_STATUS WSAPP_GwCmd_WebCenterOpt(IN UINT ulArgc, IN CHAR **argv, IN VOID *pEnv)
{
    CHAR *pcGwName;
    BS_STATUS eRet;

    pcGwName = CMD_EXP_GetCurrentModeValue(pEnv);

    WSAPP_CfgLock_WLock();
    if (argv[0][0] == 'n')
    {
        eRet = WSAPP_GW_ClrWebCenterOpt(pcGwName, argv[2]);
    }
    else
    {
        eRet = WSAPP_GW_SetWebCenterOpt(pcGwName, argv[1]);
    }
    WSAPP_CfgLock_WUnLock();

    if (eRet != BS_OK)
    {
        EXEC_OutString("Operation failed.\r\n");
    }

    return BS_OK;
}

/* description %STRING */
PLUG_API BS_STATUS WSAPP_GwCmd_Description(IN UINT ulArgc, IN CHAR **argv, IN VOID *pEnv)
{
    CHAR *pcGwName;

    pcGwName = CMD_EXP_GetCurrentModeValue(pEnv);

    WSAPP_CfgLock_WLock();
    if (BS_OK != WSAPP_GW_SetDescription(pcGwName, argv[1]))
    {
        EXEC_OutString("Operation failed.\r\n");
    }
    WSAPP_CfgLock_WUnLock();

    return BS_OK;
}

/* type tcp */
PLUG_API BS_STATUS WSAPP_GwCmd_TypeTcp(IN UINT ulArgc, IN CHAR **argv, IN VOID *pEnv)
{
    CHAR *pcGwName;

    pcGwName = CMD_EXP_GetCurrentModeValue(pEnv);

    WSAPP_CfgLock_WLock();
    if (BS_OK != WSAPP_GW_SetType(pcGwName, "TCP"))
    {
        EXEC_OutString("Service is not exist.\r\n");
    }
    WSAPP_CfgLock_WUnLock();

    return BS_OK;
}

/* type ssl [ ca %STRING local %STRING key %STRING ] */
PLUG_API BS_STATUS WSAPP_GwCmd_TypeSsl(IN UINT ulArgc, IN CHAR **argv, IN VOID *pEnv)
{
    CHAR *pcGwName;
    CHAR *pcCA = "";
    CHAR *pcLocal = "";
    CHAR *pcKeyFile = "";

    pcGwName = CMD_EXP_GetCurrentModeValue(pEnv);

    if (ulArgc > 2)
    {
        pcCA = argv[3];
        pcLocal = argv[5];
        pcKeyFile = argv[7];
    }

    WSAPP_CfgLock_WLock();
    if ((BS_OK != WSAPP_GW_SetType(pcGwName, "SSL"))
        || (BS_OK != WSAPP_GW_SetSslParam(pcGwName, pcCA, pcLocal, pcKeyFile)))
    {
        EXEC_OutString("Operation failed.\r\n");
    }
    WSAPP_CfgLock_WUnLock();

    return BS_OK;
}

/* ip address %IP */
PLUG_API BS_STATUS WSAPP_GwCmd_ConfigIPAddress(IN UINT ulArgc, IN CHAR **argv, IN VOID *pEnv)
{
    CHAR *pcGwName;
    CHAR *pcIP;

    pcGwName = CMD_EXP_GetCurrentModeValue(pEnv);
    pcIP = argv[2];

    WSAPP_CfgLock_WLock();
    if (BS_OK != WSAPP_GW_SetIP(pcGwName, pcIP))
    {
        EXEC_OutString("Operation failed.\r\n");
    }
    WSAPP_CfgLock_WUnLock();

    return BS_OK;
}

/* port %INT<1-65535> */
PLUG_API BS_STATUS WSAPP_GwCmd_ConfigPort(IN UINT ulArgc, IN CHAR **argv, IN VOID *pEnv)
{
    CHAR *pcGwName;

    pcGwName = CMD_EXP_GetCurrentModeValue(pEnv);

    WSAPP_CfgLock_WLock();
    if (BS_OK != WSAPP_GW_SetPort(pcGwName, argv[1]))
    {
        EXEC_OutString("Service is not exist.\r\n");
    }
    WSAPP_CfgLock_WUnLock();

    return BS_OK;
}

/* filter ip-acl %STRING */
PLUG_API BS_STATUS WSAPP_GwCmd_RefIpAcl(IN UINT ulArgc, IN CHAR **argv, IN VOID *pEnv)
{
    CHAR *pcGwName;

    pcGwName = CMD_EXP_GetCurrentModeValue(pEnv);

    WSAPP_CfgLock_WLock();
    if (BS_OK != WSAPP_GW_RefIpAcl(pcGwName, argv[2]))
    {
        EXEC_OutString("Operation failed.\r\n");
    }
    WSAPP_CfgLock_WUnLock();

    return BS_OK;
}

/* no filter ip-acl */
PLUG_API BS_STATUS WSAPP_GwCmd_NoRefIpAcl(IN UINT ulArgc, IN CHAR **argv, IN VOID *pEnv)
{
    CHAR *pcGwName;
    
    pcGwName = CMD_EXP_GetCurrentModeValue(pEnv);

    WSAPP_CfgLock_WLock();
    WSAPP_GW_NoRefIpAcl(pcGwName);
    WSAPP_CfgLock_WUnLock();

    return BS_OK;
}

/* service enable */
PLUG_API BS_STATUS WSAPP_GwCmd_Enable(IN UINT ulArgc, IN CHAR **argv, IN VOID *pEnv)
{
    CHAR *pcGwName;
    
    pcGwName = CMD_EXP_GetCurrentModeValue(pEnv);

    WSAPP_CfgLock_WLock();
    if (BS_OK != WSAPP_GW_Start(pcGwName))
    {
        EXEC_OutString("Operation failed.\r\n");
    }
    WSAPP_CfgLock_WUnLock();

    return BS_OK;
}

/* no service enable */
PLUG_API BS_STATUS WSAPP_GwCmd_NoEnable(IN UINT ulArgc, IN CHAR **argv, IN VOID *pEnv)
{
    CHAR *pcGwName;
    
    pcGwName = CMD_EXP_GetCurrentModeValue(pEnv);

    WSAPP_CfgLock_WLock();
    WSAPP_GW_Stop(pcGwName);
    WSAPP_CfgLock_WUnLock();

    return BS_OK;
}

static VOID wsapp_gw_cmd_SaveEach(IN WSAPP_GW_S *pstGw, IN HANDLE hFile)
{
    CHAR *pcTmp;

    pcTmp = WSAPP_GW_GetName(pstGw);

    if (0 != CMD_EXP_OutputMode(hFile, "gateway %s", pcTmp)) {
        return;
    }

    if (WSAPP_GW_IsWebCenterOptHide(pstGw))
    {
        CMD_EXP_OutputCmd(hFile, "webcenter hide");
    }

    if (WSAPP_GW_IsWebCenterOptReadonly(pstGw))
    {
        CMD_EXP_OutputCmd(hFile, "webcenter readonly");
    }

    if (pstGw->bIsSSL)
    {
        pcTmp = WSAPP_GW_GetParamCaCert(pstGw);
        if (! TXT_IS_EMPTY(pcTmp))
        {
            CMD_EXP_OutputCmd(hFile, "type ssl ca %s local %s key %s",
                pcTmp, WSAPP_GW_GetParamLocalCert(pstGw), WSAPP_GW_GetParamKeyFile(pstGw));
        }
        else
        {
            CMD_EXP_OutputCmd(hFile, "type ssl");
        }
    }
    else
    {
        CMD_EXP_OutputCmd(hFile, "type tcp");
    }

    pcTmp = WSAPP_GW_GetDesc(pstGw);
    if (!TXT_IS_EMPTY(pcTmp))
    {
        CMD_EXP_OutputCmd(hFile, "description %s ", pcTmp);
    }

    pcTmp = WSAPP_GW_GetIP(pstGw);
    if (!TXT_IS_EMPTY(pcTmp))
    {
        CMD_EXP_OutputCmd(hFile, "ip address %s", pcTmp);
    }

    pcTmp = WSAPP_GW_GetPort(pstGw);
    if (!TXT_IS_EMPTY(pcTmp))
    {
        CMD_EXP_OutputCmd(hFile, "port %s", pcTmp);
    }

    pcTmp = WSAPP_GW_GetIpAclList(pstGw);
    if (!TXT_IS_EMPTY(pcTmp))
    {
        CMD_EXP_OutputCmd(hFile, "filter ip-acl %s", pcTmp);
    }

    if (pstGw->bStart == TRUE)
    {
        CMD_EXP_OutputCmd(hFile, "service enable");
    }
    
    CMD_EXP_OutputModeQuit(hFile);

    return;
}

BS_STATUS WSAPP_GwCmd_Save(IN HANDLE hFile)
{
    WSAPP_GW_Walk(wsapp_gw_cmd_SaveEach, hFile);

    return BS_OK;
}

/* debug ws {packet|event|process|err|all} */
PLUG_API BS_STATUS WSAPP_GwCmd_DebugWs(IN UINT ulArgc, IN CHAR **argv, IN VOID *pEnv)
{
    CHAR *pcGwName;
    
    pcGwName = CMD_EXP_GetCurrentModeValue(pEnv);

    WSAPP_CfgLock_WLock();
    WSAPP_GW_SetWsDebugFlag(pcGwName, argv[2]);
    WSAPP_CfgLock_WUnLock();

    return BS_OK;
}

/* no debug ws {packet|event|process|err|all} */
PLUG_API BS_STATUS WSAPP_GwCmd_NoDebugWs(IN UINT ulArgc, IN CHAR **argv, IN VOID *pEnv)
{
    CHAR *pcGwName;
    
    pcGwName = CMD_EXP_GetCurrentModeValue(pEnv);

    WSAPP_CfgLock_WLock();
    WSAPP_GW_ClrWsDebugFlag(pcGwName, argv[2]);
    WSAPP_CfgLock_WUnLock();

    return BS_OK;
}

