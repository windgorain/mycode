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
#endif /* __cplusplus */

#define SPINLOCK_INIT_VALUE {0,0}

typedef struct
{
    volatile int current_ticket;
    volatile int user_ticket;
}SPINLOCK_S;

VOID SpinLock_Init(IN SPINLOCK_S *pstLock);
VOID SpinLock_Lock(IN SPINLOCK_S *pstLock);
VOID SpinLock_UnLock(IN SPINLOCK_S *pstLock);
BOOL_T SpinLock_TryLock(IN SPINLOCK_S *pstLock);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__SPINLOCK_UTL_H_*/


