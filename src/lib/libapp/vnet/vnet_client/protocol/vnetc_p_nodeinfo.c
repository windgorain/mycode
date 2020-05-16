
#include "bs.h"

#include "utl/txt_utl.h"

#include "../../inc/vnet_conf.h"

#include "../inc/vnetc_conf.h"
#include "../inc/vnetc_mac_tbl.h"
#include "../inc/vnetc_alias.h"
#include "../inc/vnetc_tp.h"
#include "../inc/vnetc_caller.h"
#include "../inc/vnetc_node.h"
#include "../inc/vnetc_protocol.h"
#include "../inc/vnetc_addr_monitor.h"
#include "../inc/vnetc_p_nodeinfo.h"
#include "../inc/vnetc_vnic_phy.h"

BS_STATUS VNETC_P_NodeInfo_Init()
{
    return BS_OK;
}

BS_STATUS VNETC_P_NodeInfo_Input(IN MIME_HANDLE hMime, IN VNETC_PROTOCOL_PACKET_INFO_S *pstPacketInfo)
{
    return BS_OK;
}

/* 向服务器发送自己的info */
BS_STATUS VNETC_P_NodeInfo_SendInfo()
{
    MAC_ADDR_S *pstMAC;
    CHAR szInfo[256];
    UINT uiIp;
    UINT uiMask;

    pstMAC = VNET_VNIC_PHY_GetVnicMac();
    uiIp = VNETC_AddrMonitor_GetIP();
    uiMask = VNETC_AddrMonitor_GetMask();

    BS_Snprintf(szInfo, sizeof(szInfo), "Protocol=UpdateNodeInfo,MAC=%pM,IP=%pI4,Mask=%pI4,Alias=%s,Description=%s",
        pstMAC->aucMac, &uiIp, &uiMask, VNETC_Alias_GetAlias(), VNETC_GetDescription());

    return VNETC_Protocol_SendData(VNETC_TP_GetC2STP(), szInfo, strlen(szInfo) + 1);
}


