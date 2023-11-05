/*================================================================
*   Created by LiXingang
*   Description: 模糊计算topn
*
================================================================*/
#include "bs.h"
#include "utl/num_utl.h"
#include "utl/box_utl.h"

#include "utl/vague_topn.h"

static VAGUE_TOPN_NODE_S * vague_topn_get_node(VAGUE_TOPN_RESULT_S *result,
        int index)
{
    char *nodes_mem = (char *)(void*)result->nodes;

    return (void*)(nodes_mem + (sizeof(VAGUE_TOPN_NODE_S)
                + result->key_size) * index);
}

static VAGUE_TOPN_NODE_S * vague_topn_request_node(VAGUE_TOPN_RESULT_S *result)
{
    VAGUE_TOPN_NODE_S *node;

    if (result->current_num >= result->n) {
        return NULL;
    }

    node = vague_topn_get_node(result, result->current_num);
    result->current_num ++;

    return node;
}

static inline void vaguetopn_ResultInit(VAGUE_TOPN_RESULT_S *result,
        int key_size, int n, void *nodes)
{
    result->n = n;
    result->key_size = key_size;
    result->nodes = nodes;
    result->current_num = 0;
}

static UINT vaguetopn_BoxIndex(void *hash, void *data)
{
    VAGUE_TOPN_NODE_S *node = data;

    return node->hash_factor;
}

static int vaguetopn_BoxCmp(void *hash, void *data1, void *data2)
{
    VAGUE_TOPN_NODE_S *node1 = data1;
    VAGUE_TOPN_NODE_S *node2 = data2;

    return (int)(node1->hash_factor - node2->hash_factor);
}

static inline int vaguetopn_ResultCreate(VAGUE_TOPN_RESULT_S *result,
        int key_size, int n)
{
    void *nodes;

    nodes = MEM_Malloc(VAGUE_TOPN_RESULT_MEM_SIZE(n, key_size));
    if (! nodes) {
        RETURN(BS_NO_MEMORY);
    }

    vaguetopn_ResultInit(result, key_size, n, nodes);
    RawBox_Init(&result->box, NULL, 1024, 8,
            vaguetopn_BoxIndex, vaguetopn_BoxCmp);

    return 0;
}

static inline void vaguetopn_ResultDestroy(VAGUE_TOPN_RESULT_S *result)
{
    if (result->nodes) {
        MEM_Free(result->nodes);
        result->nodes = NULL;
        Box_Fini(&result->box, NULL, NULL);
    }

    memset(result, 0, sizeof(VAGUE_TOPN_RESULT_S));
}

static inline int vaguetopn_Init(INOUT VAGUE_TOPN_S *topn,
        int hash_size, void *mem)
{
    int i;
    unsigned int *hash = mem;

    if (! NUM_IS2N(hash_size)) {
        RETURN(BS_BAD_PARA);
    }

    for (i=0; i<VAGUE_TOPN_LEVEL_NUM; i++) {
        topn->hash[i] = hash;
        hash = hash + hash_size;
    }

    topn->pass_line = 1;
    topn->hash_size = hash_size;
    topn->mask = hash_size - 1;

    return 0;
}

int VagueTopn_Create(INOUT VAGUE_TOPN_S *topn,
        int hash_size, int key_size, int n)
{
    void *mem;

    if (! NUM_IS2N(hash_size)) {
        BS_WARNNING(("The vague topn hash size is not 2^n"));
        RETURN(BS_BAD_PARA);
    }

    mem = MEM_Malloc(VAGUE_TOPN_CTRL_MEM_SIZE(hash_size));
    if (! mem) {
        RETURN(BS_NO_MEMORY);
    }

    if (0 != vaguetopn_Init(topn, hash_size, mem)) {
        VagueTopn_Destroy(topn);
        RETURN(BS_ERR);
    }

    if (BS_OK != vaguetopn_ResultCreate(&topn->result, key_size, n)) {
        VagueTopn_Destroy(topn);
        RETURN(BS_NO_MEMORY);
    }

    return 0;
}

void VagueTopn_Destroy(VAGUE_TOPN_S *topn)
{
    vaguetopn_ResultDestroy(&topn->result);

    if (topn->hash[0]) {
        MEM_Free(topn->hash[0]);
        topn->hash[0] = NULL;
    }
}

void VagueTopn_Reset(VAGUE_TOPN_S *topn)
{
    int i;
    for (i=0; i<VAGUE_TOPN_LEVEL_NUM; i++) {
        memset(topn->hash[i], 0, sizeof(unsigned int) * topn->hash_size);
    }

    VAGUE_TOPN_RESULT_S *result = &topn->result;

    Box_DelAll(&result->box, NULL ,NULL);
    vaguetopn_ResultInit(result, result->key_size, result->n, result->nodes);
}

static void vague_topn_adjust_pass_line(VAGUE_TOPN_S *topn)
{
    if (topn->pass_line == 0) {
        topn->pass_line = 1;
    } else if (topn->pass_line >= 0x10000000) {
        topn->pass_line = 0xffffffff;
    } else {
        topn->pass_line = topn->pass_line << 1;
    }

    topn->epoch ++;
}

static VAGUE_TOPN_NODE_S * vaguetopn_GetMin(VAGUE_TOPN_S *topn)
{
    VAGUE_TOPN_RESULT_S *result = &topn->result;
    VAGUE_TOPN_NODE_S *min_node = vague_topn_get_node(result, 0);
    int i;

    
    for (i=1; i<result->current_num; i++) {
        VAGUE_TOPN_NODE_S *node = vague_topn_get_node(result, i);
        if ((node->epoch != topn->epoch) && (node->freq <= min_node->freq)) {
            min_node = node;
        }
    }

    return min_node;
}

void VagueTopn_Score(VAGUE_TOPN_S *topn, void *key, int score, UINT hash_factor)
{
    int i;
    VAGUE_TOPN_RESULT_S *result = &topn->result;
    VAGUE_TOPN_NODE_S *found_node = NULL;
    unsigned int index;

    
    for (i=0; i<VAGUE_TOPN_LEVEL_NUM; i++) {
        index = hash_factor >> (i*4);
        index &= topn->mask;
        if (topn->hash[i][index] < topn->pass_line) {
            topn->hash[i][index] += score;
            if (topn->hash[i][index] < topn->pass_line) {
                return;
            }
        }
    }

    
    VAGUE_TOPN_NODE_S to_find;
    to_find.hash_factor = hash_factor;
    found_node = Box_FindData(&topn->result.box, &to_find);
    if (found_node) {
        found_node->freq ++;
        found_node->score += score;
        found_node->epoch = topn->epoch;
        return;
    } 

    
    found_node = vague_topn_request_node(result);
    if (found_node == NULL) {
        
        found_node = vaguetopn_GetMin(topn);
        if (found_node) {
            Box_Del(&result->box, found_node);
        }
    }

    if (found_node) {
        found_node->freq = 1;
        found_node->score = score;
        found_node->epoch = topn->epoch;
        found_node->hash_factor = hash_factor;
        memcpy(found_node->key, key, result->key_size);
        Box_Add(&result->box, found_node);
        return;
    }

    
    vague_topn_adjust_pass_line(topn);

    return;
}

VAGUE_TOPN_NODE_S * VagueTopn_GetByIndex(VAGUE_TOPN_S *topn, int index)
{
    VAGUE_TOPN_RESULT_S *result = &topn->result;
    if (index >= result->current_num) {
        return NULL;
    }

    return vague_topn_get_node(result, index);
}

