/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/exec_utl.h"
#include "utl/data2hex_utl.h"
#include "../h/ulc_fd.h"
#include "../h/ulc_map.h"

static ULC_MAP_FUNC_TBL_S *g_ulc_map_func_tbl[BPF_MAP_TYPE_MAX];
static char * g_ulc_map_type_name[] = {
    "unspec", "hash", "array"
};

char * ULC_MAP_TypeName(unsigned int type)
{
    if (type >= ARRAY_SIZE(g_ulc_map_type_name)) {
        return "";
    }

    return g_ulc_map_type_name[type];
}

int ULC_MAP_RegType(UINT type, ULC_MAP_FUNC_TBL_S *ops)
{
    if ((type <= 0) || (type >= BPF_MAP_TYPE_MAX)) {
		return -EINVAL;
    }

    g_ulc_map_func_tbl[type] = ops;

    return 0;
}

/* map_offset: 当前map在maps字段中的偏移 */
int ULC_MAP_Open(ULC_ELF_MAP_S *elfmap, int map_offset, char *map_name)
{
    UINT type = elfmap->type;
    int fd;
    ULC_MAP_HEADER_S *hdr;

    if ((type <= 0) || (type >= BPF_MAP_TYPE_MAX)) {
		return -EINVAL;
    }

    fd = g_ulc_map_func_tbl[type]->open_func(elfmap);
    if (fd < 0) {
        return fd;
    }

    hdr = ULC_FD_GetFileData(fd);

    hdr->fd = fd;
    hdr->map_offset = map_offset;
    strlcpy(hdr->map_name, map_name, sizeof(hdr->map_name));

    hdr->type = elfmap->type;
    hdr->size_key = elfmap->size_key;
    hdr->size_value = elfmap->size_value;
    hdr->max_elem = elfmap->max_elem;
    hdr->flags = elfmap->flags;

    return fd;
}

void ULC_MAP_Close(int fd)
{
    ULC_FD_Close(fd);
}

ULC_MAP_HEADER_S * ULC_MAP_GetByFd(int fd)
{
    if (ULC_FD_TYPE_MAP != ULC_FD_GetFileType(fd)) {
        return NULL;
    }

    return ULC_FD_GetFileData(fd);
}

/* 获取map并增加引用计数 */
ULC_MAP_HEADER_S * ULC_MAP_RefByFd(int fd)
{
    if (ULC_FD_TYPE_MAP != ULC_FD_GetFileType(fd)) {
        return NULL;
    }

    return ULC_FD_RefFileData(fd);
}

int ULC_MAP_GetByName(char *map_name)
{
    ULC_MAP_HEADER_S *hdr;
    int fd = -1;

    while ((fd = ULC_FD_GetNextOfType(ULC_FD_TYPE_MAP, fd)) >= 0) {
        hdr = ULC_FD_GetFileData(fd);
        if (! hdr) {
            continue;
        }
        if (strcmp(map_name, hdr->map_name) == 0) {
            return fd;
        }
    }

    return -1;
}

void * ULC_MAP_LookupElem(void *map, void *key)
{
    ULC_MAP_HEADER_S *hdr = map;

    if (! hdr) {
        return NULL;
    }

    return g_ulc_map_func_tbl[hdr->type]->lookup_elem_func(map, key);
}

long ULC_MAP_DeleteElem(void *map, void *key)
{
    ULC_MAP_HEADER_S *hdr = map;

    if (! hdr) {
		return -EINVAL;
    }

    return g_ulc_map_func_tbl[hdr->type]->delete_elem_func(map, key);
}

long ULC_MAP_UpdataElem(void *map, void *key, void *value, U32 flag)
{
    ULC_MAP_HEADER_S *hdr = map;

    if (! hdr) {
		return -EINVAL;
    }

    return g_ulc_map_func_tbl[hdr->type]->update_elem_func(map, key, value, flag);
}

/* 获取数组map的数组地址 */
int ULC_MAP_DirectValue(void *map, OUT U64 *addr, UINT off)
{
    ULC_MAP_HEADER_S *hdr = map;

    if (! hdr) {
		return -EINVAL;
    }

    if (! g_ulc_map_func_tbl[hdr->type]->direct_value_func) {
		return -EINVAL;
    }

    return g_ulc_map_func_tbl[hdr->type]->direct_value_func(map, addr, off);
}

void ULC_MAP_ShowMap()
{
    int fd = -1;
    int state;
    ULC_MAP_HEADER_S *hdr;

    state = RcuEngine_Lock();

    while ((fd = ULC_FD_GetNextOfType(ULC_FD_TYPE_MAP, fd)) >= 0) {
        hdr = ULC_FD_GetFileData(fd);
        if (! hdr) {
            continue;
        }
        EXEC_OutInfo("fd:%d,type:%s,flags:0x%x,key:%u,value:%u,max:%u,name:%s \r\n",
                fd, ULC_MAP_TypeName(hdr->type), hdr->flags, hdr->size_key,
                hdr->size_value, hdr->max_elem, hdr->map_name);
    }

    RcuEngine_UnLock(state);
}

void ULC_MAP_DumpMap(int map_fd)
{
    void *key = NULL;
    void *data;
    ULC_MAP_ITER_S iter = {0};

    ULC_MAP_HEADER_S *hdr = ULC_MAP_GetByFd(map_fd);
    if (! hdr) {
        return;
    }

    while ((key = g_ulc_map_func_tbl[hdr->type]->get_next_key(hdr, key, &iter))) {
        data = ULC_MAP_LookupElem(hdr, key);
        if (! data) {
            continue;
        }

        EXEC_OutString("Key:\r\n");
        EXEC_OutHex(key, hdr->size_key);
        EXEC_OutString("Value:\r\n");
        EXEC_OutHex(data, hdr->size_value);
        EXEC_OutString("\r\n");
    }
}

