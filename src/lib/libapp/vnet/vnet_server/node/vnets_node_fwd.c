/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2014-1-9
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "comp/comp_if.h"

#include "../../inc/vnet_conf.h"
#include "../../inc/vnet_node.h"

#include "../inc/vnets_mac_layer.h"
#include "../inc/vnets_phy.h"
#include "../inc/vnets_ses.h"
#include "../inc/vnets_tp.h"
#include "../inc/vnets_domain.h"
#include "../inc/vnets_node.h"
#include "../inc/vnets_context.h"
#include "../inc/vnets_node_fwd.h"


#define _VNETS_NODE_FWD_DBG_PACKET 0x1


static UINT g_ulVnetSNodeFwdDebugFlag = 0;

static BS_STATUS vnets_nodefwd_SendTo(IN UINT uiNodeID, IN MBUF_S *pstMbuf)
{
    VNETS_NODE_S *pstNode;

    pstNode = VNETS_NODE_GetNode(uiNodeID);
    if (NULL == pstNode)
    {
        MBUF_Free(pstMbuf);
        return BS_ERR;
    }

    VNETS_Context_SetSendSesID(pstMbuf, pstNode->uiSesID);

    return IFNET_LinkOutput(VNETS_SES_GetIfIndex(pstNode->uiSesID), pstMbuf, 0);
}

static int vnets_nodefwd_SendToEach(IN UINT uiNodeID, IN HANDLE hUserHandle)
{
    UINT uiSrcNodeID;
    MBUF_S *pstMbuf = hUserHandle;
    MBUF_S *pstMbufTmp;

    uiSrcNodeID = VNETS_Context_GetSrcNodeID(pstMbuf);
    if (uiNodeID == uiSrcNodeID)
    {
        return 0;
    }

    pstMbufTmp = MBUF_RawCopy(pstMbuf, 0, MBUF_TOTAL_DATA_LEN(pstMbuf), MBUF_DFT_RESERVED_HEAD_SPACE);
    if (NULL == pstMbufTmp)
    {
        return 0;
    }

    vnets_nodefwd_SendTo(uiNodeID, pstMbufTmp);

    return 0;
}

static BS_STATUS vnets_nodefwd_BroadCast(IN MBUF_S *pstMbuf)
{
    UINT uiSrcNID;
    VNETS_NODE_S *pstNode;

    uiSrcNID = VNETS_Context_GetSrcNodeID(pstMbuf);

    pstNode = VNETS_NODE_GetNode(uiSrcNID);
    if (NULL == pstNode)
    {
        return BS_ERR;
    }
    
    VNETS_Domain_WalkNode(pstNode->uiDomainID, vnets_nodefwd_SendToEach, pstMbuf);

    MBUF_Free(pstMbuf);

    return BS_OK;
}

static inline BS_STATUS vnets_node_BuildHeader(IN UINT uiDstNID, IN MBUF_S *pstMbuf, IN USHORT usProto)
{
    UINT uiSrcNID;
    VNET_NODE_PKT_HEADER_S *pstHeader;

    if (BS_OK != MBUF_Prepend (pstMbuf, sizeof(VNET_NODE_PKT_HEADER_S)))
    {
        return(BS_ERR);
    }

    if (BS_OK != MBUF_MakeContinue (pstMbuf, sizeof(VNET_NODE_PKT_HEADER_S)))
    {
        return(BS_ERR);
    }

    pstHeader = MBUF_MTOD(pstMbuf);

    uiSrcNID = VNETS_NODE_Self();

    pstHeader->usFlag = 0;
    pstHeader->usProto = htons(usProto);
    pstHeader->uiDstNodeID = htonl(uiDstNID);
    pstHeader->uiSrcNodeID = htonl(uiSrcNID);

    return BS_OK;
}

BS_STATUS VNETS_NodeFwd_FwdTo(IN UINT uiDstNodeID, IN MBUF_S *pstMbuf)
{
    if (uiDstNodeID == 0)
    {
        return vnets_nodefwd_BroadCast(pstMbuf);
    }
    else
    {
        return vnets_nodefwd_SendTo(uiDstNodeID, pstMbuf);
    }
}

BS_STATUS VNETS_NodeFwd_Output(IN UINT uiDstNodeID, IN MBUF_S *pstMBuf, IN USHORT usProto)
{
    UINT uiSrcNodeID;

    uiSrcNodeID = VNETS_NODE_Self();

    if (BS_OK != vnets_node_BuildHeader(uiDstNodeID, pstMBuf, usProto))
    {
        MBUF_Free(pstMBuf);
        return BS_OK;
    }

    VNETS_Context_SetDstNodeID(pstMBuf, uiDstNodeID);
    VNETS_Context_SetSrcNodeID(pstMBuf, uiSrcNodeID);

    return VNETS_NodeFwd_FwdTo(uiDstNodeID, pstMBuf);
}

BS_STATUS VNETS_NodeFwd_OutputBySes(IN UINT uiSesID, IN MBUF_S *pstMBuf, IN USHORT usProto)
{
    UINT uiSrcNodeID;

    uiSrcNodeID = VNETS_NODE_Self();

    if (BS_OK != vnets_node_BuildHeader(0, pstMBuf, usProto))
    {
        MBUF_Free(pstMBuf);
        return BS_OK;
    }

    VNETS_Context_SetDstNodeID(pstMBuf, 0);
    VNETS_Context_SetSrcNodeID(pstMBuf, uiSrcNodeID);

    VNETS_Context_SetSendSesID(pstMBuf, uiSesID);

    return IFNET_LinkOutput(VNETS_SES_GetIfIndex(uiSesID), pstMBuf, 0);
}


PLUG_API VOID VNETS_NodeFwd_DebugPacket(IN UINT ulArgc, IN CHAR **argv)
{
    g_ulVnetSNodeFwdDebugFlag |= _VNETS_NODE_FWD_DBG_PACKET;
}


PLUG_API VOID VNETS_NodeFwd_NoDebugPacket(IN UINT ulArgc, IN CHAR **argv)
{
    g_ulVnetSNodeFwdDebugFlag &= ~_VNETS_NODE_FWD_DBG_PACKET;
}


