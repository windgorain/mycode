/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _TRIE_2BITS_H
#define _TRIE_2BITS_H
#include "utl/trie_utl.h"
#ifdef __cplusplus
extern "C"
{
#endif

#define TRIE2B_NUM 4

typedef struct trie2b_node {
    TRIE_COMMON_S common; 
    struct trie2b_node *nexts[TRIE2B_NUM];
}TRIE_2BITS_NODE_S;

void * Trie2b_Create();
void Trie2b_Destroy(void *root, PF_TRIE_UD_FREE free_ud);

TRIE_COMMON_S * Trie2b_Insert(TRIE_COMMON_S *cur_common, UCHAR c);
TRIE_COMMON_S * Trie2b_PrefixMatch(void *root, UCHAR *data, int data_len, PF_TRIE_MATCH_CB func, void *ud);


#ifdef __cplusplus
}
#endif
#endif 
