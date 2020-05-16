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
#endif /* __cplusplus */

BS_STATUS NAT_ARP_Init();
BS_STATUS NAT_ARP_PacketInput(IN MBUF_S *pstArpPacket);
/* 根据IP得到MAC，如果得不到,则发送ARP请求，并返回BS_AGAIN. */
BS_STATUS NAT_ARP_GetMacByIp
(
    IN UINT uiIfIndex,
    IN UINT ulIpToResolve /* 网络序 */,
    IN MBUF_S *pstMbuf,
    OUT MAC_ADDR_S *pstMacAddr
);
VOID NAT_ARP_Save(IN HANDLE hFile);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__NAT_ARP_H_*/


