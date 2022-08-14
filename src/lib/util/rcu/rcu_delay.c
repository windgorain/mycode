/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/list_sl.h"
#include "utl/rcu_delay.h"

/* 检查是否可以立即动作 */
static inline BOOL_T rcudelay_do_just(RCU_DELAY_S *ctrl)
{
    if ((ctrl->counter[0] == 0) && (ctrl->counter[1] == 0)) {
        return TRUE;
    }

    return FALSE;
}

int RcuDelay_Init(RCU_DELAY_S *ctrl)
{
    memset(ctrl, 0, sizeof(RCU_DELAY_S));
    return 0;
}

/* 返回条件: 宽限期过去
   对于某些假设宽限期肯定能完成而没加Rcu Lock的情况,需要使用这个接口代替Sync接口
 */
void RcuDelay_Wait(RCU_DELAY_S *ctrl)
{
    int old_period_count;
    int now_period_count;

    old_period_count = ctrl->grace_period_count;

    while (1) {
        Sleep(100);
        now_period_count = ctrl->grace_period_count;
        if ((now_period_count - old_period_count) >= 2) {
            break;
        }
    }

    return;
}

/* 返回条件: 1. 没有任何人在Rcu区
   2. 宽限期过去
   因为检查了Rcu区是否为空,所以要求使用者必须配套使用RCU LOCK
*/
void RcuDelay_Sync(RCU_DELAY_S *ctrl)
{
    int old_period_count;
    int now_period_count;

    if (rcudelay_do_just(ctrl)) {
        return;
    }

    old_period_count = ctrl->grace_period_count;

    while (1) {
        Sleep(100);
        if (rcudelay_do_just(ctrl)) {
            break;
        }
        now_period_count = ctrl->grace_period_count;
        if ((now_period_count - old_period_count) >= 2) {
            break;
        }
    }

    return;
}

void RcuDelay_Call(RCU_DELAY_S *ctrl, RCU_NODE_S *node, PF_RCU_FREE_FUNC func)
{
    int state = ATOM_GET(&ctrl->state);
    node->pfFunc = func;
    SL_AddHead(&ctrl->list[state], &node->node);
}

void RcuDelay_Step(RCU_DELAY_S *ctrl)
{
    RCU_NODE_S *node;
    int next_state = ctrl->state == 0 ? 1 : 0;

    /* 如果有人还在使用,则继续延迟 */
    if (ctrl->counter[next_state] > 0) {
        return;
    }

    while ((node = (void*)SL_DelHead(&ctrl->list[next_state]))) {
        node->pfFunc(node);
    }

    ATOM_BARRIER();

    ctrl->grace_period_count++;
    ctrl->state = next_state;
}

