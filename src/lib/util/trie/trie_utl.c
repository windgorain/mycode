/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/bit_opt.h"
#include "utl/trie_utl.h"
#include "trie_simple.h"
#include "trie_ll.h"
#include "trie_4bits.h"
#include "trie_2bits.h"
#include "trie_darray.h"


typedef void* (*PF_TrieSimple_Create)();
typedef void (*PF_Trie_Destroy)(void *root, PF_TRIE_UD_FREE free_ud);
typedef TRIE_COMMON_S* (*PF_Trie_Insert)(TRIE_COMMON_S *cur, UCHAR c);
typedef TRIE_COMMON_S* (*PF_Trie_PrefixMatch)(void *root, UCHAR *data, int data_len, PF_TRIE_MATCH_CB func, void *ud);

typedef struct {
    PF_TrieSimple_Create create;
    PF_Trie_Destroy destroy;
    PF_Trie_Insert insert;
    PF_Trie_PrefixMatch prefix_match;
}TRIE_FUNC_S;

typedef struct {
    int type;
    void *root;
    TRIE_FUNC_S *func_tbl;
}TRIE_CTRL_S;

typedef struct {
    UCHAR *data;
    int data_len;
}TRIE_MATCH_INFO_S;

static TRIE_COMMON_S * trie_exact_match(TRIE_COMMON_S *old, TRIE_COMMON_S *cur, int matched_len, void *ud);
static TRIE_COMMON_S * trie_max_match(TRIE_COMMON_S *old, TRIE_COMMON_S *cur, int matched_len, void *ud);
static TRIE_COMMON_S * trie_wildcard_match(TRIE_COMMON_S *old, TRIE_COMMON_S *cur, int matched_len, void *ud);
static TRIE_COMMON_S * trie_hostname_match(TRIE_COMMON_S *old, TRIE_COMMON_S *cur, int matched_len, void *ud);

static PF_TRIE_MATCH_CB g_match_types[] = {
    trie_exact_match,
    trie_max_match,
    trie_wildcard_match,
    trie_hostname_match
};

static TRIE_FUNC_S g_trie_simple_func_tbl = {
    .create = TrieSimple_Create,
    .destroy = TrieSimple_Destroy,
    .insert = TrieSimple_Insert,
    .prefix_match = TrieSimple_PrefixMatch
};

static TRIE_FUNC_S g_trie_ll_func_tbl = {
    .create = Triell_Create,
    .destroy = Triell_Destroy,
    .insert = Triell_Insert,
    .prefix_match = Triell_PrefixMatch
};

static TRIE_FUNC_S g_trie_darray_func_tbl = {
    .create = Triedarray_Create,
    .destroy = Triedarray_Destroy,
    .insert = Triedarray_Insert,
    .prefix_match = Triedarray_PrefixMatch
};

static TRIE_FUNC_S g_trie_4bits_func_tbl = {
    .create = Trie4b_Create,
    .destroy = Trie4b_Destroy,
    .insert = Trie4b_Insert,
    .prefix_match = Trie4b_PrefixMatch
};

static TRIE_FUNC_S g_trie_2bits_func_tbl = {
    .create = Trie2b_Create,
    .destroy = Trie2b_Destroy,
    .insert = Trie2b_Insert,
    .prefix_match = Trie2b_PrefixMatch
};

static TRIE_COMMON_S * trie_exact_match(TRIE_COMMON_S *old, TRIE_COMMON_S *cur, int matched_len, void *ud)
{
    TRIE_MATCH_INFO_S *info = ud;
    int data_len = info->data_len;
    
    if (matched_len == data_len) {
        return cur;
    }

    return NULL;
}

static TRIE_COMMON_S * trie_max_match(TRIE_COMMON_S *old, TRIE_COMMON_S *cur, int matched_len, void *ud)
{
    return cur;
}

static TRIE_COMMON_S * trie_wildcard_match(TRIE_COMMON_S *old, TRIE_COMMON_S *cur, int matched_len, void *ud)
{
    TRIE_MATCH_INFO_S *info = ud;

    if (matched_len == info->data_len) { /* 完全匹配了 */
        return cur;
    }

    if (cur->flag & TRIE_NODE_FLAG_WILDCARD) { /* 匹配上了通配 */
        return cur;
    }

    return NULL;
}

static TRIE_COMMON_S * trie_hostname_match(TRIE_COMMON_S *old, TRIE_COMMON_S *cur, int matched_len, void *ud)
{
    TRIE_MATCH_INFO_S *info = ud;

    if (matched_len == info->data_len) { /* 完全匹配了 */
        return cur;
    }

    if (info->data[matched_len + 1] == '.') {
        return cur;
    }

    return NULL;
}

static void * trie_dft_set_ud(void *old_ud, void *uh)
{
    return uh;
}

TRIE_HANDLE Trie_Create(int type)
{
    TRIE_CTRL_S *ctrl;

    ctrl = MEM_ZMalloc(sizeof(TRIE_CTRL_S));
    if (! ctrl) {
        return NULL;
    }

    ctrl->type = type;

    switch (type) {
        case TRIE_TYPE_SIMPLE:
            ctrl->func_tbl = &g_trie_simple_func_tbl;
            break;
        case TRIE_TYPE_LL:
            ctrl->func_tbl = &g_trie_ll_func_tbl;
            break;
        case TRIE_TYPE_DARRAY:
            ctrl->func_tbl = &g_trie_darray_func_tbl;
            break;
        case TRIE_TYPE_4BITS:
            ctrl->func_tbl = &g_trie_4bits_func_tbl;
            break;
        case TRIE_TYPE_2BITS:
            ctrl->func_tbl = &g_trie_2bits_func_tbl;
            break;
        default:
            BS_DBGASSERT(0);
            MEM_Free(ctrl);
            return NULL;
    }

    ctrl->root = ctrl->func_tbl->create();
    if (! ctrl->root) {
        MEM_Free(ctrl);
        return NULL;
    }

    return ctrl;
}

void Trie_Destroy(TRIE_HANDLE trie_handle, PF_TRIE_UD_FREE free_ud)
{
    TRIE_CTRL_S *ctrl = trie_handle;
    ctrl->func_tbl->destroy(ctrl->root, free_ud);
    MEM_Free(ctrl);
}

TRIE_COMMON_S * Trie_InsertEx(TRIE_HANDLE trie_handle, UCHAR *data, int data_len, UINT flag,
        PF_TRIE_UD_SET ud_set, void * uh)
{
    TRIE_CTRL_S *ctrl = trie_handle;
    void *root = ctrl->root;
    TRIE_COMMON_S *cur;
    int i;

    if (data_len <= 0) {
        return NULL;
    }

    cur=root;
    for(i=0; i<data_len; i++) {
        cur = ctrl->func_tbl->insert(cur, data[i]);
        if (! cur) {
            return NULL;
        }
    }

    cur->ud = ud_set(cur->ud, uh);
    cur->flag |= TRIE_NODE_FLAG_MATCHED;
    cur->flag |= flag;

    return cur;
}

TRIE_COMMON_S * Trie_Insert(TRIE_HANDLE trie_handle, UCHAR *data, int data_len, UINT flag, void * ud)
{
    return Trie_InsertEx(trie_handle, data, data_len, flag, trie_dft_set_ud, ud);
}

/* 返回原来的ud */
void * Trie_Del(TRIE_HANDLE trie_handle, UCHAR *data, int data_len)
{
    TRIE_COMMON_S *common;
    void *ud;

    common = Trie_Match(trie_handle, data, data_len, TRIE_MATCH_EXACT);
    if (! common) {
        return NULL;
    }

    common->flag = 0;
    ud = common->ud;
    common->ud = NULL;

    return ud;
}

/* 部分匹配后就调用回调,回调决定返回哪个节点. 
 部分匹配:比如,data:abc, 匹配a, ab, abc 三个,都会触发回调*/
TRIE_COMMON_S * Trie_PrefixMatch(TRIE_HANDLE trie_handle, UCHAR *data, int data_len, PF_TRIE_MATCH_CB func, void *ud)
{
    TRIE_CTRL_S *ctrl = trie_handle;
    return ctrl->func_tbl->prefix_match(ctrl->root, data, data_len, func, ud);
}

/* match_type: TRIE_MATCH_x */
TRIE_COMMON_S * Trie_Match(TRIE_HANDLE trie_handle, UCHAR *data, int data_len, int match_type)
{
    TRIE_MATCH_INFO_S info;

    BS_DBGASSERT(match_type < TRIE_MATCH_INVALID);

    info.data = data;
    info.data_len = data_len;

    return Trie_PrefixMatch(trie_handle, data, data_len, g_match_types[match_type], &info);
}

