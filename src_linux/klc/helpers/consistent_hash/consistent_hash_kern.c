#include "klc/klc_base.h"
#include "helpers/consistent_hash_klc.h"

#define KLC_MODULE_NAME CONSISTENT_HASH_MODULE_NAME

KLC_DEF_MODULE();

/* 查找一致性hash的虚拟节点 */
SEC_NAME_FUNC(CONSISTENT_HASH_MATCH_NAME)
int consistent_hash_match(CONSISTENT_HASH_S *ctrl, UINT hash)
{
    int index;
    index = ConsistentHash_Match(ctrl, hash);
    if (index < 0) {
        return index;
    }
    return ctrl->nodes[index].data;
}


