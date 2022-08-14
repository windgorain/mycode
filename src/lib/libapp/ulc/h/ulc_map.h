/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _ULC_MAP_H
#define _ULC_MAP_H
#ifdef __cplusplus
extern "C"
{
#endif

#ifndef BPF_NOEXIST
#define BPF_ANY		0 /* create new element or update existing */
#define BPF_NOEXIST	1 /* create new element if it didn't exist */
#define BPF_EXIST	2 /* update existing element */
#endif

#define ULC_MAP_NAME_SIZE 128

typedef struct {
	unsigned int type;
	unsigned int size_key;
	unsigned int size_value;
	unsigned int max_elem;
	unsigned int flags;
}ULC_ELF_MAP_S;

typedef int (*PF_ULC_MAP_OPEN)(ULC_ELF_MAP_S *elfmap);
typedef void* (*PF_ULC_MAP_LOOKUP_ELEM)(void *map, void *key);
typedef long (*PF_ULC_MAP_DELETE_ELEM)(void *map, void *key);
typedef long (*PF_ULC_MAP_UPDATE_ELEM)(void *map, void *key, void *value, U32 flag);
typedef long (*PF_ULC_MAP_DIRECT_VALUE)(void *map, OUT U64 *addr, U32 off);

typedef struct {
    PF_ULC_MAP_OPEN open_func;
    PF_ULC_MAP_LOOKUP_ELEM lookup_elem_func;
    PF_ULC_MAP_DELETE_ELEM delete_elem_func;
    PF_ULC_MAP_UPDATE_ELEM update_elem_func;
    PF_ULC_MAP_DIRECT_VALUE direct_value_func;
}ULC_MAP_FUNC_TBL_S;

typedef struct {
    int fd;
    int map_offset; /* 这个map在maps section中的偏移 */
    char map_name[ULC_MAP_NAME_SIZE];

	U32 type;
	U32 size_key;
	U32 size_value;
	U32 max_elem;
	U32 flags;
}ULC_MAP_HEADER_S;

enum {
	BPF_MAP_TYPE_UNSPEC,
	BPF_MAP_TYPE_HASH,
	BPF_MAP_TYPE_ARRAY,

    BPF_MAP_TYPE_MAX
};

char * ULC_MAP_TypeName(unsigned int type);
int ULC_MAP_RegType(UINT type, ULC_MAP_FUNC_TBL_S *ops);
int ULC_MAP_Open(ULC_ELF_MAP_S *elfmap, int map_offset, char *map_name);
void ULC_MAP_Close(int fd);
int ULC_MAP_GetByName(char *map_name);
ULC_MAP_HEADER_S * ULC_MAP_GetByFd(int fd);
ULC_MAP_HEADER_S * ULC_MAP_RefByFd(int fd);

void * ULC_MAP_LookupElem(void *map, void *key);
long ULC_MAP_DeleteElem(void *map, void *key);
long ULC_MAP_UpdataElem(void *map, void *key, void *value, U32 flag);
int ULC_MAP_DirectValue(void *map, OUT UINT64 *addr, UINT off);

void ULC_MAP_ShowMap();

int ULC_MapArray_Init();
int ULC_MapHash_Init();

#ifdef __cplusplus
}
#endif
#endif //ULC_MAP_H_
