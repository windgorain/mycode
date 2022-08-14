/******************************************************************************
* Copyright (C),    LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2014-8-22
* Description: 自旋锁
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/spinlock_utl.h"

VOID SpinLock_Init(IN SPINLOCK_S *pstLock)
{
    Mem_Zero(pstLock, sizeof(SPINLOCK_S));
    return;
}

VOID SpinLock_Lock(IN SPINLOCK_S *pstLock)
{
    INT iMeTicket;

    iMeTicket = ATOM_FETCH_ADD(&pstLock->user_ticket, 1);

    while (pstLock->current_ticket != iMeTicket);
}

VOID SpinLock_UnLock(IN SPINLOCK_S *pstLock)
{
    pstLock->current_ticket++;
}

BOOL_T SpinLock_TryLock(IN SPINLOCK_S *pstLock)
{
    int iTicket = pstLock->current_ticket;
    int iNextUser = iTicket + 1;

    if (ATOM_BOOL_COMP_SWAP(&pstLock->user_ticket, &iTicket, iNextUser)) {
        return TRUE;
    }

    return FALSE;
}

