/******************************************************************************
* Copyright (C), 2000-2006,  Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2016-7-22
* Description: 
* History:     
******************************************************************************/

#ifndef __SVPN_IPTUN_NODE_H_
#define __SVPN_IPTUN_NODE_H_

#include "utl/hash_utl.h"
#include "utl/mutex_utl.h"

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

#define SVPN_IPTUN_NODE_FLAG_FREED 0x1 /* 已经释放标记 */

typedef struct
{
    HASH_NODE_S stHashNode;
    RCU_NODE_S stRcu;
    UINT uiFlag;
    UINT uiVirtualIP;
    SVPN_CONTEXT_HANDLE hSvpnContext;
    CONN_HANDLE hDownConn;
    VBUF_S stVBuf;
    MUTEX_S stMutex;
    MBUF_QUE_S stMbufQue;
    MBUF_S *pstMbufSending;
}SVPN_IPTUN_NODE_S;

SVPN_IPTUN_NODE_S * SVPN_IpTunNode_New
(
    IN SVPN_CONTEXT_HANDLE hSvpnContext,
    IN CONN_HANDLE hDownConn,
    IN UINT uiVirtualIP
);
VOID SVPN_IpTunNode_Free(IN SVPN_IPTUN_NODE_S *pstNode);
SVPN_IPTUN_NODE_S * SVPN_IpTunNode_Find(IN UINT uiVirtualIP);


#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__SVPN_IPTUN_NODE_H_*/


