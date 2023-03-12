/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2015-7-31
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/file_utl.h"
#include "utl/txt_utl.h"
#include "utl/dns_utl.h"
#include "utl/vbuf_utl.h"
#include "utl/ws_utl.h"

#include "../h/svpn_def.h"
#include "../h/svpn_debug.h"
#include "../h/svpn_context.h"

#include "svpn_tcprelay_inner.h"

SVPN_TCPRELAY_NODE_S * SVPN_TcpRelayNode_New(IN CHAR *pcServer, IN USHORT usPort)
{
    SVPN_TCPRELAY_NODE_S *pstNode;

    pstNode = MEM_ZMalloc(sizeof(SVPN_TCPRELAY_NODE_S));
    if (NULL == pstNode)
    {
        return NULL;
    }

    VBUF_Init(&pstNode->stDownVBuf);
    VBUF_Init(&pstNode->stUpVBuf);

    TXT_Strlcpy(pstNode->szServer, pcServer, sizeof(pstNode->szServer));
    pstNode->usPort = usPort;

    return pstNode;
}

VOID SVPN_TcpRelayNode_Free(IN SVPN_TCPRELAY_NODE_S *pstNode)
{
    SVPN_DBG_OUTPUT(SVPN_DBG_ID_TCP_RELAY, SVPN_DBG_FLAG_TR_PROCESS, "Free node.\r\n");
    
    if (pstNode->hUpConn != NULL)
    {
        CONN_Free(pstNode->hUpConn);
    }

    if (pstNode->hDownConn != NULL)
    {
        CONN_Free(pstNode->hDownConn);
    }

    VBUF_Finit(&pstNode->stDownVBuf);
    VBUF_Finit(&pstNode->stUpVBuf);

    MEM_Free(pstNode);
}

