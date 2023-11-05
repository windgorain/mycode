
#include "bs.h"

#include "utl/mac_table.h"
#include "utl/mbuf_utl.h"
#include "utl/txt_utl.h"

#include "../../vnet/inc/vnet_ipport.h"

#include "../../inc/vnet_conf.h"

#include "../inc/vnetc_conf.h"
#include "../inc/vnetc_tp.h"
#include "../inc/vnetc_mac_tbl.h"
#include "../inc/vnetc_protocol.h"
#include "../inc/vnetc_master.h"
#include "../inc/vnetc_c2c_direct.h"

typedef struct
{
    UINT uiIfIndex;
    UINT uiSesId;
    MAC_ADDR_S stMacAddr;
    USHORT usTimes;
}_VNETC_PEER_INFO_TIMER_CTRL_S;

static BS_STATUS vnetc_peerinfo_SendPeerInfoRequest
(
    IN UINT uiNodeID
)
{
    CHAR szInfo[256];

    snprintf(szInfo, sizeof(szInfo), "Protocol=GetNodeInfo,NodeID=%d", uiNodeID);

    return VNETC_Protocol_SendData(VNETC_TP_GetC2STP(), szInfo, strlen(szInfo) + 1);
}


BS_STATUS VNETC_PeerInfo_StartPeerInfoRequest(IN UINT uiNodeID)
{
    return vnetc_peerinfo_SendPeerInfoRequest(uiNodeID);
}

static BS_STATUS vnetc_peerinfo_DealPeerInfoReply
(
    IN UINT uiTpID,
    IN UINT uiNodeID,
    IN UINT uiDetectIp,
    IN USHORT usDetectPort
)
{
    VNETC_C2C_Direct_StartDetect(uiNodeID, uiDetectIp, usDetectPort);

    return BS_OK;
}

BS_STATUS VNETC_PeerInfo_Input(IN MIME_HANDLE hMime, IN VNETC_PROTOCOL_PACKET_INFO_S *pstPacketInfo)
{
    CHAR *pcPeerIP;
    CHAR *pcPeerPort;
    CHAR *pcPeerNodeID;
    UINT uiPeerIP;
    UINT uiPeerPort;
    UINT uiPeerNodeID;
    USHORT usPeerPort;

    pcPeerNodeID = MIME_GetKeyValue(hMime, "NodeID");
    pcPeerIP = MIME_GetKeyValue(hMime, "IP");
    pcPeerPort = MIME_GetKeyValue(hMime, "Port");

    if ((NULL == pcPeerNodeID) || (NULL == pcPeerIP) || (NULL == pcPeerPort))
    {
        return BS_BAD_PTR;
    }

    TXT_Atoui(pcPeerNodeID, &uiPeerNodeID);
    uiPeerIP = Socket_Ipsz2IpNetWitchCheck(pcPeerIP);
    TXT_Atoui(pcPeerPort, &uiPeerPort);

    usPeerPort = uiPeerPort;
    usPeerPort = htons(usPeerPort);

    return vnetc_peerinfo_DealPeerInfoReply(pstPacketInfo->uiTpID, uiPeerNodeID, uiPeerIP, usPeerPort);
}


