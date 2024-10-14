/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0
* Description:
******************************************************************************/
#ifndef _XGW_RCU_H_
#define _XGW_RCU_H_

#include "utl/rcu_qsbr.h"

#ifdef __cplusplus
extern "C" {
#endif

int XGW_RCU_SetWorkerNum(int num);

extern RCU_QSBR_S g_xgw_rcu;

static inline void XGW_RCU_Quiescent(int worker_id)
{
    RcuQsbr_Quiescent(&g_xgw_rcu, worker_id);
}

static inline void XGW_RCU_Sync(void)
{
    RcuQsbr_Sync(&g_xgw_rcu);
}

#ifdef __cplusplus
}
#endif
#endif 
