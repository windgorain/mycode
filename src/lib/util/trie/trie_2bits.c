/*================================================================
*   Created by LiXingang
*   Description: 2 bits trie
*
================================================================*/
#include "bs.h"
#include "trie_2bits.h"

static inline TRIE_2BITS_NODE_S * trie2b_try_insert_2bits(TRIE_2BITS_NODE_S *cur, UCHAR c)
{
    if (cur->nexts[c]) {
        return cur->nexts[c];
    }

    TRIE_2BITS_NODE_S *newNode = (void*)MEM_ZMalloc(sizeof(TRIE_2BITS_NODE_S));
    if (! newNode) {
        return NULL;
    }
    cur->nexts[c] = newNode;

    return newNode;
}

static inline TRIE_2BITS_NODE_S * trie2b_find_node(TRIE_2BITS_NODE_S *cur, UCHAR c)
{
    TRIE_2BITS_NODE_S *found;

    found = cur->nexts[(c >> 6) & 0x3];
    if (! found) {
        return NULL;
    }

    found = found->nexts[(c >> 4) & 0x3];
    if (! found) {
        return NULL;
    }

    found = found->nexts[(c >> 2) & 0x3];
    if (! found) {
        return NULL;
    }

    return found->nexts[c & 0x3];
}

static void trie2b_Free(TRIE_2BITS_NODE_S *node, PF_TRIE_UD_FREE free_ud)
{
    int i;

    for (i=0; i<TRIE2B_NUM; i++) {
        if (node->nexts[i] != NULL) {
            trie2b_Free(node->nexts[i], free_ud);
            return;
        }
    }

    if ((node->common.ud) && (free_ud != NULL)) {
        free_ud(node->common.ud);
    }

    MEM_Free(node);
}

static void trie2b_fini(TRIE_2BITS_NODE_S *root, PF_TRIE_UD_FREE free_ud)
{
    int i;

    for (i=0; i<TRIE2B_NUM; i++) {
        if (root->nexts[i] != NULL) {
            trie2b_Free(root->nexts[i], free_ud);
            root->nexts[i] = NULL;
        }
    }
}

void * Trie2b_Create()
{
    return MEM_ZMalloc(sizeof(TRIE_2BITS_NODE_S));
}

void Trie2b_Destroy(void *root, PF_TRIE_UD_FREE free_ud)
{
    trie2b_fini(root, free_ud);
    MEM_Free(root);
}

TRIE_COMMON_S * Trie2b_Insert(TRIE_COMMON_S *cur_common, UCHAR c)
{
    TRIE_2BITS_NODE_S *cur = (void*)cur_common;
    TRIE_2BITS_NODE_S *node;

    node = trie2b_try_insert_2bits(cur, ((c >> 6) & 0x3) );
    if (! node) {
        return NULL;
    }

    node = trie2b_try_insert_2bits(node, ((c >> 4) & 0x3) );
    if (! node) {
        return NULL;
    }

    node = trie2b_try_insert_2bits(node, ((c >> 2) & 0x3) );
    if (! node) {
        return NULL;
    }

    return (void*) trie2b_try_insert_2bits(node, (c & 0x3) );
}

/* 部分匹配后就调用回调,回调决定返回哪个节点. 
 部分匹配:比如,data:abc, 匹配a, ab, abc 三个,都会触发回调*/
TRIE_COMMON_S * Trie2b_PrefixMatch(void *root, UCHAR *data, int data_len, PF_TRIE_MATCH_CB func, void *ud)
{
    TRIE_COMMON_S *match = NULL;
    TRIE_COMMON_S *matched;
    UCHAR *read;
    UCHAR *end = data + data_len;
    int matched_len = 0;
    TRIE_2BITS_NODE_S *found;

    if (data_len <= 0) {
        return NULL;
    }

    found = root;
    read = data;

    while (read < end) {
        found = trie2b_find_node(found, *read);
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


