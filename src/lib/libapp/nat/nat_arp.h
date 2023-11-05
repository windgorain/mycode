/******************************************************************************
* Copyright (C) Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2013-4-9
* Description: 
* History:     
******************************************************************************/

#ifndef __NAT_ARP_H_
#define __NAT_ARP_H_

#ifdef __cplusplus
    extern "C" {
#endif 

BS_STATUS NAT_ARP_Init();
BS_STATUS NAT_ARP_PacketInput(IN MBUF_S *pstArpPacket);

BS_STATUS NAT_ARP_GetMacByIp
(
    IN UINT uiIfIndex,
    IN UINT ulIpToResolve ,
    IN MBUF_S *pstMbuf,
    OUT MAC_ADDR_S *pstMacAddr
);
VOID NAT_ARP_Save(IN HANDLE hFile);

#ifdef __cplusplus
    }
#endif 

#endif 


