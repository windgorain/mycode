/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0
* Description:
******************************************************************************/
#include "bs.h"
#include "utl/jhash_utl.h"
#include "utl/list_dl.h"
#include "utl/free_list.h"

#include "../h/xgw_vht.h"
#include "../h/xgw_rcu.h"

#define XGW_VHT_BUCKET_NUM   1024
#define XGW_VHT_BUCKET_MASK  (XGW_VHT_BUCKET_NUM - 1)
#define XGW_VHT_NODE_NUM     (1024*1024)

typedef struct {
    DL_NODE_S busy_link_node;
    U32 oip; 
    U32 vid; 
    U32 hip; 
}XGW_VHT_NODE_S;

static DL_HEAD_S g_xgw_vht_tbl[XGW_VHT_BUCKET_NUM];
static XGW_VHT_NODE_S g_xgw_vht_nodes[XGW_VHT_NODE_NUM];
static FREE_LIST_S g_xgw_vht_free_list;

static XGW_VHT_NODE_S * _xgw_vht_get_free_node(void)
{
    return FreeList_Get(&g_xgw_vht_free_list);
}

static DL_HEAD_S * _xgw_vht_get_bkt(U32 vid, U32 oip)
{
    U32 hash;
    hash = JHASH_2Words(vid, oip, 0);
    return &g_xgw_vht_tbl[hash & XGW_VHT_BUCKET_MASK];
}

static XGW_VHT_NODE_S * _xgw_vht_find(U32 vid, U32 oip)
{
    DL_NODE_S *n;
    XGW_VHT_NODE_S *node;
    DL_HEAD_S *bkt;

    bkt = _xgw_vht_get_bkt(vid, oip);

    DL_FOREACH(bkt, n) {
        node = container_of(n, XGW_VHT_NODE_S, busy_link_node);
        if ((node->oip == oip) && (node->vid == vid)) {
            return node;
        }
    }

    return NULL;
}

int XGW_VHT_Init(void)
{
    int i;

    FreeList_Init(&g_xgw_vht_free_list);
    FreeList_Puts(&g_xgw_vht_free_list, &g_xgw_vht_nodes, sizeof(XGW_VHT_NODE_S), XGW_VHT_NODE_NUM);
    for (i=0; i<XGW_VHT_BUCKET_NUM; i++) {
        DL_Init(&g_xgw_vht_tbl[i]);
    }

    return 0;
}


int XGW_VHT_Add(U32 vid, U32 oip, U32 hip)
{
    U32 hash;
    XGW_VHT_NODE_S * n;
    DL_HEAD_S *bkt;

    n = _xgw_vht_get_free_node();
    if (! n) {
        RETURN(BS_NO_MEMORY);
    }

    hash = JHASH_2Words(vid, oip, 0);
    bkt = &g_xgw_vht_tbl[hash & XGW_VHT_BUCKET_MASK];

    n->oip = oip;
    n->vid = vid;
    n->hip = hip;

    DL_AddHead(bkt, &n->busy_link_node);

    return 0;
}

int XGW_VHT_Del(U32 vid, U32 oip)
{
    XGW_VHT_NODE_S *n;

    n = _xgw_vht_find(vid, oip);
    if (! n) {
        RETURN(BS_NOT_FOUND);
    }

    DL_Del(&n->busy_link_node);

    XGW_RCU_Sync();

    FreeList_Put(&g_xgw_vht_free_list, n);

    return 0;
}

U32 XGW_VHT_GetHip(U32 vid, U32 oip)
{
    XGW_VHT_NODE_S *n;

    n = _xgw_vht_find(vid, oip);
    if (n) {
        return n->hip;
    }

    return 0;
}

void XGW_VHT_DelAll(void)
{
    int i;
    DL_HEAD_S *bkt;
    DL_NODE_S *n, *m;
    XGW_VHT_NODE_S *node;

    for (i=0; i<XGW_VHT_BUCKET_NUM; i++) {
        bkt = &g_xgw_vht_tbl[i];
        DL_FOREACH_SAFE(bkt, n, m) {
            node = container_of(n, XGW_VHT_NODE_S, busy_link_node);
            DL_Del(n);
            XGW_RCU_Sync();
            FreeList_Put(&g_xgw_vht_free_list, node);
        }
    }
}

void XGW_VHT_DelVid(U32 vid)
{
    int i;
    DL_HEAD_S *bkt;
    DL_NODE_S *n, *m;
    XGW_VHT_NODE_S *node;

    for (i=0; i<XGW_VHT_BUCKET_NUM; i++) {
        bkt = &g_xgw_vht_tbl[i];
        DL_FOREACH_SAFE(bkt, n, m) {
            node = container_of(n, XGW_VHT_NODE_S, busy_link_node);
            if (node->vid == vid) {
                DL_Del(n);
                XGW_RCU_Sync();
                FreeList_Put(&g_xgw_vht_free_list, node);
            }
        }
    }
}

int XGW_VHT_Walk(PF_XGW_VHT_WALK walk_func, void *ud)
{
    int i;
    DL_HEAD_S *bkt;
    DL_NODE_S *n, *m;
    XGW_VHT_NODE_S *node;

    for (i=0; i<XGW_VHT_BUCKET_NUM; i++) {
        bkt = &g_xgw_vht_tbl[i];
        DL_FOREACH_SAFE(bkt, n, m) {
            node = container_of(n, XGW_VHT_NODE_S, busy_link_node);
            walk_func(node->vid, node->oip, node->hip, ud);
        }
    }

    return 0;
}

