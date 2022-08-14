/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/bit_opt.h"
#include "utl/array_bit.h"
#include "utl/idmgr_utl.h"

typedef struct {
    IDMGR_S tbl;
    INT64 bit_size;
    UINT bits[0];
}IDMGR_BITMAP_S;


static void idmgr_bitmap_destroy(void *tbl);
static INT64 idmgr_bitmap_alloc(void *tbl);
static int idmgr_bitmap_set(void *tbl, INT64 id);
static void idmgr_bitmap_free(void *tbl, INT64 id);
static BOOL_T idmgr_bitmap_is_exist(void *tbl, INT64 id);
static INT64 idmgr_bitmap_getnext(void *tbl, INT64 curr);
static void idmgr_bitmap_reset(void *tbl);

static IDMGR_FUNC_S g_idmgr_bitmap_funcs = {
    idmgr_bitmap_destroy,
    idmgr_bitmap_alloc,
    idmgr_bitmap_set,
    idmgr_bitmap_free,
    idmgr_bitmap_is_exist,
    idmgr_bitmap_getnext,
    idmgr_bitmap_reset
};

IDMGR_S * IDMGR_BitmapCreate(INT64 bit_size)
{
    int num = NUM_UP_ALIGN(bit_size, 32)/32;

    IDMGR_BITMAP_S *ctrl = MEM_ZMalloc(sizeof(IDMGR_BITMAP_S) + sizeof(UINT)*num);
    if (! ctrl) {
        return NULL;
    }

    ctrl->bit_size = bit_size;
    ctrl->tbl.funcs = &g_idmgr_bitmap_funcs;

    return (void*)ctrl;
}

static void idmgr_bitmap_destroy(void *tbl)
{
    MEM_Free(tbl);
}

static INT64 idmgr_bitmap_alloc(void *tbl)
{
    IDMGR_BITMAP_S *ctrl = tbl;
    INT64 id = ArrayBit_GetFree(ctrl->bits, ctrl->bit_size);
    if (id >= 0) {
        ArrayBit_Set(ctrl->bits, id);
    }
    return id;
}

static int idmgr_bitmap_set(void *tbl, INT64 id)
{
    IDMGR_BITMAP_S *ctrl = tbl;
    if (id >= ctrl->bit_size) {
        RETURN(BS_OUT_OF_RANGE);
    }
    ArrayBit_Set(ctrl->bits, id);
    return 0;
}

static void idmgr_bitmap_free(void *tbl, INT64 id)
{
    IDMGR_BITMAP_S *ctrl = tbl;
    ArrayBit_Clr(ctrl->bits, id);
}

static BOOL_T idmgr_bitmap_is_exist(void *tbl, INT64 id)
{
    IDMGR_BITMAP_S *ctrl = tbl;
    if (ArrayBit_Test(ctrl->bits, id)) {
        return TRUE;
    }
    return FALSE;
}

static INT64 idmgr_bitmap_getnext(void *tbl, INT64 curr)
{
    IDMGR_BITMAP_S *ctrl = tbl;

    if (curr < 0) {
        return ArrayBit_GetBusy(ctrl->bits, ctrl->bit_size);
    }

    return ArrayBit_GetBusyAfter(ctrl->bits, ctrl->bit_size, curr);
}

static void idmgr_bitmap_reset(void *tbl)
{
    IDMGR_BITMAP_S *ctrl = tbl;
    UINT64 uint_num = NUM_UP_ALIGN(ctrl->bit_size, 32)/32;
    memset(ctrl->bits, 0, uint_num * sizeof(UINT));
}

