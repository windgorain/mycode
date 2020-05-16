/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2013-4-27
* Description: 
* History:     
******************************************************************************/

#ifndef __NAT_PHY_H_
#define __NAT_PHY_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

BS_STATUS NAT_Phy_Init();
BS_STATUS NAT_Phy_PrivatePcap(IN CHAR *pcPcap);
BS_STATUS NAT_Phy_PubPcap(IN CHAR *pcPcap);
UINT NAT_PHY_GetPubIfIndex();
UINT NAT_PHY_GetPubPcapIndex();
UINT NAT_PHY_GetPubPcapIP();
MAC_ADDR_S * NAT_PHY_GetPubPcapMAC();
UINT NAT_PHY_GetPubPcapGateWayIP();
UINT NAT_PHY_GetPcapIndexByIfIndex(IN UINT uiIfIndex);
UINT NAT_PHY_GetIP(IN UINT uiIfIndex);
UINT NAT_PHY_GetMask(IN UINT uiIfIndex);
/* 判断pub pcap是否同时是private pcap */
BOOL_T NAT_PHY_PubPcapIsPrivate();
BOOL_T NAT_PHY_IsPrivateIfIndex(IN UINT uiIfIndex);
BS_STATUS NAT_Phy_Start();
VOID NAT_Phy_Save(IN HANDLE hFile);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__NAT_PHY_H_*/


