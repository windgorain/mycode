
#include "bs.h"

#include "utl/rcu_utl.h"

#include "../inc/vnets_rcu.h"

static RCU_S g_vnets_rcu;

BS_STATUS VNETS_RCU_Init()
{
    RCU_Init(&g_vnets_rcu);

    return BS_OK;
}

VOID VNETS_RCU_Free(IN RCU_NODE_S * pstNode, IN PF_RCU_FREE_FUNC pfFreeFunc)
{
    RCU_Call(&g_vnets_rcu, pstNode, pfFreeFunc);
}

UINT VNETS_RCU_Lock()
{
    return RCU_Lock(&g_vnets_rcu);
}

VOID VNETS_RCU_UnLock(IN UINT uiPhase)
{
    RCU_UnLock(&g_vnets_rcu, uiPhase);
}

