/******************************************************************************
* Copyright (C), 2000-2006,  LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2016-12-12
* Description: ldap app 认证方案
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/object_utl.h"


NO_HANDLE g_hLdapAppSchemeNo = NULL;

BS_STATUS LDAPAPP_Schema_Init()
{
    OBJECT_PARAM_S no_param = {0};

    g_hLdapAppSchemeNo = NO_CreateAggregate(&no_param);
    if (NULL == g_hLdapAppSchemeNo)
    {
        return BS_ERR;
    }

    return BS_OK;
}

BS_STATUS LDAPAPP_Schema_Add(IN CHAR *pcName)
{
    if (NULL == NO_NewObject(g_hLdapAppSchemeNo, pcName))
    {
        return BS_NO_MEMORY;
    }

    return BS_OK;
}

VOID LDAPAPP_Schema_Del(IN CHAR *pcName)
{
    NO_FreeObjectByName(g_hLdapAppSchemeNo, pcName);
}

BOOL_T LDAPAPP_Schema_IsExist(IN CHAR *pcName)
{
    if (NULL != NO_GetObjectByName(g_hLdapAppSchemeNo, pcName))
    {
        return TRUE;
    }

    return FALSE;
}

BS_STATUS LDAPAPP_Schema_SetDescription(IN CHAR *pcName, IN CHAR *pcDesc)
{
    VOID *pNode;

    pNode = NO_GetObjectByName(g_hLdapAppSchemeNo, pcName);
    if (pNode == NULL)
    {
        return BS_NOT_FOUND;
    }

    return NO_SetKeyValue(g_hLdapAppSchemeNo, pNode, "Description", pcDesc);
}

CHAR * LDAPAPP_Schema_GetDescription(IN CHAR *pcName)
{
    VOID *pNode;

    pNode = NO_GetObjectByName(g_hLdapAppSchemeNo, pcName);
    if (pNode == NULL)
    {
        return NULL;
    }

    return NO_GetKeyValue(pNode, "Description");
}

BS_STATUS LDAPAPP_Schema_SetServerAddress(IN CHAR *pcName, IN CHAR *pcAddr, IN CHAR *pcPort)
{
    VOID *pNode;

    pNode = NO_GetObjectByName(g_hLdapAppSchemeNo, pcName);
    if (pNode == NULL)
    {
        return BS_NOT_FOUND;
    }

    NO_SetKeyValue(g_hLdapAppSchemeNo, pNode, "ServerAddress", pcAddr);
    NO_SetKeyValue(g_hLdapAppSchemeNo, pNode, "ServerPort", pcPort);

    return BS_OK;
}

CHAR * LDAPAPP_Schema_GetServerAddress(IN CHAR *pcName)
{
    VOID *pNode;

    pNode = NO_GetObjectByName(g_hLdapAppSchemeNo, pcName);
    if (pNode == NULL)
    {
        return NULL;
    }

    return NO_GetKeyValue(pNode, "ServerAddress");
}

CHAR * LDAPAPP_Schema_GetServerPort(IN CHAR *pcName)
{
    VOID *pNode;

    pNode = NO_GetObjectByName(g_hLdapAppSchemeNo, pcName);
    if (pNode == NULL)
    {
        return NULL;
    }

    return NO_GetKeyValue(pNode, "ServerPort");
}

CHAR * LDAPAPP_Schema_GetNext(IN CHAR *pcCurrent/* NULL或""表示获取第一个 */)
{
    return NO_GetNextName(g_hLdapAppSchemeNo, pcCurrent);
}

