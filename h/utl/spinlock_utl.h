/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2014-8-22
* Description: 
* History:     
******************************************************************************/

#ifndef __SPINLOCK_UTL_H_
#define __SPINLOCK_UTL_H_

#include "utl/atomic_utl.h"

#ifdef __cplusplus
    extern "C" {
#endif 

#define SPINLOCK_INIT_VALUE {0,0}

typedef struct {
    volatile S64 current_ticket;
    volatile S64 user_ticket;
}SPINLOCK_S;

static inline void SpinLock_Init(IN SPINLOCK_S *lock)
{
    lock->current_ticket = 0;
    lock->user_ticket = 0;
}

static inline void SpinLock_Lock(IN SPINLOCK_S *lock)
{
    S64 me_ticket = __sync_fetch_and_add(&lock->user_ticket, 1);
    while (lock->current_ticket != me_ticket);
}

static inline void SpinLock_UnLock(IN SPINLOCK_S *lock)
{
    lock->current_ticket++;
}

static inline BOOL_T SpinLock_TryLock(IN SPINLOCK_S *lock)
{
    S64 ticket = lock->current_ticket;
    S64 next_user = ticket + 1;

    if (__sync_bool_compare_and_swap(&lock->user_ticket, ticket, next_user)) {
        return TRUE;
    }

    return FALSE;
}

#ifdef __cplusplus
    }
#endif 

#endif 


