/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/consistent_hash.h"

/* 查找比key大的最小节点 */
int ConsistentHash_FindBigger(CONSISTENT_HASH_S *ctrl, int start, int end, UINT key)
{
    int left, right, mid;
    int found = -1;
    CONSISTENT_HASH_NODE_S *nodes = ctrl->nodes;

    left=start;
    right=end;

    while (left <= right) {
        mid = (left+right) >> 1;
        if (key == nodes[mid].hash) {
            found = mid;
            break;
        } else if (key <= nodes[mid].hash) {
            found = mid;
            right = mid - 1;
        } else {
            left = mid + 1;
        }
    }

    return found;
}

int ConsistentHash_AddNode(CONSISTENT_HASH_S *ctrl, UINT key, UINT data)
{
    CONSISTENT_HASH_NODE_S *node;

    if (ctrl->count >= ctrl->max) {
        return -1;
    }

    node = &ctrl->nodes[ctrl->count];

    int found = ConsistentHash_FindBigger(ctrl, 0, ctrl->count - 1, key);
    if (found >= 0) {
        memmove(&ctrl->nodes[found + 1], &ctrl->nodes[found],
                sizeof(CONSISTENT_HASH_NODE_S) * (ctrl->count - found));
        node = &ctrl->nodes[found];
    }

    node->hash = key;
    node->data = data;
    ctrl->count ++;

    return 0;
}