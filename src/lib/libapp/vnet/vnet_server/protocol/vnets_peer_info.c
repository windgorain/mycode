
#include "bs.h"

#include "utl/mac_table.h"
#include "utl/mbuf_utl.h"
#include "utl/txt_utl.h"

#include "../../inc/vnet_conf.h"

#include "../inc/vnets_mac_tbl.h"
#include "../inc/vnets_protocol.h"
#include "../inc/vnets_ses.h"
#include "../inc/vnets_node.h"

BS_STATUS VNETS_PeerInfo_Input(IN MIME_HANDLE hMime, IN VNETS_PROTOCOL_PACKET_INFO_S *pstPacketInfo)
{
    UINT uiDomainId;
    UINT uiSrcNodeID;
    VNETS_NODE_S *pstNode;
    UINT uiPeerNodeID;
    CHAR *pcNodeID;
    UINT usPort;
    VNETS_PHY_CONTEXT_S stPhyInfo;
    CHAR szInfo[256];

    uiSrcNodeID = VNETS_Context_GetSrcNodeID(pstPacketInfo->pstMBuf);
    if (0 == uiSrcNodeID)
    {
        return BS_ERR;
    }

    uiDomainId = VNETS_NODE_GetDomainID(uiSrcNodeID);
    if (uiDomainId == 0)
    {
        return BS_ERR;
    }
    
    pcNodeID = MIME_GetKeyValue(hMime, "NodeID");
    if (NULL == pcNodeID)
    {
        return BS_ERR;
    }

    TXT_Atoui(pcNodeID, &uiPeerNodeID);

    pstNode = VNETS_NODE_GetNode(uiPeerNodeID);
    if (NULL == pstNode)
    {
        return BS_NO_SUCH;
    }

    if (BS_OK != VNETS_SES_GetPhyInfo(pstNode->uiSesID, &stPhyInfo))
    {
        return BS_NO_SUCH;
    }

    if (stPhyInfo.enType != VNETS_PHY_TYPE_UDP)   /* 目前只有它们支持直连信息检测 */
    {
        return BS_NOT_SUPPORT;
    }

    usPort = stPhyInfo.unPhyContext.stUdpPhyContext.usPeerPort;
    usPort = ntohs(usPort);

    BS_Snprintf(szInfo, sizeof(szInfo), "Protocol=GetNodeInfo,NodeID=%s,IP=%pI4,Port=%d",
        pcNodeID, &stPhyInfo.unPhyContext.stUdpPhyContext.uiPeerIp, usPort);

    return VNETS_Protocol_SendData(pstPacketInfo->uiTpID, szInfo, strlen(szInfo) + 1);
}

