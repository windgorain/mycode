/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      lixingang  Version: 1.0  Date: 2007-5-8
* Description: 消息队列,单生产者单消费者模型
* History:     
******************************************************************************/
/* retcode所需要的宏 */
#define RETCODE_FILE_NUM RETCODE_FILE_NUM_QUE

#include "bs.h"

#include "utl/atomic_utl.h"
#include "utl/msgque_utl.h"

/* ---defines--- */
#define _MSGQUE_DFT_CAPACITY 128

MSGQUE_S * MSGQUE_Create(UINT capacity)
{
    if (capacity == 0) {
        capacity = _MSGQUE_DFT_CAPACITY;
    }

    UINT count = capacity;
    if (! NUM_IS2N(count)) {
        count = NUM_To2N(count);
    }
    UINT len = sizeof(MSGQUE_S) + count * sizeof(MSGQUE_MSG_S);

    MSGQUE_S *q = MEM_Malloc(len);
    if (NULL == q) {
        return NULL;
    }
    memset(q, 0, sizeof(MSGQUE_S));

    q->capacity = capacity;
    q->mask = count - 1;

    return q;
}

void MSGQUE_Delete(MSGQUE_S *q)
{
    if (NULL != q) {
        MEM_Free(q);
    }
}

/* 获取消息个数 */
UINT MSGQUE_Count(MSGQUE_S *q)
{
	return (q->prod - q->cons);
}

/* 获取空闲位置个数 */
UINT MSGQUE_FreeCount(MSGQUE_S *q)
{
	return q->capacity - MSGQUE_Count(q);
}

/* 消息队列是否已满 */
int MSGQUE_Full(MSGQUE_S *q)
{
	if (MSGQUE_FreeCount(q) == 0) {
        return TRUE;
    }
    return FALSE;
}

/* 消息队列是否空的 */
int MSGQUE_Empty(MSGQUE_S *q)
{
	return MSGQUE_Count(q) == 0;
}

BS_STATUS MSGQUE_WriteMsg(MSGQUE_S *q, MSGQUE_MSG_S *msg)
{
    BS_DBGASSERT(NULL != q);
    BS_DBGASSERT(NULL != msg);

    if (MSGQUE_Full(q)) {
        return BS_FULL;
    }

    q->msgs[q->prod & q->mask] = *msg;
    q->prod ++;
 
    return BS_OK;
}

BS_STATUS MSGQUE_ReadMsg(MSGQUE_S *q, OUT MSGQUE_MSG_S *msg)
{
    BS_DBGASSERT(NULL != q);
    BS_DBGASSERT(NULL != msg);

    if (MSGQUE_Empty(q)) {
        return BS_EMPTY;
    }

    *msg = q->msgs[q->cons & q->mask];
    q->cons++;

    return BS_OK;
}

