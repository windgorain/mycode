/******************************************************************************
* Copyright (C), 2000-2006,  LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2016-12-10
* Description: ldapapp的全局锁
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/mutex_utl.h"

static MUTEX_S g_stLdapAppLock;

BS_STATUS LDAPAPP_Lock_Init()
{
    MUTEX_Init(&g_stLdapAppLock);

    return BS_OK;
}

VOID LDAPAPP_Lock()
{
    MUTEX_P(&g_stLdapAppLock);
}

VOID LDAPAPP_UnLock()
{
    MUTEX_V(&g_stLdapAppLock);
}

