/*================================================================
*   Created by LiXingang, Copyright LiXingang
*   Date: 2017.1.2
*   Description: user bpf map
*
================================================================*/
#include "bs.h"
#include "utl/ufd_utl.h"
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

int UMAP_Open(UFD_S *ctx, UMAP_ELF_MAP_S *elfmap, char *map_name)
{
    UINT type = elfmap->type;
    int fd;
    UMAP_HEADER_S *hdr;

    if (! map_name) {
        map_name = "";
    }

    if ((type <= 0) || (type >= BPF_MAP_TYPE_MAX)) {
		return -EINVAL;
    }

    fd = g_umap_func_tbl[type]->open_func(ctx, elfmap);
    if (fd < 0) {
        return fd;
    }

    hdr = UFD_GetFileData(ctx, fd);

    hdr->fd = fd;
    strlcpy(hdr->map_name, map_name, sizeof(hdr->map_name));

    hdr->type = elfmap->type;
    hdr->size_key = elfmap->size_key;
    hdr->size_value = elfmap->size_value;
    hdr->max_elem = elfmap->max_elem;
    hdr->flags = elfmap->flags;

    return fd;
}

void UMAP_Close(UFD_S *ctx, int fd)
{
    UFD_Close(ctx, fd);
}

UMAP_HEADER_S * UMAP_GetByFd(UFD_S *ctx, int fd)
{
    if (UFD_FD_TYPE_MAP != UFD_GetFileType(ctx, fd)) {
        return NULL;
    }

    return UFD_GetFileData(ctx, fd);
}

/* 获取map并增加引用计数 */
UMAP_HEADER_S * UMAP_RefByFd(UFD_S *ctx, int fd)
{
    if (UFD_FD_TYPE_MAP != UFD_GetFileType(ctx, fd)) {
        return NULL;
    }

    return UFD_RefFileData(ctx, fd);
}

int UMAP_GetByName(UFD_S *ctx, char *map_name)
{
    UMAP_HEADER_S *hdr;
    int fd = -1;

    while ((fd = UFD_GetNextOfType(ctx, UFD_FD_TYPE_MAP, fd)) >= 0) {
        hdr = UFD_GetFileData(ctx, fd);
        if (! hdr) {
            continue;
        }
        if (strcmp(map_name, hdr->map_name) == 0) {
            return fd;
        }
    }

    return -1;
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

void * UMAP_LookupElemByFd(UFD_S *ctx, int fd, void *key)
{
    UMAP_HEADER_S *hdr;

    hdr = UMAP_GetByFd(ctx, fd);
    if (! hdr) {
        return NULL;
    }

    return UMAP_LookupElem(hdr, key);
}

long UMAP_DeleteElemByFd(UFD_S *ctx, int fd, void *key)
{
    UMAP_HEADER_S *hdr;

    hdr = UMAP_GetByFd(ctx, fd);
    if (! hdr) {
        RETURN(BS_ERR);
    }

    return UMAP_DeleteElem(hdr, key);
}

long UMAP_UpdateElemByFd(UFD_S *ctx, int fd, void *key, void *value, UINT flag)
{
    UMAP_HEADER_S *hdr;

    hdr = UMAP_GetByFd(ctx, fd);
    if (! hdr) {
        RETURN(BS_ERR);
    }

    return UMAP_UpdateElem(hdr, key, value, flag);
}

/* 获取数组map的数组地址 */
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

/* 注意: 调用者的*next_key和接受返回值不要用同一个变量, 因为在array map中*next_key有存储id的作用 */
void * UMAP_GetNextKey(UMAP_HEADER_S *map, void *curr_key, OUT void **next_key)
{
    return g_umap_func_tbl[map->type]->get_next_key(map, curr_key, next_key);
}

void UMAP_ShowMap(UFD_S *ctx, PF_PRINT_FUNC print_func)
{
    int fd = -1;
    int state;
    UMAP_HEADER_S *hdr;

    state = RcuEngine_Lock();

    while ((fd = UFD_GetNextOfType(ctx, UFD_FD_TYPE_MAP, fd)) >= 0) {
        hdr = UFD_GetFileData(ctx, fd);
        if (! hdr) {
            continue;
        }
        print_func("fd:%d, type:%s, flags:0x%x, key:%u, value:%u, max:%u, name:%s \r\n",
                fd, UMAP_TypeName(hdr->type), hdr->flags, hdr->size_key,
                hdr->size_value, hdr->max_elem, hdr->map_name);
    }

    RcuEngine_UnLock(state);
}

void UMAP_DumpMap(UFD_S *ctx, int map_fd, PF_PRINT_FUNC print_func)
{
    void *key = NULL;
    void *next_key;
    void *data;

    UMAP_HEADER_S *hdr = UMAP_GetByFd(ctx, map_fd);
    if (! hdr) {
        return;
    }

    while ((key = UMAP_GetNextKey(hdr, key, &next_key))) {
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

