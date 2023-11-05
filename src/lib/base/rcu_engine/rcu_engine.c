/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/rcu_delay.h"
#include "utl/mem_cap.h"
#include "utl/mutex_utl.h"
#include "utl/atomic_once.h"

static RCU_DELAY_S g_rcu_engine;
static MUTEX_S g_rcu_engine_lock;
static MEM_CAP_S g_rcu_engine_memcap;
static MTIMER_S g_rcu_engine_mtimer;

static VOID rcu_engine_timeout(HANDLE timer_handle, USER_HANDLE_S *ud)
{
    RcuDelay_Step(&g_rcu_engine);
}

static int rcu_engine_init_once(void *ud)
{
    MTimer_Add(&g_rcu_engine_mtimer, 1000, TIMER_FLAG_CYCLE, rcu_engine_timeout, NULL);
    return 0;
}

static inline void rcu_engine_timer_init()
{
    static ATOM_ONCE_S once = ATOM_ONCE_INIT_VALUE;

    AtomOnce_WaitDo(&once, rcu_engine_init_once, NULL);
}

static void rcu_engine_constructor()
{
    MUTEX_Init(&g_rcu_engine_lock);

    MemCap_Init(&g_rcu_engine_memcap, _mem_rcu_malloc, MEM_RcuFree, RcuEngine_Call);

    RcuDelay_Init(&g_rcu_engine);
}

CONSTRUCTOR(init) {
    rcu_engine_constructor();
}


void * RcuEngine_GetMemcap()
{
    return &g_rcu_engine_memcap;
}


void RcuEngine_Call(RCU_NODE_S *rcu_node, PF_RCU_FREE_FUNC rcu_func)
{
    rcu_engine_timer_init();

    MUTEX_P(&g_rcu_engine_lock);
    RcuDelay_Call(&g_rcu_engine, rcu_node, rcu_func);
    MUTEX_V(&g_rcu_engine_lock);
}

void RcuEngine_Wait()
{
    RcuDelay_Wait(&g_rcu_engine);
}

void RcuEngine_Sync()
{
    RcuDelay_Sync(&g_rcu_engine);
}


int RcuEngine_Lock()
{
    return RcuDelay_Lock(&g_rcu_engine);
}

void RcuEngine_UnLock(int state)
{
    RcuDelay_Unlock(&g_rcu_engine, state);
}

