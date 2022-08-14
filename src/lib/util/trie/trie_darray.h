/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _TRIE_DARRAY_H
#define _TRIE_DARRAY_H
#include "utl/darray_utl.h"
#include "utl/trie_utl.h"
#ifdef __cplusplus
extern "C"
{
#endif

typedef struct trie_darray_node {
    TRIE_COMMON_S common;
    UINT bits[8];
    DARRAY_HANDLE nexts;
    UCHAR c;
}TRIE_DARRAY_NODE_S;

void * Triedarray_Create();
void Triedarray_Destroy(void *root, PF_TRIE_UD_FREE free_ud);

TRIE_COMMON_S * Triedarray_Insert(TRIE_COMMON_S *cur_common, UCHAR c);
TRIE_COMMON_S * Triedarray_PrefixMatch(void *root, UCHAR *data, int data_len, PF_TRIE_MATCH_CB func, void *ud);

#ifdef __cplusplus
}
#endif
#endif //TRIE_DARRAY_H_
