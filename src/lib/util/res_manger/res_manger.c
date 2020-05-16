#include "bs.h"
#include "utl/res_manger.h"


void RESManger_Init(IN RES_MANGER_S *res_manger)
{
    DLL_HEAD_INIT(&res_manger->stLinkList);
}

void RESManger_Add(IN RES_MANGER_S *res_manger, IN RES_MANGER_NODE_S *node)
{
    DLL_ADD(&res_manger->stlinkList, node);
}

void RESManger_Del(IN RES_MANGER_S *res_manger, IN RES_MANGER_NODE_S *node)
{
    DLL_DEL(&res_manger->stLinkList, node);
}

void RESManger_WalkType(IN RES_MANGER_S *res_manger, IN UINT type, IN PF_RES_MANGER_NODE_WALK func, IN void *user_handle) 
{
    RES_MANGER_NODE_S *node, *nodetmp;

    DLL_SAFE_SCAN(&res_manger->stListHead, node, nodetmp) {
        if (node->type == type) {
            func(node, user_handle);
        }
    }
}

void RESManger_Walk(IN RES_MANGER_S *res_manger, IN PF_RES_MANGER_NODE_WALK func, IN void *user_handle)
{
    RES_MANGER_NODE_S *node, *nodetmp;

    DLL_SAFE_SCAN(&res_manger->stListHead, node, nodetmp) {
        func(node, user_handle);
    }
}

void RESManger_Free(IN RES_MANGER_S *res_manger)
{
    RES_MANGER_NODE_S *node, *nodetmp;

    DLL_SAFE_SCAN(&res_manger->stListHead, node, nodetmp) {
        DLL_DEL(&res_manger->stLinkList, node);
        node->pfFreeFunc(node);
    }
}

void RESManger_FreeType(IN RES_MANGER_S *res_manger, IN UINT type)
{
    RES_MANGER_NODE_S *node, *nodetmp;

    DLL_SAFE_SCAN(&res_manger->stListHead, node, nodetmp) {
        if (node->type == type) {
            DLL_DEL(&res_manger->stLinkList, node);
            node->pfFreeFunc(node);
        }
    }
}


