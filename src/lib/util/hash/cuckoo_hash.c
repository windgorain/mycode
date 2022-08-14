/*================================================================
*   Created by LiXingang
*   Description:布谷鸟hash 
*               为了避免hash冲突导致添加失败,需要hash空间大于
*               最大表项个数. 例如一百万会话,需要二百万的hash空间
================================================================*/
#include "bs.h"
#include "utl/rand_utl.h"
#include "utl/num_utl.h"
#include "utl/cuckoo_hash.h"

#define CUCKOO_HASH_MAX_TRY 256

static inline int cuckoohash_FirstBktIndex(CUCKOO_HASH_S *hash, UINT key_factor)
{
    return key_factor & hash->bucket_mask;
}

static inline int cuckoohash_SecondBktIndex(CUCKOO_HASH_S *hash,
        UINT key_factor)
{
    int first_index;
    int index;

    first_index = cuckoohash_FirstBktIndex(hash, key_factor);
    index = ((key_factor >> 16) ^ key_factor) & hash->bucket_mask;

    if (index == first_index) {
        index = ((key_factor >> 7) ^ 0x5bd1e995) & hash->bucket_mask;
        if (index == first_index) {
            index = (~index) & hash->bucket_mask;
        }
    }

    return index;
}

static inline int cuckoohash_PeerBktIndex(CUCKOO_HASH_S *hash,
        UINT key_factor, int cur_index)
{
    int index = cuckoohash_FirstBktIndex(hash, key_factor);

    if (index == cur_index) {
        index = cuckoohash_SecondBktIndex(hash, key_factor);
    }

    return index;
}

static inline int cuukoohash_GetLastUsedInBkt(CUCKOO_HASH_S *hash,
        int bkt_index)
{
    int i;
    int start_index = bkt_index * hash->bucket_depth;
    int end_index = start_index + hash->bucket_depth;
    CUCKOO_HASH_NODE_S *entry = hash->table;

    for (i = end_index - 1; i>= start_index; i--) {
        if (entry[i].used) {
            return i;
        }
    }

    return -1;
}

static int cuckoohash_Cmp(CUCKOO_HASH_S *hash, void *data1, void *data2)
{
    return hash->cmp_func(hash, data1, data2);
}

static int cuckoohash_FindInBkt(CUCKOO_HASH_S *hash, int bkt_index,
        UINT key_factor, void *data)
{
    int i;
    int find_index = -1;
    int start_index = bkt_index * hash->bucket_depth;
    int end_index = start_index + hash->bucket_depth;
    CUCKOO_HASH_NODE_S *entry = hash->table;

    for (i=start_index; i<end_index; i++) {
        if (! entry[i].used) {
            break;
        }
        if (entry[i].key_factor != key_factor) {
            continue;
        }
        if (cuckoohash_Cmp(hash, entry[i].data, data) == 0) {
            find_index = i;
            break;
        }
    }

    return find_index;
}

static inline int cuckoohash_FindWithKeyFactor(CUCKOO_HASH_S *hash,
        UINT key_factor, void *data)
{
    int first_bkt_index = cuckoohash_FirstBktIndex(hash, key_factor);
    int second_bkt_index;
    int find_index;

    find_index = cuckoohash_FindInBkt(hash, first_bkt_index, key_factor, data);
    if (find_index < 0) {
        second_bkt_index = cuckoohash_SecondBktIndex(hash, key_factor);
        find_index = cuckoohash_FindInBkt(hash, second_bkt_index,
                key_factor, data);
    }

    return find_index;
}

/* 尝试添加到bucket */
static int cuckoohash_Add2Bkt(CUCKOO_HASH_S *hash, int bkt_index,
        UINT key_factor, void *data)
{
    int i, added_index = -1;
    int start_index = bkt_index * hash->bucket_depth;
    int end_index = start_index + hash->bucket_depth;
    CUCKOO_HASH_NODE_S *entry = hash->table;

    for (i=start_index; i<end_index; i++) {
        if (! entry[i].used) {
            entry[i].used = 1;
            entry[i].key_factor = key_factor;
            entry[i].data = data;
            added_index = i;
            break;
        }
    }

    return added_index;
}

/* 尝试强占到bucket */
static int cuckoohash_ForceAdd2Bkt(CUCKOO_HASH_S *hash, int bkt_index,
        UINT key_factor, void *data, int max_try)
{
    int added_index;
    int force_index;
    int peer_bkt_index;
    CUCKOO_HASH_NODE_S *entry = hash->table;
    UINT old_key_factor;
    void *old_data;

    added_index = cuckoohash_Add2Bkt(hash, bkt_index, key_factor, data);
    if (added_index >= 0) {
        return added_index;
    }

    if (max_try >= CUCKOO_HASH_MAX_TRY) {
        return -1;
    }

    /* 强行占用 */
    force_index = (bkt_index * hash->bucket_depth)
        + (RAND_Get() & hash->bucket_depth_mask);

    old_key_factor = entry[force_index].key_factor;
    old_data = entry[force_index].data;
    entry[force_index].key_factor = key_factor;
    entry[force_index].data = data;

    /* 被踢走的节点去抢占别的位置 */
    peer_bkt_index = cuckoohash_PeerBktIndex(hash, old_key_factor, bkt_index);
    added_index = cuckoohash_ForceAdd2Bkt(hash, peer_bkt_index, old_key_factor,
            old_data, max_try + 1);

    /* 强占失败 */
    if (added_index < 0) {
        entry[force_index].data = old_data;
        entry[force_index].key_factor = old_key_factor;
        return -1;
    }

    return force_index;
}

static inline int cuckoohash_AddWithKeyFactor(CUCKOO_HASH_S *hash,
        UINT key_factor, void *data)
{
    int first_bkt_index = cuckoohash_FirstBktIndex(hash, key_factor);
    int second_bkt_index;
    int index;

    index = cuckoohash_Add2Bkt(hash, first_bkt_index, key_factor, data);
    if (index < 0) {
        second_bkt_index = cuckoohash_SecondBktIndex(hash, key_factor);
        index = cuckoohash_ForceAdd2Bkt(hash, second_bkt_index,
                key_factor, data, 0);
    }

    if (index >= 0) {
        hash->entry_count ++;
    }

    return index;
}

static void cuckoohash_Init(CUCKOO_HASH_S *cuckoo_hash, UINT bucket_num,
        UINT bucket_depth, void *table, UINT table_alloced)
{
    memset(cuckoo_hash, 0, sizeof(CUCKOO_HASH_S));
    memset(table, 0, sizeof(CUCKOO_HASH_NODE_S) * bucket_num * bucket_depth);
    cuckoo_hash->bucket_num = bucket_num;
    cuckoo_hash->bucket_depth = bucket_depth;
    cuckoo_hash->bucket_mask = bucket_num - 1;
    cuckoo_hash->bucket_depth_mask = bucket_depth - 1;
    cuckoo_hash->table = table;
    cuckoo_hash->table_alloced = table_alloced;
}

/* table==NULL表示自动分配 */
int CuckooHash_Init(CUCKOO_HASH_S *cuckoo_hash, CUCKOO_HASH_NODE_S *table,
        UINT bucket_num, UINT bucket_depth)
{
    if (! NUM_IS2N(bucket_num)) {
        BS_DBGASSERT(0);
        RETURN(BS_BAD_PARA);
    }

    if (! NUM_IS2N(bucket_depth)) {
        BS_DBGASSERT(0);
        RETURN(BS_BAD_PARA);
    }

    UINT table_alloced = 0;
    if (table == NULL) {
        table = MEM_Malloc(sizeof(CUCKOO_HASH_NODE_S) * bucket_num * bucket_depth);
        if (table == NULL) {
            BS_WARNNING(("Can't alloc mem"));
            RETURN(BS_NO_MEMORY);
        }
        table_alloced = 1;
    }

    cuckoohash_Init(cuckoo_hash, bucket_num,
            bucket_depth, table, table_alloced);

    return BS_OK;
}

void CuckooHash_Fini(CUCKOO_HASH_S *hash)
{
    if (hash->table_alloced) {
        MEM_Free(hash->table);
    }

    memset(hash, 0, sizeof(CUCKOO_HASH_S));
    return;
}

void CuckooHash_Reinit(CUCKOO_HASH_S *cuckoo_hash)
{
    cuckoohash_Init(cuckoo_hash, cuckoo_hash->bucket_num,
            cuckoo_hash->bucket_depth, cuckoo_hash->table,
            cuckoo_hash->table_alloced);
}

void CuckooHash_SetIndexFunc(CUCKOO_HASH_S *hash, PF_CUCKOO_INDEX index_func)
{
    hash->index_func = index_func;
}

void CuckooHash_SetCmpFunc(CUCKOO_HASH_S *hash, PF_CUCKOO_CMP cmp_func)
{
    hash->cmp_func = cmp_func;
}

/* 查找data,返回索引; 出错返回<0的值 */
int CuckooHash_Find(CUCKOO_HASH_S *hash, void *data)
{
    UINT key_factor = hash->index_func(hash, data);

    return cuckoohash_FindWithKeyFactor(hash, key_factor, data);
}

void * CuckooHash_FindData(CUCKOO_HASH_S *hash, void *data)
{
    int index = CuckooHash_Find(hash, data);
    if (index < 0) {
        return NULL;
    }

    return hash->table[index].data;
}

/* 返回所添加到的index位置,从0开始. 失败返回<0 */
int CuckooHash_Add(CUCKOO_HASH_S *hash, void *data)
{
    UINT key_factor = hash->index_func(hash, data);

    return cuckoohash_AddWithKeyFactor(hash, key_factor, data);
}

void * CuckooHash_DelIndex(CUCKOO_HASH_S *hash, int index)
{
    int bkt_index = index / hash->bucket_depth;
    int last_used_index_in_bkt;
    CUCKOO_HASH_NODE_S *entry = hash->table;
    void *data;

    if (index >= hash->bucket_num * hash->bucket_depth) {
        return NULL;
    }

    if (! entry[index].used) {
        return NULL;
    }

    data = entry[index].data;

    last_used_index_in_bkt = cuukoohash_GetLastUsedInBkt(hash, bkt_index);
    if (last_used_index_in_bkt != index) {
        entry[index] = entry[last_used_index_in_bkt];
    }
    entry[last_used_index_in_bkt].used = 0;

    hash->entry_count --;

    return data;
}

void * CuckooHash_Del(CUCKOO_HASH_S *hash, void *data)
{
    int index;
    void *del_data = NULL;

    index = CuckooHash_Find(hash, data);
    if (index >= 0) {
        del_data = CuckooHash_DelIndex(hash, index);
    }

    return del_data;
}

void * CuckooHash_GetData(CUCKOO_HASH_S *hash, int index)
{
    CUCKOO_HASH_NODE_S *entry = hash->table;

    if (index >= hash->bucket_num * hash->bucket_depth) {
        return NULL;
    }

    if (! entry[index].used) {
        return NULL;
    }

    return entry[index].data;
}

/* 获取下一个, curr=-1表示从头开始, 返回-1表示结束 */
int CuckooHash_GetNext(CUCKOO_HASH_S *hash, int curr)
{
    int index = curr + 1;
    int bkt_index;
    int start_index;
    int end_index;
    int i;
    CUCKOO_HASH_NODE_S *entry = hash->table;

    if (index >= hash->bucket_num * hash->bucket_depth) {
        return -1;
    }

    bkt_index = index / hash->bucket_depth;

    for ( ; bkt_index < hash->bucket_num; bkt_index++) {
        start_index = bkt_index * hash->bucket_depth;
        end_index = start_index + hash->bucket_depth;
        start_index = MAX(start_index, index);
        for (i = start_index; i < end_index; i++) {
            if (entry[i].used) {
                return i;
            }
        }
    }

    return -1;
}

int CuckooHash_Count(CUCKOO_HASH_S *hash)
{
    return hash->entry_count;
}

UINT CuckooHash_GetKeyFactor(CUCKOO_HASH_S *hash, int index)
{
    CUCKOO_HASH_NODE_S *entry = hash->table;

    if (index >= hash->bucket_num * hash->bucket_depth) {
        return 0;
    }

    if (! entry[index].used) {
        return 0;
    }

    return entry[index].key_factor;
}

