/*================================================================
*   Created by LiXingang
*   Description:  根据ID/KEY管理数据
*      根据ID查找data
*      根据key查找data
*      ID/KEY互相查找
*
================================================================*/
#include "bs.h"
#include "utl/map_utl.h"
#include "utl/mem_cap.h"
#include "utl/idkey_utl.h"

#define IDKEY_MAP_BUCKET (1024*8)



static void idkey_base_reset(IDKEY_HDL hCtrl, PF_IDKEY_FREE_FUNC free_func, void *ud);
static void idkey_base_destroy(IDKEY_HDL hCtrl, PF_IDKEY_FREE_FUNC free_func, void *ud);
static void idkey_base_setcapacity(IDKEY_HDL hCtrl, INT64 capacity);
static int idkey_base_set(IDKEY_HDL hCtrl, INT64 id, void *key, int key_len, void *data, UINT flag);
static INT64 idkey_base_add(IDKEY_HDL hCtrl, void *key, int key_len, void *data, UINT flag);
static void * idkey_base_del_by_key(IDKEY_HDL hCtrl, void *key, int key_len);
static void * idkey_base_del_by_id(IDKEY_HDL hCtrl, INT64 id);
static INT64 idkey_base_get_id_by_key(IDKEY_HDL hCtrl, void *key, int key_len);
static IDKEY_KL_S * idkey_base_get_key_by_id(IDKEY_HDL hCtrl, INT64 id);
static void * idkey_base_get_data_by_id(IDKEY_HDL hCtrl, INT64 id);
static void * idkey_base_get_data_by_key(IDKEY_HDL hCtrl, void *key, int key_len);
static void idkey_base_walk(IDKEY_HDL hCtrl, PF_IDKEY_WALK_FUNC walk_func, void *ud);

static IDKEY_FUNC_S g_idkey_base_funcs = {
    .reset = idkey_base_reset,
    .destroy = idkey_base_destroy,
    .set_capacity = idkey_base_setcapacity,
    .set = idkey_base_set,
    .add = idkey_base_add,
    .del_by_key = idkey_base_del_by_key,
    .del_by_id = idkey_base_del_by_id,
    .get_id_by_key = idkey_base_get_id_by_key,
    .get_key_by_id = idkey_base_get_key_by_id,
    .get_data_by_id = idkey_base_get_data_by_id,
    .get_data_by_key = idkey_base_get_data_by_key,
    .walk = idkey_base_walk
};

static void _idkey_free_node(IDKEY_IMPL_S *ctrl, IDKEY_NODE_S *node)
{
    if ((node->flag & IDKEY_NODE_FLAG_DUP_KEY) && (node->kl.key)) {
        MemCap_Free(ctrl->memcap, node->kl.key);
    }
    MemCap_Free(ctrl->memcap, node);
}

static void _idkey_del_node(IDKEY_IMPL_S *ctrl, IDKEY_NODE_S *node)
{
    if (ctrl->id_tbl) {
        MAP_Del(ctrl->id_tbl, &node->id, sizeof(node->id));
    }
    if (ctrl->key_tbl) {
        MAP_Del(ctrl->key_tbl, node->kl.key, node->kl.key_len);
    }
    _idkey_free_node(ctrl, node);
}

static void _idkey_free_each(void *data, void *ud)
{
    USER_HANDLE_S *uh = ud;
    PF_IDKEY_FREE_FUNC free_func = uh->ahUserHandle[0];
    IDKEY_IMPL_S *ctrl = uh->ahUserHandle[2];
    IDKEY_NODE_S *node = data;

    if (free_func) {
        free_func(node->data, uh->ahUserHandle[1]);
    }

    _idkey_free_node(ctrl, data);
}

static inline IDKEY_NODE_S * _idkey_find_by_id(IDKEY_IMPL_S *ctrl, UINT64 id)
{
    BS_DBGASSERT(ctrl->id_tbl != NULL);
    return MAP_Get(ctrl->id_tbl, &id, sizeof(id));
}

static inline IDKEY_NODE_S * _idkey_find_by_key(IDKEY_IMPL_S *ctrl, void *key, int key_len)
{
    BS_DBGASSERT(ctrl->key_tbl != NULL);
    return MAP_Get(ctrl->key_tbl, key, key_len);
}

static void idkey_base_reset(IDKEY_HDL hCtrl, PF_IDKEY_FREE_FUNC free_func, void *ud)
{
    IDKEY_IMPL_S *ctrl = hCtrl;
    USER_HANDLE_S uh;

    uh.ahUserHandle[0] = free_func;
    uh.ahUserHandle[1] = ud;
    uh.ahUserHandle[2] = ctrl;

    if (ctrl->id_tbl) {
        MAP_Reset(ctrl->id_tbl, _idkey_free_each, &uh);
    }
    if (ctrl->key_tbl) {
        MAP_Reset(ctrl->key_tbl, NULL, NULL);
    }

    ctrl->id_to_use = 1;
}

static void idkey_base_destroy(IDKEY_HDL hCtrl, PF_IDKEY_FREE_FUNC free_func, void *ud)
{
    IDKEY_Reset(hCtrl, free_func, ud);
    MEM_Free(hCtrl);
}

static void idkey_base_setcapacity(IDKEY_HDL hCtrl, INT64 capacity)
{
    IDKEY_IMPL_S *ctrl = hCtrl;
    MAP_SetCapacity(ctrl->id_tbl, capacity);
}

/* 不做key和ID的重复检查 */
static int idkey_base_set(IDKEY_HDL hCtrl, INT64 id, void *key, int key_len, void *data, UINT flag)
{
    IDKEY_IMPL_S *ctrl = hCtrl;
    IDKEY_NODE_S *node;
    void *key_tmp = key;

    node = MemCap_ZMalloc(ctrl->memcap, sizeof(IDKEY_NODE_S));
    if (! node) {
        RETURN(BS_NO_MEMORY);
    }

    if (flag & IDKEY_NODE_FLAG_DUP_KEY) {
        key_tmp = MemCap_Dup(ctrl->memcap, key, key_len);
        if (!key_tmp) {
            MemCap_Free(ctrl->memcap, node);
            RETURN(BS_NO_MEMORY);
        }
    }

    node->data = data;
    node->kl.key = key_tmp;
    node->kl.key_len = key_len;
    node->id = id;

    if (ctrl->id_tbl) {
        if (0 != MAP_Add(ctrl->id_tbl, &node->id, sizeof(node->id), node, 0)) {
            _idkey_free_node(ctrl, node);
            RETURN(BS_NO_RESOURCE);
        }
    }

    if (ctrl->key_tbl) {
        if (0 != MAP_Add(ctrl->key_tbl, node->kl.key, key_len, node, 0)) {
            if (ctrl->id_tbl) {
                MAP_Del(ctrl->id_tbl, &node->id, sizeof(node->id));
            }
            _idkey_free_node(ctrl, node);
            RETURN(BS_NO_RESOURCE);
        }
    }

    if (!ctrl->key_tbl && !ctrl->id_tbl) {
        MemCap_Free(ctrl->memcap,node);
    }

    return 0;
}

/* 不做key的重复检查 */
static INT64 idkey_base_add(IDKEY_HDL hCtrl, void *key, int key_len, void *data, UINT flag)
{
    IDKEY_IMPL_S *ctrl = hCtrl;

    if (0 != idkey_base_set(hCtrl, ctrl->id_to_use, key, key_len, data, flag)) {
        return -1;
    }

    UINT64 id = ctrl->id_to_use;
    ctrl->id_to_use ++;

    return id;
}

/* 删除并返回pData */
static void * idkey_base_del_by_key(IDKEY_HDL hCtrl, void *key, int key_len)
{
    void *data = NULL;
    IDKEY_NODE_S *node = _idkey_find_by_key(hCtrl, key, key_len);

    if (node) {
        data = node->data;
        _idkey_del_node(hCtrl, node);
    }

    return data;
}

/* 删除并返回pData */
static void * idkey_base_del_by_id(IDKEY_HDL hCtrl, INT64 id)
{
    void *data = NULL;
    IDKEY_NODE_S *node = _idkey_find_by_id(hCtrl, id);

    if (node) {
        data = node->data;
        _idkey_del_node(hCtrl, node);
    }

    return data;
}

static INT64 idkey_base_get_id_by_key(IDKEY_HDL hCtrl, void *key, int key_len)
{
    IDKEY_NODE_S *node = _idkey_find_by_key(hCtrl, key, key_len);
    if (! node) {
        return -1;
    }
    return node->id;
}

static IDKEY_KL_S * idkey_base_get_key_by_id(IDKEY_HDL hCtrl, INT64 id)
{
    IDKEY_NODE_S *node = _idkey_find_by_id(hCtrl, id);
    if (! node) {
        return NULL;
    }
    return &node->kl;
}

static void * idkey_base_get_data_by_id(IDKEY_HDL hCtrl, INT64 id)
{
    IDKEY_NODE_S *node = _idkey_find_by_id(hCtrl, id);
    if (! node) {
        return NULL;
    }
    return node->data;
}

static void * idkey_base_get_data_by_key(IDKEY_HDL hCtrl, void *key, int key_len)
{
    IDKEY_NODE_S *node = _idkey_find_by_key(hCtrl, key, key_len);
    if (! node) {
        return NULL;
    }
    return node->data;
}

static void idkey_base_walk(IDKEY_HDL hCtrl, PF_IDKEY_WALK_FUNC walk_func, void *ud)
{
    IDKEY_IMPL_S *ctrl = hCtrl;
    MAP_ELE_S *ele = NULL;
    IDKEY_NODE_S *node;

    if (ctrl->id_tbl) {
        while ((ele = MAP_GetNext(ctrl->id_tbl, ele))) {
            node = ele->pData;
            walk_func(&node->kl, node->id, node->data, ud);
        }
    } else if (ctrl->key_tbl) {
        while ((ele = MAP_GetNext(ctrl->key_tbl, ele))) {
            node = ele->pData;
            walk_func(&node->kl, node->id, node->data, ud);
        }
    } else {
        BS_DBGASSERT(0);
    }
}

static IDKEY_PARAM_S g_idkey_dft_param = {
    .flag = IDKEY_FLAG_ID_MAP | IDKEY_FLAG_KEY_MAP
};

IDKEY_S * IDKEY_BaseCreate(IDKEY_PARAM_S *p)
{
    if (p == NULL) {
        p = &g_idkey_dft_param;
    }

    int bucket_num = p->bucket_num;

    if (bucket_num == 0) {
        bucket_num = IDKEY_MAP_BUCKET;
    }

    IDKEY_IMPL_S *ctrl = MEM_ZMalloc(sizeof(IDKEY_IMPL_S));
    if (! ctrl) {
        return NULL;
    }

    MAP_PARAM_S map_param = {0};
    map_param.bucket_num = bucket_num;
    map_param.memcap = p->memcap;

    if (p->flag & IDKEY_FLAG_ID_MAP) {
        ctrl->id_tbl = MAP_HashCreate(&map_param);
        if (! ctrl->id_tbl) {
            idkey_base_destroy(ctrl, NULL, NULL);
            return NULL;
        }
    }

    if (p->flag & IDKEY_FLAG_KEY_MAP) {
        ctrl->key_tbl = MAP_HashCreate(&map_param);
        if (! ctrl->key_tbl) {
            idkey_base_destroy(ctrl, NULL, NULL);
            return NULL;
        }
    }

    ctrl->id_to_use = 1;
    ctrl->funcs = &g_idkey_base_funcs;

    return (void*)ctrl;
}

