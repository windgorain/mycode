/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2014-11-20
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/rwlock_utl.h"

static RWLOCK_S g_stSvpnCfgLock;

VOID SVPN_CfgLock_Init()
{
    RWLOCK_Init(&g_stSvpnCfgLock);
}

VOID SVPN_CfgLock_RLock()
{
    RWLOCK_ReadLock(&g_stSvpnCfgLock);
}

VOID SVPN_CfgLock_RUnLock()
{
    RWLOCK_ReadUnLock(&g_stSvpnCfgLock);
}

VOID SVPN_CfgLock_WLock()
{
    RWLOCK_WriteLock(&g_stSvpnCfgLock);
}

VOID SVPN_CfgLock_WUnLock()
{
    RWLOCK_WriteUnLock(&g_stSvpnCfgLock);
}

