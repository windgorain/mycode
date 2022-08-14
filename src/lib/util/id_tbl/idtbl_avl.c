/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/avllib_utl.h"
#include "utl/idtbl_utl.h"

typedef struct {
    IDTBL_S id_tbl;
    AVL_TREE avl_root;
}IDTBL_AVL_S;

typedef struct {
    AVL_NODE stLinkNode;
    UINT64 id;
    void *data;
}IDTBL_AVL_NODE_S;

static int idtbl_avl_add(void *id_tbl, UINT64 id, void *data);
static void * idtbl_avl_get(void *id_tbl, UINT64 id);
static void * idtbl_avl_del(void *id_tbl, UINT64 id);
static void idtbl_avl_reset(void *id_tbl, PF_IDTBL_FREE free_func, void *ud);
static void idtbl_avl_destroy(void *id_tbl, PF_IDTBL_FREE free_func, void *ud);

static IDTBL_FUNC_S g_idtbl_avl_funcs = {
    idtbl_avl_add,
    idtbl_avl_get,
    idtbl_avl_del,
    idtbl_avl_reset,
    idtbl_avl_destroy
};

IDTBL_S * IDTBL_AvlCreate(UINT max_id)
{
    IDTBL_AVL_S *ctrl;

    ctrl = MEM_ZMalloc(sizeof(IDTBL_AVL_S));
    if (! ctrl) {
        return NULL;
    }

    ctrl->id_tbl.funcs = &g_idtbl_avl_funcs;

    return (void*)ctrl;
}

static int idtbl_avl_cmp(void *key, void *pAvlNode)
{
    IDTBL_AVL_NODE_S *pstNode = pAvlNode;
    UINT64 id = HANDLE_UINT(key);

    if (pstNode->id == id) {
        return 0;
    }

    if (id > pstNode->id) {
        return 1;
    }

    return -1;
}

static int idtbl_avl_add(void *id_tbl, UINT64 id, void *data)
{
    IDTBL_AVL_S *ctrl = id_tbl;
    IDTBL_AVL_NODE_S *pstNode;

    pstNode = MEM_ZMalloc(sizeof(IDTBL_AVL_NODE_S));
    if (NULL == pstNode) {
        RETURN(BS_NO_MEMORY);
    }

    pstNode->id = id;
    pstNode->data = data;

    int ret = avlInsert(&ctrl->avl_root, pstNode, UINT_HANDLE(id), idtbl_avl_cmp);
    if (ret != 0) {
        MEM_Free(pstNode);
        return ret;
    }

    return 0;
}

static void * idtbl_avl_get(void *id_tbl, UINT64 id)
{
    IDTBL_AVL_S *ctrl = id_tbl;
    IDTBL_AVL_NODE_S *node;

    node = avlSearch(ctrl->avl_root, UINT_HANDLE(id), idtbl_avl_cmp);
    if (! node) {
        return NULL;
    }

    return node->data;
}

static void * idtbl_avl_del(void *id_tbl, UINT64 id)
{
    IDTBL_AVL_S *ctrl = id_tbl;
    void *data = NULL;
    IDTBL_AVL_NODE_S *pstNode;

    pstNode = avlDelete(&ctrl->avl_root, UINT_HANDLE(id), idtbl_avl_cmp);
    if (pstNode) {
        data = pstNode->data;
        MEM_Free(pstNode);
    }

    return data;
}

static void idtbl_avl_reset(void *id_tbl, PF_IDTBL_FREE free_func, void *ud)
{
    IDTBL_AVL_S *ctrl = id_tbl;
    avlTreeErase(&ctrl->avl_root, free_func, ud);
}

static void idtbl_avl_destroy(void *id_tbl, PF_IDTBL_FREE free_func, void *ud)
{
    IDTBL_AVL_S *ctrl = id_tbl;
    avlTreeErase(&ctrl->avl_root, free_func, ud);
    MEM_Free(ctrl);
}

