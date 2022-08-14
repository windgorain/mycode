/******************************************************************************
* Copyright (C), LiXingang
* Author:      Xingang.Li  Version: 1.0  Date: 2012-3-29
* Description: 
* History:     
******************************************************************************/
#include "ndis.h"

#include "vndis_def.h"    
#include "vndis_pub.h"
#include "vndis_que.h"
#include "vndis_dev.h"
#include "vndis_adapter.h"
#include "vndis_instance.h"
#include "vndis_mem.h"
#include "vndis_mac.h"


typedef struct
{
  unsigned char  opaque[16];
  UNICODE_STRING MiniportName;       // how mini-port refers to us
}VNDIS_ADAPTER_WIN2K_NDIS_MINIPORT_BLOCK_S;
 
static NDIS_OID g_auiVndisAdapterNICSupportedOids[] =
{
        OID_GEN_SUPPORTED_LIST,
        OID_GEN_HARDWARE_STATUS,
        OID_GEN_MEDIA_SUPPORTED,
        OID_GEN_MEDIA_IN_USE,
        OID_GEN_MAXIMUM_LOOKAHEAD,
        OID_GEN_MAXIMUM_FRAME_SIZE,
        OID_GEN_LINK_SPEED,
        OID_GEN_TRANSMIT_BUFFER_SPACE,
        OID_GEN_RECEIVE_BUFFER_SPACE,
        OID_GEN_TRANSMIT_BLOCK_SIZE,
        OID_GEN_RECEIVE_BLOCK_SIZE,
        OID_GEN_VENDOR_ID,
        OID_GEN_VENDOR_DESCRIPTION,
        OID_GEN_VENDOR_DRIVER_VERSION,
        OID_GEN_CURRENT_PACKET_FILTER,
        OID_GEN_CURRENT_LOOKAHEAD,
        OID_GEN_DRIVER_VERSION,
        OID_GEN_MAXIMUM_TOTAL_SIZE,
        OID_GEN_PROTOCOL_OPTIONS,
        OID_GEN_MAC_OPTIONS,
        OID_GEN_MEDIA_CONNECT_STATUS,
        OID_GEN_MAXIMUM_SEND_PACKETS,
        OID_GEN_XMIT_OK,
        OID_GEN_RCV_OK,
        OID_GEN_XMIT_ERROR,
        OID_GEN_RCV_ERROR,
        OID_GEN_RCV_NO_BUFFER,
        OID_GEN_RCV_CRC_ERROR,
        OID_GEN_TRANSMIT_QUEUE_LENGTH,
        OID_802_3_PERMANENT_ADDRESS,
        OID_802_3_CURRENT_ADDRESS,
        OID_802_3_MULTICAST_LIST,
        OID_802_3_MAC_OPTIONS,
        OID_802_3_MAXIMUM_LIST_SIZE,
        OID_802_3_RCV_ERROR_ALIGNMENT,
        OID_802_3_XMIT_ONE_COLLISION,
        OID_802_3_XMIT_MORE_COLLISIONS,
        OID_802_3_XMIT_DEFERRED,
        OID_802_3_XMIT_MAX_COLLISIONS,
        OID_802_3_RCV_OVERRUN,
        OID_802_3_XMIT_UNDERRUN,
        OID_802_3_XMIT_HEARTBEAT_FAILURE,
        OID_802_3_XMIT_TIMES_CRS_LOST,
        OID_802_3_XMIT_LATE_COLLISIONS,
        OID_PNP_CAPABILITIES,
        OID_PNP_SET_POWER,
        OID_PNP_QUERY_POWER,
        OID_PNP_ADD_WAKE_UP_PATTERN,
        OID_PNP_REMOVE_WAKE_UP_PATTERN,
        OID_PNP_ENABLE_WAKE_UP
};

static CHAR * vndis_adapter_GetOidName(IN NDIS_OID uiOid)
{
    PCHAR oidName;

    switch (uiOid){

        #undef MAKECASE
        #define MAKECASE(oidx) case oidx: oidName = #oidx; break;

        MAKECASE(OID_GEN_SUPPORTED_LIST)
        MAKECASE(OID_GEN_HARDWARE_STATUS)
        MAKECASE(OID_GEN_MEDIA_SUPPORTED)
        MAKECASE(OID_GEN_MEDIA_IN_USE)
        MAKECASE(OID_GEN_MAXIMUM_LOOKAHEAD)
        MAKECASE(OID_GEN_MAXIMUM_FRAME_SIZE)
        MAKECASE(OID_GEN_LINK_SPEED)
        MAKECASE(OID_GEN_TRANSMIT_BUFFER_SPACE)
        MAKECASE(OID_GEN_RECEIVE_BUFFER_SPACE)
        MAKECASE(OID_GEN_TRANSMIT_BLOCK_SIZE)
        MAKECASE(OID_GEN_RECEIVE_BLOCK_SIZE)
        MAKECASE(OID_GEN_VENDOR_ID)
        MAKECASE(OID_GEN_VENDOR_DESCRIPTION)
        MAKECASE(OID_GEN_CURRENT_PACKET_FILTER)
        MAKECASE(OID_GEN_CURRENT_LOOKAHEAD)
        MAKECASE(OID_GEN_DRIVER_VERSION)
        MAKECASE(OID_GEN_MAXIMUM_TOTAL_SIZE)
        MAKECASE(OID_GEN_PROTOCOL_OPTIONS)
        MAKECASE(OID_GEN_MAC_OPTIONS)
        MAKECASE(OID_GEN_MEDIA_CONNECT_STATUS)
        MAKECASE(OID_GEN_MAXIMUM_SEND_PACKETS)
        MAKECASE(OID_GEN_VENDOR_DRIVER_VERSION)
        MAKECASE(OID_GEN_SUPPORTED_GUIDS)
        MAKECASE(OID_GEN_NETWORK_LAYER_ADDRESSES)
        MAKECASE(OID_GEN_TRANSPORT_HEADER_OFFSET)
        MAKECASE(OID_GEN_MEDIA_CAPABILITIES)
        MAKECASE(OID_GEN_PHYSICAL_MEDIUM)
        MAKECASE(OID_GEN_XMIT_OK)
        MAKECASE(OID_GEN_RCV_OK)
        MAKECASE(OID_GEN_XMIT_ERROR)
        MAKECASE(OID_GEN_RCV_ERROR)
        MAKECASE(OID_GEN_RCV_NO_BUFFER)
        MAKECASE(OID_GEN_DIRECTED_BYTES_XMIT)
        MAKECASE(OID_GEN_DIRECTED_FRAMES_XMIT)
        MAKECASE(OID_GEN_MULTICAST_BYTES_XMIT)
        MAKECASE(OID_GEN_MULTICAST_FRAMES_XMIT)
        MAKECASE(OID_GEN_BROADCAST_BYTES_XMIT)
        MAKECASE(OID_GEN_BROADCAST_FRAMES_XMIT)
        MAKECASE(OID_GEN_DIRECTED_BYTES_RCV)
        MAKECASE(OID_GEN_DIRECTED_FRAMES_RCV)
        MAKECASE(OID_GEN_MULTICAST_BYTES_RCV)
        MAKECASE(OID_GEN_MULTICAST_FRAMES_RCV)
        MAKECASE(OID_GEN_BROADCAST_BYTES_RCV)
        MAKECASE(OID_GEN_BROADCAST_FRAMES_RCV)
        MAKECASE(OID_GEN_RCV_CRC_ERROR)
        MAKECASE(OID_GEN_TRANSMIT_QUEUE_LENGTH)
        MAKECASE(OID_GEN_GET_TIME_CAPS)
        MAKECASE(OID_GEN_GET_NETCARD_TIME)
        MAKECASE(OID_GEN_NETCARD_LOAD)
        MAKECASE(OID_GEN_DEVICE_PROFILE)
        MAKECASE(OID_GEN_INIT_TIME_MS)
        MAKECASE(OID_GEN_RESET_COUNTS)
        MAKECASE(OID_GEN_MEDIA_SENSE_COUNTS)
        MAKECASE(OID_PNP_CAPABILITIES)
        MAKECASE(OID_PNP_SET_POWER)
        MAKECASE(OID_PNP_QUERY_POWER)
        MAKECASE(OID_PNP_ADD_WAKE_UP_PATTERN)
        MAKECASE(OID_PNP_REMOVE_WAKE_UP_PATTERN)
        MAKECASE(OID_PNP_ENABLE_WAKE_UP)
        MAKECASE(OID_802_3_PERMANENT_ADDRESS)
        MAKECASE(OID_802_3_CURRENT_ADDRESS)
        MAKECASE(OID_802_3_MULTICAST_LIST)
        MAKECASE(OID_802_3_MAXIMUM_LIST_SIZE)
        MAKECASE(OID_802_3_MAC_OPTIONS)
        MAKECASE(OID_802_3_RCV_ERROR_ALIGNMENT)
        MAKECASE(OID_802_3_XMIT_ONE_COLLISION)
        MAKECASE(OID_802_3_XMIT_MORE_COLLISIONS)
        MAKECASE(OID_802_3_XMIT_DEFERRED)
        MAKECASE(OID_802_3_XMIT_MAX_COLLISIONS)
        MAKECASE(OID_802_3_RCV_OVERRUN)
        MAKECASE(OID_802_3_XMIT_UNDERRUN)
        MAKECASE(OID_802_3_XMIT_HEARTBEAT_FAILURE)
        MAKECASE(OID_802_3_XMIT_TIMES_CRS_LOST)
        MAKECASE(OID_802_3_XMIT_LATE_COLLISIONS)

        default: 
            oidName = "<** UNKNOWN OID **>";
            break;
    }

    return oidName;
}

static UINT vndis_adapter_GetUsing(IN VNDIS_ADAPTER_S  *pstAdapter)
{
    return pstAdapter->uiUsingCount;
}

static NDIS_STATUS vndis_adapter_SetMedium
(
    OUT PUINT pSelectedMediumIndex,
    IN PNDIS_MEDIUM pMediumArray,
    IN UINT uiMediumArraySize
)
{
    UINT        uiIndex;

    for(uiIndex = 0; uiIndex < uiMediumArraySize; ++uiIndex)
    {
        if(pMediumArray[uiIndex] == NdisMedium802_3)
        {
            break;
        }
    }

    if (uiIndex == uiMediumArraySize)
    {
        return NDIS_STATUS_UNSUPPORTED_MEDIA;
    }

    *pSelectedMediumIndex = uiIndex;

    return NDIS_STATUS_SUCCESS;
}

static VOID vndis_adapter_FreeAdapter (IN VNDIS_ADAPTER_S *pstAdapter)
{
#ifdef NDIS50_MINIPORT
    if (VNDIS_BIT_ISSET(pstAdapter->uiFlag, VNDIS_ADAPTER_REGED_SHUTDOWN_HANDLER))
    {
        NdisMDeregisterAdapterShutdownHandler(pstAdapter->hAdapterHandle);
    }
#endif

    NdisFreeSpinLock(&pstAdapter->stLock);

    VNDIS_MEM_Free(pstAdapter, sizeof(VNDIS_ADAPTER_S));

    return;
}

VOID VNDIS_Adapter_Shutdown(IN NDIS_HANDLE hMiniportAdapterContext)
{
    UNREFERENCED_PARAMETER(hMiniportAdapterContext);
    
    DEBUG_IN_FUNC();
    
    DEBUG_OUT_FUNC(0);

    return;
}

NDIS_STATUS vndis_adapter_BuildAdapterName
(
    IN VNDIS_ADAPTER_S *pstAdapter,
    IN NDIS_HANDLE configHandle
)
{
    NDIS_STATUS status;
    NDIS_STRING mkey = NDIS_STRING_CONST("MiniportName");
    NDIS_STRING vkey = NDIS_STRING_CONST("NdisVersion");
    NDIS_STATUS vstatus = NDIS_STATUS_SUCCESS;
    NDIS_CONFIGURATION_PARAMETER *vparm;
    NDIS_CONFIGURATION_PARAMETER *parm;

    NdisReadConfiguration (&status, &parm, configHandle, &mkey, NdisParameterString);
    if (status == NDIS_STATUS_SUCCESS)
	{
        if (parm->ParameterType == NdisParameterString)
        {
            if (RtlUnicodeStringToAnsiString (
                    &pstAdapter->sNameAnsi,
                    &parm->ParameterData.StringData,
                    TRUE) != STATUS_SUCCESS)
            {
                status = NDIS_STATUS_RESOURCES;
            }
        }
	}
    else
	{
        NdisReadConfiguration (&vstatus, &vparm, configHandle, &vkey, NdisParameterInteger);
        if (vstatus == NDIS_STATUS_SUCCESS && vparm->ParameterData.IntegerData == 0x50000)
        {
            if (RtlUnicodeStringToAnsiString (&pstAdapter->sNameAnsi,
                &((VNDIS_ADAPTER_WIN2K_NDIS_MINIPORT_BLOCK_S *)pstAdapter->hAdapterHandle)->MiniportName,
                TRUE) != STATUS_SUCCESS)
            {
                status = NDIS_STATUS_RESOURCES;
            }
            else
            {
                status = NDIS_STATUS_SUCCESS;
            }
        }
	}

    if ((status != NDIS_STATUS_SUCCESS) || (NULL == pstAdapter->sNameAnsi.Buffer))
    {
        return NDIS_STATUS_RESOURCES;
    }

    return NDIS_STATUS_SUCCESS;
}

/* 获取MTU */
VOID vndis_adapter_GetMTU
(
    IN VNDIS_ADAPTER_S *pstAdapter,
    IN NDIS_HANDLE configHandle
)
{
    NDIS_STRING key = NDIS_STRING_CONST("MTU");
    NDIS_STATUS enStatus;
    NDIS_CONFIGURATION_PARAMETER *pParm;
    int iMtu;

    NdisReadConfiguration (&enStatus, &pParm, configHandle, &key, NdisParameterInteger);
    if (enStatus == NDIS_STATUS_SUCCESS)
    {
        if (pParm->ParameterType == NdisParameterInteger)
        {
            iMtu = pParm->ParameterData.IntegerData;

            if (iMtu < VNDIS_MINIMUM_MTU)
            {
                iMtu = VNDIS_MINIMUM_MTU;
            }
            if (iMtu > VNDIS_MAXIMUM_MTU)
            {
                iMtu = VNDIS_MAXIMUM_MTU;
            }
            pstAdapter->uiMtu = iMtu;
        }
    }

    return;
}

/* 获取连接状态 */
VOID vndis_adapter_GetIfAllowNonAdmin
(
    IN VNDIS_ADAPTER_S *pstAdapter,
    IN NDIS_HANDLE configHandle
)
{
    NDIS_STRING key = NDIS_STRING_CONST("AllowNonAdmin");
    NDIS_STATUS enStatus;
    NDIS_CONFIGURATION_PARAMETER *pParm;

    NdisReadConfiguration (&enStatus, &pParm, configHandle, &key, NdisParameterInteger);
    if (enStatus == NDIS_STATUS_SUCCESS)
    {
        if (pParm->ParameterType == NdisParameterInteger)
        {
            if (pParm->ParameterData.IntegerData)
            {
                pstAdapter->bEnableNonAdmin = TRUE;
            }
        }
    }

    return;
}

/* 获取连接状态 */
VOID vndis_adapter_BuildMac
(
    IN VNDIS_ADAPTER_S *pstAdapter,
    IN NDIS_HANDLE configHandle
)
{
    NDIS_STRING key = NDIS_STRING_CONST("MAC");
    ANSI_STRING mac_string;
    BOOLEAN l_MacFromRegistry = FALSE;
    NDIS_STATUS enStatus;
    NDIS_CONFIGURATION_PARAMETER *pParm;
    UINT i;

    NdisReadConfiguration (&enStatus, &pParm, configHandle, &key, NdisParameterString);
    if (enStatus == NDIS_STATUS_SUCCESS)
    {
        if (pParm->ParameterType == NdisParameterString)
        {
            if (RtlUnicodeStringToAnsiString (&mac_string, &pParm->ParameterData.StringData, TRUE)
                    == STATUS_SUCCESS)
            {
                l_MacFromRegistry = VNDIS_MAC_ParseMAC (pstAdapter->aucMacAddr, mac_string.Buffer);
                RtlFreeAnsiString (&mac_string);
            }
        }
    }

    if (l_MacFromRegistry == FALSE)
    {
        VNDIS_MAC_GenerateRandomMac(pstAdapter->aucMacAddr, (UCHAR*)pstAdapter->sNameAnsi.Buffer);
    }

    for (i=0; i<sizeof(MACADDR); i++)
    {
        pstAdapter->aucBroadcastMac[i] = 0xFF;
    }

    return;
}

static NDIS_STATUS vndis_adapter_ReadParaFromReg
(
    IN VNDIS_ADAPTER_S *pstAdapter,
    IN NDIS_HANDLE hWrapperConfigurationContext
)
{
    NDIS_STATUS enStatus;
    NDIS_HANDLE hConfigHandle;
    BOOLEAN bFalse = FALSE;

    pstAdapter->uiMtu = VNDIS_DEF_MTU;

    NdisOpenConfiguration (&enStatus, &hConfigHandle, hWrapperConfigurationContext);
    if (enStatus != NDIS_STATUS_SUCCESS)
    {
	    return enStatus;
    }

    do {
        enStatus = vndis_adapter_BuildAdapterName(pstAdapter, hConfigHandle);
        if (NDIS_STATUS_SUCCESS != enStatus)
        {
            break;
        }
        vndis_adapter_GetMTU(pstAdapter, hConfigHandle);
        vndis_adapter_GetIfAllowNonAdmin(pstAdapter, hConfigHandle);
        vndis_adapter_BuildMac(pstAdapter, hConfigHandle);
    } while(bFalse);

    NdisCloseConfiguration (hConfigHandle);

    return enStatus;
}

static VOID vndis_adapter_SetAttributes
(
    IN VNDIS_ADAPTER_S *pstAdapter,
    IN NDIS_HANDLE hMiniportAdapterHandle
)
{
    /* 设置属性 */
    NdisMSetAttributesEx(hMiniportAdapterHandle,
        (NDIS_HANDLE)pstAdapter,
        0,
#ifdef NDIS50_MINIPORT
        NDIS_ATTRIBUTE_DESERIALIZE | NDIS_ATTRIBUTE_USES_SAFE_BUFFER_APIS, 
#else
        NDIS_ATTRIBUTE_DESERIALIZE,
#endif
        NdisInterfaceInternal);

#ifdef NDIS50_MINIPORT
    NdisMRegisterAdapterShutdownHandler(
        pstAdapter->hAdapterHandle,
        (PVOID) pstAdapter,
        (ADAPTER_SHUTDOWN_HANDLER) VNDIS_Adapter_Shutdown);
    VNDIS_BIT_SET(pstAdapter->uiFlag, VNDIS_ADAPTER_REGED_SHUTDOWN_HANDLER);
#endif

    return;
}

NDIS_STATUS VNDIS_Adapter_Init
(
    OUT PNDIS_STATUS pOpenErrorStatus,
    OUT PUINT pSelectedMediumIndex,
    IN PNDIS_MEDIUM pMediumArray,
    IN UINT uiMediumArraySize,
    IN NDIS_HANDLE hMiniportAdapterHandle,
    IN NDIS_HANDLE hWrapperConfigurationContext
)
{
    NDIS_STATUS enStatus;
    BOOLEAN     bFalse = FALSE;
    VNDIS_ADAPTER_S *pstAdapter = NULL;

    UNREFERENCED_PARAMETER(pOpenErrorStatus);

    DEBUG_IN_FUNC();

    do {
        enStatus = vndis_adapter_SetMedium(pSelectedMediumIndex, pMediumArray, uiMediumArraySize);
        if (NDIS_STATUS_SUCCESS != enStatus)
        {
            break;
        }

        pstAdapter = VNDIS_MEM_Malloc(sizeof(VNDIS_ADAPTER_S));
        if (NULL == pstAdapter)
        {
            enStatus = NDIS_STATUS_RESOURCES;
            break;
        }

        NdisZeroMemory(pstAdapter, sizeof(VNDIS_ADAPTER_S));

        pstAdapter->hAdapterHandle = hMiniportAdapterHandle;
        NdisAllocateSpinLock(&pstAdapter->stLock);

        enStatus = vndis_adapter_ReadParaFromReg(pstAdapter, hWrapperConfigurationContext);
        if (NDIS_STATUS_SUCCESS != enStatus)
        {
            break;
        }

        vndis_adapter_SetAttributes(pstAdapter, hMiniportAdapterHandle);

        enStatus = VNDIS_DEV_CreateDevice(&pstAdapter->stDev, pstAdapter->sNameAnsi.Buffer);
        if (enStatus != NDIS_STATUS_SUCCESS)
        {
            break;
        }

        if (pstAdapter->bEnableNonAdmin)
        {
            VNDIS_DEV_AllowNonAdmin(&pstAdapter->stDev);
        }
    }while (bFalse);

    if (enStatus != NDIS_STATUS_SUCCESS)
    {
        if (pstAdapter != NULL)
        {
            vndis_adapter_FreeAdapter(pstAdapter);
        }
    }
    else
    {
        VNDIS_Instance_Add(pstAdapter);
    }

    DEBUG_OUT_FUNC(enStatus);

    return enStatus;
}

NDIS_STATUS VNDIS_Adapter_AdapterReset (OUT PBOOLEAN pbAddressingReset, IN NDIS_HANDLE hAdapterContext)
{
    (VOID) pbAddressingReset;
    (VOID) hAdapterContext;

    DEBUG_IN_FUNC();
    DEBUG_OUT_FUNC(0);
    
    return NDIS_STATUS_SUCCESS;
}

VOID VNDIS_Adapter_Halt(IN  NDIS_HANDLE hMiniportAdapterContext)
{
    VNDIS_ADAPTER_S *pstAdapter = (VNDIS_ADAPTER_S *) hMiniportAdapterContext;

    DEBUG_IN_FUNC();

    VNDIS_Instance_Del(pstAdapter);

    while (vndis_adapter_GetUsing(pstAdapter) != 0)
    {
        NdisMSleep(500000);
    }

    VNDIS_Adapter_Shutdown(hMiniportAdapterContext);

    VNDIS_DEV_DestoryDevice(&pstAdapter->stDev);
    vndis_adapter_FreeAdapter(pstAdapter);

    DEBUG_OUT_FUNC(0);

    return;
}

VOID VNDIS_Adapter_SetMediaStatus (IN VNDIS_ADAPTER_S *pstAdapter, IN BOOLEAN bState)
{
    DEBUG_IN_FUNC();

    if (pstAdapter->bMediaStateConnectted != bState)
    {
        if (bState == TRUE)
        {
            NdisMIndicateStatus (pstAdapter->hAdapterHandle, NDIS_STATUS_MEDIA_CONNECT, NULL, 0);
        }
        else
        {
            NdisMIndicateStatus (pstAdapter->hAdapterHandle, NDIS_STATUS_MEDIA_DISCONNECT, NULL, 0);
        }

        NdisMIndicateStatusComplete (pstAdapter->hAdapterHandle);
        pstAdapter->bMediaStateConnectted = bState;
    }

    DEBUG_OUT_FUNC(0);
}

VOID VNDIS_Adapter_IncUsing(IN VNDIS_ADAPTER_S  *pstAdapter)
{
    NdisAcquireSpinLock(&pstAdapter->stLock);
    pstAdapter->uiUsingCount ++;
    NdisReleaseSpinLock(&pstAdapter->stLock);
}

VOID VNDIS_Adapter_DecUsing(IN VNDIS_ADAPTER_S  *pstAdapter)
{
    NdisAcquireSpinLock(&pstAdapter->stLock);
    pstAdapter->uiUsingCount --;
    NdisReleaseSpinLock(&pstAdapter->stLock);
}

NDIS_STATUS VNDIS_Adapter_Receive
(
    OUT PNDIS_PACKET p_Packet,
    OUT PUINT p_Transferred,
    IN NDIS_HANDLE p_AdapterContext,
    IN NDIS_HANDLE p_ReceiveContext,
    IN UINT p_Offset,
    IN UINT p_ToTransfer
)
{
    (VOID)p_Packet;
    (VOID)p_Transferred;
    (VOID)p_AdapterContext;
    (VOID)p_ReceiveContext;
    (VOID)p_Offset;
    (VOID)p_ToTransfer;

    return NDIS_STATUS_SUCCESS;
}

VOID VNDIS_Adapter_SendPackets
(
    IN  NDIS_HANDLE             hMiniportAdapterContext,
    IN  PPNDIS_PACKET           pPacketArray,
    IN  UINT                    uiNumberOfPackets
)
{
    VNDIS_ADAPTER_S  *pstAdapter = NULL;
    UINT             uiPacketCount;
    NDIS_STATUS      enStatus;
    PNDIS_PACKET     pstPacket;
    NDIS_HANDLE      hAdapterHandle;
    VNDIS_DEV_S      *pstDev;

    (VOID)hMiniportAdapterContext;

    DEBUG_IN_FUNC();

    pstAdapter = VNDIS_Instance_SearchByAdapter(hMiniportAdapterContext);
    if (NULL == pstAdapter)
    {
        hAdapterHandle = ((VNDIS_ADAPTER_S*)hMiniportAdapterContext)->hAdapterHandle;
        pstDev = NULL;
    }
    else
    {
        hAdapterHandle = pstAdapter->hAdapterHandle;
        pstDev = &pstAdapter->stDev;
    }

    if (pstAdapter != NULL)
    {
        for(uiPacketCount=0; uiPacketCount < uiNumberOfPackets; uiPacketCount++)
        {
            pstPacket = pPacketArray[uiPacketCount];
            enStatus = VNDIS_DEV_SendPacket(pstDev, pstPacket);

            NDIS_SET_PACKET_STATUS(pstPacket, enStatus);
            if (enStatus != NDIS_STATUS_PENDING)
            {
                NdisMSendComplete(hAdapterHandle, pstPacket, enStatus);
            }

            if ((enStatus == NDIS_STATUS_SUCCESS) || (enStatus == NDIS_STATUS_PENDING))
            {
                pstAdapter->ulTx ++;
            }
            else
            {
                pstAdapter->ulTxErr ++;
            }
        }

        VNDIS_Adapter_DecUsing(pstAdapter);
    }
    else
    {
        for(uiPacketCount=0; uiPacketCount < uiNumberOfPackets; uiPacketCount++)
        {
            pstPacket = pPacketArray[uiPacketCount];
            NDIS_SET_PACKET_STATUS(pstPacket, NDIS_STATUS_FAILURE);
            NdisMSendComplete(hAdapterHandle, pstPacket, NDIS_STATUS_FAILURE);
        }
    }

    DEBUG_OUT_FUNC(0);

    return;
}

static NDIS_STATUS vndis_adapter_NICSetMulticastList
(
    IN VNDIS_ADAPTER_S *pstAdapter,
    IN PVOID       pInformationBuffer,
    IN ULONG       ulInformationBufferLength,
    OUT PULONG     pBytesRead,
    OUT PULONG     pBytesNeeded
)
{
    *pBytesNeeded = ETH_LENGTH_OF_ADDRESS;
    *pBytesRead = ulInformationBufferLength;

    if ((ulInformationBufferLength % ETH_LENGTH_OF_ADDRESS) != 0)
    {
        return NDIS_STATUS_INVALID_LENGTH;
    }

    if (ulInformationBufferLength > sizeof(VNDIS_MC_LIST))
    {
        *pBytesNeeded = sizeof(VNDIS_MC_LIST);
        return NDIS_STATUS_MULTICAST_FULL;
    }

    NdisZeroMemory(&pstAdapter->stMcList, sizeof(VNDIS_MC_LIST));
    
    VNDIS_MEM_Copy(&pstAdapter->stMcList,
                   pInformationBuffer,
                   ulInformationBufferLength);
    
    pstAdapter->uiMCListSize = ulInformationBufferLength / ETH_LENGTH_OF_ADDRESS;


    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS VNDIS_Adapter_SetInformation
(
    IN NDIS_HANDLE hMiniportAdapterContext,
    IN NDIS_OID    uiOid,
    IN PVOID       pInformationBuffer,
    IN ULONG       ulInformationBufferLength,
    OUT PULONG     pulBytesRead,
    OUT PULONG     pulBytesNeeded
)
{
    NDIS_STATUS     enStatus = NDIS_STATUS_INVALID_OID;
    VNDIS_ADAPTER_S *pstAdapter;

    DEBUG_IN_FUNC();
    DEBUGP(("     OID:%s\n", vndis_adapter_GetOidName(uiOid)));

    *pulBytesRead = 0;
    *pulBytesNeeded = 0;

    pstAdapter = VNDIS_Instance_SearchByAdapter(hMiniportAdapterContext);

    if (pstAdapter != NULL)
    {
        switch(uiOid)
        {
            case OID_802_3_MULTICAST_LIST:
            {
                enStatus = vndis_adapter_NICSetMulticastList(
                                pstAdapter,
                                pInformationBuffer,
                                ulInformationBufferLength,
                                pulBytesRead,
                                pulBytesNeeded);
                break;
            }

            case OID_GEN_CURRENT_PACKET_FILTER:
            {
                if(ulInformationBufferLength != sizeof(ULONG))
                {
                    *pulBytesNeeded = sizeof(ULONG);
                    enStatus = NDIS_STATUS_INVALID_LENGTH;
                    break;
                }

                *pulBytesRead = ulInformationBufferLength;                             
                enStatus = NDIS_STATUS_SUCCESS;
                break;
            }

            case OID_GEN_CURRENT_LOOKAHEAD:
            {
                if(ulInformationBufferLength != sizeof(ULONG)){
                    *pulBytesNeeded = sizeof(ULONG);
                    enStatus = NDIS_STATUS_INVALID_LENGTH;
                    break;
                }                
                pstAdapter->ulLookahead = *(PULONG)pInformationBuffer;                

                *pulBytesRead = sizeof(ULONG);
                enStatus = NDIS_STATUS_SUCCESS;
                break;
            }

            default:
            {
                enStatus = NDIS_STATUS_INVALID_OID;
                break;
            }
        }

        VNDIS_Adapter_DecUsing(pstAdapter);
        
        if(enStatus == NDIS_STATUS_SUCCESS)
        {
            *pulBytesRead = ulInformationBufferLength;
        }
    }

    DEBUG_OUT_FUNC(enStatus);

    return(enStatus);
}

NDIS_STATUS VNDIS_Adapter_QueryInformation
(
    IN NDIS_HANDLE  hMiniportAdapterContext,
    IN NDIS_OID     uiOid,
    IN PVOID        pInformationBuffer,
    IN ULONG        ulInformationBufferLength,
    OUT PULONG      pulBytesWritten,
    OUT PULONG      pulBytesNeeded
)
{
    NDIS_STATUS             enStatus = NDIS_STATUS_SUCCESS;
    VNDIS_ADAPTER_S         *pstAdapter = NULL;
    NDIS_HARDWARE_STATUS    enHardwareStatus = NdisHardwareStatusReady;
    NDIS_MEDIUM             enMedium = NdisMedium802_3;
    UCHAR                   szVendorDesc[] = "EZVnet";
    ULONG                   ulInfo;
    USHORT                  usInfo;                                                              
    ULONG64                 ulInfo64;
    PVOID                   pInfo=&ulInfo;
    ULONG                   ulInfoLen = sizeof(ulInfo);  

    (VOID) hMiniportAdapterContext;

    pstAdapter = VNDIS_Instance_SearchByAdapter(hMiniportAdapterContext);

    *pulBytesWritten = 0;
    *pulBytesNeeded = 0;

    if (NULL == pstAdapter)
    {
        return NDIS_STATUS_NOT_SUPPORTED;
    }

    switch(uiOid)
    {
        case OID_GEN_SUPPORTED_LIST:
        {
            pInfo = (PVOID)g_auiVndisAdapterNICSupportedOids;
            ulInfoLen = sizeof(g_auiVndisAdapterNICSupportedOids);
            break;
        }

        case OID_GEN_HARDWARE_STATUS:
        {
            pInfo = (PVOID) &enHardwareStatus;
            ulInfoLen = sizeof(NDIS_HARDWARE_STATUS);
            break;
        }

        case OID_GEN_MEDIA_SUPPORTED:
        case OID_GEN_MEDIA_IN_USE:
        {
            pInfo = (PVOID) &enMedium;
            ulInfoLen = sizeof(NDIS_MEDIUM);
            break;
        }

        case OID_GEN_CURRENT_LOOKAHEAD:
        case OID_GEN_MAXIMUM_LOOKAHEAD:
        {
            if(pstAdapter->ulLookahead == 0)
            {
                pstAdapter->ulLookahead = VNDIS_MAX_ETH_PACKET_SIZE;
            }
            
            ulInfo = pstAdapter->ulLookahead;
            pInfo = &ulInfo;
            break;            
        }

        case OID_GEN_MAXIMUM_FRAME_SIZE:
        {
            ulInfo = pstAdapter->uiMtu;
            pInfo = &ulInfo;
            break;
        }

        case OID_GEN_MAXIMUM_TOTAL_SIZE:
        case OID_GEN_TRANSMIT_BLOCK_SIZE:
        case OID_GEN_RECEIVE_BLOCK_SIZE:
        {
            ulInfo = (ULONG) VNDIS_MAX_ETH_PACKET_SIZE;
            pInfo = &ulInfo;
            break;
        }
            
        case OID_GEN_MAC_OPTIONS:
        {
            ulInfo = NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA | 
                NDIS_MAC_OPTION_TRANSFERS_NOT_PEND |
                NDIS_MAC_OPTION_NO_LOOPBACK |
                NDIS_MAC_OPTION_RECEIVE_SERIALIZED;
            pInfo = &ulInfo;
            break;
        }

        case OID_GEN_LINK_SPEED:
        {
            ulInfo = 100000;
            pInfo = &ulInfo;
            break;
        }

        case OID_GEN_TRANSMIT_BUFFER_SPACE:
        {
            ulInfo = pstAdapter->uiMtu;
            pInfo = &ulInfo;
            break;
        }

        case OID_GEN_RECEIVE_BUFFER_SPACE:
        {
            ulInfo = VNDIS_MAX_ETH_PACKET_SIZE;
            pInfo = &ulInfo;
            break;
        }

        case OID_GEN_VENDOR_ID:
        {
            ulInfo = 0xffffff;
            pInfo = &ulInfo;
            break;
        }

        case OID_GEN_VENDOR_DESCRIPTION:
        {
            pInfo = szVendorDesc;
            ulInfoLen = sizeof(szVendorDesc);
            break;
        }
            
        case OID_GEN_VENDOR_DRIVER_VERSION:
        {
            ulInfo = (VNDIS_DEV_DRIVER_MAJOR_VERSION << 8) | VNDIS_DEV_DRIVER_MINOR_VERSION;
            pInfo = &ulInfo;
            break;
        }

        case OID_GEN_DRIVER_VERSION:
        {
            usInfo = (VNDIS_DEV_DRIVER_MAJOR_VERSION << 8) | VNDIS_DEV_DRIVER_MINOR_VERSION;
            pInfo = (PVOID) &usInfo;
            ulInfoLen = sizeof(USHORT);
            break;
        }

        case OID_GEN_MAXIMUM_SEND_PACKETS:
        {
            ulInfo = 5;
            pInfo = &ulInfo;
            break;
        }

        case OID_GEN_MEDIA_CONNECT_STATUS:
        {
            ulInfo = pstAdapter->bMediaStateConnectted	? NdisMediaStateConnected : NdisMediaStateDisconnected;
            pInfo = &ulInfo;
            break;
        }
            
        case OID_GEN_CURRENT_PACKET_FILTER:
        {
            ulInfo = (NDIS_PACKET_TYPE_ALL_LOCAL | NDIS_PACKET_TYPE_BROADCAST
                     | NDIS_PACKET_TYPE_DIRECTED | NDIS_PACKET_TYPE_ALL_FUNCTIONAL);
            pInfo = &ulInfo;
            break;
        }
                       
        case OID_PNP_CAPABILITIES:
        {
            enStatus = NDIS_STATUS_NOT_SUPPORTED;
            break;
        }

        case OID_802_3_PERMANENT_ADDRESS:
        {
            pInfo = pstAdapter->aucMacAddr;
            ulInfoLen = ETH_LENGTH_OF_ADDRESS;
            break;
        }

        case OID_802_3_CURRENT_ADDRESS:
        {
            pInfo = pstAdapter->aucMacAddr;
            ulInfoLen = ETH_LENGTH_OF_ADDRESS;
            break;
        }

        case OID_802_3_MAXIMUM_LIST_SIZE:
        {
            ulInfo = VNDIS_NIC_MAX_MCAST_LIST;
            pInfo = &ulInfo;
            break;
        }
            
        case OID_802_3_MAC_OPTIONS:
        {
            ulInfo = 0;
            pInfo = &ulInfo;
            break;
        }

        case OID_GEN_XMIT_OK:
        {
            ulInfo64 = pstAdapter->ulTx;
            pInfo = &ulInfo64;
            if ((ulInformationBufferLength >= sizeof(ULONG64)) || (ulInformationBufferLength == 0))
            {
                ulInfoLen = sizeof(ULONG64);
            }
            else
            {
                ulInfoLen = sizeof(ULONG);
            }
            *pulBytesNeeded =  sizeof(ULONG64);                        
            break;
        }
    
        case OID_GEN_RCV_OK:
        {
            ulInfo64 = pstAdapter->ulRx;
            pInfo = &ulInfo64;
            if ((ulInformationBufferLength >= sizeof(ULONG64)) || (ulInformationBufferLength == 0))
            {
                ulInfoLen = sizeof(ULONG64);
            }
            else
            {
                ulInfoLen = sizeof(ULONG);
            }            
            *pulBytesNeeded =  sizeof(ULONG64);                        
            break;
        }

        case OID_GEN_XMIT_ERROR:
        {
            ulInfo = pstAdapter->ulTxErr;
            pInfo = &ulInfo;
            break;
        }
    
        case OID_GEN_RCV_ERROR:
        {
            ulInfo = pstAdapter->ulRxErr;
            pInfo = &ulInfo;
            break;
        }
    
        case OID_GEN_RCV_NO_BUFFER:
        {
            ulInfo = 0;
            pInfo = &ulInfo;
            break;
        }
    
        case OID_GEN_RCV_CRC_ERROR:
        {
            ulInfo = 0;
            pInfo = &ulInfo;
            break;
        }
    
        case OID_GEN_TRANSMIT_QUEUE_LENGTH:
        {
            ulInfo = 0;
            pInfo = &ulInfo;
            break;
        }
    
        case OID_802_3_RCV_ERROR_ALIGNMENT:
        {
            ulInfo = 0;
            pInfo = &ulInfo;
            break;
        }
    
        case OID_802_3_XMIT_ONE_COLLISION:
        {
            ulInfo = 0;
            pInfo = &ulInfo;
            break;
        }
    
        case OID_802_3_XMIT_MORE_COLLISIONS:
        {
            ulInfo = 0;
            pInfo = &ulInfo;
            break;
        }
    
        case OID_802_3_XMIT_DEFERRED:
        {
            ulInfo = 0;
            pInfo = &ulInfo;
            break;
        }
    
        case OID_802_3_XMIT_MAX_COLLISIONS:
        {
            ulInfo = 0;
            pInfo = &ulInfo;
            break;
        }
    
        case OID_802_3_RCV_OVERRUN:
        {
            ulInfo = 0;
            pInfo = &ulInfo;
            break;
        }
    
        case OID_802_3_XMIT_UNDERRUN:
        {
            ulInfo = 0;
            pInfo = &ulInfo;
            break;
        }
    
        case OID_802_3_XMIT_HEARTBEAT_FAILURE:
        {
            ulInfo = 0;
            pInfo = &ulInfo;
            break;
        }
    
        case OID_802_3_XMIT_TIMES_CRS_LOST:
        {
            ulInfo = 0;
            pInfo = &ulInfo;
            break;
        }
    
        case OID_802_3_XMIT_LATE_COLLISIONS:
        {
            ulInfo = 0;
            pInfo = &ulInfo;
            break;
        }
          
        default:
        {
            enStatus = NDIS_STATUS_NOT_SUPPORTED;
            break;
        }
    }

    VNDIS_Adapter_DecUsing(pstAdapter);

    if(enStatus == NDIS_STATUS_SUCCESS)
    {
        if(ulInfoLen <= ulInformationBufferLength)
        {
            *pulBytesWritten = ulInfoLen;
            if(ulInfoLen)
            {
                VNDIS_MEM_Copy(pInformationBuffer, pInfo, ulInfoLen);
            }
        }
        else
        {
            *pulBytesNeeded = ulInfoLen;
            enStatus = NDIS_STATUS_BUFFER_TOO_SHORT;
        }
    }

    return(enStatus);
}



