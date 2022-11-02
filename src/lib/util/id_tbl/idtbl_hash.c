/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/map_utl.h"
#include "utl/idtbl_utl.h"

typedef struct {
    IDTBL_S id_tbl;
    MAP_HANDLE hash_map;
}IDTBL_HASH_S;

static int idtbl_hash_add(void *id_tbl, UINT64 id, void *data);
static void * idtbl_hash_get(void *id_tbl, UINT64 id);
static void * idtbl_hash_del(void *id_tbl, UINT64 id);
static void idtbl_hash_reset(void *id_tbl, PF_IDTBL_FREE free_func, void *ud);
static void idtbl_hash_destroy(void *id_tbl, PF_IDTBL_FREE free_func, void *ud);

static IDTBL_FUNC_S g_idtbl_hash_funcs = {
    idtbl_hash_add,
    idtbl_hash_get,
    idtbl_hash_del,
    idtbl_hash_reset,
    idtbl_hash_destroy
};

IDTBL_S * IDTBL_HashCreate(UINT bucket_num)
{
    IDTBL_HASH_S *ctrl;

    ctrl = MEM_ZMalloc(sizeof(IDTBL_HASH_S));
    if (! ctrl) {
        return NULL;
    }

    MAP_PARAM_S map_param = {0};
    map_param.bucket_num = bucket_num;

    ctrl->hash_map = MAP_HashCreate(&map_param);
    if (! ctrl->hash_map) {
        MEM_Free(ctrl);
        return NULL;
    }

    ctrl->id_tbl.funcs = &g_idtbl_hash_funcs;

    return (void*)ctrl;
}

static inline void * idtbl_get_key(UINT64 *id)
{
    if (sizeof(void*) == sizeof(UINT64)) {
        return (void*)(ULONG)(*id);
    }
    return id;
}

static inline int idtbl_get_key_len()
{
    if (sizeof(void*) == sizeof(UINT64)) {
        return 0;
    }
    return sizeof(UINT64);
}

static int idtbl_hash_add(void *id_tbl, UINT64 id, void *data)
{
    IDTBL_HASH_S *ctrl = id_tbl;
    return MAP_Add(ctrl->hash_map, idtbl_get_key(&id), idtbl_get_key_len(), data, MAP_FLAG_DUP_KEY);
}

static void * idtbl_hash_get(void *id_tbl, UINT64 id)
{
    IDTBL_HASH_S *ctrl = id_tbl;
    return MAP_Get(ctrl->hash_map, idtbl_get_key(&id), idtbl_get_key_len());
}

static void * idtbl_hash_del(void *id_tbl, UINT64 id)
{
    IDTBL_HASH_S *ctrl = id_tbl;
    return MAP_Del(ctrl->hash_map, idtbl_get_key(&id), idtbl_get_key_len());
}

static void idtbl_hash_reset(void *id_tbl, PF_IDTBL_FREE free_func, void *ud)
{
    IDTBL_HASH_S *ctrl = id_tbl;
    MAP_Reset(ctrl->hash_map, free_func, ud);
}

static void idtbl_hash_destroy(void *id_tbl, PF_IDTBL_FREE free_func, void *ud)
{
    IDTBL_HASH_S *ctrl = id_tbl;
    MAP_Destroy(ctrl->hash_map, free_func, ud);
    MEM_Free(ctrl);
}

