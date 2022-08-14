/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-7-22
* Description: 
* History:     
******************************************************************************/
#include "bs.h"
    
#include "utl/file_utl.h"
#include "utl/txt_utl.h"
#include "utl/dns_utl.h"
#include "utl/vbuf_utl.h"
#include "utl/ws_utl.h"
#include "utl/conn_utl.h"
#include "utl/eth_utl.h"
#include "utl/mutex_utl.h"
#include "app/wan_pub.h"
#include "app/if_pub.h"
#include "app/svpn_pub.h"
    
#include "../h/svpn_def.h"
    
#include "../h/svpn_context.h"
#include "../h/svpn_ippool.h"

#include "svpn_iptun_node.h"


#define SVPN_IP_TUNNEL_VBUF_SIZE 2048

typedef struct
{
    MUTEX_S stMutex;
    HASH_HANDLE hHashInstance;
}_SVPN_IPTUNNODE_CTRL_S;

static _SVPN_IPTUNNODE_CTRL_S g_stSvpnIpTunNodeCtrl;

static VOID _svpn_iptunnode_RcuFree(IN VOID *pstRcuNode)
{
    SVPN_IPTUN_NODE_S *pstNode;

    pstNode = container_of(pstRcuNode, SVPN_IPTUN_NODE_S, stRcu);
    
    if (pstNode->hDownConn != NULL)
    {
        CONN_Free(pstNode->hDownConn);
    }

    VBUF_Finit(&pstNode->stVBuf);

    if (pstNode->uiVirtualIP != 0)
    {
        SVPN_IPPOOL_FreeIP(pstNode->hSvpnContext, pstNode->uiVirtualIP);
    }

    MUTEX_Final(&pstNode->stMutex);

    if (NULL != pstNode->pstMbufSending)
    {
        MBUF_Free(pstNode->pstMbufSending);
    }

    MBUF_QUE_FREE_ALL(&pstNode->stMbufQue);

    MEM_Free(pstNode);
}

static UINT _svpn_iptunnode_HashIndex(IN VOID *pstHashNode)
{
    SVPN_IPTUN_NODE_S *pstNode = pstHashNode;
    
    return pstNode->uiVirtualIP;
}

SVPN_IPTUN_NODE_S * SVPN_IpTunNode_New
(
    IN SVPN_CONTEXT_HANDLE hSvpnContext,
    IN CONN_HANDLE hDownConn,
    IN UINT uiVirtualIP /* net order */
)
{
    SVPN_IPTUN_NODE_S *pstNode;

    pstNode = MEM_ZMalloc(sizeof(SVPN_IPTUN_NODE_S));
    if (NULL == pstNode)
    {
        return NULL;
    }

    VBUF_Init(&pstNode->stVBuf);

    if (BS_OK != VBUF_ExpandTo(&pstNode->stVBuf, SVPN_IP_TUNNEL_VBUF_SIZE))
    {
        VBUF_Finit(&pstNode->stVBuf);
        MEM_Free(pstNode);
        return NULL;
    }

    pstNode->hSvpnContext = hSvpnContext;
    pstNode->uiVirtualIP = uiVirtualIP;
    pstNode->hDownConn = hDownConn;
    MBUF_QUE_INIT(&pstNode->stMbufQue, 10);
    MUTEX_Init(&pstNode->stMutex);

    MUTEX_P(&g_stSvpnIpTunNodeCtrl.stMutex);
    HASH_Add(g_stSvpnIpTunNodeCtrl.hHashInstance, pstNode);
    MUTEX_V(&g_stSvpnIpTunNodeCtrl.stMutex);

    return pstNode;
}


VOID SVPN_IpTunNode_Free(IN SVPN_IPTUN_NODE_S *pstNode)
{
    MUTEX_P(&g_stSvpnIpTunNodeCtrl.stMutex);
    if (pstNode->uiFlag & SVPN_IPTUN_NODE_FLAG_FREED)
    {
        MUTEX_V(&g_stSvpnIpTunNodeCtrl.stMutex);
        return;
    }

    pstNode->uiFlag |= SVPN_IPTUN_NODE_FLAG_FREED;
    HASH_Del(g_stSvpnIpTunNodeCtrl.hHashInstance, pstNode);
    MUTEX_V(&g_stSvpnIpTunNodeCtrl.stMutex);

    RcuEngine_Call(&pstNode->stRcu, _svpn_iptunnode_RcuFree);
}

static INT _svpn_iptunnode_Cmp(IN VOID * pstHashNode, IN VOID * pstNodeToFind)
{
    SVPN_IPTUN_NODE_S *pstNode1 = pstHashNode;
    SVPN_IPTUN_NODE_S *pstNode2 = pstNodeToFind;

    return pstNode1->uiVirtualIP - pstNode2->uiVirtualIP;
}

SVPN_IPTUN_NODE_S * SVPN_IpTunNode_Find(IN UINT uiVirtualIP/* net order */)
{
    SVPN_IPTUN_NODE_S stNodeToFind;
    SVPN_IPTUN_NODE_S *pstNodeFound;

    stNodeToFind.uiVirtualIP = uiVirtualIP;
    
    MUTEX_P(&g_stSvpnIpTunNodeCtrl.stMutex);
    pstNodeFound = HASH_Find(g_stSvpnIpTunNodeCtrl.hHashInstance, _svpn_iptunnode_Cmp, &stNodeToFind);
    MUTEX_V(&g_stSvpnIpTunNodeCtrl.stMutex);

    return pstNodeFound;
}

BS_STATUS SVPN_IpTunNode_Init()
{
    g_stSvpnIpTunNodeCtrl.hHashInstance = HASH_CreateInstance(NULL, 1024, _svpn_iptunnode_HashIndex);
    MUTEX_Init(&g_stSvpnIpTunNodeCtrl.stMutex);

	return BS_OK;
}

