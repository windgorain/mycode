/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2015-12-3
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/rwlock_utl.h"

static RWLOCK_S g_stWsAppCfgLock;

VOID WSAPP_CfgLock_Init()
{
    RWLOCK_Init(&g_stWsAppCfgLock);
}

VOID WSAPP_CfgLock_RLock()
{
    RWLOCK_ReadLock(&g_stWsAppCfgLock);
}

VOID WSAPP_CfgLock_RUnLock()
{
    RWLOCK_ReadUnLock(&g_stWsAppCfgLock);
}

VOID WSAPP_CfgLock_WLock()
{
    RWLOCK_WriteLock(&g_stWsAppCfgLock);
}

VOID WSAPP_CfgLock_WUnLock()
{
    RWLOCK_WriteUnLock(&g_stWsAppCfgLock);
}

