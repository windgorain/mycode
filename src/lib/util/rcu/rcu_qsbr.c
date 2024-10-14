/*================================================================
*   Created by LiXingang
*   Author: LiXingang  Version: 1.0  Date: 2008-9-30
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/atomic_utl.h"
#include "utl/rcu_qsbr.h"

static inline void _rcuqsbr_sync(RCU_QSBR_S *ctrl)
{
    int i;
    int state = ATOM_INC_FETCH(&ctrl->writer_state);

    for (i=0; i<ctrl->reader_num; i++) {
        while (1) {
            int reader_state = ATOM_GET(&ctrl->reader_state[i]);
            int result = state - reader_state;
            if (result <= 0) {
                break;
            }
            Sleep(0);
        }
    }
}

int RcuQsbr_SetReaderNum(RCU_QSBR_S *ctrl, int reader_num)
{
    if (reader_num > RCU_QSBR_MAX_READER) {
        BS_DBGASSERT(0);
        RETURN(BS_BAD_PARA);
    }
    ctrl->reader_num = reader_num;
    return 0;
}


void RcuQsbr_Sync(RCU_QSBR_S *ctrl)
{
    _rcuqsbr_sync(ctrl);
}

