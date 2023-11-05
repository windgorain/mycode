/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2007-8-27
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#ifdef IN_WINDOWS

#include "utl/vnic_lib.h"
#include "utl/txt_utl.h"
#include "utl/my_ip_helper.h"
#include "vnic_inner.h"

VNIC_HANDLE VNIC_Create (IN UCHAR *pcVnicFilePath)
{
    HANDLE hVnic;

    if (BS_OK != _OS_VNIC_Create(pcVnicFilePath, &hVnic))
    {
        return NULL;
    }

    return hVnic;
}

BS_STATUS VNIC_Delete (IN VNIC_HANDLE hVnic)
{
    BS_DBGASSERT (0 != hVnic);
    
    return _OS_VNIC_Delete(hVnic);
}

BS_STATUS VNIC_Ioctl
(
    IN VNIC_HANDLE hVnic,
    IN UINT ulCmd,
    IN UCHAR *pucInBuf,
    IN UINT ulInBufLen,
    OUT UCHAR *pucOutBuf,
    IN UINT ulOutBufLen,
    OUT UINT *puiOutBufLen
)
{
    BS_DBGASSERT (0 != hVnic);
    
    return _OS_VNIC_Ioctl(hVnic, ulCmd, pucInBuf, ulInBufLen, pucOutBuf, ulOutBufLen, puiOutBufLen);
}

BS_STATUS VNIC_SetAdapterGuid(IN VNIC_HANDLE hVnic, IN CHAR *pcGuid)
{
    BS_DBGASSERT (0 != hVnic);
    
    return _OS_VNIC_SetAdapterGuid(hVnic, pcGuid);
}

CHAR * VNIC_GetAdapterGuid(IN VNIC_HANDLE hVnic)
{
    BS_DBGASSERT (0 != hVnic);
    
    return _OS_VNIC_GetAdapterGuid(hVnic);
}

BS_STATUS VNIC_Read
(
    IN VNIC_HANDLE hVnic, 
    IN BS_WAIT_E eMode, 
    IN UINT ulTime,
    OUT UCHAR *pucData, 
    IN UINT ulDataLen, 
    OUT UINT *pulReadLen
)
{
    BS_DBGASSERT (NULL != pucData);
    BS_DBGASSERT (NULL != pulReadLen);
    BS_DBGASSERT (0 != hVnic);

    return _OS_VNIC_Read(hVnic, eMode, ulTime, pucData, ulDataLen, pulReadLen);
}

BS_STATUS VNIC_Write (IN VNIC_HANDLE hVnic, OUT UCHAR *pucData, IN UINT ulDataLen, OUT UINT *pulWriteLen)
{
    BS_DBGASSERT (NULL != pucData);
    BS_DBGASSERT (NULL != pulWriteLen);
    BS_DBGASSERT (0 != hVnic);

    return _OS_VNIC_Write(hVnic, pucData, ulDataLen, pulWriteLen);
}


BS_STATUS VNIC_AddIP (IN VNIC_HANDLE hVnic, IN UINT ulIp, IN UINT ulMask)
{
    BS_DBGASSERT (0 != hVnic);

    return _OS_VNIC_AddIP(hVnic, ulIp, ulMask);
}


BS_STATUS VNIC_GetIP(IN VNIC_HANDLE hVnic, OUT UINT *puiIp, OUT UINT *puiMask)
{
    BS_DBGASSERT (0 != hVnic);

    return _OS_VNIC_GetIP(hVnic, puiIp, puiMask);
}

BS_STATUS VNIC_AddDns(IN VNIC_HANDLE hVnic, IN UINT uiDns, IN UINT uiIndex)
{
    BS_DBGASSERT (0 != hVnic);

    if (uiDns == 0)
    {
        return BS_BAD_PARA;
    }

    return _OS_VNIC_AddDns(hVnic, uiDns, uiIndex);
}

BS_STATUS VNIC_SetDns(IN VNIC_HANDLE hVnic, IN UINT uiDns)
{
    BS_DBGASSERT (0 != hVnic);

    if (uiDns == 0)
    {
        return BS_BAD_PARA;
    }

    return _OS_VNIC_SetDns(hVnic, uiDns);
}

BS_STATUS VNIC_DelDns(IN VNIC_HANDLE hVnic, IN UINT uiDns)
{
    BS_DBGASSERT (0 != hVnic);

    if (uiDns == 0)
    {
        return BS_BAD_PARA;
    }

    return _OS_VNIC_DelDns(hVnic, uiDns);
}

BS_STATUS VNIC_GetAdapterIndex(IN VNIC_HANDLE hVnic, OUT UINT *puiIndex)
{
    return _OS_VNIC_GetAdapterIndex(hVnic, puiIndex);
}

BS_STATUS VNIC_Signale (IN VNIC_HANDLE hVnic)
{
    BS_DBGASSERT (0 != hVnic);

    return _OS_VNIC_Signale (hVnic);
}

#endif
