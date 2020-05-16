/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2014-6-8
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
#include "../inc/vnets_node_ctrl.h"


#define _VNETS_NODE_INPUT_DBG_PACKET 0x1

#define _VNETS_NODE_INPUT_FLAG_ONLINE 0x1

typedef struct
{
    UINT uiFlag;
    PF_VNETS_NODE_PROTO_FUNC pfFunc;
}VNETS_NODE_FUNC_TBL_S;


static UINT g_ulVnetSNodeInputDebugFlag = 0;

static VNETS_NODE_FUNC_TBL_S g_apfVnetsNodeProtoTbl[VNET_NODE_PKT_PROTO_MAX] = 
{
    {0, VNETS_TP_Input},
    {_VNETS_NODE_INPUT_FLAG_ONLINE, VNETS_MacLayer_Input}
};

static BS_STATUS vnets_nodeinput_ReplyOffline(IN MBUF_S *pstMbuf)
{
    BS_STATUS eRet;

    /* 回应重新认证应答 */
    eRet = VNETS_NodeCtrl_ReplyOffline(VNETS_Context_GetRecvSesID(pstMbuf));
    MBUF_Free(pstMbuf);

    return eRet;
}

static BS_STATUS vnets_node_DeliverUp(IN UINT uiSrcNodeID, IN MBUF_S *pstMbuf)
{
    VNET_NODE_PKT_HEADER_S *pstHeader;
    USHORT usProto;

    pstHeader = MBUF_MTOD(pstMbuf);
    usProto = ntohs(pstHeader->usProto);    

    if (g_apfVnetsNodeProtoTbl[usProto].uiFlag & _VNETS_NODE_INPUT_FLAG_ONLINE)
    {
        /* 检查是否在线 */
        if (FALSE == VNETS_Context_CheckFlag(pstMbuf, VNETS_CONTEXT_FLAG_ONLINE))
        {
            return vnets_nodeinput_ReplyOffline(pstMbuf);
        }
    }

    MBUF_CutHead(pstMbuf, sizeof(VNET_NODE_PKT_HEADER_S));

    return g_apfVnetsNodeProtoTbl[usProto].pfFunc(pstMbuf);
}


static BOOL_T vnets_node_CheckOnline(IN UINT uiSrcNodeID, IN UINT uiCookie)
{
    VNETS_NODE_S *pstNode;

    pstNode = VNETS_NODE_GetNode(uiSrcNodeID);
    if (NULL == pstNode)
    {
        return FALSE;
    }

    if (uiCookie != pstNode->uiCookie)
    {
        return FALSE;
    }

    return TRUE;
}

BS_STATUS VNETS_NodeInput(IN MBUF_S *pstMbuf)
{
    VNET_NODE_PKT_HEADER_S *pstHeader;
    UINT uiDstNodeID;
    UINT uiSrcNodeID;
    UINT uiCookie;
    BS_STATUS eRet;
    MBUF_S *pstMbufNew;
    CHAR *pcOnline = "Offline";

    if (BS_OK != MBUF_MakeContinue(pstMbuf, sizeof(VNET_NODE_PKT_HEADER_S)))
    {
        MBUF_Free(pstMbuf);
        return BS_ERR;
    }

    pstHeader = MBUF_MTOD(pstMbuf);

    uiDstNodeID = ntohl(pstHeader->uiDstNodeID);
    uiSrcNodeID = ntohl(pstHeader->uiSrcNodeID);
    uiCookie = pstHeader->uiCookie;

    VNETS_Context_SetSrcNodeID(pstMbuf, uiSrcNodeID);
    VNETS_Context_SetDstNodeID(pstMbuf, uiDstNodeID);

    /* 检查是否在线 */
    if (vnets_node_CheckOnline(uiSrcNodeID, uiCookie) == TRUE)
    {
        VNETS_Context_SetFlagBit(pstMbuf, VNETS_CONTEXT_FLAG_ONLINE);
        pcOnline = "Online";
    }
    else
    {
        VNETS_Context_ClrFlagBit(pstMbuf, VNETS_CONTEXT_FLAG_ONLINE);
    }

    BS_DBG_OUTPUT(g_ulVnetSNodeInputDebugFlag, _VNETS_NODE_INPUT_DBG_PACKET,
        ("VNETS-NODE-INPUT: Receive packet: SourceNID %s%x(%s), DestNID %s%d.\r\n",
        VNET_NODE_GetTypeStringByNID(uiSrcNodeID), VNET_NID_INDEX(uiSrcNodeID), pcOnline,        
        VNET_NODE_GetTypeStringByNID(uiDstNodeID), VNET_NID_INDEX(uiDstNodeID)));

    if (uiDstNodeID == VNET_NID_SERVER)
    {
        eRet = vnets_node_DeliverUp(uiSrcNodeID, pstMbuf);
    }
    else
    {
        /* 没有指定目的地址, 则会进行广播,也要上送一份 */
        if (uiDstNodeID == 0)
        {
            pstMbufNew = MBUF_RawCopy(pstMbuf, 0, MBUF_TOTAL_DATA_LEN(pstMbuf), MBUF_DFT_RESERVED_HEAD_SPACE);
            if (NULL != pstMbufNew)
            {
                vnets_node_DeliverUp(uiSrcNodeID, pstMbufNew);
            }
        }

        /* 检查是否在线 */
        if (FALSE == VNETS_Context_CheckFlag(pstMbuf, VNETS_CONTEXT_FLAG_ONLINE))
        {
            return vnets_nodeinput_ReplyOffline(pstMbuf);
        }

        eRet = VNETS_NodeFwd_FwdTo(uiDstNodeID, pstMbuf);
    }

    return eRet;
}


/* debug node packet input */
PLUG_API VOID VNETS_NodeInput_DebugPacket(IN UINT ulArgc, IN CHAR **argv)
{
    g_ulVnetSNodeInputDebugFlag |= _VNETS_NODE_INPUT_DBG_PACKET;
}

/* no debug node packet input */
PLUG_API VOID VNETS_NodeInput_NoDebugPacket(IN UINT ulArgc, IN CHAR **argv)
{
    g_ulVnetSNodeInputDebugFlag &= ~_VNETS_NODE_INPUT_DBG_PACKET;
}


