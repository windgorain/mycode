/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _RCU_QSBR_H
#define _RCU_QSBR_H
#ifdef __cplusplus
extern "C"
{
#endif

#define RCU_QSBR_MAX_READER 256

typedef struct {
    int reader_num;
    volatile int writer_state;
    volatile int reader_state[RCU_QSBR_MAX_READER];
}RCU_QSBR_S;

int RcuQsbr_SetReaderNum(RCU_QSBR_S *ctrl, int reader_num);
void RcuQsbr_Sync(RCU_QSBR_S *ctrl);


static inline void RcuQsbr_Quiescent(RCU_QSBR_S *ctrl, int reader_id)
{
    BS_DBGASSERT(reader_id < RCU_QSBR_MAX_READER);
    ctrl->reader_state[reader_id] = ctrl->writer_state;
}

#ifdef __cplusplus
}
#endif
#endif 
