/*================================================================
*   Created by LiXingang, Copyright LiXingang
*   Description: 
*
================================================================*/
#ifndef _UMAP_UTL_H
#define _UMAP_UTL_H
#include "utl/ufd_utl.h"
#include "utl/ulc_def.h"
#ifdef __cplusplus
extern "C"
{
#endif

#define UMAP_UPDATE_ANY		    0 /* create new element or update existing */
#define UMAP_UPDATE_NOEXIST	    1 /* create new element if it didn't exist */
#define UMAP_UPDATE_EXIST	    2 /* update existing element */

#define UMAP_ELF_MAP_MIN_SIZE 20 /* 最小的map size, 包含 type, size_key, size_value, max_elem, flag */

/* 和bpf的map声明结构保持一致 */
typedef struct {
	unsigned int type;
	unsigned int size_key;
	unsigned int size_value;
	unsigned int max_elem;
	unsigned int flags;
}UMAP_ELF_MAP_S;

typedef int (*PF_UMAP_OPEN)(UFD_S *ctx, UMAP_ELF_MAP_S *elfmap);
typedef void* (*PF_UMAP_LOOKUP_ELEM)(void *map, const void *key);
typedef long (*PF_UMAP_DELETE_ELEM)(void *map, const void *key);
typedef long (*PF_UMAP_UPDATE_ELEM)(void *map, const void *key, const void *value, U32 flag);
typedef void* (*PF_UMAP_GETNEXT_KEY)(void *map, void *key, OUT void **next_key);
typedef long (*PF_UMAP_DIRECT_VALUE)(void *map, OUT U64 *addr, U32 off);

typedef struct {
    PF_UMAP_OPEN open_func;
    PF_UMAP_LOOKUP_ELEM lookup_elem_func;
    PF_UMAP_DELETE_ELEM delete_elem_func;
    PF_UMAP_UPDATE_ELEM update_elem_func;
    PF_UMAP_GETNEXT_KEY get_next_key;
    PF_UMAP_DIRECT_VALUE direct_value_func;
}UMAP_FUNC_TBL_S;

#define UMAP_NAME_SIZE 128

typedef struct {
    int fd;
    char map_name[UMAP_NAME_SIZE];
	UINT type;
	UINT size_key;
	UINT size_value;
	UINT max_elem;
	UINT flags;
}UMAP_HEADER_S;

char * UMAP_TypeName(unsigned int type);
int UMAP_RegType(UINT type, UMAP_FUNC_TBL_S *ops);
int UMAP_Open(UFD_S *ctx, UMAP_ELF_MAP_S *elfmap, char *map_name);
void UMAP_Close(UFD_S *ctx, int fd);
int UMAP_GetByName(UFD_S *ctx, char *map_name);
UMAP_HEADER_S * UMAP_GetByFd(UFD_S *ctx, int fd);
UMAP_HEADER_S * UMAP_RefByFd(UFD_S *ctx, int fd);

void * UMAP_LookupElem(UMAP_HEADER_S *map, const void *key);
long UMAP_DeleteElem(UMAP_HEADER_S *map, const void *key);
long UMAP_UpdateElem(UMAP_HEADER_S *map, const void *key, const void *value, U32 flag);
void * UMAP_LookupElemByFd(UFD_S *ctx, int fd, void *key);
long UMAP_DeleteElemByFd(UFD_S *ctx, int fd, void *key);
long UMAP_UpdateElemByFd(UFD_S *ctx, int fd, void *key, void *value, UINT flag);
void * UMAP_GetNextKey(UMAP_HEADER_S *map, void *curr_key, OUT void **next_key);
int UMAP_DirectValue(UMAP_HEADER_S *map, OUT UINT64 *addr, UINT off);

void UMAP_ShowMap(UFD_S *ctx, PF_PRINT_FUNC print_func);
void UMAP_DumpMap(UFD_S *ctx, int map_fd, PF_PRINT_FUNC print_func);

#ifdef __cplusplus
}
#endif
#endif //UMAP_UTL_H_
