/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2013-1-24
* Description: 
* History:     
******************************************************************************/

#ifndef __WAN_ARP_H_
#define __WAN_ARP_H_

#include "comp/comp_wan.h"


#ifdef __cplusplus
    extern "C" {
#endif 

BS_STATUS WAN_ARP_Init();

BS_STATUS WAN_ARP_PacketInput(IN MBUF_S *pstArpPacket);


BS_STATUS WAN_ARP_GetMacByIp
(
    IN UINT uiIfIndex,
    IN UINT ulIpToResolve ,
    IN MBUF_S *pstMbuf,
    OUT MAC_ADDR_S *pstMacAddr
);


#ifdef __cplusplus
    }
#endif 

#endif 


