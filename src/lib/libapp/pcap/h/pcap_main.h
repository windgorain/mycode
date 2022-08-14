/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2013-1-15
* Description: 
* History:     
******************************************************************************/

#ifndef __PCAP_MAIN_H_
#define __PCAP_MAIN_H_

#include "utl/mbuf_utl.h"
#include "utl/pcap_agent.h"
#include "utl/netinfo_utl.h"

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

BS_STATUS PCAP_Main_Init();
BS_STATUS PCAP_InitPcapList();
BS_STATUS PCAP_Main_SetNdis(IN UINT uiIndex, IN CHAR *pcNdisName);
CHAR * PCAP_Main_GetNdisName(IN UINT uiIndex);
BS_STATUS PCAP_Main_Start(IN UINT uiIndex);
BS_STATUS PCAP_Main_Stop(IN UINT uiIndex);
BOOL_T PCAP_Main_IsStart(IN UINT uiIndex);
VOID PCAP_Main_UnregService
(
    IN UINT uiIndex,
    IN USHORT usProtoType,
    IN PF_PCAP_AGENT_PKT_INPUT_FUNC pfFunc
);


#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__PCAP_MAIN_H_*/


