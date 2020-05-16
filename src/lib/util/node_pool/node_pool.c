/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/list_sl.h"
#include "utl/node_pool.h"

int NodePool_Init(NODE_POOL_S *node_pool)
{
    memset(node_pool, 0, sizeof(NODE_POOL_S));

    return 0;
}

int NodePool_PutNodes(NODE_POOL_S *node_pool, void *nodes, UINT node_len,
        UINT max_nodes)
{
    int i;
    unsigned char *tmp = nodes;
    void *node;

    for (i=max_nodes; i>=0; i--) {
        node = tmp + i*node_len;
        SL_AddHead(&node_pool->free_list, node);
    }

    return 0;
}

void * NodePool_Get(NODE_POOL_S *node_pool)
{
    return (void*) SL_DelHead(&node_pool->free_list);
}

void NodePool_Free(NODE_POOL_S *node_pool, void *node)
{
    SL_AddHead(&node_pool->free_list, node);
}

