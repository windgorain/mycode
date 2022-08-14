/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/atomic_utl.h"
#include "utl/rcu_qsbr.h"

static inline BOOL_T rcuqsbr_Wait(RCU_QSBR_S *ctrl, int state)
{
    int i;

    for (i=0; i<ctrl->reader_num; i++) {
        int reader_state = ATOM_GET(&ctrl->reader_state[i]);
        int result = state - reader_state;
        if (result > 0) {
            return FALSE;
        }
    }

    return TRUE;
}

void RcuQsbr_Free(RCU_QSBR_S *ctrl, RCU_NODE_S *node, PF_RCU_FREE_FUNC func)
{
    int state = ATOM_INC_FETCH(&ctrl->writer_state);

    while (! rcuqsbr_Wait(ctrl, state)) {
        Sleep(100);
    }

    func(node);
}

void RcuQsbr_Quiescent(RCU_QSBR_S *ctrl, int reader_id)
{
    BS_DBGASSERT(reader_id < RCU_QSBR_MAX_READER);
    ctrl->reader_state[reader_id] = ctrl->writer_state;
}

void RcuQsbr_SetReaderNum(RCU_QSBR_S *ctrl, int reader_num)
{
    BS_DBGASSERT(reader_num <= RCU_QSBR_MAX_READER);
    ctrl->reader_num = reader_num;
}
