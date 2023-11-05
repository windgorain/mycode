/*================================================================
*   Created by LiXingang
*   Description: 最简单版本的trie
*
================================================================*/
#include "bs.h"
#include "trie_simple.h"

static void trie_simple_Free(TRIE_SIMPLE_NODE_S *node, PF_TRIE_UD_FREE free_ud)
{
    int i;

    for (i=0; i<TRIE_CHAR_NUM; i++) {
        if (node->nexts[i] != NULL) {
            trie_simple_Free(node->nexts[i], free_ud);
            return;
        }
    }

    if ((node->common.ud) && (free_ud != NULL)) {
        free_ud(node->common.ud);
    }

    MEM_Free(node);
}

static void trie_simple_fini(TRIE_SIMPLE_NODE_S *root, PF_TRIE_UD_FREE free_ud)
{
    int i;

    for (i=0; i<TRIE_CHAR_NUM; i++) {
        if (root->nexts[i] != NULL) {
            trie_simple_Free(root->nexts[i], free_ud);
            root->nexts[i] = NULL;
        }
    }
}

void * TrieSimple_Create()
{
    return MEM_ZMalloc(sizeof(TRIE_SIMPLE_NODE_S));
}

void TrieSimple_Destroy(void *root, PF_TRIE_UD_FREE free_ud)
{
    trie_simple_fini(root, free_ud);
    MEM_Free(root);
}

TRIE_COMMON_S * TrieSimple_Insert(TRIE_COMMON_S *cur_common, UCHAR c)
{
    TRIE_SIMPLE_NODE_S *cur = (void*)cur_common;

    if (! cur->nexts[c]) {
        cur->nexts[c] = MEM_ZMalloc(sizeof(TRIE_SIMPLE_NODE_S));
    }

    return (void*) cur->nexts[c];
}


TRIE_COMMON_S * TrieSimple_PrefixMatch(void *root, UCHAR *data, int data_len, PF_TRIE_MATCH_CB func, void *ud)
{
    TRIE_COMMON_S *match = NULL;
    TRIE_COMMON_S *matched;
    UCHAR *read;
    UCHAR *end = data + data_len;
    int matched_len = 0;
    TRIE_SIMPLE_NODE_S *found;

    if (data_len <= 0) {
        return NULL;
    }

    found = root;
    read = data;

    while (read < end) {
        found = found->nexts[*read];
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

