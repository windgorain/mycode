/*================================================================
*   Created by LiXingang
*   Author: lixingang  Version: 1.0  Date: 2015-5-2
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/net.h"
#include "utl/lpm_utl.h"
#include "utl/map_utl.h"
#include "utl/mem_cap.h"

typedef struct {
    UINT ip;
    UCHAR depth;
    UCHAR reserve[3];
}LPM_RECORD_KEY_S;

typedef struct {
    LPM_RECORD_KEY_S key;
    UINT64 nexthop;
}LPM_RECORD_NODE_S;

static int _lpm_recording_walk(IN MAP_ELE_S *pstEle, IN VOID *pUserHandle)
{
    LPM_RECORD_NODE_S *node = pstEle->pData;
    USER_HANDLE_S *uh = pUserHandle;
    PF_LPM_WALK_CB walk_func = uh->ahUserHandle[0];

    return walk_func(node->key.ip, node->key.depth, node->nexthop, uh->ahUserHandle[1]);
}

static void _lpm_recording_node_free(void *data, void *ud)
{
    LPM_S *lpm = ud;
    MemCap_Free(lpm->memcap, data);
}

static void _lpm_del_recording_node(IN LPM_S *lpm, UINT ip, UCHAR depth)
{
    LPM_RECORD_KEY_S key = {0};

    key.ip = ip;
    key.depth = depth;

    LPM_RECORD_NODE_S *node = MAP_Del(lpm->recording_map, &key, sizeof(key));
    if (node) {
        MemCap_Free(lpm->memcap, node);
    }
}

static LPM_RECORD_NODE_S * _lpm_get_parent(IN LPM_S *lpm, UINT ip, UCHAR depth)
{
    int i;
    UINT mask;
    LPM_RECORD_KEY_S key = {0};
    LPM_RECORD_NODE_S *node = NULL;

    for (i=depth-1; i>=0; i--) {
        mask = PREFIX_2_MASK(i);
        key.ip = ip & mask; 
        key.depth = i;
        node = MAP_Get(lpm->recording_map, &key, sizeof(key));
        if (node) {
            break;
        }
    }

    return node;
}

int LPM_EnableRecording(IN LPM_S *lpm)
{
    if (lpm->recording_map) {
        return 0;
    }

    lpm->recording_map = MAP_AvlCreate(lpm->memcap);
    if (! lpm->recording_map) {
        RETURN(BS_NO_MEMORY);
    }

    return 0;
}

int LPM_Add(IN LPM_S *lpm, UINT ip, UCHAR depth, UINT64 nexthop )
{
    UINT mask = PREFIX_2_MASK(depth);
    ip = ip & mask;

    BS_DBGASSERT(lpm->level);

    if (! lpm->recording_map) {
        return lpm->funcs->add_func(lpm, ip, depth, nexthop);
    }

    LPM_RECORD_NODE_S *node = MemCap_ZMalloc(lpm->memcap, sizeof(LPM_RECORD_NODE_S));
    if (! node) {
        RETURN(BS_NO_MEMORY);
    }

    node->key.ip = ip;
    node->key.depth = depth;
    node->nexthop = nexthop;

    int ret = MAP_Add(lpm->recording_map, &node->key, sizeof(LPM_RECORD_KEY_S), node, 0);
    if (ret != 0) {
        MemCap_Free(lpm->memcap, node);
        return ret;
    }

    ret = lpm->funcs->add_func(lpm, ip, depth, nexthop);
    if (ret != 0) {
        MAP_Del(lpm->recording_map, &node->key, sizeof(LPM_RECORD_KEY_S));
        MemCap_Free(lpm->memcap, node);
    }

    return ret;
}

void LPM_Reset(LPM_S *lpm)
{
    if (lpm->recording_map) {
        MAP_Reset(lpm->recording_map, _lpm_recording_node_free, lpm);
    }

    lpm->funcs->reset_func(lpm);
}

void LPM_Final(LPM_S *lpm)
{
    if (lpm->recording_map) {
        MAP_Destroy(lpm->recording_map, _lpm_recording_node_free, lpm);
        lpm->recording_map = NULL;
    }

    lpm->funcs->final_func(lpm);
}

int LPM_SetLevel(LPM_S *lpm, int level, int first_bit_num)
{
    int i;

    BS_DBGASSERT(level>= 2);
    BS_DBGASSERT(level<= 8);
    BS_DBGASSERT(first_bit_num >= 8);
    BS_DBGASSERT(first_bit_num <= 24);
    BS_DBGASSERT(lpm->bit_num[0] == 0);

    int other_bit_num = (32 - first_bit_num) / (level - 1);

    BS_DBGASSERT(other_bit_num * (level - 1) + first_bit_num == 32);

    int ret = lpm->funcs->set_level_func(lpm, level, first_bit_num);
    if (ret < 0) {
        return ret;
    }

    lpm->bit_num[0] = first_bit_num;
    for (i=1; i<level; i++) {
        lpm->bit_num[i] = other_bit_num;
    }

    lpm->level = level;

    return 0;
}

int LPM_Del(IN LPM_S *lpm, UINT ip, UCHAR depth, UCHAR new_depth, UINT64 new_nexthop)
{
    UINT mask = PREFIX_2_MASK(depth);
    ip = ip & mask;

    if (! lpm->recording_map) {
        return lpm->funcs->del_func(lpm, ip, depth, new_depth, new_nexthop);
    }

    _lpm_del_recording_node(lpm, ip, depth);

    LPM_RECORD_NODE_S * node = _lpm_get_parent(lpm, ip, depth);
    if (node) {
        
        new_depth = node->key.depth;
        new_nexthop = node->nexthop;
    }

    return lpm->funcs->del_func(lpm, ip, depth, new_depth, new_nexthop);
}

int LPM_WalkRecording(LPM_S *lpm, PF_LPM_WALK_CB walk_func, void *ud)
{
    USER_HANDLE_S uh;

    if (! lpm->recording_map) {
        RETURN(BS_NOT_INIT);
    }

    uh.ahUserHandle[0] = walk_func;
    uh.ahUserHandle[1] = ud;

    MAP_Walk(lpm->recording_map, _lpm_recording_walk, &uh);

    return 0;
}

int LPM_FindRecording(LPM_S *lpm, UINT ip, UCHAR depth, OUT UINT64 *nexthop)
{
    UINT mask = PREFIX_2_MASK(depth);
    LPM_RECORD_KEY_S key = {0};

    if (! lpm->recording_map) {
        RETURN(BS_NOT_INIT);
    }

    key.ip = ip & mask;
    key.depth = depth;

    LPM_RECORD_NODE_S *node = MAP_Get(lpm->recording_map, &key, sizeof(key));
    if (! node) {
        RETURN(BS_NO_SUCH);
    }

    if (nexthop) {
        *nexthop = node->nexthop;
    }

    return 0;
}

