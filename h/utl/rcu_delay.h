/*================================================================
*   Created by LiXingang
*   Description: 延迟释放
*
================================================================*/
#ifndef _RCU_DELAY_H
#define _RCU_DELAY_H
#include "utl/list_sl.h"
#include "utl/rcu_utl.h"
#ifdef __cplusplus
extern "C"
{
#endif

typedef struct {
    SL_HEAD_S list[2];
    volatile int counter[2];
    volatile int grace_period_count;
    volatile int state;
}RCU_DELAY_S;

int RcuDelay_Init(RCU_DELAY_S *ctrl);
void RcuDelay_Call(RCU_DELAY_S *ctrl, RCU_NODE_S *node, PF_RCU_FREE_FUNC func);
void RcuDelay_Wait(RCU_DELAY_S *ctrl);
void RcuDelay_Sync(RCU_DELAY_S *ctrl);
void RcuDelay_Step(RCU_DELAY_S *ctrl);

/* 如果能确保调用者的临界区使用时间不超过一个step时间，则可以不用调用此函数;
 step时间,即定时释放资源的间隔时间 */
static inline int RcuDelay_Lock(RCU_DELAY_S *ctrl)
{
    int state = ctrl->state;
    ATOM_INC_FETCH(&ctrl->counter[state]);
    return state;
}

static inline void RcuDelay_Unlock(RCU_DELAY_S *ctrl, int state)
{
    ATOM_DEC_FETCH(&ctrl->counter[state]);
}

#ifdef __cplusplus
}
#endif
#endif //RCU_DELAY_H_
