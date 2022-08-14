/*================================================================
*   Created by LiXingang
*   Description: simple queue
*
================================================================*/
#include "bs.h"
#include "utl/que_utl.h"

void SQUE_Init(SQUE_S * s)
{
    s->head = s->tail = 0;
    s->count = 0;
}

void SQUE_Final(SQUE_S * s)
{
    while (SQUE_Count(s)) {
        SQUE_Pop(s);
    }
}

int SQUE_Push(SQUE_S * s, void *data)
{
    SQUE_NODE_S * q;
    if (!s->head) {
        q = s->tail = s->head = MEM_ZMalloc(sizeof(SQUE_NODE_S));
        if (! q) {
            RETURN(BS_NO_MEMORY);
        }
        q->data = data;
        q->next = 0;
    } else {
        q =  MEM_ZMalloc(sizeof(SQUE_NODE_S));
        if (! q) {
            RETURN(BS_NO_MEMORY);
        }
        q->data = data;
        q->next = 0;
        s->tail->next = q;
        s->tail = q;
    }
    s->count++;

    return BS_OK;
}

void * SQUE_Pop(SQUE_S *s)
{
    void * data = NULL;
    SQUE_NODE_S * q;
    if (s->head) {
        q = s->head;
        data = q->data;
        s->head = s->head->next;
        s->count--;
        if (!s->head) {
            s->tail = 0;
            s->count = 0;
        }
        MEM_Free(q);
    }
    return data;
}

int SQUE_Count(SQUE_S * s)
{
    return s->count;
}

