/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2014-5-11
* Description: 
* History:     
******************************************************************************/

#include "bs.h"

#include "utl/txt_utl.h"
#include "utl/eth_utl.h"
#include "utl/socket_utl.h"

#include "../../inc/vnet_conf.h"

#include "../inc/vnets_protocol.h"
#include "../inc/vnets_ses.h"
#include "../inc/vnets_node.h"
#include "../inc/vnets_domain.h"

BS_STATUS VNETS_P_NodeInfo_Input(IN MIME_HANDLE hMime, IN VNETS_PROTOCOL_PACKET_INFO_S *pstPacketInfo)
{
    UINT uiDomainId;
    UINT uiNodeID;
    CHAR *pcIP;
    CHAR *pcMask;
    CHAR *pcMac;
    CHAR *pcDescription;
    CHAR *pcAlias;
    VNETS_NODE_INFO_S stNodeInfo = {0};

    uiNodeID = VNETS_Context_GetSrcNodeID(pstPacketInfo->pstMBuf);
    if (0 == uiNodeID)
    {
        return BS_ERR;
    }
    
    uiDomainId = VNETS_NODE_GetDomainID(uiNodeID);
    if (uiDomainId == 0)
    {
        return BS_ERR;
    }

    pcIP = MIME_GetKeyValue(hMime, "IP");
    pcMask = MIME_GetKeyValue(hMime, "Mask");
    pcMac = MIME_GetKeyValue(hMime, "MAC");
    pcDescription = MIME_GetKeyValue(hMime, "Description");
    pcAlias = MIME_GetKeyValue(hMime, "Alias");

    if ((NULL == pcIP) || (NULL == pcMask) || (NULL == pcMac))
    {
        return BS_ERR;
    }

    if (NULL == pcAlias)
    {
        pcAlias = "";
    }

    stNodeInfo.uiIP = Socket_Ipsz2IpNetWitchCheck(pcIP);
    stNodeInfo.uiMask = Socket_Ipsz2IpNetWitchCheck(pcMask);
    STRING_2_MAC_ADDR(pcMac, stNodeInfo.stMACAddr.aucMac);
    TXT_Strlcpy(stNodeInfo.szAlias, pcAlias, sizeof(stNodeInfo.szAlias));
    TXT_Strlcpy(stNodeInfo.szDescription, pcDescription, sizeof(stNodeInfo.szDescription));

    return VNETS_NODE_SetNodeInfo(uiNodeID, &stNodeInfo);
}


