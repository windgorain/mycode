/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _TRIE_SIMPLE_H
#define _TRIE_SIMPLE_H
#include "utl/trie_utl.h"
#ifdef __cplusplus
extern "C"
{
#endif

#define TRIE_CHAR_NUM 256

typedef struct trie_simple_node {
    TRIE_COMMON_S common; /* 必须为第一个成员 */
    struct trie_simple_node *nexts[TRIE_CHAR_NUM];
}TRIE_SIMPLE_NODE_S;

void * TrieSimple_Create();
void TrieSimple_Destroy(void *root, PF_TRIE_UD_FREE free_ud);

TRIE_COMMON_S * TrieSimple_Insert(TRIE_COMMON_S *cur_common, UCHAR c);
TRIE_COMMON_S * TrieSimple_PrefixMatch(void *root, UCHAR *data, int data_len, PF_TRIE_MATCH_CB func, void *ud);


#ifdef __cplusplus
}
#endif
#endif //TRIE_SIMPLE_H_
