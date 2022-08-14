/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2012-3-29
* Description: 
* History:     
******************************************************************************/

#include "ndis.h"

#include "vndis_def.h"
#include "vndis_que.h"
#include "vndis_pub.h"
#include "vndis_dev.h"
#include "vndis_adapter.h"
#include "vndis_instance.h"
#include "vndis_mem.h"
#include "vndis_init.h"

#pragma NDIS_INIT_FUNCTION(DriverEntry)

static NDIS_HANDLE g_NdisWrapperHandle;

NDIS_HANDLE VNDIS_Init_GetWrapperHandle()
{
    return g_NdisWrapperHandle;
}

DRIVER_UNLOAD VNDIS_Init_UnLoad;
VOID VNDIS_Init_UnLoad(IN  PDRIVER_OBJECT  pDriverObject)
{
    UNREFERENCED_PARAMETER(pDriverObject);

    DEBUG_IN_FUNC();

    VNDIS_Instance_UnInit();

    DEBUG_OUT_FUNC(0);

    return;
}

NDIS_STATUS DriverEntry(PVOID pDriverObject, PVOID pRegistryPath)
{
    NDIS_STATUS enStatus = NDIS_STATUS_SUCCESS;
    NDIS_MINIPORT_CHARACTERISTICS stMPChar;

    NdisMInitializeWrapper(&g_NdisWrapperHandle, pDriverObject, pRegistryPath, NULL);
    if(!g_NdisWrapperHandle)
    {
        DEBUGP(("NdisMInitializeWrapper failed.\n"));
        return NDIS_STATUS_FAILURE;
    }

    NdisZeroMemory(&stMPChar, sizeof(stMPChar));
    stMPChar.MajorNdisVersion          = VNDIS_MAJOR_VERSION;
    stMPChar.MinorNdisVersion          = VNDIS_MINOR_VERSION;
    stMPChar.InitializeHandler         = VNDIS_Adapter_Init;
    stMPChar.HaltHandler               = VNDIS_Adapter_Halt;
    stMPChar.ResetHandler              = VNDIS_Adapter_AdapterReset;
    stMPChar.SetInformationHandler     = VNDIS_Adapter_SetInformation;
    stMPChar.QueryInformationHandler   = VNDIS_Adapter_QueryInformation;
    stMPChar.SendPacketsHandler        = VNDIS_Adapter_SendPackets;
    stMPChar.TransferDataHandler       = VNDIS_Adapter_Receive;

    enStatus = NdisMRegisterMiniport(g_NdisWrapperHandle, &stMPChar, sizeof(stMPChar));
    if (enStatus != NDIS_STATUS_SUCCESS)
    {
        DEBUGP(("NdisMRegisterMiniport error, enStatus = 0x%08x\n", enStatus));
        NdisTerminateWrapper(g_NdisWrapperHandle, NULL);
    }
    else
    {
        VNDIS_Instance_Init();
        NdisMRegisterUnloadHandler(g_NdisWrapperHandle, VNDIS_Init_UnLoad);
    }

    return enStatus;
}


