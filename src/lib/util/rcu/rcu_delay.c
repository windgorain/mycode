/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/list_sl.h"
#include "utl/rcu_delay.h"

int RcuDelay_Init(RCU_DELAY_S *ctrl)
{
    memset(ctrl, 0, sizeof(RCU_DELAY_S));
    return 0;
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

    if (ctrl->counter[next_state] > 0) {
        return;
    }

    while ((node = (void*)SL_DelHead(&ctrl->list[next_state]))) {
        node->pfFunc(node);
    }

    ATOM_BARRIER();

    ctrl->state = next_state;
}
