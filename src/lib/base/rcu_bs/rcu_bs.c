/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-7-11
* Description: BS提供的全局RCU, 用于全局进行RCU操作.
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/rcu_utl.h"

static RCU_S g_bs_rcu;

BS_STATUS RcuBs_Init()
{
    RCU_Init(&g_bs_rcu);

    return BS_OK;
}

VOID _RcuBs_Free(IN RCU_NODE_S *pstRcuNode, IN PF_RCU_FREE_FUNC pfFreeFunc)
{
    RCU_Call(&g_bs_rcu, pstRcuNode, pfFreeFunc);
}

UINT _RcuBs_Lock()
{
    return RCU_Lock(&g_bs_rcu);
}

VOID _RcuBs_UnLock(IN UINT uiPhase)
{
    RCU_UnLock(&g_bs_rcu, uiPhase);
}

