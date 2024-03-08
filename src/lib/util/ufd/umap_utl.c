/*================================================================
*   Created by LiXingang, Copyright LiXingang
*   Date: 2017.1.2
*   Description: user bpf map
*
================================================================*/
#include "bs.h"
#include "utl/umap_utl.h"

static char * g_umap_type_name[] = {
    "unspec", "hash", "array"
};

extern UMAP_FUNC_TBL_S g_umap_hash_ops;
extern UMAP_FUNC_TBL_S g_umap_array_ops;

static UMAP_FUNC_TBL_S *g_umap_func_tbl[BPF_MAP_TYPE_MAX] = {
    [BPF_MAP_TYPE_HASH] = &g_umap_hash_ops,
    [BPF_MAP_TYPE_ARRAY] = &g_umap_array_ops,
};

char * UMAP_TypeName(unsigned int type)
{
    if (type >= ARRAY_SIZE(g_umap_type_name)) {
        return "";
    }

    return g_umap_type_name[type];
}

void * UMAP_Open(UMAP_ELF_MAP_S *elfmap, char *map_name)
{
    UINT type = elfmap->type;
    UMAP_HEADER_S *hdr;

    if (! map_name) {
        map_name = "";
    }

    if ((type <= 0) || (type >= BPF_MAP_TYPE_MAX)) {
		return NULL;
    }

    hdr = g_umap_func_tbl[type]->open_func(elfmap);
    if (! hdr) {
        return NULL;
    }

    hdr->ref_count = 1;
    hdr->type = elfmap->type;
    hdr->size_key = elfmap->size_key;
    hdr->size_value = elfmap->size_value;
    hdr->max_elem = elfmap->max_elem;
    hdr->flags = elfmap->flags;
    strlcpy(hdr->map_name, map_name, sizeof(hdr->map_name));

    return hdr;
}

void UMAP_Close(UMAP_HEADER_S *map)
{
    map->ref_count --;

    if (map->ref_count <= 0) {
        g_umap_func_tbl[map->type]->destroy_func(map);
    }
}

void * UMAP_LookupElem(UMAP_HEADER_S *map, const void *key)
{
    if (! map) {
        return NULL;
    }

    return g_umap_func_tbl[map->type]->lookup_elem_func(map, key);
}

long UMAP_DeleteElem(UMAP_HEADER_S *map, const void *key)
{
    if (! map) {
		return -EINVAL;
    }

    return g_umap_func_tbl[map->type]->delete_elem_func(map, key);
}

long UMAP_UpdateElem(UMAP_HEADER_S *map, const void *key, const void *value, U32 flag)
{
    if (! map) {
		return -EINVAL;
    }

    return g_umap_func_tbl[map->type]->update_elem_func(map, key, value, flag);
}


int UMAP_DirectValue(UMAP_HEADER_S *map, OUT U64 *addr, UINT off)
{
    if (! map) {
        RETURN(BS_ERR);
    }

    if (! g_umap_func_tbl[map->type]->direct_value_func) {
        RETURN(BS_ERR);
    }

    return g_umap_func_tbl[map->type]->direct_value_func(map, addr, off);
}


int UMAP_GetNextKey(UMAP_HEADER_S *map, void *curr_key, OUT void **next_key)
{
    return g_umap_func_tbl[map->type]->get_next_key_func(map, curr_key, next_key);
}

#if 0
void UMAP_DumpMap(UFD_S *ctx, int map_fd, PF_PRINT_FUNC print_func)
{
    void *key = NULL;
    void *data;
    int ret;

    UMAP_HEADER_S *hdr = UMAP_GetByFd(ctx, map_fd);
    if (! hdr) {
        return;
    }

    while ((ret = UMAP_GetNextKey(hdr, key, &key)) == 0) {
        data = UMAP_LookupElem(hdr, key);
        if (! data) {
            continue;
        }

        print_func("Key:\r\n");
        MEM_Print(key, hdr->size_key, (void*)print_func);
        print_func("Value:\r\n");
        MEM_Print(data, hdr->size_value, (void*)print_func);
        print_func("\r\n");
    }
}
#endif
