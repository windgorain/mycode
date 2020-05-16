/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-6-30
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/txt_utl.h"
#include "utl/conn_utl.h"
#include "utl/ssl_utl.h"
#include "utl/mime_utl.h"
#include "utl/http_lib.h"
#include "utl/vbuf_utl.h"

#include "../h/svpnc_conf.h"
#include "../h/svpnc_utl.h"

#include "svpnc_tcprelay_inner.h"

SVPNC_TCPRELAY_NODE_S * SVPNC_TRNode_New()
{
    SVPNC_TCPRELAY_NODE_S *pstNode;

    pstNode = MEM_ZMalloc(sizeof(SVPNC_TCPRELAY_NODE_S));
    if (NULL == pstNode)
    {
        return NULL;
    }

    VBUF_Init(&pstNode->stDownVBuf);
    VBUF_Init(&pstNode->stUpVBuf);
    pstNode->hHttpHeadParser = HTTP_CreateHeadParser();

    if (NULL == pstNode->hHttpHeadParser)
    {
        SVPNC_TRNode_Free(pstNode);
        return NULL;
    }

    return pstNode;
}

VOID SVPNC_TRNode_Free(IN SVPNC_TCPRELAY_NODE_S *pstNode)
{
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

    if (pstNode->hHttpHeadParser != NULL)
    {
        HTTP_DestoryHeadParser(pstNode->hHttpHeadParser);
    }

    FSM_Finit(&pstNode->stFsm);

    MEM_Free(pstNode);
}


