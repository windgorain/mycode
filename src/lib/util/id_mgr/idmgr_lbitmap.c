/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/bit_opt.h"
#include "utl/large_bitmap.h"
#include "utl/array_bit.h"
#include "utl/idmgr_utl.h"

typedef struct {
    IDMGR_S tbl;
    LBITMAP_HANDLE lbitmap;
}IDMGR_LBITMAP_S;


static void idmgr_lbitmap_destroy(void *tbl);
static INT64 idmgr_lbitmap_alloc(void *tbl);
static int idmgr_lbitmap_set(void *tbl, INT64 id);
static void idmgr_lbitmap_free(void *tbl, INT64 id);
static BOOL_T idmgr_lbitmap_is_exist(void *tbl, INT64 id);
static INT64 idmgr_lbitmap_getnext(void *tbl, INT64 curr);
static void idmgr_lbitmap_reset(void *tbl);

static IDMGR_FUNC_S g_idmgr_lbitmap_funcs = {
    idmgr_lbitmap_destroy,
    idmgr_lbitmap_alloc,
    idmgr_lbitmap_set,
    idmgr_lbitmap_free,
    idmgr_lbitmap_is_exist,
    idmgr_lbitmap_getnext,
    idmgr_lbitmap_reset
};

IDMGR_S * IDMGR_LBitmapCreate()
{
    IDMGR_LBITMAP_S *ctrl = MEM_ZMalloc(sizeof(IDMGR_LBITMAP_S));
    if (! ctrl) {
        return NULL;
    }

    ctrl->lbitmap = LBitMap_Create(NULL);
    if (! ctrl->lbitmap) {
        MEM_Free(ctrl);
        return NULL;
    }

    ctrl->tbl.funcs = &g_idmgr_lbitmap_funcs;

    return (void*)ctrl;
}

static void idmgr_lbitmap_destroy(void *tbl)
{
    IDMGR_LBITMAP_S *ctrl = tbl;
    if (ctrl->lbitmap) {
        LBitMap_Destory(ctrl->lbitmap);
    }
    MEM_Free(ctrl);
}

static INT64 idmgr_lbitmap_alloc(void *tbl)
{
    IDMGR_LBITMAP_S *ctrl = tbl;
    UINT id;

    if (0 != LBitMap_AllocByRange(ctrl->lbitmap, 0, 0xffffffff, &id)) {
        return -1;
    }

    return id;
}

static int idmgr_lbitmap_set(void *tbl, INT64 id)
{
    IDMGR_LBITMAP_S *ctrl = tbl;

    LBitMap_SetBit(ctrl->lbitmap, id);

    return 0;
}

static void idmgr_lbitmap_free(void *tbl, INT64 id)
{
    IDMGR_LBITMAP_S *ctrl = tbl;
    LBitMap_ClrBit(ctrl->lbitmap, id);
}

static BOOL_T idmgr_lbitmap_is_exist(void *tbl, INT64 id)
{
    IDMGR_LBITMAP_S *ctrl = tbl;
    if (LBitMap_IsBitSetted(ctrl->lbitmap, id)) {
        return TRUE;
    }
    return FALSE;
}

static INT64 idmgr_lbitmap_getnext(void *tbl, INT64 curr)
{
    IDMGR_LBITMAP_S *ctrl = tbl;
    UINT id;

    if (curr < 0) {
        if (0 == LBitMap_GetFirstBusyBit(ctrl->lbitmap, &id)) {
            return id;
        } else {
            return -1;
        }
    }

    if (0 != LBitMap_GetNextBusyBit(ctrl->lbitmap, curr, &id)) {
        return -1;
    }

    return id;
}

static void idmgr_lbitmap_reset(void *tbl)
{
    IDMGR_LBITMAP_S *ctrl = tbl;
    LBitMap_Reset(ctrl->lbitmap);
}

