#ifndef QUEUE_OBJECT_H_
#define QUEUE_OBJECT_H_
#include <stdint.h>

#if defined (__cplusplus)
extern "C" {
#endif

typedef struct queue_object_s queue_object_t, (*queue_object);

typedef void (*queue_object_data_destructor)(void* data);

typedef struct queue_object_s {
    int     type;      
    int     len;

    queue_object_data_destructor delete_data;  
    char   data[0];
} queue_object_s;

static inline void delete_queue_object(queue_object qo)
{
    if (qo->delete_data != NULL) {
        qo->delete_data(qo->data);
    }

	MEM_Free(qo);

    return;
}
#if defined (__cplusplus)
}
#endif

#endif /* QUEUE_OBJECT_H_ */
