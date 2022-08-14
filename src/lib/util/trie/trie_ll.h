/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _TRIE_LL_H
#define _TRIE_LL_H

#include "utl/list_sl.h"
#include "utl/trie_utl.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct trie_ll_node {
    TRIE_COMMON_S common; /* 必须为第一个成员 */
    SL_NODE_S link_node;
    SL_HEAD_S next;
    UINT bits[8];
    UCHAR c;
}TRIE_LL_NODE_S;

void * Triell_Create();
void Triell_Destroy(void *root, PF_TRIE_UD_FREE free_ud);

TRIE_COMMON_S * Triell_Insert(TRIE_COMMON_S *cur_common, UCHAR c);
TRIE_COMMON_S * Triell_PrefixMatch(void *root, UCHAR *data, int data_len, PF_TRIE_MATCH_CB func, void *ud);

#ifdef __cplusplus
}
#endif
#endif //TRIE_LL_H_
