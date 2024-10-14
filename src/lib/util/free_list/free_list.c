/*================================================================
*   Created by LiXingang
*   Author: Xingang.Li  Version: 1.0  Date: 2017-10-2
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/list_sl.h"
#include "utl/free_list.h"

void FreeList_Init(FREE_LIST_S *list)
{
    memset(list, 0, sizeof(FREE_LIST_S));
}

void FreeList_Puts(FREE_LIST_S *list, void *nodes, UINT node_len, UINT nodes_count)
{
    int i;
    unsigned char *tmp = nodes;
    void *node;

    for (i=nodes_count-1; i>=0; i--) {
        node = tmp + i*node_len;
        SL_AddHead(&list->free_list, node);
    }
}

