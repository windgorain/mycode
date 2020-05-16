/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2008-10-25
* Description: 
* History:     
******************************************************************************/
/* retcode所需要的宏 */
#define RETCODE_FILE_NUM VNET_RETCODE_FILE_NUM_CLIENTCMD
            
#include "bs.h"

#include "utl/eth_utl.h"
#include "utl/txt_utl.h"
#include "utl/http_aly.h"

#include "../../inc/vnet_conf.h"

#include "../inc/vnetc_conf.h"
#include "../inc/vnetc_alias.h"

static BOOL_T g_bVnetClientIsEnable = FALSE;

/* server _STRING_<1-255> */
/* 地址格式为:
 udp://Server:Port
 icp://Server
*/
PLUG_API BS_STATUS VNETC_CMD_SetServer(IN UINT ulArgc, IN CHAR **argv)
{
    if (ulArgc < 2)
    {
        RETURN(BS_BAD_PARA);
    }

    return VNETC_SetServer(argv[1]);
}

/* domain _STRING_<1-128> */
PLUG_API BS_STATUS VNETC_CMD_SetDomain(IN UINT ulArgc, IN CHAR **argv)
{
    if (ulArgc < 2)
    {
        RETURN(BS_BAD_PARA);
    }

    VNETC_SetServerDomain(argv[1]);

    return BS_OK;
}

/* user _STRING_<1-128> */
PLUG_API BS_STATUS VNETC_CMD_SetUser(IN UINT ulArgc, IN CHAR **argv)
{
    if (ulArgc < 2)
    {
        RETURN(BS_BAD_PARA);
    }

    VNETC_SetUserName(argv[1]);

    return BS_OK;
}

/* password { simple _STRING_<1-128> | cipher _STRING_<1-171> }  */
PLUG_API BS_STATUS VNETC_CMD_SetPassword(IN UINT ulArgc, IN CHAR **argv)
{
    if (ulArgc < 3)
    {
        RETURN(BS_BAD_PARA);
    }

    if (argv[1][0] == 's')
    {
        VNETC_SetUserPasswdSimple(argv[2]);
    }
    else
    {
        VNETC_SetUserPasswdCipher(argv[2]);
    }

    return BS_OK;
}

/* vnet-client direct */
PLUG_API BS_STATUS VNETC_CMD_SupportDirect(IN UINT ulArgc, IN CHAR **argv)
{
    VNETC_CONF_SetSupportDirect(TRUE);
    return BS_OK;
}

/* no vnet-client direct */
PLUG_API BS_STATUS VNETC_CMD_NoSupportDirect(IN UINT ulArgc, IN CHAR **argv)
{
    VNETC_CONF_SetSupportDirect(FALSE);
    return BS_OK;
}

/* start */
PLUG_API BS_STATUS VNETC_CMD_Start(IN UINT ulArgc, IN CHAR **argv)
{
    if (BS_OK != VNETC_Start())
    {
        EXEC_OutString("Can't connect server.\r\n");
        RETURN(BS_CAN_NOT_CONNECT);
    }

    g_bVnetClientIsEnable = TRUE;

    return BS_OK;
}

/* stop */
PLUG_API BS_STATUS VNETC_CMD_Stop(IN UINT ulArgc, IN CHAR **argv)
{
    return VNETC_Stop();
}

/* alias %STRING */
PLUG_API BS_STATUS VNETC_CMD_SetAlias(IN UINT ulArgc, IN CHAR **argv)
{
    UINT uiLen;
    
    if (ulArgc < 2)
    {
        return BS_ERR;
    }

    uiLen = strlen(argv[1]);
    if ((uiLen <= 0) || (uiLen > VNET_CONF_MAX_ALIAS_LEN))
    {
        return BS_OUT_OF_RANGE;
    }

    return VNETC_Alias_SetAlias(argv[1]);
}

/* description %STRING */
PLUG_API BS_STATUS VNETC_CMD_SetDescription(IN UINT ulArgc, IN CHAR **argv)
{
    UINT uiLen;
    
    if (ulArgc < 2)
    {
        return BS_ERR;
    }

    uiLen = strlen(argv[1]);
    if ((uiLen <= 0) || (uiLen > VNET_CONF_MAX_DES_LEN))
    {
        return BS_OUT_OF_RANGE;
    }

    return VNETC_SetDescription(argv[1]);
}

PLUG_API BS_STATUS VNETC_CMD_CmdSave(IN HANDLE hFileHandle)
{
    CHAR *pcString;
    CHAR *pcServerAddress;

    pcServerAddress = VNETC_GetServerAddress();
    if (pcServerAddress[0] != '\0')
    {
        CMD_EXP_OutputCmd(hFileHandle, "server %s", pcServerAddress);
    }

    if (FALSE == VNETC_CONF_IsSupportDirect())
    {
        CMD_EXP_OutputCmd(hFileHandle, "no connection direct");
    }

    pcString = VNETC_GetDomainName();
    if (pcString[0] != '\0')
    {
        CMD_EXP_OutputCmd(hFileHandle, "domain %s", pcString);
    }

    pcString = VNETC_GetUserName();
    if (pcString[0] != '\0')
    {
        CMD_EXP_OutputCmd(hFileHandle, "user %s", pcString);
    }

    pcString = VNETC_GetUserPasswd();
    if (pcString[0] != '\0')
    {
        CMD_EXP_OutputCmd(hFileHandle, "password cipher %s", pcString);
    }

    pcString = VNETC_Alias_GetAlias();
    if (pcString[0] != '\0')
    {
        CMD_EXP_OutputCmd(hFileHandle, "alias %s", pcString);
    }

    pcString = VNETC_GetDescription();
    if (pcString[0] != '\0')
    {
        CMD_EXP_OutputCmd(hFileHandle, "description %s ", pcString);
    }

    if (g_bVnetClientIsEnable == TRUE)
    {
        CMD_EXP_OutputCmd(hFileHandle, "start");
    }

    return BS_OK;
}


BS_STATUS VNETC_CMD_Init()
{
    return BS_OK;
}

