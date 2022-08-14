/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/idtbl_utl.h"

typedef struct {
    IDTBL_S id_tbl;
    UINT max_id;
    void *data[0];
}IDTBL_ARRAY_S;

static int idtbl_array_add(void *id_tbl, UINT64 id, void *data);
static void * idtbl_array_get(void *id_tbl, UINT64 id);
static void * idtbl_array_del(void *id_tbl, UINT64 id);
static void idtbl_array_reset(void *id_tbl, PF_IDTBL_FREE free_func, void *ud);
static void idtbl_array_destroy(void *id_tbl, PF_IDTBL_FREE free_func, void *ud);

static IDTBL_FUNC_S g_idtbl_array_funcs = {
    idtbl_array_add,
    idtbl_array_get,
    idtbl_array_del,
    idtbl_array_reset,
    idtbl_array_destroy
};

IDTBL_S * IDTBL_ArrayCreate(UINT max_id)
{
    IDTBL_ARRAY_S *ctrl;
    UINT size = sizeof(IDTBL_ARRAY_S) + sizeof(void*) * max_id;

    ctrl = MEM_ZMalloc(size);
    if (! ctrl) {
        return NULL;
    }

    ctrl->max_id = max_id;
    ctrl->id_tbl.funcs = &g_idtbl_array_funcs;

    return (void*)ctrl;
}

static int idtbl_array_add(void *id_tbl, UINT64 id, void *data)
{
    IDTBL_ARRAY_S *ctrl = id_tbl;

    if (id >= ctrl->max_id) {
        BS_WARNNING(("Out of range"));
        RETURN(BS_OUT_OF_RANGE);
    }

    ctrl->data[id] = data;

    return 0;
}

static void * idtbl_array_get(void *id_tbl, UINT64 id)
{
    IDTBL_ARRAY_S *ctrl = id_tbl;

    if (id >= ctrl->max_id) {
        BS_WARNNING(("Out of range"));
        return NULL;
    }

    return ctrl->data[id];
}

static void * idtbl_array_del(void *id_tbl, UINT64 id)
{
    IDTBL_ARRAY_S *ctrl = id_tbl;
    void *data;

    if (id >= ctrl->max_id) {
        BS_WARNNING(("Out of range"));
        return NULL;
    }

    data = ctrl->data[id];
    ctrl->data[id] = NULL;

    return data;
}

static void idtbl_array_delete_all(IDTBL_ARRAY_S *ctrl, PF_IDTBL_FREE free_func, void *ud)
{
    UINT i;

    if (! free_func) {
        return;
    }

    for (i=0; i<ctrl->max_id; i++) {
        if (ctrl->data[i]) {
            free_func(ctrl->data[i], ud);
        }
    }
}

static void idtbl_array_reset(void *id_tbl, PF_IDTBL_FREE free_func, void *ud)
{
    IDTBL_ARRAY_S *ctrl = id_tbl;
    idtbl_array_delete_all(ctrl, free_func, ud);
    memset(ctrl->data, 0, sizeof(void*)*ctrl->max_id);
}

static void idtbl_array_destroy(void *id_tbl, PF_IDTBL_FREE free_func, void *ud)
{
    IDTBL_ARRAY_S *ctrl = id_tbl;
    idtbl_array_delete_all(ctrl, free_func, ud);
    MEM_Free(ctrl);
}

