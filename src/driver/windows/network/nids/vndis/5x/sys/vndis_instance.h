/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2012-3-30
* Description: 
* History:     
******************************************************************************/

#ifndef __VNDIS_INSTACE_H_
#define __VNDIS_INSTACE_H_

#ifdef __cplusplus
    extern "C" {
#endif 


VOID VNDIS_Instance_Init();
VOID VNDIS_Instance_UnInit();
VOID VNDIS_Instance_Add(IN VNDIS_ADAPTER_S *pstAdapter);
VOID VNDIS_Instance_Del(IN VNDIS_ADAPTER_S *pstAdapter);
VNDIS_ADAPTER_S * VNDIS_Instance_Search(IN PDEVICE_OBJECT pstDeviceObject);
VNDIS_ADAPTER_S * VNDIS_Instance_SearchByAdapter(IN VNDIS_ADAPTER_S *pstAdapterToFound);

#ifdef __cplusplus
    }
#endif 

#endif 


