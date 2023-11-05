/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2012-9-8
* Description: 
* History:     
******************************************************************************/

#ifndef __VNETC_P_NEIGHBOR_H_
#define __VNETC_P_NEIGHBOR_H_

#ifdef __cplusplus
    extern "C" {
#endif 

BS_STATUS VNETC_P_NodeInfo_Init();

BS_STATUS VNETC_P_NodeInfo_Input(IN MIME_HANDLE hMime, IN VNETC_PROTOCOL_PACKET_INFO_S *pstPacketInfo);


BS_STATUS VNETC_P_NodeInfo_SendInfo();


#ifdef __cplusplus
    }
#endif 

#endif 


