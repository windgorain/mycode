/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0
* Description:
******************************************************************************/
#include "bs.h"
#include "../h/xgw_rcu.h"

RCU_QSBR_S g_xgw_rcu;

int XGW_RCU_SetWorkerNum(int num)
{
    return RcuQsbr_SetReaderNum(&g_xgw_rcu, num);
}

