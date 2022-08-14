#include "bs.h"
#include "utl/list_stq.h"
#include "utl/que_utl.h"

typedef struct BLOCK_QUE_STRUCT {
    STQ_HEAD_S head;
    MUTEX_S mutex;
    COND_S cond;
    UINT need_wait: 1;
}BLOCK_QUE_S;


BLOCKQUE_HANDLE BlockQue_Create()
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

void BlockQue_Destroy(BLOCKQUE_HANDLE q, PF_BLOCK_QUE_FREE free_func, void *ud)
{
    BlockQue_DelAll(q, free_func, ud);
    MUTEX_Final(&q->mutex);
    COND_Final(&q->cond);

    MEM_Free(q);

    return;
}

void BlockQue_DelAll(BLOCKQUE_HANDLE q, PF_BLOCK_QUE_FREE free_func, void *ud)
{
    void *node, *next_node;

    MUTEX_P(&q->mutex);
    STQ_FOREACH_SAFE(&q->head, node, next_node) {
        free_func(node, ud);
    }
    MUTEX_V(&q->mutex);
}

BOOL_T BlockQue_IsEmpty(BLOCKQUE_HANDLE q)
{
    return STQ_IsEmpty(&q->head);
}

BOOL_T BlockQue_IsNeedWake(BLOCKQUE_HANDLE q)
{
    return q->need_wait;
}

void BlockQue_SetNeedWait(BLOCKQUE_HANDLE q, BOOL_T need)
{
    MUTEX_P(&q->mutex);
    q->need_wait = need;
    if (! need) {
        COND_WakeAll(&q->cond);
    }
    MUTEX_V(&q->mutex);
}

void BlockQue_Put(BLOCKQUE_HANDLE q, STQ_NODE_S *node)
{
    MUTEX_P(&q->mutex);
    STQ_AddTail(&q->head, node);
    MUTEX_V(&q->mutex);
    COND_Wake(&q->cond);
}

/* 非阻塞获取 */
void * BlockQue_Poll(BLOCKQUE_HANDLE q)
{
    STQ_NODE_S *node;

    MUTEX_P(&q->mutex);
    node = STQ_DelHead(&q->head);
    MUTEX_V(&q->mutex);

    return node;
}

/* 阻塞式获取 */
void * BlockQue_Take(BLOCKQUE_HANDLE q)
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

/* 通过判断阻塞标记来决定是否阻塞式获取 */
void * BlockQue_CondTake(BLOCKQUE_HANDLE q)
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

void * BlockQue_Peek(BLOCKQUE_HANDLE q)
{
    STQ_NODE_S *node;

    MUTEX_P(&q->mutex);
    node = STQ_First(&(q->head));
    MUTEX_V(&q->mutex);

    return node;
}

int BlockQue_Count(BLOCKQUE_HANDLE q)
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

