/*****************************************************************************
*   Created by LiXingang, Copyright LiXingang
*   Description: Mem Block: 申请大块内存,并划分成固定大小的entry进行分配
*
*****************************************************************************/
#include "bs.h"
#include "utl/mem_block.h"

/* 为blk申请内存 */
static int _mem_block_init_block(MEM_BLOCK_CTX_S *ctx, int blk)
{
    void * mem = MEM_Malloc(ctx->entry_size * ctx->entry_count_per_block);
    if (! mem) {
        RETURN(BS_NO_MEMORY);
    }

    ctx->blks[blk].block = mem;
    ctx->blks[blk].free_count = ctx->entry_count_per_block;
    FreeList_Puts(&ctx->blks[blk].free_list, mem, ctx->entry_size, ctx->entry_count_per_block);

    return 0;
}

/* 获取可用的block */
static int _mem_block_find_valid_block(MEM_BLOCK_CTX_S *ctx)
{
    int i;
    int ret;

    for (i=0; i<ctx->max_block; i++) {
        if (ctx->blks[i].free_count) {
            return i;
        }
        if (! ctx->blks[i].block) {
            if ((ret = _mem_block_init_block(ctx, i) < 0)) {
                return ret;
            }
            return i;
        }
    }

    return -1;
}

/* 查找最后一个已经申请内存的block */
static int _mem_block_find_last_mem_block(MEM_BLOCK_CTX_S *ctx)
{
    int i;

    for (i=ctx->max_block-1; i>=0; i--) {
        if (ctx->blks[i].block) {
            return i;
        }
    }

    return -1;
}

static int _mem_block_find_block_by_entry(MEM_BLOCK_CTX_S *ctx, char *entry)
{
    int i;
    char *block;
    char *last;

    for (i=0; i<ctx->max_block; i++) {
        block = ctx->blks[i].block;
        if (! block) {
            return -1;
        }
        last = block + (ctx->entry_count_per_block - 1) * ctx->entry_size;
        if (NUM_IN_RANGE(entry, block, last)) {
            return i;
        }
    }

    return -1;
}

static void * _mem_block_alloc_entry(MEM_BLOCK_CTX_S *ctx)
{
    int blk = _mem_block_find_valid_block(ctx);

    if (blk < 0) {
        return NULL;
    }

    ctx->blks[blk].free_count --;
    return FreeList_Get(&ctx->blks[blk].free_list);
}

static void _mem_block_free_entry(MEM_BLOCK_CTX_S *ctx, char *entry)
{
    int blk = _mem_block_find_block_by_entry(ctx, entry);
    if (blk < 0) {
        BS_DBGASSERT(0);
        return;
    }

    FreeList_Put(&ctx->blks[blk].free_list, entry);
    ctx->blks[blk].free_count ++;

    if (ctx->blks[blk].free_count < ctx->entry_count_per_block) {
        return;
    }

    /* 当前block的所有entry都释放了,释放整个block */
    MEM_Free(ctx->blks[blk].block);
    ctx->blks[blk].block = NULL;

    /* 将之后的最后一个mem block拿上来填充空位 */
    int last_blk = _mem_block_find_last_mem_block(ctx);
    if (blk < last_blk) {
        ctx->blks[blk] = ctx->blks[last_blk];
        Mem_Zero(&ctx->blks[last_blk], sizeof(MEM_BLOCK_S));
    }
}

int MemBlock_Init(INOUT MEM_BLOCK_CTX_S *ctx)
{
    int i;

    for (i=0; i<ctx->max_block; i++) {
        FreeList_Init(&ctx->blks[i].free_list);
    }

    return 0;
}

void MemBlock_Fini(INOUT MEM_BLOCK_CTX_S *ctx)
{
    int i;

    for (i=0; i<ctx->max_block; i++) {
        if (ctx->blks[i].block) {
            MEM_Free(ctx->blks[i].block);
        }
        FreeList_Init(&ctx->blks[i].free_list);
    }
}

MEM_BLOCK_CTX_S * MemBlock_Create(UINT entry_size, UINT entry_count_per_block, UINT max_block)
{
    MEM_BLOCK_CTX_S *ctx = MEM_ZMalloc(sizeof(MEM_BLOCK_CTX_S) + max_block * sizeof(MEM_BLOCK_S));

    ctx->entry_size = entry_size;
    ctx->entry_count_per_block = entry_count_per_block;
    ctx->max_block = max_block;

    MemBlock_Init(ctx);

    return ctx;
}

void MemBlock_Destroy(MEM_BLOCK_CTX_S *ctx)
{
    MemBlock_Fini(ctx);
    MEM_Free(ctx);
}

void * MemBlock_Alloc(MEM_BLOCK_CTX_S *ctx)
{
    return _mem_block_alloc_entry(ctx);
}

void MemBlock_Free(MEM_BLOCK_CTX_S *ctx, void *mem)
{
    _mem_block_free_entry(ctx, mem);
}

