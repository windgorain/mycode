/*================================================================
*   Created by LiXingang
*   Description: link list trie
*                bitmap + link list的trie树
*
================================================================*/
#include "bs.h"
#include "utl/bit_opt.h"
#include "utl/array_bit.h"
#include "trie_ll.h"

static inline TRIE_LL_NODE_S * triell_get_node(TRIE_LL_NODE_S *this_node, UCHAR c)
{
    SL_NODE_S *link_node;
    TRIE_LL_NODE_S *node;

    if (! ArrayBit_Test(this_node->bits, c)) {
        return NULL;
    }

    for (link_node = SL_First(&this_node->next); link_node; link_node = SL_Next(link_node)) {
        node = container_of(link_node, TRIE_LL_NODE_S, link_node);
        if (node->c == c) {
            return node;
        }
    }

    return NULL;
}

static inline TRIE_LL_NODE_S * triell_create_node(TRIE_LL_NODE_S *this_node, UCHAR c)
{
    TRIE_LL_NODE_S *node;

    node = MEM_ZMalloc(sizeof(TRIE_LL_NODE_S));
    if (! node) {
        return NULL;
    }

    node->c = c;
    SL_AddHead(&this_node->next, &node->link_node);
    ArrayBit_Set(this_node->bits, c);

    return node;
}

static void triell_free_nexts(TRIE_LL_NODE_S *this_node, PF_TRIE_UD_FREE free_ud)
{
    SL_NODE_S *link_node;
    TRIE_LL_NODE_S *node;

    link_node = SL_First(&this_node->next);
    while (link_node) {
        node = container_of(link_node, TRIE_LL_NODE_S, link_node);
        triell_free_nexts(node, free_ud);
        link_node = SL_Next(link_node);
        if ((node->common.ud) && (free_ud != NULL)) {
            free_ud(node->common.ud);
        }
        MEM_Free(node);
    }
    SL_Init(&this_node->next);
    memset(this_node->bits, 0, sizeof(this_node->bits));
}

void * Triell_Create()
{
    return MEM_ZMalloc(sizeof(TRIE_LL_NODE_S));
}

void Triell_Destroy(void *root, PF_TRIE_UD_FREE free_ud)
{
    triell_free_nexts(root, free_ud);
    MEM_Free(root);
}

TRIE_COMMON_S * Triell_Insert(TRIE_COMMON_S *cur_common, UCHAR c)
{
    TRIE_LL_NODE_S *cur = (void*) cur_common;

    void *next = triell_get_node(cur, c);
    if (next) {
        return next;
    }

    return (void*) triell_create_node(cur, c);
}


TRIE_COMMON_S * Triell_PrefixMatch(void *root, UCHAR *data, int data_len, PF_TRIE_MATCH_CB func, void *ud)
{
    TRIE_COMMON_S *match = NULL;
    TRIE_COMMON_S *matched;
    UCHAR *read;
    UCHAR *end = data + data_len;
    int matched_len = 0;
    TRIE_LL_NODE_S *found;

    if (data_len <= 0) {
        return NULL;
    }

    found = root;
    read = data;

    while (read < end) {
        found = triell_get_node(found, *read);
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

