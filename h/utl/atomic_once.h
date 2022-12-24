/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _ATOMIC_ONCE_H
#define _ATOMIC_ONCE_H
#include "utl/atomic_utl.h"
#ifdef __cplusplus
extern "C"
{
#endif

typedef struct {
    volatile int begin;
    volatile int end;
}ATOM_ONCE_S;

#define ATOM_ONCE_INIT_VALUE {0}

typedef int (*PF_ATOM_ONCE_CB)(void *ud);

/* 多线程: 只执行一次 */
static inline void AtomOnce_Do(ATOM_ONCE_S *once, PF_ATOM_ONCE_CB func, void *ud)
{
    int to = 0;

    if (once->end) {
        /* 已经Do完成 */
        return;
    }

    if (ATOM_BOOL_COMP_SWAP(&once->begin, &to, 1)) {
        func(ud);
        ATOM_BARRIER();
        ATOM_SET(&once->end, 1);
    } 

    return;
}

/* 多线程: 只执行一次,并且等待其完成 */
static inline void AtomOnce_WaitDo(ATOM_ONCE_S *once, PF_ATOM_ONCE_CB func, void *ud)
{
    int to = 0;

    if (once->end) {
        /* 已经Do完成 */
        return;
    }

    if (ATOM_BOOL_COMP_SWAP(&once->begin, &to, 1)) {
        func(ud);
        ATOM_BARRIER();
        ATOM_SET(&once->end, 1);
    } else {
        while (0 == ATOM_GET(&once->end)) {
            /* 等待处理完成 */
        }
    }

    return;
}




#ifdef __cplusplus
}
#endif
#endif //ATOMIC_ONCE_H_
