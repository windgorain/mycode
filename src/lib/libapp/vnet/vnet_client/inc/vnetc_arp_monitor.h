/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2008-9-5
* Description: 
* History:     
******************************************************************************/

#ifndef __VNETC_ARP_MONITOR_H_
#define __VNETC_ARP_MONITOR_H_

#ifdef __cplusplus
    extern "C" {
#endif 

VOID VNETC_ArpMonitor_PacketMonitor(IN MBUF_S *pstMbuf);
VOID VNETC_ArpMonitor_ProcArpRequest(IN UINT ulIfIndex, IN MBUF_S *pstMbuf);

#ifdef __cplusplus
    }
#endif 

#endif 

