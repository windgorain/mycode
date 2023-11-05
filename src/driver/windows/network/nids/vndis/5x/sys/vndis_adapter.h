/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2012-3-29
* Description: 
* History:     
******************************************************************************/

#ifndef __VNDIS_ADAPTER_H_
#define __VNDIS_ADAPTER_H_

#ifdef __cplusplus
    extern "C" {
#endif 

#define VNDIS_ADAPTER_REGED_SHUTDOWN_HANDLER 0x1  
#define VNDIS_ADAPTER_SURPRISE_REMOVED       0x2

#define VNDIS_NIC_MAX_MCAST_LIST 32

typedef struct 
{
  VNDIS_ETH_ADDR_S list[VNDIS_NIC_MAX_MCAST_LIST];
}VNDIS_MC_LIST;

typedef struct
{
    LIST_ENTRY stListNode;

    NDIS_SPIN_LOCK  stLock;
    UINT uiUsingCount;  

    NDIS_HANDLE hAdapterHandle;
    UINT uiFlag;
    UINT uiMtu;
    UINT uiInstanceID;
    ANSI_STRING sNameAnsi;
    BOOLEAN bMediaStateConnectted;
    BOOLEAN bEnableNonAdmin;
    MACADDR aucMacAddr;
    MACADDR aucBroadcastMac;
    VNDIS_MC_LIST stMcList;
    UINT uiMCListSize;
    ULONG ulLookahead;

    ULONG64 ulRx, ulTx;
    UINT   ulRxErr, ulTxErr;

    VNDIS_DEV_S stDev;
}VNDIS_ADAPTER_S;



NDIS_STATUS VNDIS_Adapter_Init
(
    OUT PNDIS_STATUS pOpenErrorStatus,
    OUT PUINT pSelectedMediumIndex,
    IN PNDIS_MEDIUM pMediumArray,
    IN UINT uiMediumArraySize,
    IN NDIS_HANDLE hMiniportAdapterHandle,
    IN NDIS_HANDLE hWrapperConfigurationContext
);

VOID VNDIS_Adapter_IncUsing(IN VNDIS_ADAPTER_S  *pstAdapter);
VOID VNDIS_Adapter_DecUsing(IN VNDIS_ADAPTER_S  *pstAdapter);

VOID VNDIS_Adapter_Shutdown(IN NDIS_HANDLE hMiniportAdapterContext);

NDIS_STATUS VNDIS_Adapter_AdapterReset (OUT PBOOLEAN pbAddressingReset, IN NDIS_HANDLE hAdapterContext);

VOID VNDIS_Adapter_Halt(IN  NDIS_HANDLE hMiniportAdapterContext);

VOID VNDIS_Adapter_SetMediaStatus (IN VNDIS_ADAPTER_S *pstAdapter, IN BOOLEAN bState);

NDIS_STATUS VNDIS_Adapter_Receive
(
    OUT PNDIS_PACKET p_Packet,
    OUT PUINT p_Transferred,
    IN NDIS_HANDLE p_AdapterContext,
    IN NDIS_HANDLE p_ReceiveContext,
    IN UINT p_Offset,
    IN UINT p_ToTransfer
);

VOID VNDIS_Adapter_SendPackets
(
    IN  NDIS_HANDLE             hMiniportAdapterContext,
    IN  PPNDIS_PACKET           pPacketArray,
    IN  UINT                    uiNumberOfPackets
);

NDIS_STATUS VNDIS_Adapter_SetInformation
(
    IN NDIS_HANDLE hMiniportAdapterContext,
    IN NDIS_OID    uiOid,
    IN PVOID       pInformationBuffer,
    IN ULONG       ulInformationBufferLength,
    OUT PULONG     pulBytesRead,
    OUT PULONG     pulBytesNeeded
);

NDIS_STATUS VNDIS_Adapter_QueryInformation
(
    IN NDIS_HANDLE  hMiniportAdapterContext,
    IN NDIS_OID     uiOid,
    IN PVOID        pInformationBuffer,
    IN ULONG        ulInformationBufferLength,
    OUT PULONG      pulBytesWritten,
    OUT PULONG      pulBytesNeeded
);

#ifdef __cplusplus
    }
#endif 

#endif 


