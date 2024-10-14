#include "bs.h"
#include "utl/list_stq.h"
#include "utl/que_utl.h"

typedef struct BLOCK_QUE_S {
    STQ_HEAD_S head;
    MUTEX_S mutex;
    COND_S cond;
    UINT need_wait: 1;
}BLOCK_QUE_S;

BLOCK_QUE_S * BlockQue_Create(void)
{
    BLOCK_QUE_S *q = MEM_ZMalloc(sizeof(BLOCK_QUE_S));
    if (! q) {
        return NULL;
    }

    MUTEX_InitNormal(&q->mutex);
    COND_Init(&q->cond);

    STQ_Init(&(q->head));

    q->need_wait = 1;

    return q;
}

void BlockQue_Destroy(BLOCK_QUE_S *q, PF_BLOCK_QUE_FREE free_func, void *ud)
{
    BlockQue_DelAll(q, free_func, ud);
    MUTEX_Final(&q->mutex);
    COND_Final(&q->cond);

    MEM_Free(q);

    return;
}

void BlockQue_DelAll(BLOCK_QUE_S *q, PF_BLOCK_QUE_FREE free_func, void *ud)
{
    void *node, *next_node;

    MUTEX_P(&q->mutex);
    STQ_FOREACH_SAFE(&q->head, node, next_node) {
        free_func(node, ud);
    }
    MUTEX_V(&q->mutex);
}

BOOL_T BlockQue_IsEmpty(BLOCK_QUE_S *q)
{
    return STQ_IsEmpty(&q->head);
}

BOOL_T BlockQue_IsNeedWake(BLOCK_QUE_S *q)
{
    return q->need_wait;
}

void BlockQue_SetNeedWait(BLOCK_QUE_S *q, BOOL_T need)
{
    MUTEX_P(&q->mutex);
    q->need_wait = need;
    if (! need) {
        COND_WakeAll(&q->cond);
    }
    MUTEX_V(&q->mutex);
}

void BlockQue_Put(BLOCK_QUE_S *q, STQ_NODE_S *node)
{
    MUTEX_P(&q->mutex);
    STQ_AddTail(&q->head, node);
    MUTEX_V(&q->mutex);
    COND_Wake(&q->cond);
}


void * BlockQue_Poll(BLOCK_QUE_S *q)
{
    STQ_NODE_S *node;

    MUTEX_P(&q->mutex);
    node = STQ_DelHead(&q->head);
    MUTEX_V(&q->mutex);

    return node;
}


void * BlockQue_Take(BLOCK_QUE_S *q)
{
    STQ_NODE_S *node;

    MUTEX_P(&q->mutex);
    while (BlockQue_IsEmpty(q)) {
        COND_Wait(&q->cond, &q->mutex);
    }
    node = STQ_DelHead(&q->head);
    MUTEX_V(&q->mutex);

    return node;
}


void * BlockQue_CondTake(BLOCK_QUE_S *q)
{
    STQ_NODE_S *node;

    MUTEX_P(&q->mutex);
    while (BlockQue_IsNeedWake(q) && BlockQue_IsEmpty(q)) {
        COND_Wait(&q->cond, &q->mutex);
    }
    node = STQ_DelHead(&q->head);
    MUTEX_V(&q->mutex);

    return node;
}

void * BlockQue_Peek(BLOCK_QUE_S *q)
{
    STQ_NODE_S *node;

    MUTEX_P(&q->mutex);
    node = STQ_First(&(q->head));
    MUTEX_V(&q->mutex);

    return node;
}

int BlockQue_Count(BLOCK_QUE_S *q)
{
    int num = 0;
    void *node;

    MUTEX_P(&q->mutex);
    STQ_FOREACH(&q->head, node) {
        num++;
    }
    MUTEX_V(&q->mutex);

    return num;
}

