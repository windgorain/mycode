/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-6-22
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "../h/svpnc_conf.h"
#include "../h/svpnc_func.h"

/* server  xxx */
PLUG_API BS_STATUS SVPNC_CMD_SetServer(IN UINT ulArgc, IN CHAR **argv)
{
    if (ulArgc < 2)
    {
        return (BS_BAD_PARA);
    }

    return SVPNC_SetServer(argv[1]);
}

/* port  xxx */
PLUG_API BS_STATUS SVPNC_CMD_SetPort(IN UINT ulArgc, IN CHAR **argv)
{
    if (ulArgc < 2)
    {
        return (BS_BAD_PARA);
    }

    return SVPNC_SetPort(argv[1]);
}

/* type  {tcp | ssl} */
PLUG_API BS_STATUS SVPNC_CMD_SetType(IN UINT ulArgc, IN CHAR **argv)
{
    if (ulArgc < 2)
    {
        return (BS_BAD_PARA);
    }

    return SVPNC_SetConnType(argv[1]);
}

/* username _STRING_<1-128> */
PLUG_API BS_STATUS SVPNC_CMD_SetUser(IN UINT ulArgc, IN CHAR **argv)
{
    if (ulArgc < 2)
    {
        return (BS_BAD_PARA);
    }

    SVPNC_SetUserName(argv[1]);

    return BS_OK;
}

/* password { simple _STRING_<1-128> | cipher _STRING_<1-171> }  */
PLUG_API BS_STATUS SVPC_CMD_SetPassword(IN UINT ulArgc, IN CHAR **argv)
{
    if (ulArgc < 3)
    {
        return (BS_BAD_PARA);
    }

    if (argv[1][0] == 's')
    {
        SVPNC_SetUserPasswdSimple(argv[2]);
    }
    else
    {
        
    }

    return BS_OK;
}

/* start */
PLUG_API BS_STATUS SVPNC_CMD_Start(IN UINT ulArgc, IN CHAR **argv)
{
    return SVPNC_Login();
}

PLUG_API BS_STATUS SVPNC_CMD_Save(IN HANDLE hFileHandle)
{
    CHAR *pcString;
    USHORT usPort;

    pcString = SVPNC_GetServer();
    if (pcString[0] != '\0')
    {
        CMD_EXP_OutputCmd(hFileHandle, "server %s", pcString);
    }
    
    usPort = SVPNC_GetServerPort();
    if (usPort != 443)
    {
        CMD_EXP_OutputCmd(hFileHandle, "port %d", usPort);
    }

    if (SVPNC_GetConnType() == SVPNC_CONN_TYPE_TCP)
    {
        CMD_EXP_OutputCmd(hFileHandle, "type tcp");
    }

    pcString = SVPNC_GetUserName();
    if (pcString[0] != '\0')
    {
        CMD_EXP_OutputCmd(hFileHandle, "username %s", pcString);
    }

    pcString = SVPNC_GetUserPasswd();
    if (pcString[0] != '\0')
    {
        CMD_EXP_OutputCmd(hFileHandle, "password simple %s", pcString);
    }


    return BS_OK;
}


BS_STATUS SVPNC_CMD_Init()
{
    return BS_OK;
}


