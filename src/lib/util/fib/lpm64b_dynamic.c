/*================================================================
*   Created：LiXingang All rights reserved.
*   Author: lixingang  Version: 1.0  Date: 2015-5-2
*   Description: 
*
================================================================*/
#include "bs.h"

#include "utl/net.h"
#include "utl/lpm_utl.h"
#include "utl/num_utl.h"
#include "utl/mem_cap.h"

static void dlpm64b_reset(void *lpm);
static void dlpm64b_final(void *lpm);
static int dlpm64b_set_level(void *lpm, int level, int first_bit_num);
static int dlpm64b_add(void *lpm, UINT ip, UCHAR depth, UINT64 nexthop);
static int dlpm64b_del(void *lpm, UINT ip, UCHAR depth, UCHAR new_depth, UINT64 new_nexthop);
static int dlpm64b_lookup(void *lpm, UINT ip, OUT UINT64 *next_hop);
static void dlpm64b_walk(void *lpm, PF_LPM_WALK_CB walk_func, void *ud);

static LPM_FUNC_S g_dlpm64b_funcs = {
    .reset_func = dlpm64b_reset,
    .final_func = dlpm64b_final,
    .set_level_func = dlpm64b_set_level,
    .add_func = dlpm64b_add,
    .del_func = dlpm64b_del,
    .lookup_func = dlpm64b_lookup,
    .walk_func = dlpm64b_walk
};

static LPM64B_ENTRY_S * dlpm64b_alloc_entry(IN LPM_S *lpm, IN int level)
{
    LPM64B_ENTRY_S *entry;
    int num = (1 << lpm->bit_num[1]);

    entry = MemCap_ZMalloc(lpm->memcap, sizeof(LPM64B_ENTRY_S) * num);
    if (! entry) {
        return NULL;
    }

    entry->state = LPM_ENTRY_STATE_INVALID;

    return entry;
}

static int dlpm64b_add_inner(LPM_S *lpm, int level, LPM64B_ENTRY_S *entrys, UINT ip, UCHAR depth, UINT64 nexthop)
{
    int level_depth = 0;
    int up_depth = 0; 
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
            next_entrys = dlpm64b_alloc_entry(lpm, level+1);
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
            entry->nexthop = (UINT64)(unsigned long)next_entrys;
        } 

        return dlpm64b_add_inner(lpm, level + 1, (void*)(unsigned long)entry->nexthop, ip, depth, nexthop);
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
                ret = dlpm64b_add_inner(lpm, level + 1, (void*)(unsigned long)entrys[i].nexthop, ip, depth, nexthop);
                if (ret < 0) {
                    return ret;
                }
            }
        }
        return 0;
    }
}


static int dlpm64b_can_recycle(LPM_S *lpm, int level, LPM64B_ENTRY_S *entrys, int to_state)
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

static int dlpm64b_del_inner(LPM_S *lpm, int level, LPM64B_ENTRY_S *entrys,
        UINT ip, UCHAR depth, UCHAR new_depth, UINT64 new_nexthop)
{
    int level_depth = 0;
    int up_depth = 0; 
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

        dlpm64b_del_inner(lpm, level + 1, (void*)(unsigned long)entry->nexthop, ip, depth, new_depth, new_nexthop);

        if (dlpm64b_can_recycle(lpm, level+1, (void*)(unsigned long)entry->nexthop, state)) {
            MemCap_Free(lpm->memcap, (void*)(unsigned long)entry->nexthop);
            entry->nexthop = new_nexthop;
            entry->depth = new_depth;
            entry->state = state;
        }
    } else {
        num =  (1 << (level_depth - depth));
        num = MIN(num, 1<<lpm->bit_num[level]);

        for (i = index; i < (index + num); i++) {
            if (entrys[i].state == LPM_ENTRY_STATE_GROUP) {
                dlpm64b_del_inner(lpm, level + 1, (void*)(unsigned long)entrys[i].nexthop, ip, depth, new_depth, new_nexthop);
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

static int dlpm64b_lookup_inner(LPM_S *lpm, UINT ip, OUT UINT64 *next_hop)
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
        mask = PREFIX_2_MASK(start_bit); 
        index = ip & (~mask);  
        index = index >> (32 - stop_bit);  
        entry = &entrys[index];
        if (entry->state == LPM_ENTRY_STATE_VALID) {
            *next_hop = entry->nexthop;
            return 0;
        } else if (entry->state == LPM_ENTRY_STATE_GROUP) {
            entrys = (void*)(unsigned long)entry->nexthop;
        } else {
            break;
        }
        start_bit = stop_bit;
    }

    return BS_NOT_FOUND;
}

static int dlpm64b_walk_inner(LPM_S *lpm, int level, LPM64B_ENTRY_S *entrys,
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
            if ((ret = dlpm64b_walk_inner(lpm, level + 1,
                            (void*)(unsigned long)entrys[i].nexthop, ip, walk_func, ud)) < 0) {
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

static void dlpm64b_freeall(LPM_S *lpm, int level, LPM64B_ENTRY_S *entrys)
{
    int num = 1<<lpm->bit_num[level];
    int i;

    for (i=0; i<num; i++) {
        if (entrys[i].state == LPM_ENTRY_STATE_GROUP) {
            dlpm64b_freeall(lpm, level + 1, (void*)(unsigned long)entrys[i].nexthop);
        }
    }

    MemCap_Free(lpm->memcap, entrys);
}

static void dlpm64b_final(void *plpm)
{
    LPM_S *lpm = plpm;
    dlpm64b_freeall(lpm, 0, lpm->array);
    memset(lpm, 0, sizeof(LPM_S));
}


static int dlpm64b_set_level(void *plpm, int level, int first_bit_num)
{
    LPM_S *lpm = plpm;

    lpm->array = MemCap_ZMalloc(lpm->memcap, sizeof(LPM64B_ENTRY_S) * (1 << first_bit_num));
    if (! lpm->array) {
        RETURN(BS_NO_MEMORY);
    }

    return 0;
}

static int dlpm64b_add(void *plpm, UINT ip, UCHAR depth, UINT64 nexthop)
{
    LPM_S *lpm = plpm;

    if ((depth < 1) || (depth > 32)) {
        return -1;
    }

    return dlpm64b_add_inner(lpm, 0, lpm->array, ip, depth, nexthop);
}

static int dlpm64b_del(void *plpm, UINT ip, UCHAR depth, UCHAR new_depth, UINT64 new_nexthop)
{
    LPM_S *lpm = plpm;
    return dlpm64b_del_inner(lpm, 0, lpm->array, ip, depth, new_depth, new_nexthop);
}

static int dlpm64b_lookup(void *lpm, UINT ip, OUT UINT64 *next_hop)
{
    return dlpm64b_lookup_inner(lpm, ip, next_hop);
}

static void dlpm64b_walk(void *plpm, PF_LPM_WALK_CB walk_func, void *ud)
{
    LPM_S *lpm = plpm;
    dlpm64b_walk_inner(lpm, 0, lpm->array, 0, walk_func, ud);
}

static void dlpm64b_reset(void *plpm)
{
    LPM_S *lpm = plpm;
    void *memcap = lpm->memcap;

    dlpm64b_final(lpm);

    lpm->memcap = memcap;
    lpm->funcs = &g_dlpm64b_funcs;
}

int DLPM64B_Init(IN LPM_S *lpm, void *memcap)
{
    memset(lpm, 0, sizeof(LPM_S));
    lpm->memcap = memcap;
    lpm->funcs = &g_dlpm64b_funcs;

    return 0;
}

