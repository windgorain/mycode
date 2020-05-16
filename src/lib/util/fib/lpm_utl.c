/*================================================================
*   Created by LiXingang: 2018.12.02
*   Description: 
*
================================================================*/
#include "bs.h"

#include "utl/net.h"
#include "utl/lpm_utl.h"
#include "utl/num_utl.h"

int LPM_Init(IN LPM_S *lpm, IN UINT array_size, IN LPM_ENTRY_S *array)
{
    memset(lpm, 0, sizeof(LPM_S));

    lpm->array_size = array_size;
    lpm->array = array;

    return 0;
}

/* 
 设置每级的位数,除了第一级,要求其他级别都相同,以免形成碎片
 如: 16-8-8, 24-8, 24-4-4, 16-4-4-4-4
 比如16-8-8则设置为:
 LPM_SetLevel(lpm, 0, 16);
 LPM_SetLevel(lpm, 1, 8);
 LPM_SetLevel(lpm, 2, 8);
 */
int LPM_SetLevel(IN LPM_S *lpm, IN int level, IN int bit_num)
{
    if (level >= 32) {
        return -1;
    }
    if ((bit_num < 1) || (bit_num > 32)) {
        return -1;
    }

    lpm->bit_num[level] = bit_num;

    return 0;
}

static int lpm_alloc_entry(IN LPM_S *lpm, IN int level)
{
    int start;
    int i;
    int num;

    /* 跳过第一级的所有表项 */
    start = (1 << lpm->bit_num[0]);

    /* 开始查找空闲块 */
    num = (1 << lpm->bit_num[level]);
    for (i=start; i<lpm->array_size; i+=num) {
        if (lpm->array[i].state == LPM_ENTRY_STATE_FREE) {
            lpm->array[i].state = LPM_ENTRY_STATE_INVALID;
            break;
        }
    }

    if (i >= lpm->array_size) {
        return -1;
    }

    return i;
}

static void lpm_free_entrys(LPM_S *lpm, int index)
{
    lpm->array[index].state = LPM_ENTRY_STATE_FREE;
}

static int lpm_Add(LPM_S *lpm, int level, LPM_ENTRY_S *entrys,
        UINT ip, UCHAR depth, UINT nexthop)
{
    int level_depth = 0;
    int up_depth = 0; /* 此级之前的bit_num之和 */
    UINT mask;
    UINT index;
    UINT num;
    int i;
    int ret;
    int next_level_index;
    LPM_ENTRY_S *entry;

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
                    lpm->array[i].state = LPM_ENTRY_STATE_VALID;
                    lpm->array[i].depth = entrys[index].depth;
                    lpm->array[i].nexthop = entrys[index].nexthop;
                }
            }
            entry->state = LPM_ENTRY_STATE_GROUP;
            entry->depth= 0;
            entry->nexthop = next_level_index;
        } 

        return lpm_Add(lpm, level + 1, lpm->array + entry->nexthop,
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
                ret = lpm_Add(lpm, level + 1, lpm->array + entrys[i].nexthop, 
                        ip, depth, nexthop);
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
static int lpm_CanRecycle(LPM_S *lpm, int level, LPM_ENTRY_S *entrys)
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
    }

    return 1;
}

static int lpm_Del(LPM_S *lpm, int level, LPM_ENTRY_S *entrys,
        UINT ip, UCHAR depth, UCHAR new_depth, UINT new_nexthop)
{
    int level_depth = 0;
    int up_depth = 0; /* 此级之前的bit_num之和 */
    UINT mask;
    UINT index;
    UINT num;
    int i;
    LPM_ENTRY_S *entry;
    UINT state;

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

        lpm_Del(lpm, level + 1, lpm->array + entry->nexthop,
                ip, depth, new_depth, new_nexthop);

        if (lpm_CanRecycle(lpm, level+1, lpm->array + entry->nexthop)) {
            lpm_free_entrys(lpm, entry->nexthop);
            entrys[i].nexthop = new_nexthop;
            entrys[i].depth = new_depth;
            entrys[i].state = state;
        }
    } else {
        num =  (1 << (level_depth - depth));
        num = MIN(num, 1<<lpm->bit_num[level]);

        for (i = index; i < (index + num); i++) {
            if (entrys[i].state == LPM_ENTRY_STATE_GROUP) {
                lpm_Del(lpm, level + 1, lpm->array + entry->nexthop,
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

int LPM_Add(IN LPM_S *lpm, UINT ip/*host order*/, UCHAR depth, UINT nexthop)
{
    UINT ip_masked;
    UINT mask;

    if ((depth < 1) || (depth > 32)) {
        return -1;
    }

    mask = PREFIX_2_MASK(depth);

    ip_masked = ip & mask;

    return lpm_Add(lpm, 0, lpm->array, ip_masked, depth, nexthop);
}

int LPM_Del(IN LPM_S *lpm, UINT ip/*host order*/, UCHAR depth, UCHAR new_depth, UINT nexthop)
{
    return lpm_Del(lpm, 0, lpm->array, ip, depth, new_depth, nexthop);
}
