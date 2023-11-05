/*================================================================
*   Created by LiXingang
*   Description: dynamic array trie
*                bitmap + 动态数组的trie树
*
================================================================*/
#include "bs.h"
#include "utl/bit_opt.h"
#include "utl/array_bit.h"
#include "utl/darray_utl.h"
#include "trie_darray.h"

static inline TRIE_DARRAY_NODE_S * triedarray_get_node(TRIE_DARRAY_NODE_S *this_node, UCHAR c)
{
    TRIE_DARRAY_NODE_S *node;
    int count;
    int i;

    if (! ArrayBit_Test(this_node->bits, c)) {
        return NULL;
    }

    count = DARRAY_GetSize(this_node->nexts);
    for (i=0; i<count; i++) {
        node = DARRAY_Get(this_node->nexts, i);
        if (node && node->c == c) {
            return node;
        }
    }

    return NULL;
}

static inline TRIE_DARRAY_NODE_S * triedarray_create_node(TRIE_DARRAY_NODE_S *this_node, UCHAR c)
{
    TRIE_DARRAY_NODE_S *node;

    node = MEM_ZMalloc(sizeof(TRIE_DARRAY_NODE_S));
    if (! node) {
        return NULL;
    }

    if (! this_node->nexts) {
        this_node->nexts = DARRAY_Create(1, 1);
        if (! this_node->nexts) {
            MEM_Free(node);
            return NULL;
        }
    }

    node->c = c;

    if (DARRAY_INVALID_INDEX == DARRAY_Add(this_node->nexts, node)) {
        MEM_Free(node);
        return NULL;
    }

    ArrayBit_Set(this_node->bits, c);

    return node;
}

static void triedarray_free_nexts(TRIE_DARRAY_NODE_S *this_node, PF_TRIE_UD_FREE free_ud)
{
    TRIE_DARRAY_NODE_S *node;
    UINT count;
    UINT i;

    if (! this_node->nexts) {
        return;
    }

    count = DARRAY_GetSize(this_node->nexts);
    for (i=0; i<count; i++) {
        node = DARRAY_Get(this_node->nexts, i);
        if (node) {
            triedarray_free_nexts(node, free_ud);
            if ((node->common.ud) && (free_ud != NULL)) {
                free_ud(node->common.ud);
            }
            MEM_Free(node);
        }
    }

    DARRAY_Destory(this_node->nexts);
    this_node->nexts = NULL;
    memset(this_node->bits, 0, sizeof(this_node->bits));
}

void * Triedarray_Create()
{
    return MEM_ZMalloc(sizeof(TRIE_DARRAY_NODE_S));
}

void Triedarray_Destroy(void *root, PF_TRIE_UD_FREE free_ud)
{
    triedarray_free_nexts(root, free_ud);
    MEM_Free(root);
}

TRIE_COMMON_S * Triedarray_Insert(TRIE_COMMON_S *cur_common, UCHAR c)
{
    TRIE_DARRAY_NODE_S *cur = (void*) cur_common;

    void *next = triedarray_get_node(cur, c);
    if (next) {
        return next;
    }

    return (void*) triedarray_create_node(cur, c);
}


TRIE_COMMON_S * Triedarray_PrefixMatch(void *root, UCHAR *data, int data_len, PF_TRIE_MATCH_CB func, void *ud)
{
    TRIE_COMMON_S *match = NULL;
    TRIE_COMMON_S *matched;
    UCHAR *read;
    UCHAR *end = data + data_len;
    int matched_len = 0;
    TRIE_DARRAY_NODE_S *found;

    if (data_len <= 0) {
        return NULL;
    }

    found = root;
    read = data;

    while (read < end) {
        found = triedarray_get_node(found, *read);
        if (! found) {
            break;
        }

        matched_len ++;

        if (found->common.flag & TRIE_NODE_FLAG_MATCHED) {
            matched = func(match, &found->common, matched_len, ud);
            if (matched) {
                match = matched;
            }
        }

        read ++;
    }

    return match;
}

