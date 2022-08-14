/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2012-3-30
* Description: 
* History:     
******************************************************************************/

#ifndef __VNDIS_INIT_H_
#define __VNDIS_INIT_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

NDIS_STATUS DriverEntry(PVOID pDriverObject, PVOID pRegistryPath);
DRIVER_UNLOAD VNDIS_Init_UnLoad;
VOID VNDIS_Init_UnLoad(__in IN  PDRIVER_OBJECT  DriverObject);
NDIS_HANDLE VNDIS_Init_GetWrapperHandle();

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__VNDIS_INIT_H_*/



