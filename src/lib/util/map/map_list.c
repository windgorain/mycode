/******************************************************************************
* Copyright (C), 2000-2006,  Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2017-6-3
* Description: list map
* History: rename from aggregate to map
******************************************************************************/
#include "bs.h"
#include "utl/jhash_utl.h"
#include "utl/mem_utl.h"
#include "utl/mem_cap.h"
#include "utl/map_utl.h"

typedef struct
{
    DLL_HEAD_S list;
    UINT count;
}_MAP_LIST_S;

typedef struct
{
    DLL_NODE_S list_node;
    MAP_ELE_S stEle;
}_MAP_LIST_NODE_S;

static void map_list_destroy(MAP_HANDLE map, PF_MAP_FREE_FUNC free_func, void *ud);
static void map_list_reset(MAP_HANDLE map, PF_MAP_FREE_FUNC free_func, void *ud);
static BS_STATUS map_list_add(MAP_HANDLE map, VOID *pKey, UINT uiKeyLen, VOID *pData, UINT flag);
static MAP_ELE_S * map_list_get_ele(MAP_HANDLE map, void *key, UINT key_len);
static void * map_list_get(IN MAP_HANDLE map, IN VOID *pKey, IN UINT uiKeyLen);
static void * map_list_del(IN MAP_HANDLE map, IN VOID *pKey, IN UINT uiKeyLen);
static void map_list_del_all(MAP_HANDLE map, PF_MAP_FREE_FUNC func, void * pUserData);
static UINT map_list_count(MAP_HANDLE map);
static void map_list_walk(IN MAP_HANDLE map, IN PF_MAP_WALK_FUNC pfWalkFunc, IN VOID *pUserData);
static MAP_ELE_S * map_list_getnext(MAP_HANDLE map, MAP_ELE_S *pstCurrent);

static MAP_FUNC_S g_map_list_funcs = {
    .destroy_func = map_list_destroy,
    .reset_func = map_list_reset,
    .add_func = map_list_add,
    .get_ele_func = map_list_get_ele,
    .get_func = map_list_get,
    .del_func = map_list_del,
    .del_all_func = map_list_del_all,
    .count_func = map_list_count,
    .walk_func = map_list_walk,
    .getnext_func = map_list_getnext
};

static int _map_list_cmp(MAP_ELE_S *key, void * pstCmpNode)
{
    _MAP_LIST_NODE_S *pstNode = pstCmpNode;

    if (pstNode->stEle.uiKeyLen == 0) {
        /* keylen==0, 则表示key本身是数字,不是指针 */
        return (INT)HANDLE_UINT(key->pKey) - (INT)HANDLE_UINT(pstNode->stEle.pKey);
    }

    return MEM_Cmp(key->pKey, key->uiKeyLen, pstNode->stEle.pKey, pstNode->stEle.uiKeyLen);
}

int _map_list_cmp_list_node(DLL_NODE_S *pstNode1, DLL_NODE_S *pstNode2, HANDLE hUserHandle)
{
    _MAP_LIST_NODE_S *node1 = (void*)pstNode1;
    _MAP_LIST_NODE_S *node2 = (void*)pstNode2;

    if (node1->stEle.uiKeyLen == 0) {
        /* keylen==0, 则表示key本身是数字,不是指针 */
        return (INT)HANDLE_UINT(node1->stEle.pKey) - (INT)HANDLE_UINT(node2->stEle.pKey);
    }

    return MEM_Cmp(node1->stEle.pKey, node1->stEle.uiKeyLen,
            node2->stEle.pKey, node2->stEle.uiKeyLen);
}

static _MAP_LIST_NODE_S * _map_list_find(IN MAP_CTRL_S *map, IN VOID *pKey, IN UINT uiKeyLen)
{
    _MAP_LIST_S *list_map = map->impl_map;
    MAP_ELE_S ele;
    _MAP_LIST_NODE_S *node;
    int ret;

    ele.pKey = pKey;
    ele.uiKeyLen = uiKeyLen;

    DLL_SCAN(&list_map->list, node) {
        ret = _map_list_cmp(&ele, node);
        if (ret == 0) {
            return node;
        } else if (ret < 0) {
            break;
        }
    }

    return NULL;
}

static _MAP_LIST_NODE_S * _map_list_next_dict(MAP_CTRL_S *map, MAP_ELE_S *ele)
{
    _MAP_LIST_S *list_map = map->impl_map;
    _MAP_LIST_NODE_S *node;

    DLL_SCAN(&list_map->list, node) {
        if (_map_list_cmp(ele, node) < 0) {
            return node;
        }
    }

    return NULL;
}

static void map_list_reset(MAP_HANDLE map, PF_MAP_FREE_FUNC free_func, void *ud)
{
    map_list_del_all(map, free_func, ud);
}

static void map_list_destroy(MAP_HANDLE map, PF_MAP_FREE_FUNC free_func, void *ud)
{
    _MAP_LIST_S *list_map = map->impl_map;

    map_list_del_all(map, free_func, ud);
    MemCap_Free(map->memcap, list_map);
    MemCap_Free(map->memcap, map);
}

static BS_STATUS map_list_add(MAP_HANDLE map, VOID *pKey, UINT uiKeyLen, VOID *pData, UINT flag)
{
    _MAP_LIST_S *list_map = map->impl_map;
    _MAP_LIST_NODE_S *pstNode;

    if ((map->uiCapacity != 0) && (map->uiCapacity <= list_map->count)) {
        RETURN(BS_NO_RESOURCE);
    }

    pstNode = MemCap_ZMalloc(map->memcap, sizeof(_MAP_LIST_NODE_S));
    if (NULL == pstNode) {
        RETURN(BS_NO_MEMORY);
    }

    if ((flag & MAP_FLAG_DUP_KEY) && (uiKeyLen != 0)) {
        pstNode->stEle.pKey = MemCap_Dup(map->memcap, pKey, uiKeyLen);
        if (NULL == pstNode->stEle.pKey) {
            MemCap_Free(map->memcap, pstNode);
            RETURN(BS_NO_MEMORY);
        }
        pstNode->stEle.dup_key = 1;
    } else {
        pstNode->stEle.pKey = pKey;
    }

    pstNode->stEle.uiKeyLen = uiKeyLen;
    pstNode->stEle.pData = pData;

    DLL_SortAdd(&list_map->list, &pstNode->list_node, _map_list_cmp_list_node, NULL);

    list_map->count ++;

    return BS_OK;
}

static MAP_ELE_S * map_list_get_ele(MAP_HANDLE map, void *key, UINT key_len)
{
    _MAP_LIST_NODE_S *pstFind;

    pstFind = _map_list_find(map, key, key_len);
    if (NULL == pstFind) {
        return NULL;
    }

    return &pstFind->stEle;
}

static void * map_list_get(IN MAP_HANDLE map, IN VOID *pKey, IN UINT uiKeyLen)
{
    _MAP_LIST_NODE_S *pstFind;

    pstFind = _map_list_find(map, pKey, uiKeyLen);
    if (NULL == pstFind) {
        return NULL;
    }

    return pstFind->stEle.pData;
}

/* 从集合中删除并返回pData */
static void * map_list_del(IN MAP_HANDLE map, IN VOID *pKey, IN UINT uiKeyLen)
{
    _MAP_LIST_S *list_map = map->impl_map;
    _MAP_LIST_NODE_S *pstNode;
    VOID *pData;

    pstNode = _map_list_find(map, pKey, uiKeyLen);
    if (! pstNode) {
        return NULL;
    }

    DLL_DEL(&list_map->list, &pstNode->list_node);
    pData = pstNode->stEle.pData;

    if (pstNode->stEle.dup_key) {
        MemCap_Free(map->memcap, pstNode->stEle.pKey);
    }
    MemCap_Free(map->memcap, pstNode);

    list_map->count --;

    return pData;
}

static void map_list_del_all(MAP_HANDLE map, PF_MAP_FREE_FUNC func, void * pUserData)
{
    _MAP_LIST_S *list_map = map->impl_map;
    _MAP_LIST_NODE_S *node, *next;

    DLL_SAFE_SCAN(&list_map->list, node, next) {
        DLL_DEL(&list_map->list, &node->list_node);
        if (func) {
            func(node->stEle.pData, pUserData);
        }
        if (node->stEle.dup_key) {
            MemCap_Free(map->memcap, node->stEle.pKey);
        }
        MemCap_Free(map->memcap, node);
    }

    list_map->count = 0;
}

static UINT map_list_count(MAP_HANDLE map)
{
    _MAP_LIST_S *list_map = map->impl_map;
    return list_map->count;
}

static void map_list_walk(MAP_HANDLE map, PF_MAP_WALK_FUNC pfWalkFunc, VOID *pUserData)
{
    _MAP_LIST_S *list_map = map->impl_map;
    _MAP_LIST_NODE_S *node, *next;

    DLL_SAFE_SCAN(&list_map->list, node, next) {
        if (BS_WALK_STOP == pfWalkFunc(&node->stEle, pUserData)) {
            break;
        }
    }
}

/* 按照字典序获取下一个 */
static MAP_ELE_S * map_list_getnext(MAP_HANDLE map, MAP_ELE_S *pstCurrent)
{
    _MAP_LIST_S *list_map = map->impl_map;
    _MAP_LIST_NODE_S *node;

    if (NULL == pstCurrent) {
        node = DLL_FIRST(&list_map->list);
    } else {
        node = _map_list_next_dict(map, pstCurrent);
    }

    if (! node) {
        return NULL;
    }

    return &node->stEle;
}

MAP_HANDLE MAP_ListCreate(void *memcap/*可以为NULL*/)
{
    MAP_CTRL_S *ctrl;
    _MAP_LIST_S *list_map;

    ctrl = MemCap_ZMalloc(memcap, sizeof(MAP_CTRL_S));
    if (! ctrl) {
        return NULL;
    }
    ctrl->memcap = memcap;
    ctrl->funcs = &g_map_list_funcs;

    list_map = MemCap_ZMalloc(memcap, sizeof(_MAP_LIST_S));
    if (NULL == list_map) {
        MemCap_Free(memcap, ctrl);
        return NULL;
    }
    ctrl->impl_map = list_map;

    return ctrl;
}


