/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/list_sl.h"
#include "utl/free_list.h"

int FreeList_Init(FREE_LIST_S *list)
{
    memset(list, 0, sizeof(FREE_LIST_S));

    return 0;
}

void FreeList_Put(FREE_LIST_S *list, void *node)
{
    SL_AddHead(&list->free_list, node);
}

int FreeList_Puts(FREE_LIST_S *list, void *nodes, UINT node_len,
        UINT nodes_count)
{
    int i;
    unsigned char *tmp = nodes;
    void *node;

    for (i=nodes_count-1; i>=0; i--) {
        node = tmp + i*node_len;
        SL_AddHead(&list->free_list, node);
    }

    return 0;
}

void * FreeList_Get(FREE_LIST_S *list)
{
    return (void*) SL_DelHead(&list->free_list);
}

