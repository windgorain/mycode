/*================================================================
*   Created by LiXingang, Copyright LiXingang
*   Description: ufd map
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

/* map_def_offset: 当前map_def在maps字段中的偏移 */
int UMAP_Open(UMAP_ELF_MAP_S *elfmap, int map_def_offset, char *map_name)
{
    UINT type = elfmap->type;
    int fd;
    UMAP_HEADER_S *hdr;

    if ((type <= 0) || (type >= BPF_MAP_TYPE_MAX)) {
		return -EINVAL;
    }

    fd = g_umap_func_tbl[type]->open_func(elfmap);
    if (fd < 0) {
        return fd;
    }

    hdr = UFD_GetFileData(fd);

    hdr->fd = fd;
    hdr->map_def_offset = map_def_offset;
    strlcpy(hdr->map_name, map_name, sizeof(hdr->map_name));

    hdr->type = elfmap->type;
    hdr->size_key = elfmap->size_key;
    hdr->size_value = elfmap->size_value;
    hdr->max_elem = elfmap->max_elem;
    hdr->flags = elfmap->flags;

    return fd;
}

void UMAP_Close(int fd)
{
    UFD_Close(fd);
}

UMAP_HEADER_S * UMAP_GetByFd(int fd)
{
    if (UFD_FD_TYPE_MAP != UFD_GetFileType(fd)) {
        return NULL;
    }

    return UFD_GetFileData(fd);
}

/* 获取map并增加引用计数 */
UMAP_HEADER_S * UMAP_RefByFd(int fd)
{
    if (UFD_FD_TYPE_MAP != UFD_GetFileType(fd)) {
        return NULL;
    }

    return UFD_RefFileData(fd);
}

int UMAP_GetByName(char *map_name)
{
    UMAP_HEADER_S *hdr;
    int fd = -1;

    while ((fd = UFD_GetNextOfType(UFD_FD_TYPE_MAP, fd)) >= 0) {
        hdr = UFD_GetFileData(fd);
        if (! hdr) {
            continue;
        }
        if (strcmp(map_name, hdr->map_name) == 0) {
            return fd;
        }
    }

    return -1;
}

void * UMAP_LookupElem(void *map, void *key)
{
    UMAP_HEADER_S *hdr = map;

    if (! hdr) {
        return NULL;
    }

    return g_umap_func_tbl[hdr->type]->lookup_elem_func(map, key);
}

long UMAP_DeleteElem(void *map, void *key)
{
    UMAP_HEADER_S *hdr = map;

    if (! hdr) {
		return -EINVAL;
    }

    return g_umap_func_tbl[hdr->type]->delete_elem_func(map, key);
}

long UMAP_UpdateElem(void *map, void *key, void *value, U32 flag)
{
    UMAP_HEADER_S *hdr = map;

    if (! hdr) {
		return -EINVAL;
    }

    return g_umap_func_tbl[hdr->type]->update_elem_func(map, key, value, flag);
}

void * UMAP_LookupElemByFd(int fd, void *key)
{
    UMAP_HEADER_S *hdr;

    hdr = UMAP_GetByFd(fd);
    if (! hdr) {
        return NULL;
    }

    return UMAP_LookupElem(hdr, key);
}

long UMAP_DeleteElemByFd(int fd, void *key)
{
    UMAP_HEADER_S *hdr;

    hdr = UMAP_GetByFd(fd);
    if (! hdr) {
        RETURN(BS_ERR);
    }

    return UMAP_DeleteElem(hdr, key);
}

long UMAP_UpdataElemByFd(int fd, void *key, void *value, UINT flag)
{
    UMAP_HEADER_S *hdr;

    hdr = UMAP_GetByFd(fd);
    if (! hdr) {
        RETURN(BS_ERR);
    }

    return UMAP_UpdateElem(hdr, key, value, flag);
}

/* 获取数组map的数组地址 */
int UMAP_DirectValue(void *map, OUT U64 *addr, UINT off)
{
    UMAP_HEADER_S *hdr = map;

    if (! hdr) {
		return -EINVAL;
    }

    if (! g_umap_func_tbl[hdr->type]->direct_value_func) {
		return -EINVAL;
    }

    return g_umap_func_tbl[hdr->type]->direct_value_func(map, addr, off);
}

/* 注意: 调用者的*next_key和接受返回值不要用同一个变量, 因为在array map中*next_key有存储id的作用 */
void * UMAP_GetNextKey(void *map, void *curr_key, OUT void **next_key)
{
    UMAP_HEADER_S *hdr = map;
    return g_umap_func_tbl[hdr->type]->get_next_key(hdr, curr_key, next_key);
}

void UMAP_ShowMap(PF_PRINT_FUNC print_func)
{
    int fd = -1;
    int state;
    UMAP_HEADER_S *hdr;

    state = RcuEngine_Lock();

    while ((fd = UFD_GetNextOfType(UFD_FD_TYPE_MAP, fd)) >= 0) {
        hdr = UFD_GetFileData(fd);
        if (! hdr) {
            continue;
        }
        print_func("fd:%d,type:%s,flags:0x%x,key:%u,value:%u,max:%u,name:%s \r\n",
                fd, UMAP_TypeName(hdr->type), hdr->flags, hdr->size_key,
                hdr->size_value, hdr->max_elem, hdr->map_name);
    }

    RcuEngine_UnLock(state);
}

void UMAP_DumpMap(int map_fd, PF_PRINT_FUNC print_func)
{
    void *key = NULL;
    void *next_key;
    void *data;

    UMAP_HEADER_S *hdr = UMAP_GetByFd(map_fd);
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

