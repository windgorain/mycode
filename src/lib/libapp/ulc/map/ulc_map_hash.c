/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/jhash_utl.h"
#include "utl/hash_utl.h"
#include "../h/ulc_fd.h"
#include "../h/ulc_map.h"

typedef struct {
    ULC_MAP_HEADER_S hdr; /* 必须为第一个成员 */
    U32 buckets_num;
    HASH_HANDLE hash_tbl;
    MUTEX_S lock;
}ULC_MAP_HASH_S;

typedef struct {
    HASH_NODE_S hash_node; /* 必须为第一个成员 */
    ULC_MAP_HASH_S *ctrl;
    void *key;
    void *value;
}ULC_MAP_HASH_NODE_S;

static UINT _ulc_map_hash_hash_index(void *node)
{
    ULC_MAP_HASH_NODE_S *n = node;
    return JHASH_GeneralBuffer(n->key, n->ctrl->hdr.size_key, 0);
}

static int _ulc_map_hash_cmp(void *node, void *key)
{
    ULC_MAP_HASH_NODE_S *n = node;
    ULC_MAP_HASH_NODE_S *k = key;

    return memcmp(n->key, k->key, k->ctrl->hdr.size_key);
}

static void _ulc_map_hash_free_node(ULC_MAP_HASH_NODE_S *node)
{
    if (node->key) {
        RcuEngine_Free(node->key);
        node->key = NULL;
    }
    if (node->value) {
        RcuEngine_Free(node->value);
        node->value = NULL;
    }
    RcuEngine_Free(node);
}

static void _ulc_map_hash_hash_free_node(HASH_HANDLE hash_tbl, void *node, void *ud)
{
    _ulc_map_hash_free_node(node);
}

static void _ulc_map_hash_destroy_map(void *f)
{
    ULC_MAP_HASH_S *ctrl = f;

    HASH_DelAll(ctrl->hash_tbl, _ulc_map_hash_hash_free_node, NULL);
    HASH_DestoryInstance(ctrl->hash_tbl);
    MUTEX_Final(&ctrl->lock);
    RcuEngine_Free(ctrl);
}

static int ulc_map_hash_open(ULC_ELF_MAP_S *elfmap)
{
    int fd;

    if ((! elfmap) || (elfmap->max_elem == 0) || (elfmap->size_key == 0)) {
		return -EINVAL;
    }

    ULC_MAP_HASH_S *ctrl = RcuEngine_ZMalloc(sizeof(ULC_MAP_HASH_S));
    if (! ctrl) {
        return -ENOMEM;
    }

    ctrl->buckets_num = NUM_To2N(elfmap->max_elem);

    ctrl->hash_tbl = HASH_CreateInstance(RcuEngine_GetMemcap(), ctrl->buckets_num, _ulc_map_hash_hash_index);
    if (! ctrl->hash_tbl) {
        RcuEngine_Free(ctrl);
        return -ENOMEM;
    }

    fd = ULC_FD_Open(ULC_FD_TYPE_MAP, ctrl, _ulc_map_hash_destroy_map);
    if (fd < 0) {
        RcuEngine_Free(ctrl);
        return fd;
    }

    MUTEX_Init(&ctrl->lock);

    return fd;
}

static void * ulc_map_hash_lookup_elem(void *map, void *key)
{
    ULC_MAP_HASH_S *ctrl = map;
    ULC_MAP_HASH_NODE_S *found;
    ULC_MAP_HASH_NODE_S node;

    if ((!map) || (!key)) {
        return NULL;
    }

    node.ctrl = ctrl;
    node.key = key;

    found = HASH_Find(ctrl->hash_tbl, _ulc_map_hash_cmp, &node);
    if (! found) {
        return NULL;
    }

    return found->value;
}

static long ulc_map_hash_delete_elem(void *map, void *key)
{
    ULC_MAP_HASH_S *ctrl = map;
    ULC_MAP_HASH_NODE_S *old;

    old = ulc_map_hash_lookup_elem(map, key);
    if (! old) {
        return -ENOENT;
    }

    MUTEX_P(&ctrl->lock);
    HASH_Del(ctrl->hash_tbl, old);
    MUTEX_V(&ctrl->lock);

    _ulc_map_hash_free_node(old);

    return 0;
}

static long ulc_map_hash_update_elem(void *map, void *key, void *value, U32 flag)
{
    ULC_MAP_HASH_S *ctrl = map;
    ULC_MAP_HASH_NODE_S *node;
    ULC_MAP_HASH_NODE_S *old;

    if ((!map) || (!key) || (!value)){
		return -EINVAL;
    }

    old = ulc_map_hash_lookup_elem(map, key);

    if ((flag == BPF_EXIST) && (! old)){
		return -ENOENT;
    }

    if ((flag == BPF_NOEXIST) && (old)){
		return -EEXIST;
    }

    node = RcuEngine_ZMalloc(sizeof(ULC_MAP_HASH_NODE_S));
    if (! node) {
        return -ENOMEM;
    }

    node->key = RcuEngine_MemDup(key, ctrl->hdr.size_key);
    node->value = RcuEngine_MemDup(value, ctrl->hdr.size_value);

    if ((!node->key) || (!node->value)) {
        _ulc_map_hash_free_node(node);
        return -ENOMEM;
    }

    MUTEX_P(&ctrl->lock);
    HASH_Add(ctrl->hash_tbl, node);
    if (old) {
        HASH_Del(ctrl->hash_tbl, old);
    }
    MUTEX_V(&ctrl->lock);

    if (old) {
        _ulc_map_hash_free_node(old);
    }

    return 0;
}

static ULC_MAP_FUNC_TBL_S g_ulc_map_hash_ops = {
    .open_func = ulc_map_hash_open,
    .lookup_elem_func = ulc_map_hash_lookup_elem,
    .delete_elem_func = ulc_map_hash_delete_elem,
    .update_elem_func = ulc_map_hash_update_elem,
    .direct_value_func = NULL,
};

int ULC_MapHash_Init()
{
    return ULC_MAP_RegType(BPF_MAP_TYPE_HASH, &g_ulc_map_hash_ops);
}

