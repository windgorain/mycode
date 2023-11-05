/******************************************************************************
* Copyright (C),    LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2013-6-8
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/mbuf_utl.h"
#include "comp/comp_if.h"

#include "../../inc/vnet_node.h"

#include "../inc/vnetc_tp.h"
#include "../inc/vnetc_fsm.h"
#include "../inc/vnetc_conf.h"
#include "../inc/vnetc_node.h"
#include "../inc/vnetc_context.h"
#include "../inc/vnetc_mac_fw.h"
#include "../inc/vnetc_node_pkt.h"
#include "../inc/vnetc_peer_info.h"
#include "../inc/vnetc_ses_c2s.h"
#include "../inc/vnetc_node_self.h"
#include "../inc/vnetc_vnic_phy.h"

typedef struct
{
    UINT uiFlag;
    PF_VNETC_NODE_PROTO_FUNC pfFunc;
}VNETC_NODE_FUNC_TBL_S;


static VNETC_NODE_FUNC_TBL_S g_apfVnetcNodeProtoTbl[VNET_NODE_PKT_PROTO_MAX] = 
{
    {0, VNETC_TP_PktInput},
    {0, VNETC_MAC_FW_Input}
};

static inline VOID vnetc_node_DirectDetect(IN VNETC_NID_S *pstNode)
{
    if (VNET_NID_TYPE(pstNode->uiNID) == VNET_NID_TYPE_CLIENT)
    {
        if (pstNode->ucDirectStatus == VNETC_NODE_DIRECT_INIT)
        {
            pstNode->ucDirectStatus = VNETC_NODE_DIRECT_DETECTING;
            VNETC_PeerInfo_StartPeerInfoRequest(pstNode->uiNID);
        }
    }
}

static inline BS_STATUS vnetc_node_Send2Vnic(IN MBUF_S * pstMbuf)
{
    return IFNET_LinkOutput(VNETC_VNIC_PHY_GetVnicIfIndex(), pstMbuf, 0);
}

static inline BS_STATUS vnetc_node_BuildHeader
(
    IN UINT uiDstNodeID,
    IN UINT uiSesID,
    IN UCHAR ucDirectStatus,
    IN MBUF_S *pstMbuf,
    IN USHORT usProtoType
)
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

    pstHeader->usFlag = 0;
    if (ucDirectStatus == VNETC_NODE_DIRECT_FAILED)
    {
        pstHeader->usFlag = htons(VNET_NODE_PKT_FLAG_GIVE_DETECTER);
    }

    uiSrcNID = VNETC_NODE_Self();

    pstHeader->usProto = htons(usProtoType);
    pstHeader->uiDstNodeID = htonl(uiDstNodeID);
    pstHeader->uiSrcNodeID = htonl(uiSrcNID);

    if (uiSesID == VNETC_SesC2S_GetSesId())
    {
        pstHeader->uiCookie = VNETC_NODE_SelfCookie();
    }
    else
    {
        pstHeader->uiCookie = 0;
    }

    return BS_OK;
}

static BS_STATUS vnetc_node_SendPkt(IN VNETC_NID_S *pstNode, IN MBUF_S *pstMbuf, IN USHORT usProto)
{
    UINT uiSesID;
    UINT uiIfIndex;

    uiSesID = pstNode->uiSesID;
    uiIfIndex = pstNode->uiIfIndex;

    if (uiIfIndex == 0)
    {
        uiSesID = VNETC_SesC2S_GetSesId();
        uiIfIndex = VNETC_CONF_GetC2SIfIndex();
        vnetc_node_DirectDetect(pstNode);
    }

    VNETC_Context_SetDstNID(pstMbuf, pstNode->uiNID);
    VNETC_Context_SetSendSesID(pstMbuf, uiSesID);
    VNETC_Context_SetSrcNID(pstMbuf, VNETC_NODE_Self());

    if (BS_OK != vnetc_node_BuildHeader(pstNode->uiNID, uiSesID, pstNode->ucDirectStatus, pstMbuf, usProto))
    {
        MBUF_Free(pstMbuf);
        return BS_ERR;
    }

    return IFNET_LinkOutput(uiIfIndex, pstMbuf, 0);
}

static BS_STATUS vnetc_node_VnicPktBroadCast(IN MBUF_S *pstMbuf, IN USHORT usProto)
{
    UINT uiSesID;
    UINT uiIfIndex;

    uiSesID = VNETC_SesC2S_GetSesId();
    uiIfIndex = VNETC_CONF_GetC2SIfIndex();

    VNETC_Context_SetDstNID(pstMbuf, 0);
    VNETC_Context_SetSendSesID(pstMbuf, uiSesID);
    VNETC_Context_SetSrcNID(pstMbuf, VNETC_NODE_Self());        

    if (BS_OK != vnetc_node_BuildHeader(0, uiSesID, 0, pstMbuf, usProto))
    {
        MBUF_Free(pstMbuf);
        return BS_ERR;
    }

    return IFNET_LinkOutput(uiIfIndex, pstMbuf, 0);
}

static BS_STATUS vnetc_node_NetPktBroadCast(IN MBUF_S *pstMbuf)
{
    return vnetc_node_Send2Vnic(pstMbuf);
}

BS_STATUS VNETC_NODE_BroadCast(IN MBUF_S *pstMbuf, IN USHORT usProto)
{
    UINT uiSrcNID;
    BS_STATUS eRet;

    uiSrcNID = VNETC_Context_GetSrcNID(pstMbuf);

    switch (VNET_NID_TYPE(uiSrcNID))
    {
        case VNET_NID_TYPE_CLIENT:
        case VNET_NID_TYPE_SERVER:
        {
            eRet = vnetc_node_NetPktBroadCast(pstMbuf);
            break;
        }

        case VNET_NID_TYPE_VNIC:
        {
            eRet = vnetc_node_VnicPktBroadCast(pstMbuf, usProto);
            break;
        }

        default:
        {
            eRet = BS_NOT_SUPPORT;
            MBUF_Free(pstMbuf);
            break;
        }
    }

    return eRet;
}

static inline VOID vnetc_node_TrigerDirectDetect(IN VNET_NODE_PKT_HEADER_S *pstPktHeader)
{
    UINT uiSrcNodeID;
    USHORT usFlag;
    VNETC_NID_S *pstNode;

    usFlag = ntohs(pstPktHeader->usFlag);
    if ((usFlag & VNET_NODE_PKT_FLAG_GIVE_DETECTER) == 0)
    {
        return;
    }

    uiSrcNodeID = ntohl(pstPktHeader->uiSrcNodeID);

    pstNode = VNETC_NODE_GetNode(uiSrcNodeID);
    if (NULL == pstNode)
    {
        return;
    }

    vnetc_node_DirectDetect(pstNode);    
}

BS_STATUS VNETC_NODE_PktInput(IN MBUF_S *pstMbuf)
{
    UINT uiSrcNodeID;
    UINT uiDstNodeID;
    UINT uiSesID;
    UINT uiIfIndex;
    USHORT usProto;
    USHORT usFlag;
    VNET_NODE_PKT_HEADER_S *pstPktHeader;

    if (BS_OK != MBUF_MakeContinue(pstMbuf,sizeof(VNET_NODE_PKT_HEADER_S)))
    {
        MBUF_Free(pstMbuf);
        return BS_ERR;
    }

    pstPktHeader = MBUF_MTOD(pstMbuf);

    usFlag = ntohs(pstPktHeader->usFlag);
    if (usFlag & VNET_NODE_PKT_FLAG_NOT_ONLINE)
    {
        VNETC_FSM_EventHandle(VNETC_FSM_EVENT_REAUTH);
        MBUF_Free (pstMbuf);
        return BS_OK;
    }

    usProto = ntohs(pstPktHeader->usProto);

    if (usProto >= VNET_NODE_PKT_PROTO_MAX)
    {
        MBUF_Free (pstMbuf);
        return(BS_ERR);
    }

    uiSrcNodeID = ntohl(pstPktHeader->uiSrcNodeID);
    uiDstNodeID = ntohl(pstPktHeader->uiDstNodeID);
    VNETC_Context_SetDstNID(pstMbuf, uiDstNodeID);
    VNETC_Context_SetSrcNID(pstMbuf, uiSrcNodeID);

    if (VNET_NID_TYPE(uiSrcNodeID) == VNET_NID_TYPE_CLIENT)
    {
        uiSesID = VNETC_Context_GetRecvSesID(pstMbuf);
        uiIfIndex = MBUF_GET_RECV_IF_INDEX(pstMbuf);

        
        if (uiSesID == VNETC_SesC2S_GetSesId())
        {
            VNETC_NODE_Learn(uiSrcNodeID, 0, 0);
        }
        else
        {
            VNETC_NODE_Learn(uiSrcNodeID, uiIfIndex, uiSesID);
        }
    }

    vnetc_node_TrigerDirectDetect(pstPktHeader);

    MBUF_CutHead (pstMbuf, sizeof (VNET_NODE_PKT_HEADER_S));

    return g_apfVnetcNodeProtoTbl[usProto].pfFunc(pstMbuf);
}

BS_STATUS VNETC_NODE_PktOutput(IN UINT uiDstNID, IN MBUF_S *pstMbuf, IN USHORT usProto)
{
    VNETC_NID_S *pstNode;

    pstNode = VNETC_NODE_GetNode(uiDstNID);
    if (NULL == pstNode)
    {
        MBUF_Free(pstMbuf);
        return BS_ERR;
    }

    if (pstNode->uiFlag & VNETC_NODE_FLAG_INNER)
    {
        return IFNET_LinkOutput(pstNode->uiIfIndex, pstMbuf, 0);
    }

    return vnetc_node_SendPkt(pstNode, pstMbuf, usProto);
}

