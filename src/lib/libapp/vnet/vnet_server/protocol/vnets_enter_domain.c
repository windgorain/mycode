#include "bs.h"

#include "utl/mac_table.h"
#include "utl/mbuf_utl.h"

#include "../../inc/vnet_conf.h"

#include "../inc/vnets_mac_tbl.h"
#include "../inc/vnets_domain.h"
#include "../inc/vnets_node.h"
#include "../inc/vnets_protocol.h"
#include "../inc/vnets_dc.h"
#include "../inc/vnets_master.h"
#include "../inc/vnets_p_nodeinfo.h"

BS_STATUS VNETS_EnterDomain_Input(IN MIME_HANDLE hMime, IN VNETS_PROTOCOL_PACKET_INFO_S *pstPacketInfo)
{
    UINT uiDomainID;
    UINT uiNodeId;
    BOOL_T bPermitBroadcast = FALSE;
    CHAR *pcDomainName;
    CHAR szInfo[256];

    uiNodeId = VNETS_Context_GetSrcNodeID(pstPacketInfo->pstMBuf);
    if (uiNodeId == 0)
    {
        return BS_ERR;
    }

    pcDomainName = MIME_GetKeyValue(hMime, "Domain");
    if (NULL == pcDomainName)
    {
        return BS_ERR;
    }

    uiDomainID = VNETS_Domain_Add(pcDomainName, uiNodeId);
    if (0 != uiDomainID)
    {
        bPermitBroadcast = VNETS_DC_IsBroadcastPermit(pcDomainName);
    }

    VNETS_NODE_SetDomainID(uiNodeId, uiDomainID);

    snprintf(szInfo, sizeof(szInfo), "Protocol=EnterDomain,Type=EnterDomainReply,Result=OK,PermitBroadcast:%s",
        bPermitBroadcast == TRUE ? "True" : "False");

    VNETS_Protocol_SendData(pstPacketInfo->uiTpID, szInfo, strlen(szInfo) + 1);

    return BS_OK;
}

static BS_WALK_RET_E vnets_enterdomain_KickAll(IN UINT uiNodeID, IN HANDLE hUserHandle)
{
    VNETS_NODE_S *pstNode;
    CHAR *pcKickOut = "Protocol=EnterDomain,Type=KickOut";

    pstNode = VNETS_NODE_GetNode(uiNodeID);
    if (NULL == pstNode)
    {
        return BS_WALK_CONTINUE;
    }

    VNETS_Protocol_SendData(pstNode->uiTpID, pcKickOut, strlen(pcKickOut) + 1);

    VNETS_Domain_DelNode(pstNode->uiDomainID, uiNodeID);

    return BS_WALK_CONTINUE;
}

static BS_WALK_RET_E vnets_enterdomain_RebootDomain(IN UINT uiNodeID, IN HANDLE hUserHandle)
{
    VNETS_NODE_S *pstNode;
    CHAR *pcKickOut = "Protocol=EnterDomain,Type=RebootDomain";

    pstNode = VNETS_NODE_GetNode(uiNodeID);
    if (NULL == pstNode)
    {
        return BS_WALK_CONTINUE;
    }

    VNETS_Protocol_SendData(pstNode->uiTpID, pcKickOut, strlen(pcKickOut) + 1);

    VNETS_Domain_DelNode(pstNode->uiDomainID, uiNodeID);

    return BS_WALK_CONTINUE;
}

BS_STATUS VNETS_EnterDomain_KickAll(IN UINT uiDomainID)
{
    VNETS_Domain_WalkNode(uiDomainID, vnets_enterdomain_KickAll, NULL);

    return BS_OK;
}

BS_STATUS VNETS_EnterDomain_RebootDomain(IN UINT uiDomainID)
{
    VNETS_Domain_WalkNode(uiDomainID, vnets_enterdomain_RebootDomain, NULL);

    return BS_OK;
}


