/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0
* Description:
******************************************************************************/
#include "bs.h"
#include "../h/xgw_cfg_lock.h"

static MUTEX_S g_xgw_cfg_lock;

int XGW_CFGLOCK_Init(void)
{
    MUTEX_InitNormal(&g_xgw_cfg_lock);
    return 0;
}

void XGW_CFGLOCK_Lock(void)
{
    MUTEX_P(&g_xgw_cfg_lock);
}

void XGW_CFGLOCK_Unlock(void)
{
    MUTEX_V(&g_xgw_cfg_lock);
}


