/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _TRIE_4BITS_H
#define _TRIE_4BITS_H
#include "utl/trie_utl.h"
#ifdef __cplusplus
extern "C"
{
#endif

#define TRIE4B_NUM 16

typedef struct trie4b_node {
    TRIE_COMMON_S common;
    struct trie4b_node *nexts[TRIE4B_NUM];
}TRIE_4BITS_NODE_S;

void * Trie4b_Create();
void Trie4b_Destroy(void *root, PF_TRIE_UD_FREE free_ud);

TRIE_COMMON_S * Trie4b_Insert(TRIE_COMMON_S *cur_common, UCHAR c);
TRIE_COMMON_S * Trie4b_PrefixMatch(void *root, UCHAR *data, int data_len, PF_TRIE_MATCH_CB func, void *ud);


#ifdef __cplusplus
}
#endif
#endif 
