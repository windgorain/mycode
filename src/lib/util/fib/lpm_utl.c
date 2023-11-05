/*================================================================
*   Created：LiXingang All rights reserved.
*   Description: 
*
================================================================*/
#include "bs.h"

#include "utl/net.h"
#include "utl/free_list.h"
#include "utl/lpm_utl.h"
#include "utl/num_utl.h"

static void lpm_reset(void *lpm);
static void lpm_final(void *lpm);
static int lpm_set_level(void *lpm, int level, int first_bit_num);
static int lpm_add(void *lpm, UINT ip, UCHAR depth, UINT64 nexthop);
static int lpm_del(void *lpm, UINT ip, UCHAR depth, UCHAR new_depth, UINT64 new_nexthop);
static int lpm_lookup(void *lpm, UINT ip, OUT UINT64 *next_hop);
static void lpm_walk(void *lpm, PF_LPM_WALK_CB walk_func, void *ud);

static LPM_FUNC_S g_lpm_funcs = {
    .reset_func = lpm_reset,
    .final_func = lpm_final,
    .set_level_func = lpm_set_level,
    .add_func = lpm_add,
    .del_func = lpm_del,
    .lookup_func = lpm_lookup,
    .walk_func = lpm_walk
};


static inline int lpm_index_2_block(LPM_S *lpm, int index)
{
    int start = (1 << lpm->bit_num[0]);
    BS_DBGASSERT(index >= start);
    return (index - start) >> lpm->bit_num[1];
}


static inline int lpm_block_2_index(LPM_S *lpm, int block)
{
    int start = (1 << lpm->bit_num[0]);
    return start + (block << lpm->bit_num[1]);
}

static inline void * lpm_block_2_entry(LPM_S *lpm, int block)
{
    return (LPM_ENTRY_S *)lpm->array + lpm_block_2_index(lpm, block);
}

static inline int lpm_entry_2_index(LPM_S *lpm, void *entry)
{
    LPM_ENTRY_S *start = lpm->array;
    LPM_ENTRY_S *this = entry;

    return this - start;
}

static int lpm_alloc_entry(IN LPM_S *lpm, IN int level)
{
    LPM_ENTRY_S *entry;
    int num = (1 << lpm->bit_num[1]);

    entry = FreeList_Get(&lpm->free_list);
    if (! entry) {
        return -1;
    }

    memset(entry, 0, sizeof(LPM_ENTRY_S) * num);

    int index = lpm_entry_2_index(lpm, entry);
    entry->state = LPM_ENTRY_STATE_INVALID;

    return index;
}

static void lpm_free_entrys(LPM_S *lpm, int index)
{
    LPM_ENTRY_S *array = lpm->array;
    LPM_ENTRY_S *this = &array[index];

    this->state = LPM_ENTRY_STATE_FREE;
    FreeList_Put(&lpm->free_list, this);
}

static int lpm_add_inner(LPM_S *lpm, int level, LPM_ENTRY_S *entrys,
        UINT ip, UCHAR depth, UINT nexthop)
{
    int level_depth = 0;
    int up_depth = 0; 
    UINT mask;
    UINT index;
    UINT num;
    int i;
    int ret;
    int next_level_index;
    LPM_ENTRY_S *entry;
    LPM_ENTRY_S *array = lpm->array;

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
            next_level_index = lpm_alloc_entry(lpm, level+1);
            if (next_level_index < 0) {
                return -1;
            }
            if (entry->state == LPM_ENTRY_STATE_VALID) {
                num = 1 << lpm->bit_num[level + 1];
                for (i=next_level_index; i<next_level_index + num; i++) {
                    array[i].state = LPM_ENTRY_STATE_VALID;
                    array[i].depth = entrys[index].depth;
                    array[i].nexthop = entrys[index].nexthop;
                }
            }
            entry->state = LPM_ENTRY_STATE_GROUP;
            entry->depth= 0;
            entry->nexthop = lpm_index_2_block(lpm, next_level_index);
        } 

        return lpm_add_inner(lpm, level + 1, lpm_block_2_entry(lpm, entry->nexthop),
                ip, depth, nexthop);
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
                ret = lpm_add_inner(lpm, level + 1, lpm_block_2_entry(lpm, entrys[i].nexthop), 
                        ip, depth, nexthop);
                if (ret < 0) {
                    return ret;
                }
            }
        }
        return 0;
    }
}


static int lpm_can_recycle(LPM_S *lpm, int level, LPM_ENTRY_S *entrys, int to_state)
{
    int num;
    int i;
    int level_depth = 0;
    int up_depth = 0; 

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
        
        if ((to_state == LPM_ENTRY_STATE_INVALID) && (entrys[i].state == LPM_ENTRY_STATE_VALID)) {
            return 0;
        }
    }

    return 1;
}

static int lpm_del_inner(LPM_S *lpm, int level, LPM_ENTRY_S *entrys,
        UINT ip, UCHAR depth, UCHAR new_depth, UINT new_nexthop)
{
    int level_depth = 0;
    int up_depth = 0; 
    UINT mask;
    UINT index;
    UINT num;
    int i;
    LPM_ENTRY_S *entry;
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

        lpm_del_inner(lpm, level + 1, lpm_block_2_entry(lpm, entry->nexthop),
                ip, depth, new_depth, new_nexthop);

        if (lpm_can_recycle(lpm, level+1, lpm_block_2_entry(lpm, entry->nexthop), state)) {
            lpm_free_entrys(lpm, lpm_block_2_index(lpm, entry->nexthop));
            entry->nexthop = new_nexthop;
            entry->depth = new_depth;
            entry->state = state;
        }
    } else {
        num =  (1 << (level_depth - depth));
        num = MIN(num, 1<<lpm->bit_num[level]);

        for (i = index; i < (index + num); i++) {
            if (entrys[i].state == LPM_ENTRY_STATE_GROUP) {
                lpm_del_inner(lpm, level + 1, lpm_block_2_entry(lpm, entrys[i].nexthop),
                        ip, depth, new_depth, new_nexthop);
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

static int lpm_lookup_inner(LPM_S *lpm, UINT ip, OUT UINT64 *next_hop)
{
    LPM_ENTRY_S *entrys = lpm->array;
    LPM_ENTRY_S *entry;
    int start_bit = 0;
    int stop_bit;
    UINT mask;
    int i;
    UINT index;

    for (i=0; i<lpm->level; i++) {
        stop_bit = start_bit + lpm->bit_num[i];
        mask = PREFIX_2_MASK(start_bit); 
        index = ip & (~mask);  
        index = index >> (32 - stop_bit);  
        entry = &entrys[index];
        if (entry->state == LPM_ENTRY_STATE_VALID) {
            *next_hop = entry->nexthop;
            return 0;
        } else if (entry->state == LPM_ENTRY_STATE_GROUP) {
            entrys = lpm_block_2_entry(lpm, entry->nexthop);
        } else {
            break;
        }
        start_bit = stop_bit;
    }

    return BS_NOT_FOUND;
}

static int lpm_walk_inner(LPM_S *lpm, int level, LPM_ENTRY_S *entrys,
        UINT ip, PF_LPM_WALK_CB walk_func, void *ud)
{
    int num = 1<<lpm->bit_num[level];
    int ret;
    int i;
    int level_depth = 0;
    int up_depth = 0; 
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
            if ((ret = lpm_walk_inner(lpm, level + 1,
                        lpm_block_2_entry(lpm, entrys[i].nexthop), ip, walk_func, ud)) < 0) {
                return ret;
            }
        } else if (entrys[i].state == LPM_ENTRY_STATE_VALID) {
            if ((ret = walk_func(ip, entrys[i].depth, entrys[i].nexthop, ud)) < 0) {
                return ret;
            }
        }
    }

    return 0;
}

static void lpm_final(void *plpm)
{
    LPM_S *lpm = plpm;

    if (lpm->array_alloced) {
        MEM_Free(lpm->array);
    }

    memset(lpm, 0, sizeof(LPM_S));
}

static int lpm_init_freelist(LPM_S *lpm)
{
    int start = (1 << lpm->bit_num[0]);
    int num = (1 << lpm->bit_num[1]);
    int left = lpm->array_size - start;
    int block_count = left / num;
    int i;

    for (i=0; i<block_count; i++) {
        void *entry = lpm_block_2_entry(lpm, i);
        FreeList_Put(&lpm->free_list, entry);
    }

    return 0;
}



static int lpm_set_level(void *plpm, int level, int first_bit_num)
{
    LPM_S *lpm = plpm;

    BS_DBGASSERT(level>= 2);
    BS_DBGASSERT(level<= 8);
    BS_DBGASSERT(first_bit_num >= 8);
    BS_DBGASSERT(first_bit_num <= 24);
    BS_DBGASSERT(lpm->bit_num[0] == 0);

    int other_bit_num = (32 - first_bit_num) / (level - 1);

    BS_DBGASSERT(other_bit_num * (level - 1) + first_bit_num == 32);
    BS_DBGASSERT(lpm->array_size > (1<<first_bit_num));

    int i;
    lpm->bit_num[0] = first_bit_num;
    for (i=1; i<level; i++) {
        lpm->bit_num[i] = other_bit_num;
    }

    lpm->level = level;

    lpm_init_freelist(lpm);

    return 0;
}

static int lpm_add(void *plpm, UINT ip, UCHAR depth, UINT64 nexthop)
{
    LPM_S *lpm = plpm;
    UINT ip_masked;
    UINT mask;

    BS_DBGASSERT(nexthop <= 0xffffff);

    if ((depth < 1) || (depth > 32)) {
        return -1;
    }

    mask = PREFIX_2_MASK(depth);

    ip_masked = ip & mask;

    return lpm_add_inner(lpm, 0, lpm->array, ip_masked, depth, nexthop);
}

static int lpm_del(void *plpm, UINT ip, UCHAR depth, UCHAR new_depth, UINT64 new_nexthop)
{
    LPM_S *lpm = plpm;
    return lpm_del_inner(lpm, 0, lpm->array, ip, depth, new_depth, new_nexthop);
}

static int lpm_lookup(void *lpm, UINT ip, OUT UINT64 *next_hop)
{
    return lpm_lookup_inner(lpm, ip, next_hop);
}

static void lpm_walk(void *plpm, PF_LPM_WALK_CB walk_func, void *ud)
{
    LPM_S *lpm = plpm;
    lpm_walk_inner(lpm, 0, lpm->array, 0, walk_func, ud);
}

static void lpm_reset(void *plpm)
{
    LPM_S *lpm = plpm;
    LPM_ENTRY_S *array = lpm->array;
    UINT array_size = lpm->array_size;
    UINT array_alloced = lpm->array_alloced;

    memset(lpm, 0, sizeof(LPM_S));
    FreeList_Init(&lpm->free_list);

    lpm->array_alloced = array_alloced;
    lpm->array_size = array_size;
    lpm->array = array;
    lpm->funcs = &g_lpm_funcs;
}


int LPM_Init(IN LPM_S *lpm, IN UINT array_size, IN LPM_ENTRY_S *array)
{
    memset(lpm, 0, sizeof(LPM_S));

    
    if (! array) {
        array = MEM_ZMalloc(array_size * sizeof(LPM_ENTRY_S));
        if (! array) {
            RETURN(BS_NO_MEMORY);
        }
        lpm->array_alloced = 1;
    }

    FreeList_Init(&lpm->free_list);

    lpm->array_size = array_size;
    lpm->array = array;
    lpm->funcs = &g_lpm_funcs;

    return 0;
}

