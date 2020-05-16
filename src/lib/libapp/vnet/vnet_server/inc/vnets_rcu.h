
#ifndef __VNETS_RCU_H_
#define __VNETS_RCU_H_

#include "utl/rcu_utl.h"

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

BS_STATUS VNETS_RCU_Init();
VOID VNETS_RCU_Free(IN RCU_NODE_S * pstNode, IN PF_RCU_FREE_FUNC pfFreeFunc);
UINT VNETS_RCU_Lock();
VOID VNETS_RCU_UnLock(IN UINT uiPhase);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__VNETS_RCU_H_*/


