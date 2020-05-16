/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2008-9-30
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/spinlock_utl.h"
#include "utl/rcu_utl.h"

void RCU_Init(INOUT RCU_S *rcu)
{
    memset(rcu, 0, sizeof(RCU_S));
    SpinLock_Init(&rcu->stLock);
}

VOID RCU_Call
(
    IN RCU_HANDLE hRcuHandle,
    IN RCU_NODE_S *pstRcuNode,
    IN PF_RCU_FREE_FUNC pfFreeFunc
)
{
    RCU_S *pstRcuCtrl = hRcuHandle;
    BOOL_T bFreeJust = TRUE;

    pstRcuNode->pfFunc = pfFreeFunc;

    SpinLock_Lock(&pstRcuCtrl->stLock);
    if ((pstRcuCtrl->auiCounter[0] + pstRcuCtrl->auiCounter[1]) > 0)
    {
        SL_AddHead(&pstRcuCtrl->free_list[pstRcuCtrl->uiCurrentPhase],
                &pstRcuNode->node);
        bFreeJust = FALSE;
    }
    SpinLock_UnLock(&pstRcuCtrl->stLock);

    if (bFreeJust == TRUE)
    {
        pfFreeFunc(pstRcuNode);
    }
}

int RCU_Lock(IN RCU_HANDLE hRcuHandle)
{
    RCU_S *pstRcuCtrl = hRcuHandle;
    int phase;

    SpinLock_Lock(&pstRcuCtrl->stLock);
    phase = pstRcuCtrl->uiCurrentPhase;
    pstRcuCtrl->auiCounter[phase]++;
    SpinLock_UnLock(&pstRcuCtrl->stLock);

    return phase;
}

VOID RCU_UnLock(IN RCU_HANDLE hRcuHandle, IN UINT uiPhase)
{
    RCU_S *pstRcuCtrl = hRcuHandle;
    SL_HEAD_S free_list = {0};
    RCU_NODE_S *pstNode;
    UINT uiCurrentPhase;
    UINT uiNextPhase;

    SpinLock_Lock(&pstRcuCtrl->stLock);

    pstRcuCtrl->auiCounter[uiPhase] --;

    uiCurrentPhase = pstRcuCtrl->uiCurrentPhase;
    uiNextPhase = (uiCurrentPhase + 1) & 0x1;

    if (pstRcuCtrl->auiCounter[uiNextPhase] == 0)
    {
        if (pstRcuCtrl->auiCounter[uiCurrentPhase] == 0)
        {
            SL_Append(&free_list, &pstRcuCtrl->free_list[uiCurrentPhase]);
        }

        SL_Append(&free_list, &pstRcuCtrl->free_list[uiNextPhase]);
        pstRcuCtrl->uiCurrentPhase = uiNextPhase;
    }

    SpinLock_UnLock(&pstRcuCtrl->stLock);

    while ((pstNode = (void*)SL_DelHead(&free_list))) {
        pstNode->pfFunc(pstNode);
    }
}

