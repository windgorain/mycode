/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2015-8-1
* Description: 
* History:     
******************************************************************************/

#ifndef __SVPN_TCPRELAY_INNER_H_
#define __SVPN_TCPRELAY_INNER_H_

#include "utl/conn_utl.h"
#include "comp/comp_wsapp.h"

#ifdef __cplusplus
    extern "C" {
#endif 

typedef struct
{
    CHAR szServer[DNS_MAX_LABEL_LEN + 1];
    USHORT usPort;
    SVPN_CONTEXT_HANDLE hSvpnContext;
    CONN_HANDLE hDownConn;
    CONN_HANDLE hUpConn;
    VBUF_S stDownVBuf;   
    VBUF_S stUpVBuf;     
}SVPN_TCPRELAY_NODE_S;

SVPN_TCPRELAY_NODE_S * SVPN_TcpRelayNode_New(IN CHAR *pcServer, IN USHORT usPort);
VOID SVPN_TcpRelayNode_Free(IN SVPN_TCPRELAY_NODE_S *pstNode);

#ifdef __cplusplus
    }
#endif 

#endif 


