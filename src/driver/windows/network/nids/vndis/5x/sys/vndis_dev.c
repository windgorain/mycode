/******************************************************************************
* Copyright (C), LiXingang
* Author:      Xingang.Li  Version: 1.0  Date: 2012-3-30
* Description:
* History:
******************************************************************************/
#include <ntifs.h>
#include <ndis.h>
#include <ntstrsafe.h>

#include "vndis_def.h"
#include "vndis_que.h"
#include "vndis_pub.h"
#include "vndis_dev.h"
#include "vndis_adapter.h"
#include "vndis_instance.h"
#include "vndis_mem.h"
#include "vndis_init.h"

#define VNDIS_COPY_MAC(dst, src) \
{   \
    (dst)[0] = (src)[0];    \
    (dst)[1] = (src)[1];    \
    (dst)[2] = (src)[2];    \
    (dst)[3] = (src)[3];    \
    (dst)[4] = (src)[4];    \
    (dst)[5] = (src)[5];    \
}

#define VNDIS_DEV_PACKET_QUEUE_SIZE   64
#define VNDIS_DEV_IRP_QUEUE_SIZE      16

#define VNDIS_DEV_MAX_DEV_NAME_ASCII 80
#define VNDIS_DEV_SYSDEVICEDIR      "\\Device\\"
#define VNDIS_DEV_USERDEVICEDIR     "\\DosDevices\\Global\\"
#define VNDIS_DEV_VNDIS_SUFFIX      ".nds"

#define  VNDIS_DEV_LOCK_QUE(pstDev)      NdisAcquireSpinLock(&(pstDev)->stQueLock)
#define  VNDIS_DEV_UNLOCK_QUE(pstDev)    NdisReleaseSpinLock(&(pstDev)->stQueLock)

DRIVER_CANCEL vndis_dev_CancelIRPCallback;
VOID vndis_dev_CancelIRPCallback
(
    IN PDEVICE_OBJECT pstDeviceObject,
    IN PIRP pstIRP
);
DRIVER_DISPATCH vndis_dev_DeviceHook;
NTSTATUS vndis_dev_DeviceHook (IN PDEVICE_OBJECT pstDeviceObject, IN PIRP pstIRP);


/* 根据Adapter的名字获取Dev的SubName, 比如\device\_INF_{XXX}的SubName就是{XXX} */
static CHAR * vndis_dev_GetDevSubNameByAdapterName
(
    IN CHAR *pcAdapterName
)
{
    CHAR *pcDevName;

    pcDevName = pcAdapterName;
    while (*pcDevName != '{')
    {
        if (*pcDevName == '\0')
        {
            return NULL;
        }
        pcDevName ++;
    }

    return pcDevName;
}

/* 获取DEV的UNICODE名字 */
static NDIS_STATUS vndis_dev_GetDevName
(
    IN VNDIS_DEV_S *pstDev,
    IN CHAR *pcAdapterName,
    OUT UNICODE_STRING *pstDevName
)
{
    CHAR *pcSubName = "vndis";
    CHAR szDevAsciiName[VNDIS_DEV_MAX_DEV_NAME_ASCII];
    CHAR szDevAsciiLinkName[VNDIS_DEV_MAX_DEV_NAME_ASCII];
    NTSTATUS enStatus;
    ANSI_STRING l_TapString, l_LinkString;

    (VOID) pcAdapterName;

#if 0
    pcSubName = vndis_dev_GetDevSubNameByAdapterName(pcAdapterName);
    if (NULL == pcSubName)
    {
        return NDIS_STATUS_RESOURCES;
    }
#endif

    enStatus = RtlStringCchPrintfExA(szDevAsciiName,
         VNDIS_DEV_MAX_DEV_NAME_ASCII,
         NULL,
         NULL,
         STRSAFE_FILL_BEHIND_NULL | STRSAFE_IGNORE_NULLS,
         "%s%s%s",
         VNDIS_DEV_SYSDEVICEDIR,
         pcSubName,
         VNDIS_DEV_VNDIS_SUFFIX);
    if (enStatus != STATUS_SUCCESS)
    {
        return NDIS_STATUS_RESOURCES;
    }

    enStatus = RtlStringCchPrintfExA(szDevAsciiLinkName,
        VNDIS_DEV_MAX_DEV_NAME_ASCII,
        NULL,
        NULL,
        STRSAFE_FILL_BEHIND_NULL | STRSAFE_IGNORE_NULLS,
        "%s%s%s",
        VNDIS_DEV_USERDEVICEDIR,
        pcSubName,
        VNDIS_DEV_VNDIS_SUFFIX);
    if (enStatus != STATUS_SUCCESS)
    {
        return NDIS_STATUS_RESOURCES;
    }

    l_TapString.Buffer = szDevAsciiName;
    l_TapString.Length = (USHORT) strlen(szDevAsciiName);
    l_LinkString.Buffer = szDevAsciiLinkName;
    l_LinkString.Length = (USHORT) strlen(szDevAsciiLinkName);

    if (RtlAnsiStringToUnicodeString (pstDevName, &l_TapString, TRUE) != STATUS_SUCCESS)
    {
        return NDIS_STATUS_RESOURCES;
    }

    if (RtlAnsiStringToUnicodeString(&pstDev->stDevLinkName, &l_LinkString, TRUE) != STATUS_SUCCESS)
    {
        RtlFreeUnicodeString (pstDevName);
        return NDIS_STATUS_RESOURCES;
    }

    pstDev->bDevLinkNameCreated = TRUE;

    return NDIS_STATUS_SUCCESS;
}


static BOOLEAN vndis_dev_CancelIRP
(
    IN VNDIS_DEV_S *pstDev,
    IN PIRP pstIRP
)
{
    BOOLEAN bExists = FALSE;

    DEBUG_IN_FUNC();

    if (pstDev != NULL)
    {
        VNDIS_DEV_LOCK_QUE(pstDev);
        if (VNDIS_QUE_ExtractPop(pstDev->hIrpQue, pstIRP) != NULL)
        {
            bExists = TRUE;
        }
        VNDIS_DEV_UNLOCK_QUE(pstDev);
    }

    if (bExists == TRUE)
    {
        IoSetCancelRoutine (pstIRP, NULL);
        pstIRP->IoStatus.Status = STATUS_CANCELLED;
        pstIRP->IoStatus.Information = 0;
    }

    DEBUG_OUT_FUNC(bExists);

    return bExists;
}

VOID vndis_dev_CancelIRPCallback
(
    IN PDEVICE_OBJECT pstDeviceObject,
    IN PIRP pstIRP
)
{
    VNDIS_ADAPTER_S *pstAdapter;
    BOOLEAN bExists = FALSE;

    DEBUG_IN_FUNC();

    pstAdapter = VNDIS_Instance_Search(pstDeviceObject);
    if (NULL == pstAdapter)
    {
        bExists = vndis_dev_CancelIRP(NULL, pstIRP);
    }
    else
    {
        bExists = vndis_dev_CancelIRP(&pstAdapter->stDev, pstIRP);
        VNDIS_Adapter_DecUsing(pstAdapter);
    }

    IoReleaseCancelSpinLock (pstIRP->CancelIrql);

    if (bExists == TRUE)
    {
        IoCompleteRequest (pstIRP, IO_NO_INCREMENT);
    }

    DEBUG_OUT_FUNC(0);
}

static BOOLEAN vndis_dev_FlushIrpQue(IN VNDIS_DEV_S *pstDev)
{
    PIRP pstIRP;
    BOOLEAN bHaveIrp = FALSE;

    DEBUG_IN_FUNC();

    do {
        VNDIS_DEV_LOCK_QUE(pstDev);
        pstIRP = (PIRP)VNDIS_QUE_Pop(pstDev->hIrpQue);
        VNDIS_DEV_UNLOCK_QUE(pstDev);

        if (NULL != pstIRP)
        {
            vndis_dev_CancelIRP(NULL, pstIRP);
            IoCompleteRequest(pstIRP, IO_NO_INCREMENT);
            bHaveIrp = TRUE;
        }
    } while(NULL != pstIRP);

    DEBUG_OUT_FUNC(bHaveIrp);

    return bHaveIrp;
}

static VOID vndis_dev_FlushPacketQue(IN VNDIS_DEV_S *pstDev)
{
    VNDIS_DEV_PACKET_S *pstPacket;

    DEBUG_IN_FUNC();

    do {
        VNDIS_DEV_LOCK_QUE(pstDev);
        pstPacket = (VNDIS_DEV_PACKET_S*)VNDIS_QUE_Pop(pstDev->hPacketQue);
        VNDIS_DEV_UNLOCK_QUE(pstDev);

        if (NULL != pstPacket)
        {
            VNDIS_MEM_Free(pstPacket, sizeof(VNDIS_DEV_PACKET_S));
        }
    } while(NULL != pstPacket);

    DEBUG_OUT_FUNC(0);

    return;
}

static BOOLEAN vndis_dev_FlushQue(IN VNDIS_DEV_S *pstDev)
{
    BOOLEAN bHaveIrp;

    DEBUG_IN_FUNC();

    bHaveIrp = vndis_dev_FlushIrpQue(pstDev);
    vndis_dev_FlushPacketQue(pstDev);

    DEBUG_OUT_FUNC(bHaveIrp);

    return bHaveIrp;
}

/* 释放DEV的资源 */
static VOID vndis_dev_FreeDev(IN VNDIS_DEV_S *pstDev)
{
    if (vndis_dev_FlushQue(pstDev) == TRUE)
    {
        NdisMSleep (500000);  /* 等待用户态处理完IRP */
    }

    if (pstDev->bDevLinkNameCreated == TRUE)
    {
        RtlFreeUnicodeString(&pstDev->stDevLinkName);
        pstDev->bDevLinkNameCreated = FALSE;
    }

    if (pstDev->pstDeviceObject != NULL)
    {
        NdisMDeregisterDevice (pstDev->hDeviceHandle);
        pstDev->pstDeviceObject = NULL;
        pstDev->hDeviceHandle = NULL;
    }

    if (pstDev->hPacketQue != NULL)
    {
        VNDIS_QUE_Destory(pstDev->hPacketQue);
        pstDev->hPacketQue = NULL;
    }

    if (pstDev->hIrpQue != NULL)
    {
        VNDIS_QUE_Destory(pstDev->hIrpQue);
        pstDev->hIrpQue = NULL;
    }

    if (pstDev->bQueLockInit == TRUE)
    {
        pstDev->bQueLockInit = FALSE;
        NdisFreeSpinLock(&pstDev->stQueLock);
    }
}

static NTSTATUS vndis_dev_DealIoctlRequest
(
    IN VNDIS_ADAPTER_S *pstAdapter,
    IN PIO_STACK_LOCATION pstIrpSp,
    IN PIRP pstIRP
)
{
    VOID *pIrpBuf = pstIRP->AssociatedIrp.SystemBuffer;
    UINT uiInformationLen = 0;
    NTSTATUS enStatus = STATUS_SUCCESS;
    UINT uiOutputBufferLen = pstIrpSp->Parameters.DeviceIoControl.OutputBufferLength;
    UINT uiSize;

    DEBUG_IN_FUNC();

    switch (pstIrpSp->Parameters.DeviceIoControl.IoControlCode)
    {
        case VNDIS_PUB_IOCTL_GET_MAC:
        {
            if (uiOutputBufferLen >= sizeof (MACADDR))
            {
                VNDIS_COPY_MAC((UCHAR*)pIrpBuf, pstAdapter->aucMacAddr);
                uiInformationLen = sizeof (MACADDR);
            }
            else
            {
                enStatus = STATUS_BUFFER_TOO_SMALL;
            }
            break;
        }

        case VNDIS_PUB_IOCTL_GET_VERSION:
        {
            VNDIS_PUB_VERSION_S *pstVer;
            UINT uiDbg = 0;
#if DBG
            uiDbg = 1;
#endif

            uiSize = sizeof(VNDIS_PUB_VERSION_S);

            if (uiOutputBufferLen >= uiSize)
            {
                pstVer = pIrpBuf;
                pstVer->uiMajorVer = VNDIS_DEV_DRIVER_MAJOR_VERSION;
                pstVer->uiMinorVer = VNDIS_DEV_DRIVER_MINOR_VERSION;
                pstVer->uiDbg = uiDbg;

                uiInformationLen = uiSize;
            }
            else
            {
                enStatus = STATUS_BUFFER_TOO_SMALL;
            }

            break;
        }

        case VNDIS_PUB_IOCTL_GET_MTU:
        {
            uiSize = sizeof (UINT) * 1;
            if (uiOutputBufferLen >= uiSize)
            {
                ((PUINT) pIrpBuf)[0] = pstAdapter->uiMtu;
                uiInformationLen = uiSize;
            }
            else
            {
                enStatus = STATUS_BUFFER_TOO_SMALL;
            }

            break;
        }

        case VNDIS_PUB_IOCTL_GET_MEDIA_STATUS:
        {
            uiSize = sizeof (UINT) * 1;
            if (uiOutputBufferLen >= uiSize)
            {
                ((PUINT) pIrpBuf)[0] = pstAdapter->bMediaStateConnectted;
                uiInformationLen = uiSize;
            }
            else
            {
                enStatus = STATUS_BUFFER_TOO_SMALL;
            }
            break;
        }

        case VNDIS_PUB_IOCTL_SET_MEDIA_STATUS:
        {
            if (uiOutputBufferLen >= (sizeof (UINT) * 1))
            {
                UINT uiParm = ((UINT *)pIrpBuf)[0];
                VNDIS_Adapter_SetMediaStatus (pstAdapter, (BOOLEAN) uiParm);
                uiInformationLen = 1;
            }
            else
            {
                enStatus = STATUS_INVALID_PARAMETER;
            }
            break;
        }

        case VNDIS_PUB_IOCTL_GET_STATUS:
        {
            VNDIS_PUB_STATUS_S *pstStatus;

            uiSize = sizeof (VNDIS_PUB_STATUS_S);

            if (uiOutputBufferLen >= uiSize)
            {
                pstStatus = pIrpBuf;
                pstStatus->uiTxLow = pstAdapter->ulTx & 0xffffffff;
                pstStatus->uiTxHigh = (pstAdapter->ulTx >> 32)& 0xffffffff;
                pstStatus->uiRxLow = pstAdapter->ulRx & 0xffffffff;
                pstStatus->uiRxHigh = (pstAdapter->ulRx >> 32) & 0xffffffff;
                pstStatus->uiTxErr = pstAdapter->ulTxErr;
                pstStatus->uiRxErr = pstAdapter->ulRxErr;

                uiInformationLen = uiSize;
            }
            else
            {
                enStatus = STATUS_BUFFER_TOO_SMALL;
            }

            break;
        }

        case VNDIS_PUB_IOCTL_GET_ADAPTER_NAME:
        {
            CHAR *pcName = pstAdapter->sNameAnsi.Buffer;

            if (pcName == NULL)
            {
                enStatus = STATUS_UNSUCCESSFUL;
            }
            else
            {
                uiSize = (UINT)strlen(pcName) + 1;

                if (uiOutputBufferLen >= uiSize)
                {
                    VNDIS_MEM_Copy(pIrpBuf, pcName, uiSize);
                    uiInformationLen = uiSize;
                }
                else
                {
                    enStatus = STATUS_BUFFER_TOO_SMALL;
                }
            }

            break;
        }

        default:
        {
            enStatus = STATUS_INVALID_PARAMETER;
            break;
        }
    }

    pstIRP->IoStatus.Information = uiInformationLen;

    DEBUG_OUT_FUNC(enStatus);

    return enStatus;
}

static NTSTATUS vndis_dev_DealCreateRequest
(
    IN VNDIS_ADAPTER_S *pstAdapter,
    IN PIO_STACK_LOCATION pstIrpSp,
    IN PIRP pstIRP
)
{
    (VOID) pstIrpSp;
    (VOID) pstAdapter;
    (VOID) pstIRP;

    DEBUG_IN_FUNC();
    DEBUG_OUT_FUNC(0);
    /* TODO : */

    return STATUS_SUCCESS;
}

static NTSTATUS vndis_dev_DealCloseRequest
(
    IN VNDIS_ADAPTER_S *pstAdapter,
    IN PIO_STACK_LOCATION pstIrpSp,
    IN PIRP pstIRP
)
{
    (VOID) pstIrpSp;
    (VOID) pstIRP;

    DEBUG_IN_FUNC();

    vndis_dev_FlushQue(&pstAdapter->stDev);
    VNDIS_Adapter_SetMediaStatus (pstAdapter, FALSE);

    DEBUG_OUT_FUNC(0);

    return STATUS_SUCCESS;
}

static NTSTATUS vndis_dev_CopyPacketToIrp
(
    IN PIRP pstIRP,
    IN VNDIS_DEV_PACKET_S *pstPacket,
    IN UINT ulIrpBufLen
)
{
    NTSTATUS enStatus = STATUS_SUCCESS;

    IoSetCancelRoutine (pstIRP, NULL);

    if (ulIrpBufLen < pstPacket->uiPacketLen)
    {
        pstIRP->IoStatus.Information = 0;
        pstIRP->IoStatus.Status = STATUS_BUFFER_OVERFLOW;
        enStatus = STATUS_BUFFER_OVERFLOW;
    }
    else
    {
        pstIRP->IoStatus.Information = pstPacket->uiPacketLen;
        VNDIS_MEM_Copy (pstIRP->AssociatedIrp.SystemBuffer,
            pstPacket->aucPacketData,
            pstPacket->uiPacketLen);

        pstIRP->IoStatus.Status = STATUS_SUCCESS;
    }

    VNDIS_MEM_Free(pstPacket, sizeof(VNDIS_DEV_PACKET_S));

    return STATUS_SUCCESS;
}

static NTSTATUS vndis_dev_DealReadRequest
(
    IN VNDIS_ADAPTER_S *pstAdapter,
    IN PIO_STACK_LOCATION pstIrpSp,
    IN PIRP pstIRP
)
{
    VNDIS_DEV_PACKET_S *pstPacket;
    UINT uiIrpBufLen;
    VNDIS_DEV_S *pstDev;
    NTSTATUS enNtStatus;
    BOOLEAN bFalse = FALSE;

    DEBUG_IN_FUNC();

	uiIrpBufLen = pstIrpSp->Parameters.Read.Length;

    do {
        if (pstAdapter->bMediaStateConnectted == FALSE)
        {
            enNtStatus = STATUS_UNSUCCESSFUL;
            break;
        }

    	if (pstIRP->MdlAddress == NULL)
        {
            enNtStatus = STATUS_INVALID_PARAMETER;
            break;
        }

        pstIRP->AssociatedIrp.SystemBuffer = MmGetSystemAddressForMdlSafe(pstIRP->MdlAddress, NormalPagePriority);
    	if (pstIRP->AssociatedIrp.SystemBuffer == NULL)
        {
            enNtStatus = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        pstDev = &pstAdapter->stDev;

        enNtStatus = STATUS_SUCCESS;

        VNDIS_DEV_LOCK_QUE(pstDev);
        pstPacket = VNDIS_QUE_Pop(pstDev->hPacketQue);
        if (NULL == pstPacket)
        {
            pstIRP->IoStatus.Information = uiIrpBufLen;

            if (VNDIS_QUE_Push(pstDev->hIrpQue, pstIRP) == NULL)
            {
                pstIRP->IoStatus.Information = 0;
                enNtStatus = STATUS_UNSUCCESSFUL;
            }
            else
            {
                IoSetCancelRoutine(pstIRP, vndis_dev_CancelIRPCallback);
                IoMarkIrpPending(pstIRP);
                enNtStatus = STATUS_PENDING;
            }
        }
        VNDIS_DEV_UNLOCK_QUE(pstDev);

        if (pstPacket != NULL)
        {
            enNtStatus = vndis_dev_CopyPacketToIrp(pstIRP, pstPacket, uiIrpBufLen);
        }
    } while (bFalse);

    DEBUG_OUT_FUNC(enNtStatus);

    return enNtStatus;
}

/* 获取以太链路层头的长度, 返回0xffff表示出错 */
static USHORT vndis_dev_GetLinkHeadLen(IN UCHAR *pucData, IN UINT uiDataLen)
{
    USHORT usOffSet;
    VNDIS_ETH_HEADER_S *pstHeader;
    VNDIS_ETH_SNAPENCAP_S* pstSnapHdr;
    USHORT usPktLenOrType;

    if (uiDataLen <= sizeof(VNDIS_ETH_HEADER_S))
    {
        return 0xffff;
    }

    pstHeader = (VNDIS_ETH_HEADER_S*)pucData;

    usPktLenOrType = ntohs(pstHeader->proto);

    usOffSet = sizeof(VNDIS_ETH_HEADER_S);

    if (ETH_IS_PKTTYPE(usPktLenOrType))
    {
        if(ETHERTYPE_ISIS2 == usPktLenOrType)
        {
            /* ISIS2类型 */
            usOffSet += ETH_LLC_LEN;
        }
    }
    else if (ETH_IS_PKTLEN(usPktLenOrType))
    {
        pstSnapHdr = (VNDIS_ETH_SNAPENCAP_S*)pucData;

        if (usPktLenOrType > uiDataLen - usOffSet)
        {
            return 0xffff;  /* 非法帧长度，丢弃 */
        }

        /* Ethernet_SNAP格式 */
        if ((0xaa == pstSnapHdr->ucDSAP ) && (0xaa == pstSnapHdr->ucSSAP))
        {
            usOffSet += (ETH_LLC_LEN + ETH_SNAP_LEN);
        }
        /*raw 封装*/
        else if ((0xff == pstSnapHdr->ucDSAP) && (0xff == pstSnapHdr->ucSSAP))
        {
        }
        /* ISIS */
        else if ((0xfe == pstSnapHdr->ucDSAP) &&
                 (0xfe == pstSnapHdr->ucSSAP) &&
                 (0x03 == pstSnapHdr->ucCtrl))
        {
            usOffSet += ETH_LLC_LEN;
        }
        /*llc(纯802.3封装)*/
        else
        {
            usOffSet += ETH_LLC_LEN;
        }
    }
    else
    {
        /* 保留字段未使用 */
        return 0xffff;
    }

    if (usOffSet >= uiDataLen)
    {
        return 0xffff;
    }

    return usOffSet;
}

static NTSTATUS vndis_dev_DealWriteRequet
(
    IN VNDIS_ADAPTER_S *pstAdapter,
    IN PIO_STACK_LOCATION pstIrpSp,
    IN PIRP pstIRP
)
{
    NTSTATUS enStatus = STATUS_SUCCESS;
    BOOLEAN bFalse = FALSE;
    USHORT usEthHeadLen;

    DEBUG_IN_FUNC();

    do {
        if (pstAdapter->bMediaStateConnectted == FALSE)
        {
            enStatus = STATUS_UNSUCCESSFUL;
            break;
        }

        if (pstIRP->MdlAddress == NULL)
        {
            enStatus = STATUS_INVALID_PARAMETER;
            break;
        }

        pstIRP->AssociatedIrp.SystemBuffer = MmGetSystemAddressForMdlSafe(pstIRP->MdlAddress, NormalPagePriority);
    	if (pstIRP->AssociatedIrp.SystemBuffer == NULL)
        {
            enStatus = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        usEthHeadLen = vndis_dev_GetLinkHeadLen(pstIRP->AssociatedIrp.SystemBuffer, pstIrpSp->Parameters.Write.Length);
        if (usEthHeadLen == 0xffff)
        {
            enStatus = STATUS_UNSUCCESSFUL;
            break;
        }

    	pstIRP->IoStatus.Information = pstIrpSp->Parameters.Write.Length;

    	NdisMEthIndicateReceive(pstAdapter->hAdapterHandle,
    	   (NDIS_HANDLE) pstAdapter,
    	   (VOID*) pstIRP->AssociatedIrp.SystemBuffer,
    	   usEthHeadLen,
    	   (VOID*)((unsigned char *) pstIRP->AssociatedIrp.SystemBuffer + sizeof(VNDIS_ETH_HEADER_S)),
    	   pstIrpSp->Parameters.Write.Length - usEthHeadLen,
    	   pstIrpSp->Parameters.Write.Length - usEthHeadLen);

    	NdisMEthIndicateReceiveComplete (pstAdapter->hAdapterHandle);
    }while (bFalse);

    if (enStatus == STATUS_SUCCESS)
    {
        pstAdapter->ulRx ++;
    }
    else
    {
        pstAdapter->ulRxErr ++;
    }

    DEBUG_OUT_FUNC(enStatus);

	return enStatus;
}

static NTSTATUS vndis_dev_DealRequest
(
    IN VNDIS_ADAPTER_S *pstAdapter,
    IN PIO_STACK_LOCATION pstIrpSp,
    IN PIRP pstIRP
)
{
    NTSTATUS enNtStatus = STATUS_SUCCESS;

    DEBUG_IN_FUNC();

	pstIRP->IoStatus.Status = STATUS_SUCCESS;
	pstIRP->IoStatus.Information = 0;

    switch (pstIrpSp->MajorFunction)
    {
        case IRP_MJ_READ:
        {
            enNtStatus = vndis_dev_DealReadRequest(pstAdapter, pstIrpSp, pstIRP);
            break;
        }

        case IRP_MJ_WRITE:
        {
            enNtStatus = vndis_dev_DealWriteRequet(pstAdapter, pstIrpSp, pstIRP);
            break;
        }

        case IRP_MJ_DEVICE_CONTROL:
        {
            enNtStatus = vndis_dev_DealIoctlRequest(pstAdapter, pstIrpSp, pstIRP);
            break;
        }

        case IRP_MJ_CREATE:
        {
            enNtStatus = vndis_dev_DealCreateRequest(pstAdapter, pstIrpSp, pstIRP);
            break;
        }

        case IRP_MJ_CLOSE:
        {
            enNtStatus = vndis_dev_DealCloseRequest(pstAdapter, pstIrpSp, pstIRP);
            break;
        }

        default:
        {
            enNtStatus = STATUS_UNSUCCESSFUL;
            break;
        }
    }

    DEBUG_OUT_FUNC(enNtStatus);

    return enNtStatus;
}

NTSTATUS vndis_dev_DeviceHook (IN PDEVICE_OBJECT pstDeviceObject, IN PIRP pstIRP)
{
    VNDIS_ADAPTER_S *pstAdapter;
    PIO_STACK_LOCATION pstIrpSp;
    NTSTATUS enNtStatus = STATUS_SUCCESS;

    DEBUG_IN_FUNC();

    pstIrpSp = IoGetCurrentIrpStackLocation (pstIRP);
    pstIRP->IoStatus.Status = STATUS_SUCCESS;
    pstIRP->IoStatus.Information = 0;

    pstAdapter = VNDIS_Instance_Search(pstDeviceObject);

    if (pstAdapter != NULL)
    {
        enNtStatus = vndis_dev_DealRequest(pstAdapter, pstIrpSp, pstIRP);
        VNDIS_Adapter_DecUsing(pstAdapter);
    }
    else
    {
        if (pstIrpSp->MajorFunction == IRP_MJ_CLOSE)
        {
            enNtStatus = STATUS_SUCCESS;
        }
        else
        {
            enNtStatus = STATUS_NO_SUCH_DEVICE;
        }
    }

    pstIRP->IoStatus.Status = enNtStatus;

    if (STATUS_PENDING != enNtStatus)
    {
        IoCompleteRequest (pstIRP, IO_NO_INCREMENT);
    }

    DEBUG_OUT_FUNC(enNtStatus);

    return enNtStatus;
}

/* 注册一个Device */
static NDIS_STATUS vndis_dev_RegDev
(
    IN VNDIS_DEV_S *pstDev,
    IN UNICODE_STRING      *pstDevName
)
{
    PDRIVER_DISPATCH    DispatchTable[IRP_MJ_MAXIMUM_FUNCTION+1];
    NTSTATUS            enNtStatus;

    NdisZeroMemory(DispatchTable, (IRP_MJ_MAXIMUM_FUNCTION+1) * sizeof(PDRIVER_DISPATCH));
    DispatchTable[IRP_MJ_CREATE] = vndis_dev_DeviceHook;
    DispatchTable[IRP_MJ_CLOSE] = vndis_dev_DeviceHook;
    DispatchTable[IRP_MJ_DEVICE_CONTROL] = vndis_dev_DeviceHook;
    DispatchTable[IRP_MJ_READ] = vndis_dev_DeviceHook;
    DispatchTable[IRP_MJ_WRITE] = vndis_dev_DeviceHook;

    enNtStatus = NdisMRegisterDevice(VNDIS_Init_GetWrapperHandle(),
         pstDevName,
         &pstDev->stDevLinkName,
         DispatchTable,
         &pstDev->pstDeviceObject,
         &pstDev->hDeviceHandle
     );

    if (enNtStatus != STATUS_SUCCESS)
    {
        return NDIS_STATUS_RESOURCES;
    }

    pstDev->pstDeviceObject->Flags |= DO_DIRECT_IO;

    return NDIS_STATUS_SUCCESS;
}

static NDIS_STATUS vndis_dev_DevInit
(
    IN VNDIS_DEV_S *pstDev
)
{
    NdisAllocateSpinLock(&pstDev->stQueLock);
    pstDev->bQueLockInit = TRUE;

    pstDev->hPacketQue = VNDIS_QUE_Create(VNDIS_DEV_PACKET_QUEUE_SIZE);
    pstDev->hIrpQue    = VNDIS_QUE_Create(VNDIS_DEV_IRP_QUEUE_SIZE);

    if ((pstDev->hPacketQue == NULL) || (pstDev->hIrpQue == NULL))
    {
        return NDIS_STATUS_RESOURCES;
    }

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS VNDIS_DEV_CreateDevice
(
    IN VNDIS_DEV_S *pstDev,
    IN CHAR *pcAdapterName
)
{
    NDIS_STATUS         enStatus = NDIS_STATUS_SUCCESS;
    UNICODE_STRING      stDevName;
    BOOLEAN             bFalse = FALSE;

    enStatus = vndis_dev_GetDevName(pstDev, pcAdapterName, &stDevName);
    if (enStatus != NDIS_STATUS_SUCCESS)
    {
        return enStatus;
    }

    do {
        enStatus = vndis_dev_DevInit(pstDev);
        if (NDIS_STATUS_SUCCESS != enStatus)
        {
            break;
        }

        enStatus = vndis_dev_RegDev(pstDev, &stDevName);
        if (NDIS_STATUS_SUCCESS != enStatus)
        {
            break;
        }
    } while(bFalse);

    RtlFreeUnicodeString(&stDevName);

    if (enStatus != NDIS_STATUS_SUCCESS)
    {
        vndis_dev_FreeDev(pstDev);
    }

    return enStatus;
}

VOID VNDIS_DEV_DestoryDevice(IN VNDIS_DEV_S *pstDev)
{
    vndis_dev_FreeDev(pstDev);
}

VOID VNDIS_DEV_AllowNonAdmin(IN VNDIS_DEV_S *pstDev)
{
    NTSTATUS stat;
    SECURITY_DESCRIPTOR sd;
    OBJECT_ATTRIBUTES oa;
    IO_STATUS_BLOCK isb;
    HANDLE hand = NULL;

    NdisZeroMemory (&sd, sizeof (sd));
    NdisZeroMemory (&oa, sizeof (oa));
    NdisZeroMemory (&isb, sizeof (isb));

    if (pstDev->bDevLinkNameCreated != TRUE)
    {
        DEBUGP (("[TAP] AllowNonAdmin: UnicodeLinkName is uninitialized\n"));
        return;
    }

    stat = RtlCreateSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
    if (stat != STATUS_SUCCESS)
    {
        DEBUGP (("[TAP] AllowNonAdmin: RtlCreateSecurityDescriptor failed\n"));
        return;
    }

    InitializeObjectAttributes (&oa, &pstDev->stDevLinkName, OBJ_KERNEL_HANDLE, NULL, NULL);

    stat = ZwOpenFile(&hand, WRITE_DAC, &oa, &isb, 0,0);
    if (stat != STATUS_SUCCESS)
    {
        DEBUGP (("[TAP] AllowNonAdmin: ZwOpenFile failed, status=0x%08x\n", (unsigned int)stat));
        return;
    }

    stat = ZwSetSecurityObject (hand, DACL_SECURITY_INFORMATION, &sd);
    if (stat != STATUS_SUCCESS)
    {
        DEBUGP (("[TAP] AllowNonAdmin: ZwSetSecurityObject failed\n"));
        return;
    }

    stat = ZwClose (hand);
    if (stat != STATUS_SUCCESS)
    {
        DEBUGP (("[TAP] AllowNonAdmin: ZwClose failed\n"));
        return;
    }
}

static VOID vndis_dev_CopyPacket
(
    IN PNDIS_BUFFER pNdisBufferFrom,
    IN UINT uiPacketLength,
    IN VNDIS_DEV_PACKET_S *pstPacketTo
)
{
    UINT uiIndex = 0;
    UINT uiBufferLength = 0;
    VOID *pBuffer = NULL;
    PNDIS_BUFFER pNdisBuffer = pNdisBufferFrom;

    uiIndex = 0;
    while ((uiIndex < uiPacketLength) && (pNdisBuffer != NULL))
    {
        NdisQueryBufferSafe(pNdisBuffer, &pBuffer, &uiBufferLength, NormalPagePriority);
        VNDIS_MEM_Copy(pstPacketTo->aucPacketData + uiIndex, pBuffer, uiBufferLength);
        uiIndex += uiBufferLength;
        NdisGetNextBuffer (pNdisBuffer, &pNdisBuffer);
    }

    pstPacketTo->uiPacketLen = uiPacketLength;

    return;
}

NDIS_STATUS VNDIS_DEV_SendPacket
(
    IN VNDIS_DEV_S *pstDev,
    IN PNDIS_PACKET pPacket
)
{
    VNDIS_DEV_PACKET_S *pstPacket;
    PNDIS_BUFFER pNdisBuffer;
    UINT uiPacketLength;
    UINT uiPhysicalBufferCount;
    UINT BufferCount;
    PIRP pstIRP;
    NDIS_STATUS enStatus;

    NdisQueryPacket (pPacket, &uiPhysicalBufferCount, &BufferCount, &pNdisBuffer, &uiPacketLength);

    if ((uiPacketLength > VNDIS_MAX_ETH_PACKET_SIZE) || (pNdisBuffer == NULL))
    {
        return NDIS_STATUS_FAILURE;
    }

    if (VNDIS_QUE_IsFull(pstDev->hPacketQue) == TRUE)
    {
        return NDIS_STATUS_FAILURE;
    }

    pstPacket = VNDIS_MEM_Malloc(sizeof(VNDIS_DEV_PACKET_S));
    if (NULL == pstPacket)
    {
        return NDIS_STATUS_FAILURE;
    }

    vndis_dev_CopyPacket(pNdisBuffer, uiPacketLength, pstPacket);

    enStatus = NDIS_STATUS_SUCCESS;

    VNDIS_DEV_LOCK_QUE(pstDev);
    pstIRP = VNDIS_QUE_Pop(pstDev->hIrpQue);
    if (NULL == pstIRP)
    {
        if (VNDIS_QUE_Push(pstDev->hPacketQue, pstPacket) == NULL)
        {
            enStatus = NDIS_STATUS_FAILURE;
        }
    }
    VNDIS_DEV_UNLOCK_QUE(pstDev);

    if (enStatus != NDIS_STATUS_SUCCESS)
    {
        VNDIS_MEM_Free(pstPacket, sizeof(VNDIS_DEV_PACKET_S));
    }

    if (NULL != pstIRP)
    {
        if (STATUS_SUCCESS == vndis_dev_CopyPacketToIrp(pstIRP, pstPacket, (UINT)pstIRP->IoStatus.Information))
        {
            IoCompleteRequest (pstIRP, IO_NETWORK_INCREMENT);
        }
        else
        {
            IoCompleteRequest (pstIRP, IO_NO_INCREMENT);
        }
    }

    return NDIS_STATUS_SUCCESS;
}


