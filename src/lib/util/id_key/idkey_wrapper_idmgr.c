/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/idmgr_utl.h"
#include "utl/idkey_utl.h"

typedef struct {
    IDKEY_FUNC_S *funcs;
    void *memcap;
    IDKEY_S *impl;
    IDMGR_S *idmgr;
}IDKEY_WRAPPER_IDMGR_S;

static void idkey_wrapper_idmgr_reset(IDKEY_HDL hCtrl, PF_IDKEY_FREE_FUNC free_func, void *ud);
static void idkey_wrapper_idmgr_destroy(IDKEY_HDL hCtrl, PF_IDKEY_FREE_FUNC free_func, void *ud);
static void idkey_wrapper_idmgr_setcapacity(IDKEY_HDL hCtrl, INT64 capacity);
static int idkey_wrapper_idmgr_set(IDKEY_HDL hCtrl, INT64 id, void *key, int key_len, void *data, UINT flag);
static INT64 idkey_wrapper_idmgr_add(IDKEY_HDL hCtrl, void *key, int key_len, void *data, UINT flag);
static void * idkey_wrapper_idmgr_del_by_key(IDKEY_HDL hCtrl, void *key, int key_len);
static void * idkey_wrapper_idmgr_del_by_id(IDKEY_HDL hCtrl, INT64 id);
static INT64 idkey_wrapper_idmgr_get_id_by_key(IDKEY_HDL hCtrl, void *key, int key_len);
static IDKEY_KL_S * idkey_wrapper_idmgr_get_key_by_id(IDKEY_HDL hCtrl, INT64 id);
static void * idkey_wrapper_idmgr_get_data_by_id(IDKEY_HDL hCtrl, INT64 id);
static void * idkey_wrapper_idmgr_get_data_by_key(IDKEY_HDL hCtrl, void *key, int key_len);
static void idkey_wrapper_idmgr_walk(IDKEY_HDL hCtrl, PF_IDKEY_WALK_FUNC walk_func, void *ud);

static IDKEY_FUNC_S g_idkey_wrapper_idmgr_funcs = {
    .reset = idkey_wrapper_idmgr_reset,
    .destroy = idkey_wrapper_idmgr_destroy,
    .set_capacity = idkey_wrapper_idmgr_setcapacity,
    .set = idkey_wrapper_idmgr_set,
    .add = idkey_wrapper_idmgr_add,
    .del_by_key = idkey_wrapper_idmgr_del_by_key,
    .del_by_id = idkey_wrapper_idmgr_del_by_id,
    .get_id_by_key = idkey_wrapper_idmgr_get_id_by_key,
    .get_key_by_id = idkey_wrapper_idmgr_get_key_by_id,
    .get_data_by_id = idkey_wrapper_idmgr_get_data_by_id,
    .get_data_by_key = idkey_wrapper_idmgr_get_data_by_key,
    .walk = idkey_wrapper_idmgr_walk
};

static void idkey_wrapper_idmgr_free_subres(IDKEY_S *impl, IDMGR_S *idmgr)
{
    if (impl) {
        IDKEY_Destroy(impl, NULL, NULL);
    }
    if (idmgr) {
        IDMGR_Destroy(idmgr);
    }
}

/* 此函数会吸收掉impl和idmgr */
IDKEY_S * IDKEY_WrapperIdmgrCreate(IDKEY_S *impl, IDMGR_S *idmgr)
{
    if ((!impl) || (!idmgr)) {
        idkey_wrapper_idmgr_free_subres(impl, idmgr);
        return NULL;
    }

    IDKEY_WRAPPER_IDMGR_S *ctrl = MEM_ZMalloc(sizeof(IDKEY_WRAPPER_IDMGR_S));
    if (! ctrl) {
        idkey_wrapper_idmgr_free_subres(impl, idmgr);
        return NULL;
    }

    ctrl->impl = impl;
    ctrl->idmgr = idmgr;
    ctrl->funcs = &g_idkey_wrapper_idmgr_funcs;

    return (void*)ctrl;
}

static void idkey_wrapper_idmgr_reset(IDKEY_HDL hCtrl, PF_IDKEY_FREE_FUNC free_func, void *ud)
{
    IDKEY_WRAPPER_IDMGR_S *ctrl = hCtrl;
    IDKEY_Reset(ctrl->impl, free_func, ud);
    IDMGR_Reset(ctrl->idmgr);
}

static void idkey_wrapper_idmgr_destroy(IDKEY_HDL hCtrl, PF_IDKEY_FREE_FUNC free_func, void *ud)
{
    IDKEY_WRAPPER_IDMGR_S *ctrl = hCtrl;
    if (ctrl->impl) {
        IDKEY_Destroy(ctrl->impl, free_func, ud);
    }
    if (ctrl->idmgr) {
        IDMGR_Destroy(ctrl->idmgr);
    }
    MEM_Free(ctrl);
}

static void idkey_wrapper_idmgr_setcapacity(IDKEY_HDL hCtrl, INT64 capacity)
{
    IDKEY_WRAPPER_IDMGR_S *ctrl = hCtrl;
    IDKEY_SetCapacity(ctrl->impl, capacity);
}

/* 做ID的重复检查 */
static int idkey_wrapper_idmgr_set(IDKEY_HDL hCtrl, INT64 id, void *key, int key_len, void *data, UINT flag)
{
    IDKEY_WRAPPER_IDMGR_S *ctrl = hCtrl;
    int ret;

    if (IDMGR_IsExist(ctrl->idmgr, id)) {
        RETURN(BS_ALREADY_EXIST);
    }

    if (0 != (ret = IDMGR_Set(ctrl->idmgr, id))){
        return ret;
    }

    if (0 != (ret = IDKEY_Set(ctrl->impl, id, key, key_len, data, flag))) {
        IDMGR_Free(ctrl->idmgr, id);
        return ret;
    }

    return 0;
}

static INT64 idkey_wrapper_idmgr_add(IDKEY_HDL hCtrl, void *key, int key_len, void *data, UINT flag)
{
    IDKEY_WRAPPER_IDMGR_S *ctrl = hCtrl;
    INT64 id = IDMGR_Alloc(ctrl->idmgr);

    if (id < 0) {
        return id;
    }

    int ret = IDKEY_Set(ctrl->impl, id, key, key_len, data, flag);
    if (ret != 0) {
        IDMGR_Free(ctrl->idmgr, id);
        return -1;
    }

    return id;
}

/* 删除并返回pData */
static void * idkey_wrapper_idmgr_del_by_key(IDKEY_HDL hCtrl, void *key, int key_len)
{
    IDKEY_WRAPPER_IDMGR_S *ctrl = hCtrl;

    INT64 id = IDKEY_GetIDByKey(ctrl->impl, key, key_len);
    if (id < 0) {
        return NULL;
    }

    IDMGR_Free(ctrl->idmgr, id);

    return IDKEY_DelByID(ctrl->impl, id);
}

/* 删除并返回pData */
static void * idkey_wrapper_idmgr_del_by_id(IDKEY_HDL hCtrl, INT64 id)
{
    IDKEY_WRAPPER_IDMGR_S *ctrl = hCtrl;
    IDMGR_Free(ctrl->idmgr, id);
    return IDKEY_DelByID(ctrl->impl, id);
}

static INT64 idkey_wrapper_idmgr_get_id_by_key(IDKEY_HDL hCtrl, void *key, int key_len)
{
    IDKEY_WRAPPER_IDMGR_S *ctrl = hCtrl;
    return IDKEY_GetIDByKey(ctrl->impl, key, key_len);
}

static IDKEY_KL_S * idkey_wrapper_idmgr_get_key_by_id(IDKEY_HDL hCtrl, INT64 id)
{
    IDKEY_WRAPPER_IDMGR_S *ctrl = hCtrl;
    return IDKEY_GetKeyByID(ctrl->impl, id);
}

static void * idkey_wrapper_idmgr_get_data_by_id(IDKEY_HDL hCtrl, INT64 id)
{
    IDKEY_WRAPPER_IDMGR_S *ctrl = hCtrl;
    return IDKEY_GetDataByID(ctrl->impl, id);
}

static void * idkey_wrapper_idmgr_get_data_by_key(IDKEY_HDL hCtrl, void *key, int key_len)
{
    IDKEY_WRAPPER_IDMGR_S *ctrl = hCtrl;
    return IDKEY_GetDataByKey(ctrl->impl, key, key_len);
}

static void idkey_wrapper_idmgr_walk(IDKEY_HDL hCtrl, PF_IDKEY_WALK_FUNC walk_func, void *ud)
{
    IDKEY_WRAPPER_IDMGR_S *ctrl = hCtrl;
    IDKEY_Walk(ctrl->impl, walk_func, ud);
}
