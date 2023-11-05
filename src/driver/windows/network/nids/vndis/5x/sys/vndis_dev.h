/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2012-3-30
* Description: 
* History:     
******************************************************************************/

#ifndef __VNDIS_DEV_H_
#define __VNDIS_DEV_H_

#ifdef __cplusplus
    extern "C" {
#endif 


#define VNDIS_DEV_DRIVER_MAJOR_VERSION 1
#define VNDIS_DEV_DRIVER_MINOR_VERSION 0

typedef struct
{
    UNICODE_STRING      stDevLinkName;
    BOOLEAN             bDevLinkNameCreated;

    PDEVICE_OBJECT      pstDeviceObject;
    NDIS_HANDLE         hDeviceHandle;

    NDIS_SPIN_LOCK      stQueLock;
    BOOLEAN             bQueLockInit;
    VNDIS_QUE_HANDLE    hPacketQue;
    VNDIS_QUE_HANDLE    hIrpQue;
}VNDIS_DEV_S;

typedef struct
{
    UINT uiPacketLen;
    UCHAR aucPacketData[VNDIS_MAX_ETH_PACKET_SIZE];
}VNDIS_DEV_PACKET_S;

NDIS_STATUS VNDIS_DEV_CreateDevice
(
    IN VNDIS_DEV_S *pstDev,
    IN CHAR *pcAdapterName
);
VOID VNDIS_DEV_DestoryDevice(IN VNDIS_DEV_S *pstDev);
VOID VNDIS_DEV_AllowNonAdmin(IN VNDIS_DEV_S *pstDev);

NDIS_STATUS VNDIS_DEV_SendPacket
(
    IN VNDIS_DEV_S *pstDev,
    IN PNDIS_PACKET pPacket
);

#ifdef __cplusplus
    }
#endif 

#endif 


