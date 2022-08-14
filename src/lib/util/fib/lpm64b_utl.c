/*================================================================
*   Created：LiXingang All rights reserved.
*   Description: 
*
================================================================*/
#include "bs.h"

#include "utl/net.h"
#include "utl/lpm_utl.h"
#include "utl/num_utl.h"

static void lpm64b_reset(void *lpm);
static void lpm64b_final(void *lpm);
static int lpm64b_set_level(void *lpm, int level, int first_bit_num);
static int lpm64b_add(void *lpm, UINT ip/*host order*/, UCHAR depth, UINT64 nexthop);
static int lpm64b_del(void *lpm, UINT ip/*host order*/, UCHAR depth, UCHAR new_depth, UINT64 new_nexthop);
static int lpm64b_lookup(void *lpm, UINT ip/*host order*/, OUT UINT64 *next_hop);
static void lpm64b_walk(void *plpm, PF_LPM_WALK_CB walk_func, void *ud);

static LPM_FUNC_S g_lpm64b_funcs = {
    .reset_func = lpm64b_reset,
    .final_func = lpm64b_final,
    .set_level_func = lpm64b_set_level,
    .add_func = lpm64b_add,
    .del_func = lpm64b_del,
    .lookup_func = lpm64b_lookup,
    .walk_func = lpm64b_walk
};

/* 计算block的起始index */
static inline int lpm64b_block_2_index(LPM_S *lpm, UINT block)
{
    int block_index = HANDLE_UINT(block);
    int start = (1 << lpm->bit_num[0]);
    return start + (block_index << lpm->bit_num[1]);
}

static inline void * lpm64b_block_2_entry(LPM_S *lpm, UINT block)
{
    return (LPM64B_ENTRY_S *)lpm->array + lpm64b_block_2_index(lpm, block);
}

static LPM64B_ENTRY_S * lpm64b_alloc_entry(IN LPM_S *lpm, IN int level)
{
    LPM64B_ENTRY_S *entry;
    int num = (1 << lpm->bit_num[1]);

    entry = FreeList_Get(&lpm->free_list);
    if (! entry) {
        return NULL;
    }

    memset(entry, 0, sizeof(LPM64B_ENTRY_S) * num);

    entry->state = LPM_ENTRY_STATE_INVALID;

    return entry;
}

static void lpm64b_free_entrys(LPM_S *lpm, LPM64B_ENTRY_S *entrys)
{
    LPM64B_ENTRY_S *this = entrys;
    this->state = LPM_ENTRY_STATE_FREE;
    FreeList_Put(&lpm->free_list, this);
}

static int lpm64b_add_inner(LPM_S *lpm, int level, LPM64B_ENTRY_S *entrys, UINT ip, UCHAR depth, UINT64 nexthop)
{
    int level_depth = 0;
    int up_depth = 0; /* 此级之前的bit_num之和 */
    UINT mask;
    UINT index;
    UINT num;
    int i;
    int ret;
    LPM64B_ENTRY_S *next_entrys = NULL;
    LPM64B_ENTRY_S *entry;

    for (i=0; i<=level; i++) {
        level_depth += lpm->bit_num[i];
    }

    up_depth = level_depth - lpm->bit_num[level];

    mask = PREFIX_2_MASK(up_depth);
    index = ip & (~mask);
    index = index >> (32 - level_depth);
    entry = &entrys[index];

    if (depth > level_depth) {
        if (entry->state != LPM_ENTRY_STATE_GROUP) {
            next_entrys = lpm64b_alloc_entry(lpm, level+1);
            if (! next_entrys) {
                return -1;
            }
            if (entry->state == LPM_ENTRY_STATE_VALID) {
                num = 1 << lpm->bit_num[level + 1];
                for (i=0; i<num; i++) {
                    next_entrys[i].state = LPM_ENTRY_STATE_VALID;
                    next_entrys[i].depth = entrys[index].depth;
                    next_entrys[i].nexthop = entrys[index].nexthop;
                }
            }
            entry->state = LPM_ENTRY_STATE_GROUP;
            entry->depth= 0;
            entry->nexthop = (UINT64)(ULONG) next_entrys;
        } 

        return lpm64b_add_inner(lpm, level + 1, (void*)(ULONG)entry->nexthop, ip, depth, nexthop);
    } else {
        num =  (1 << (level_depth - depth));
        num = MIN(num, 1<<lpm->bit_num[level]);

        for (i = index; i < (index + num); i++) {
            if (entrys[i].state != LPM_ENTRY_STATE_GROUP) {
                if (entrys[i].depth <= depth) {
                    entrys[i].nexthop = nexthop;
                    entrys[i].depth = depth;
                    entrys[i].state = LPM_ENTRY_STATE_VALID;
                }
            } else {
                ret = lpm64b_add_inner(lpm, level + 1, (void*)(ULONG) entrys[i].nexthop, ip, depth, nexthop);
                if (ret < 0) {
                    return ret;
                }
            }
        }
        return 0;
    }
}

/*
   如果下级全部invalid了，则可以回收它
   如果当前级别没有下一级且所有depth小于当前level,则可以释放
 */
static int lpm64b_can_recycle(LPM_S *lpm, int level, LPM64B_ENTRY_S *entrys, int to_state)
{
    int num;
    int i;
    int level_depth = 0;
    int up_depth = 0; /* 此级之前的bit_num之和 */

    for (i=0; i<=level; i++) {
        level_depth += lpm->bit_num[i];
    }

    up_depth = level_depth - lpm->bit_num[level];

    num = 1<<lpm->bit_num[level];

    for (i=0; i<num; i++) {
        if (entrys[i].state == LPM_ENTRY_STATE_GROUP) {
            return 0;
        }
        if (entrys[i].depth > up_depth) {
            return 0;
        }
        /* 目的state为invalid,但存在valid子项,则挖洞 */
        if ((to_state == LPM_ENTRY_STATE_INVALID) && (entrys[i].state == LPM_ENTRY_STATE_VALID)) {
            return 0;
        }
    }

    return 1;
}

static int lpm64b_del_inner(LPM_S *lpm, int level, LPM64B_ENTRY_S *entrys,
        UINT ip, UCHAR depth, UCHAR new_depth, UINT64 new_nexthop)
{
    int level_depth = 0;
    int up_depth = 0; /* 此级之前的bit_num之和 */
    UINT mask;
    UINT index;
    UINT num;
    int i;
    LPM64B_ENTRY_S *entry;
    UINT state;

    if (! entrys) {
        BS_DBGASSERT(0);
        return BS_ERR;
    }

    if ((new_depth == 0) && (new_nexthop == 0)) {
        state = LPM_ENTRY_STATE_INVALID;
    } else {
        state = LPM_ENTRY_STATE_VALID;
    }

    for (i=0; i<=level; i++) {
        level_depth += lpm->bit_num[i];
    }

    up_depth = level_depth - lpm->bit_num[level];

    mask = PREFIX_2_MASK(up_depth);
    index = ip & (~mask);
    index = index >> (32 - level_depth);
    entry = &entrys[index];

    if (depth > level_depth) {
        if (entry->state != LPM_ENTRY_STATE_GROUP) {
            return 0;
        }

        lpm64b_del_inner(lpm, level + 1, (void*)(ULONG)entry->nexthop, ip, depth, new_depth, new_nexthop);

        if (lpm64b_can_recycle(lpm, level+1, (void*)(ULONG)entry->nexthop, state)) {
            lpm64b_free_entrys(lpm, (void*)(ULONG)entry->nexthop);
            entry->nexthop = new_nexthop;
            entry->depth = new_depth;
            entry->state = state;
        }
    } else {
        num =  (1 << (level_depth - depth));
        num = MIN(num, 1<<lpm->bit_num[level]);

        for (i = index; i < (index + num); i++) {
            if (entrys[i].state == LPM_ENTRY_STATE_GROUP) {
                lpm64b_del_inner(lpm, level + 1, (void*)(ULONG)entrys[i].nexthop, ip, depth, new_depth, new_nexthop);
            } else {
                if (entrys[i].depth <= depth) {
                    entrys[i].nexthop = new_nexthop;
                    entrys[i].depth = new_depth;
                    entrys[i].state = state;
                }
            }
        }
    }

    return 0;
}

static int lpm64b_lookup_inner(LPM_S *lpm, UINT ip, OUT UINT64 *next_hop)
{
    LPM64B_ENTRY_S *entrys = lpm->array;
    LPM64B_ENTRY_S *entry;
    int start_bit = 0;
    int stop_bit;
    UINT mask;
    int i;
    UINT index;

    for (i=0; i<lpm->level; i++) {
        stop_bit = start_bit + lpm->bit_num[i];
        mask = PREFIX_2_MASK(start_bit); /* 获取上级mask */
        index = ip & (~mask);  /* 抹去上级对应bits */
        index = index >> (32 - stop_bit);  /* 移动到stop位 */
        entry = &entrys[index];
        if (entry->state == LPM_ENTRY_STATE_VALID) {
            *next_hop = entry->nexthop;
            return 0;
        } else if (entry->state == LPM_ENTRY_STATE_GROUP) {
            entrys = (void*)(ULONG)entry->nexthop;
        } else {
            break;
        }
        start_bit = stop_bit;
    }

    return BS_NOT_FOUND;
}

static int lpm64b_walk_inner(LPM_S *lpm, int level, LPM64B_ENTRY_S *entrys,
        UINT ip, PF_LPM_WALK_CB walk_func, void *ud)
{
    int num = 1<<lpm->bit_num[level];
    int i;
    int level_depth = 0;
    int up_depth = 0; /* 此级之前的bit_num之和 */
    int down_depth;
    UINT up_mask;

    for (i=0; i<=level; i++) {
        level_depth += lpm->bit_num[i];
    }

    up_depth = level_depth - lpm->bit_num[level];
    down_depth = 32 - level_depth;
    up_mask = PREFIX_2_MASK(up_depth);

    for (i=0; i<num; i++) {
        ip = ip & up_mask;
        ip |= (i << down_depth);
        if (entrys[i].state == LPM_ENTRY_STATE_GROUP) {
            if (BS_WALK_STOP == lpm64b_walk_inner(lpm, level + 1, (void*)(ULONG) entrys[i].nexthop, ip, walk_func, ud)) {
                return BS_WALK_STOP;
            }
        } else if (entrys[i].state == LPM_ENTRY_STATE_VALID) {
            if (BS_WALK_STOP == walk_func(ip, entrys[i].depth, entrys[i].nexthop, ud)) {
                return BS_WALK_STOP;
            }
        }
    }

    return BS_WALK_CONTINUE;
}

static void lpm64b_final(void *plpm)
{
    LPM_S *lpm = plpm;

    if (lpm->array_alloced) {
        MEM_Free(lpm->array);
    }

    memset(lpm, 0, sizeof(LPM_S));
}

static int lpm64b_init_freelist(LPM_S *lpm)
{
    int start = (1 << lpm->bit_num[0]);
    int num = (1 << lpm->bit_num[1]);
    int left = lpm->array_size - start;
    int block_count = left / num;
    int i;

    for (i=0; i<block_count; i++) {
        void *entry = lpm64b_block_2_entry(lpm, i);
        FreeList_Put(&lpm->free_list, entry);
    }

    return 0;
}

/* 
 设置每级的位数,除了第一级,要求其他级别都相同
 如: 16-8-8, 24-8, 24-4-4, 16-4-4-4-4
 比如16-8-8则设置为:
 LPM_SetLevel(lpm, 3, 16);
 */
static int lpm64b_set_level(void *plpm, int level, int first_bit_num)
{
    LPM_S *lpm = plpm;
    BS_DBGASSERT(level>= 2);
    BS_DBGASSERT(level<= 8);
    BS_DBGASSERT(first_bit_num >= 8);
    BS_DBGASSERT(first_bit_num <= 24);

    int other_bit_num = (32 - first_bit_num) / (level - 1);

    BS_DBGASSERT(other_bit_num * (level - 1) + first_bit_num == 32);
    BS_DBGASSERT(lpm->array_size > (1<<first_bit_num));

    int i;
    lpm->bit_num[0] = first_bit_num;
    for (i=1; i<level; i++) {
        lpm->bit_num[i] = other_bit_num;
    }

    lpm->level = level;

    lpm64b_init_freelist(lpm);

    return 0;
}

static int lpm64b_add(void *plpm, UINT ip/*host order*/, UCHAR depth, UINT64 nexthop)
{
    LPM_S *lpm = plpm;
    UINT ip_masked;
    UINT mask;

    if ((depth < 1) || (depth > 32)) {
        return -1;
    }

    mask = PREFIX_2_MASK(depth);

    ip_masked = ip & mask;

    return lpm64b_add_inner(lpm, 0, lpm->array, ip_masked, depth, nexthop);
}

static int lpm64b_del(void *plpm, UINT ip/*host order*/, UCHAR depth, UCHAR new_depth, UINT64 new_nexthop)
{
    LPM_S *lpm = plpm;
    return lpm64b_del_inner(lpm, 0, lpm->array, ip, depth, new_depth, new_nexthop);
}

static int lpm64b_lookup(void *lpm, UINT ip/*host order*/, OUT UINT64 *next_hop)
{
    return lpm64b_lookup_inner(lpm, ip, next_hop);
}

static void lpm64b_walk(void *plpm, PF_LPM_WALK_CB walk_func, void *ud)
{
    LPM_S *lpm = plpm;
    lpm64b_walk_inner(lpm, 0, lpm->array, 0, walk_func, ud);
}

static void lpm64b_reset(void *plpm)
{
    LPM_S *lpm = plpm;
    LPM64B_ENTRY_S *array = lpm->array;
    UINT array_size = lpm->array_size;
    UINT array_alloced = lpm->array_alloced;

    memset(lpm, 0, sizeof(LPM_S));
    FreeList_Init(&lpm->free_list);

    lpm->array_alloced = array_alloced;
    lpm->array_size = array_size;
    lpm->array = array;
    lpm->funcs = &g_lpm64b_funcs;
}

int LPM64B_Init(IN LPM_S *lpm, IN UINT array_size, IN LPM64B_ENTRY_S *array/* 可以为NULL */)
{
    memset(lpm, 0, sizeof(LPM_S));

    /* 如果外面没有传入内存,则自动申请 */
    if (! array) {
        array = MEM_ZMalloc(array_size * sizeof(LPM64B_ENTRY_S));
        if (! array) {
            RETURN(BS_NO_MEMORY);
        }
        lpm->array_alloced = 1;
    }

    FreeList_Init(&lpm->free_list);

    lpm->array_size = array_size;
    lpm->array = array;
    lpm->funcs = &g_lpm64b_funcs;

    return 0;
}

