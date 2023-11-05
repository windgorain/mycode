/******************************************************************************
* Copyright (C), 2000-2006,  LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2016-12-10
* Description: 
* History:     
******************************************************************************/
#include "bs.h"
#include "utl/txt_utl.h"
#include "utl/exec_utl.h"

#include "ldapapp_inner.h"


PLUG_API BS_STATUS LDAPAPP_CmdEnterSchemaView(IN UINT uiArgc, IN CHAR **ppcArgv, IN VOID *pEnv)
{
    BS_STATUS eRet = BS_OK;

    LDAPAPP_Lock();
    if (! LDAPAPP_Schema_IsExist(ppcArgv[1]))
    {
        eRet = LDAPAPP_Schema_Add(ppcArgv[1]);
    }
    LDAPAPP_UnLock();

    if (BS_OK != eRet)
    {
        EXEC_OutInfo("%s.\r\n", ErrInfo_Get(eRet));
        return BS_ERR;
    }

    return eRet;
}


PLUG_API BS_STATUS LDAPAPP_CmdNoSchemaView(IN UINT uiArgc, IN CHAR **ppcArgv, IN VOID *pEnv)
{
    LDAPAPP_Lock();
    LDAPAPP_Schema_Del(ppcArgv[2]);
    LDAPAPP_UnLock();

    return BS_OK;
}


PLUG_API BS_STATUS LDAPAPP_CmdDescription(IN UINT uiArgc, IN CHAR **ppcArgv, IN VOID *pEnv)
{
    CHAR *pcName;
    BS_STATUS eRet;

    pcName = CMD_EXP_GetCurrentModeValue(pEnv);

    LDAPAPP_Lock();
    eRet = LDAPAPP_Schema_SetDescription(pcName, ppcArgv[1]);
    LDAPAPP_UnLock();

    if (BS_OK != eRet)
    {
        EXEC_OutInfo("%s.\r\n", ErrInfo_Get(eRet));
        return BS_ERR;
    }

    return BS_OK;
}


PLUG_API BS_STATUS LDAPAPP_CmdServerAddress(IN UINT uiArgc, IN CHAR **ppcArgv, IN VOID *pEnv)
{
    CHAR *pcName;
    BS_STATUS eRet;
    CHAR *pcPort = NULL;

    pcName = CMD_EXP_GetCurrentModeValue(pEnv);

    if (uiArgc >= 4)
    {
        pcPort = ppcArgv[3];
    }

    LDAPAPP_Lock();
    eRet = LDAPAPP_Schema_SetServerAddress(pcName, ppcArgv[1], pcPort);
    LDAPAPP_UnLock();

    if (BS_OK != eRet)
    {
        EXEC_OutInfo("%s.\r\n", ErrInfo_Get(eRet));
        return BS_ERR;
    }

    return BS_OK;
}

static VOID _ldapapp_Save(IN HANDLE hFile)
{
    CHAR *pcName = NULL;
    CHAR *pcTmp;
    CHAR *pcTmp2;

    while ((pcName = LDAPAPP_Schema_GetNext(pcName)) != NULL)
    {
        if (0 != CMD_EXP_OutputMode(hFile, "ldap-schema %s", pcName)) {
            continue;
        }

        pcTmp = LDAPAPP_Schema_GetDescription(pcName);
        if (!TXT_IS_EMPTY(pcTmp))
        {
            CMD_EXP_OutputCmd(hFile, "description %s", pcTmp);
        }

        pcTmp = LDAPAPP_Schema_GetServerAddress(pcName);
        pcTmp2 = LDAPAPP_Schema_GetServerPort(pcName);
        if (!TXT_IS_EMPTY(pcTmp))
        {
            if (!TXT_IS_EMPTY(pcTmp2))
            {
                CMD_EXP_OutputCmd(hFile, "server-address %s port %s", pcTmp, pcTmp2);
            }
            else
            {
                CMD_EXP_OutputCmd(hFile, "server-address %s", pcTmp);
            }
        }

        CMD_EXP_OutputModeQuit(hFile);
    }
}

PLUG_API BS_STATUS LDAPAPP_Save(IN HANDLE hFile)
{
    LDAPAPP_Lock();
    _ldapapp_Save(hFile);
    LDAPAPP_UnLock();

    return BS_OK;
}

BS_STATUS LDAPAPP_Cmd_Init()
{
    return BS_OK;
}

