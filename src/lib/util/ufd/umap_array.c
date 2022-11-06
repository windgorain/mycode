/*================================================================
*   Created by LiXingang, Copyright LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/ufd_utl.h"
#include "utl/umap_utl.h"

typedef struct {
    UMAP_HEADER_S hdr; /* 必须为第一个成员 */
    UCHAR data[0];
}UMAP_ARRAY_S;

static void _umap_array_destroy_map(void *f)
{
    UMAP_ARRAY_S *ctrl = f;
    RcuEngine_Free(ctrl);
}

static int _umap_array_open(UMAP_ELF_MAP_S *elfmap)
{
    int fd;
    int len;

    if ((! elfmap) || (elfmap->max_elem == 0) || (elfmap->size_key != sizeof(int))) {
		return -EINVAL;
    }

    len = sizeof(UMAP_ARRAY_S) + (elfmap->size_value * elfmap->max_elem);

    UMAP_ARRAY_S *ctrl = RcuEngine_ZMalloc(len);
    if (! ctrl) {
        return -ENOMEM;
    }

    fd = UFD_Open(UFD_FD_TYPE_MAP, ctrl, _umap_array_destroy_map);
    if (fd < 0) {
        RcuEngine_Free(ctrl);
        return fd;
    }

    return fd;
}

static void * _umap_array_lookup_elem(void *map, void *key)
{
    UMAP_ARRAY_S *ctrl = map;
    UCHAR *data;
    UINT index;

    if ((!map) || (!key)) {
        return NULL;
    }

	index = *(UINT*)key;

    if (index >= ctrl->hdr.max_elem) {
        return NULL;
    }

    data = ctrl->data;

    data += (index * ctrl->hdr.size_value);

    return data;
}

static long _umap_array_delete_elem(void *map, void *key)
{
	return -EINVAL;
}

static long _umap_array_update_elem(void *map, void *key, void *value, U32 flag)
{
    UMAP_ARRAY_S *ctrl = map;
    void *old;

    if ((!map) || (!key) || (!value)) {
		return -EINVAL;
    }

    if (flag == UMAP_UPDATE_NOEXIST) {
		return -EEXIST;
    }

	UINT index = *(UINT *)key;

	if (index >= ctrl->hdr.max_elem) {
		return -E2BIG;
    }

    old = _umap_array_lookup_elem(map, key);

    memcpy(old, value, ctrl->hdr.size_value);

    return 0;
}

static long _umap_array_direct_value(void *map, OUT U64 *value, U32 off)
{
    UMAP_ARRAY_S *ctrl = map;

    if ((!map) || (!value)) {
		return -EINVAL;
    }

	if (off >= ctrl->hdr.max_elem * ctrl->hdr.size_value) {
		return -E2BIG;
    }

    *value = (long)ctrl->data;

    return 0;
}

/* key: NULL表示Get第一个 */
static void * _umap_array_getnext_key(void *map, void *key, OUT void **next_key)
{
    UMAP_ARRAY_S *ctrl = map;
    int n = 0;

    if (! next_key) {
        return NULL;
    }

    if (key) {
        n = *(int*)key;
        n ++;
    }

    if (n >= ctrl->hdr.max_elem) {
        return NULL;
    }

    *(int*)next_key = n;

    return next_key;
}

UMAP_FUNC_TBL_S g_umap_array_ops = {
    .open_func = _umap_array_open,
    .lookup_elem_func = _umap_array_lookup_elem,
    .delete_elem_func = _umap_array_delete_elem,
    .update_elem_func = _umap_array_update_elem,
    .get_next_key = _umap_array_getnext_key,
    .direct_value_func = _umap_array_direct_value,
};


