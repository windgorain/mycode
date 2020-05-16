/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2007-8-27
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

/* func */
#ifdef IN_WINDOWS

#include "utl/vnic_lib.h"
#include "utl/txt_utl.h"
#include "utl/my_ip_helper.h"
#include "vnic_inner.h"

typedef struct
{
    HANDLE hFileHandle;
    CHAR szAdapterGuid[256];
    OVERLAPPED stReadOverLapped;
    OVERLAPPED stWriteOverLapped;
}_OS_VNIC_CTRL_S;


BS_STATUS _OS_VNIC_Create (IN UCHAR *pcVnicFilePath, OUT HANDLE *phVnicId)
{
    _OS_VNIC_CTRL_S *pstCtrl;

    pstCtrl = MEM_Malloc (sizeof(_OS_VNIC_CTRL_S));
    if (NULL == pstCtrl)
    {
        RETURN(BS_NO_MEMORY);
    }
    Mem_Zero (pstCtrl, sizeof(_OS_VNIC_CTRL_S));

    /* 非阻塞方式创建 */
    pstCtrl->stReadOverLapped.hEvent = CreateEvent (NULL, FALSE, FALSE, NULL);
    if (NULL == pstCtrl->stReadOverLapped.hEvent)
    {
        MEM_Free (pstCtrl);
        RETURN(BS_NO_RESOURCE);
    }

    pstCtrl->stWriteOverLapped.hEvent = CreateEvent (NULL, FALSE, FALSE, NULL);
    if (NULL == pstCtrl->stWriteOverLapped.hEvent)
    {
        CloseHandle (pstCtrl->stReadOverLapped.hEvent);
        MEM_Free (pstCtrl);
        RETURN(BS_NO_RESOURCE);
    }

    pstCtrl->hFileHandle = CreateFile (pcVnicFilePath,
            			       GENERIC_READ | GENERIC_WRITE,
            			       0, /* was: FILE_SHARE_READ */
            			       0,
            			       OPEN_EXISTING,
            			       FILE_ATTRIBUTE_SYSTEM | FILE_FLAG_OVERLAPPED,
            			       0);

    if (INVALID_HANDLE_VALUE == pstCtrl->hFileHandle)
    {
        CloseHandle (pstCtrl->stReadOverLapped.hEvent);
        CloseHandle (pstCtrl->stWriteOverLapped.hEvent);
        MEM_Free (pstCtrl);
        RETURN(BS_ERR);
    }

    *phVnicId = pstCtrl;
    
    return BS_OK;
}

BS_STATUS _OS_VNIC_Ioctl
(
    IN HANDLE hVnicId,
    IN UINT ulCmd,
    IN UCHAR *pucInBuf,
    IN UINT ulInBufLen,
    OUT UCHAR *pucOutBuf,
    IN UINT ulOutBufLen,
    OUT UINT *pulOutBufLen
)
{
    _OS_VNIC_CTRL_S *pstCtrl;
    UINT ulOutLen;
    UINT ulRet;

    pstCtrl = (_OS_VNIC_CTRL_S *)hVnicId;

    *pulOutBufLen = 0;

    ulRet = DeviceIoControl (pstCtrl->hFileHandle, ulCmd,
    			  pucInBuf, ulInBufLen, pucOutBuf, ulOutBufLen, &ulOutLen, NULL);

    if (ulRet == 0)
    {
        RETURN(BS_ERR);
    }

    *pulOutBufLen = ulOutLen;

    return BS_OK;
}

BS_STATUS _OS_VNIC_SetAdapterGuid(HANDLE hVnicId, CHAR *pcGuid)
{
    _OS_VNIC_CTRL_S *pstCtrl;

    pstCtrl = (_OS_VNIC_CTRL_S *)hVnicId;

    TXT_Strlcpy(pstCtrl->szAdapterGuid, pcGuid, sizeof(pstCtrl->szAdapterGuid));

    return BS_OK;
}

CHAR * _OS_VNIC_GetAdapterGuid(IN HANDLE hVnicId)
{
    _OS_VNIC_CTRL_S *pstCtrl;

    pstCtrl = (_OS_VNIC_CTRL_S *)hVnicId;

    return pstCtrl->szAdapterGuid;
}

BS_STATUS _OS_VNIC_Read 
(
    IN  HANDLE    hVnicId,
    IN  BS_WAIT_E eMode, 
    IN  UINT     ulTime,
    OUT UCHAR     *pucData,
    IN  UINT     ulDataLen,
    OUT UINT     *pulReadLen
)
{
    _OS_VNIC_CTRL_S *pstCtrl;
    OVERLAPPED *pstOverLapped;
    UINT ulReadLen;
    INT lRet;
    INT err;

    pstCtrl = (_OS_VNIC_CTRL_S *)hVnicId;

    *pulReadLen = 0;
    pstOverLapped = &pstCtrl->stReadOverLapped;

    lRet = ReadFile(pstCtrl->hFileHandle, pucData, ulDataLen, &ulReadLen, pstOverLapped);
    if (0 != lRet)
    {
        *pulReadLen = ulReadLen;
        return BS_OK;
    }

    if (BS_NO_WAIT == eMode)
    {
        return (BS_TIME_OUT);
    }

    err = GetLastError ();
    if (ERROR_IO_PENDING != err)
    {
        RETURN(BS_ERR);
    }

    /* 未完成，需要等待 */
    if (BS_WAIT_FOREVER == ulTime)
    {
        ulTime = INFINITE;
    }

    lRet = WaitForSingleObject (pstOverLapped->hEvent, ulTime);
    if (WAIT_OBJECT_0 == lRet)
    {
        if (FALSE == GetOverlappedResult (pstCtrl->hFileHandle, pstOverLapped, &ulReadLen, FALSE))
        {
            RETURN(BS_NOT_COMPLETE);
        }

        *pulReadLen = ulReadLen;
        return BS_OK;
    }
    else if (WAIT_TIMEOUT == lRet)
    {
        return (BS_TIME_OUT);
    }

    RETURN(BS_ERR);
}

BS_STATUS _OS_VNIC_Write (IN HANDLE hVnicId, OUT UCHAR *pucData, IN UINT ulDataLen, OUT UINT *pulWriteLen)
{
    _OS_VNIC_CTRL_S *pstCtrl;
    OVERLAPPED *pstOverLapped = NULL;
    UINT ulWriteLen;
    INT lRet;
    INT  err;

    pstCtrl = (_OS_VNIC_CTRL_S *) hVnicId;
    *pulWriteLen = 0;

    pstOverLapped = &pstCtrl->stWriteOverLapped;

    lRet = WriteFile(pstCtrl->hFileHandle, pucData, ulDataLen, &ulWriteLen, pstOverLapped);
    if (lRet != 0)
    {
        *pulWriteLen = ulWriteLen;
        return BS_OK;
    }

    err = GetLastError(); 
    if (err != ERROR_IO_PENDING)
    {
        return BS_ERR;
    }

    lRet = WaitForSingleObject (pstOverLapped->hEvent, INFINITE);
    if (WAIT_OBJECT_0 == lRet)
    {
        if (FALSE == GetOverlappedResult (pstCtrl->hFileHandle, pstOverLapped, &ulWriteLen, FALSE))
        {
            RETURN(BS_NOT_COMPLETE);
        }
    }

    *pulWriteLen = ulWriteLen;

    return BS_OK;
}

BS_STATUS _OS_VNIC_AddIP(HANDLE hVnicId, UINT ulIp, UINT ulMask)
{
    _OS_VNIC_CTRL_S *pstCtrl;

    pstCtrl = (_OS_VNIC_CTRL_S *) hVnicId;

    if (pstCtrl->szAdapterGuid[0] == '\0')
    {
        BS_WARNNING(("Not set adapter guid yet."));
        RETURN(BS_NOT_READY);
    }

    return IPCMD_AddIP(pstCtrl->szAdapterGuid, ulIp, ulMask, 0);
}

BS_STATUS _OS_VNIC_GetIP(HANDLE hVnicId, UINT *puiIp, UINT *puiMask)
{
    _OS_VNIC_CTRL_S *pstCtrl;

    pstCtrl = (_OS_VNIC_CTRL_S *) hVnicId;

    if (pstCtrl->szAdapterGuid[0] == '\0')
    {
        BS_WARNNING(("Not set adapter guid yet."));
        RETURN(BS_NOT_READY);
    }

    return My_IP_Helper_GetIPAddress(pstCtrl->szAdapterGuid, puiIp, puiMask);
}

BS_STATUS _OS_VNIC_AddDns(HANDLE hVnicId, UINT uiDns, UINT uiIndex)
{
    _OS_VNIC_CTRL_S *pstCtrl;

    pstCtrl = (_OS_VNIC_CTRL_S *) hVnicId;

    if (pstCtrl->szAdapterGuid[0] == '\0')
    {
        BS_WARNNING(("Not set adapter guid yet."));
        RETURN(BS_NOT_READY);
    }

    return IPCMD_AddDns(pstCtrl->szAdapterGuid, uiDns, uiIndex);
}

BS_STATUS _OS_VNIC_DelDns(IN HANDLE hVnicId, IN UINT uiDns)
{
    _OS_VNIC_CTRL_S *pstCtrl;

    pstCtrl = (_OS_VNIC_CTRL_S *) hVnicId;

    if (pstCtrl->szAdapterGuid[0] == '\0')
    {
        BS_WARNNING(("Not set adapter guid yet."));
        RETURN(BS_NOT_READY);
    }

    return IPCMD_DelDns(pstCtrl->szAdapterGuid);
}

BS_STATUS _OS_VNIC_SetDns(HANDLE hVnicId, UINT uiDns)
{
    _OS_VNIC_CTRL_S *pstCtrl;

    pstCtrl = (_OS_VNIC_CTRL_S *) hVnicId;

    if (pstCtrl->szAdapterGuid[0] == '\0')
    {
        BS_WARNNING(("Not set adapter guid yet."));
        RETURN(BS_NOT_READY);
    }

    return IPCMD_SetDns(pstCtrl->szAdapterGuid, uiDns);
}

BS_STATUS _OS_VNIC_GetAdapterIndex
(
    IN HANDLE hVnicId,
    OUT UINT *puiIndex
)
{
    _OS_VNIC_CTRL_S *pstCtrl;

    pstCtrl = (_OS_VNIC_CTRL_S *) hVnicId;

    if (pstCtrl->szAdapterGuid[0] == '\0')
    {
        BS_WARNNING(("Not set adapter guid yet."));
        RETURN(BS_NOT_READY);
    }

    return My_IP_Helper_GetAdapterIndex(pstCtrl->szAdapterGuid, puiIndex);
}

BS_STATUS _OS_VNIC_Delete(IN HANDLE hVnicId)
{
    _OS_VNIC_CTRL_S *pstCtrl;

    pstCtrl = (_OS_VNIC_CTRL_S *) hVnicId;

    if (pstCtrl->stReadOverLapped.hEvent)
    {
        CloseHandle(pstCtrl->stReadOverLapped.hEvent);
    }
	
	if (pstCtrl->stWriteOverLapped.hEvent)
    {
        CloseHandle(pstCtrl->stWriteOverLapped.hEvent);
    }

    CloseHandle(pstCtrl->hFileHandle);

    MEM_Free(pstCtrl);

    return BS_OK;    
}

BS_STATUS _OS_VNIC_Signale(IN HANDLE hVnicId)
{
    _OS_VNIC_CTRL_S *pstCtrl;

    pstCtrl = (_OS_VNIC_CTRL_S *) hVnicId;

    SetEvent (pstCtrl->stReadOverLapped.hEvent);

    return BS_OK;    
}
#endif
