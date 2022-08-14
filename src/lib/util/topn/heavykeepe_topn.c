/*================================================================
*   Created by LiXingang
*   Description: 模糊计算topn
*
================================================================*/
#include <math.h>
#include "bs.h"
#include "utl/num_utl.h"
#include "utl/box_utl.h"
#include "utl/jhash_utl.h"
#include "utl/rand_utl.h"
#include "utl/heavykeep_topn.h"

/*pow(1.08,n), n最大288，计算概率*/
static UINT g_topn_topn_index[]={
1,1,1,1,1,1,1,1,1,1,
2,2,2,2,2,3,3,3,3,4,
4,5,5,5,6,6,7,7,8,9,
10,10,11,12,13,14,15,17,18,20,
21,23,25,27,29,31,34,37,40,43,
46,50,54,59,63,68,74,80,86,93,
101,109,118,127,137,148,160,173,187,202,
218,236,254,275,297,321,346,374,404,436,
471,509,550,594,642,693,748,808,873,943,
1018,1100,1188,1283,1386,1497,1616,1746,1885,2036,
2199,2375,2565,2771,2992,3232,3490,3770,4071,4397,
4749,5129,5539,5982,6461,6978,7536,8139,8790,9493,
10252,11073,11959,12915,13949,15065,16270,17571,18977,20495,
22135,23906,25818,27884,30115,32524,35126,37936,40971,44248,
47788,51611,55740,60200,65016,70217,75834,81901,88453,95529,
103172,111426,120340,129967,140364,151594,163721,176819,190964,206242,
222741,240560,259805,280589,303037,327280,353462,381739,412278,445261,
480881,519352,560900,605772,654234,706573,763099,824147,890078,961285,
1038187,1121242,1210942,1307817,1412443,1525438,1647473,1779271,1921613,2075342,
2241369,2420679,2614333,2823480,3049359,3293307,3556772,3841314,4148619,4480508,
4838949,5226065,5644150,6095682,6583337,7110004,7678804,8293109,8956557,9673082,
10446929,11282683,12185298,13160122,14212931,15349966,16577963,17904200,19336536,20883459,
22554136,24358467,26307144,28411716,30684653,33139426,35790580,38653826,41746132,45085823,
48692689,52588104,56795152,61338765,66245866,71545535,77269178,83450712,90126769,97336911,
105123864,113533773,122616475,132425793,143019856,154461445,166818360,180163829,194576936,210143091,
226954538,245110901,264719773,285897355,308769143,333470675,360148329,388960195,420077011,453683172,
489977826,529176052,571510136,617230947,666609423,719938177,777533231,839735889,906914760,979467941,
1057825377,1142451407,1233847519,1332555321,1439159747,1554292526,1678635929,1812926803,1957960947,2114597823,
2283765649,2466466901,2663784253,2876886993,3107037953,3355600989,3624049068,3913972994,4227090833};

int HeavyKeep_Topn_Create(INOUT HEAVYKEEP_TOPN_S *topn, int hash_size, int key_size, int cycle)
{
    void *mem;

    if (! NUM_IS2N(hash_size)) {
        BS_WARNNING(("The heavy keep topn hash size is not 2^n"));
        RETURN(BS_BAD_PARA);
    }

    mem = MEM_ZMalloc(HEAVYKEEP_TOPN_CTRL_MEM_SIZE(key_size, hash_size));
    if (! mem) {
        RETURN(BS_NO_MEMORY);
    }
    topn->hash = mem;
    topn->hash_size = hash_size;
    topn->key_size = key_size;
    topn->cycle = cycle;

    return 0;
}

void HeavyKeep_Topn_Destroy(HEAVYKEEP_TOPN_S *topn)
{
    if (topn->hash) {
        MEM_Free(topn->hash);
        topn->hash = NULL;
    }
}

void HeavyKeep_Topn_Reset(HEAVYKEEP_TOPN_S *topn)
{
    memset(topn->hash, 0, HEAVYKEEP_TOPN_CTRL_MEM_SIZE(topn->key_size, topn->hash_size));
    return;
}


static void _heavykeep_add_hashnode(HEAVYKEEP_TOPN_NODE_S*node, void* key, int key_length, int value)
{
    memcpy(node->key, key, key_length);
    node->freq = 1;
    node->score = value;
}

static HEAVYKEEP_TOPN_NODE_S* _heavykeep_refresh(HEAVYKEEP_TOPN_NODE_S* node,
        void* key, int key_length, int value, UINT factor)
{
    if(node->freq > 288) {
        return NULL;
    }
    UINT x = g_topn_topn_index[node->freq];

    if(RAND_Get() % x == 0) {
        node->freq--;
        if(node->freq == 0) {
            memcpy(node->key, key, key_length);
            node->score=value;
            node->freq = 1;
            node->factor = factor;
            return node;
        }
    }
    
    return NULL;
}

static inline HEAVYKEEP_TOPN_NODE_S * _heavykeep_get_node(HEAVYKEEP_TOPN_S *topn, int index)
{
    char *ptr = (void*)topn->hash;
    return (void*) (ptr + (sizeof(HEAVYKEEP_TOPN_NODE_S) + topn->key_size) * index);
}

static inline void _heavykeep_build_hash(UINT mask, UINT hash_factor, OUT UINT *hash)
{
    BS_DBGASSERT(HEAVYKEEP_TOPN_HASH_NUM == 4);

    hash[0] = hash_factor & mask;
    hash[1] = (hash_factor >> 16) & mask;
    hash[2] = (hash_factor ^ (hash_factor >> 8)) & mask;
    hash[3] = (hash_factor ^ (hash_factor >> 16)) & mask;
}

/* score with hash factor */
void HeavyKeep_Topn_FScore(HEAVYKEEP_TOPN_S *topn, void *key, int key_len, UINT hash_factor, UINT score)
{
    int i;
    unsigned int index;
    UINT mask = topn->hash_size - 1;
    UINT hash_index[HEAVYKEEP_TOPN_HASH_NUM];
    HEAVYKEEP_TOPN_NODE_S * node = NULL;

    BS_DBGASSERT(key_len == topn->key_size);

    _heavykeep_build_hash(mask, hash_factor, hash_index);

    for(i=0; i<topn->cycle; i++) {
        index = hash_index[i];
        node = _heavykeep_get_node(topn, index);
        if(node->freq==0) {
            _heavykeep_add_hashnode(node, key, key_len, score);
            node->factor = hash_factor;
        }else if (node->factor == hash_factor)  {
            node->freq++;
            node->score += score;
        }else {
            _heavykeep_refresh(node, key, key_len, score, hash_factor);
        }
    }

	topn->freq++;
	topn->score += score;

    return;
}

void HeavyKeep_Topn_Score(HEAVYKEEP_TOPN_S *topn, void *key, int key_len, UINT score)
{
    UINT jhash;

    jhash = JHASH_GeneralBuffer(key, key_len, 0);
    HeavyKeep_Topn_FScore(topn, key, key_len, jhash, score);
   
    return;
}

static void heavykeep_topn_add_result(OUT HEAVYKEEP_TOPN_RESULT_S *result, HEAVYKEEP_TOPN_NODE_S *node)
{
    int i, min_index;
    HEAVYKEEP_TOPN_NODE_S *node2;
    UINT64 min_freq;

    /* 判断是否已经存在 */
    for (i=0; i<result->current_num; i++) {
        node2 = result->nodes[i];
        if (node->factor == node2->factor) {
            if(node->freq > node2->freq) {
                node2->freq = node->freq;
            }
            return;
        }
    }

    /* 如果不满,则直接添加 */
    if (result->current_num < result->n) {
        result->nodes[result->current_num] = node;
        result->current_num ++;
        return;
    }

    /* 替换掉最小的那个 */
    min_index = 0;
    min_freq = result->nodes[0]->freq;
    for (i=1; i<result->current_num; i++) {
        if (min_freq > result->nodes[i]->freq) {
            min_freq = result->nodes[i]->freq;
            min_index = i;
        }
    }

    if (node->freq > result->nodes[min_index]->freq) {
        result->nodes[min_index] = node;
    }

    return;
}

int HeavyKeep_Topn_Get(HEAVYKEEP_TOPN_S *topn, OUT HEAVYKEEP_TOPN_RESULT_S *result)
{
    int i;
    HEAVYKEEP_TOPN_NODE_S * node = NULL;

    result->current_num = 0;

    for (i=0; i<topn->hash_size; i++) {
        node = _heavykeep_get_node(topn, i);
        if (node->freq == 0) {
            continue;
        }
        heavykeep_topn_add_result(result, node);
    }

    return result->current_num;
}

VOID HeavyKeep_Topn_SetCycle(HEAVYKEEP_TOPN_S* topn, int cycle) 
{
    if (cycle > HEAVYKEEP_TOPN_HASH_NUM) {
        cycle = HEAVYKEEP_TOPN_HASH_NUM;
    }
    topn->cycle = cycle;
    return;
}
