/*****************************************************************************
*   Created by LiXingang, Copyright LiXingang
*   Description: 
*
*****************************************************************************/
#ifndef _MEM_BLOCK_H
#define _MEM_BLOCK_H
#include "utl/free_list.h"
#ifdef __cplusplus
extern "C"
{
#endif

typedef struct {
    void *block;
    UINT free_count; 
    FREE_LIST_S free_list;
}MEM_BLOCK_S;

typedef struct {
    UINT entry_size; 
    UINT entry_count_per_block; 
    UINT max_block; 
    MEM_BLOCK_S blks[0];
}MEM_BLOCK_CTX_S;

int MemBlock_Init(INOUT MEM_BLOCK_CTX_S *ctx);
void MemBlock_Fini(INOUT MEM_BLOCK_CTX_S *ctx);
MEM_BLOCK_CTX_S * MemBlock_Create(UINT entry_size, UINT entry_count_per_block, UINT max_block);
void MemBlock_Destroy(MEM_BLOCK_CTX_S *ctx);
void * MemBlock_Alloc(MEM_BLOCK_CTX_S *ctx);
void MemBlock_Free(MEM_BLOCK_CTX_S *ctx, void *mem);

#ifdef __cplusplus
}
#endif
#endif 
