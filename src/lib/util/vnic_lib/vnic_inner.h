/*================================================================
*   Created：2018.10.02 LiXingang All rights reserved.
*   Description：
*
================================================================*/
#ifndef __VNIC_INNER_H_
#define __VNIC_INNER_H_
#ifdef __cplusplus
extern "C"
{
#endif

BS_STATUS _OS_VNIC_Create (IN UCHAR *pcVnicFilePath, OUT HANDLE *phVnicId);
BS_STATUS _OS_VNIC_Delete(IN HANDLE hVnicId);
BS_STATUS _OS_VNIC_SetAdapterGuid(HANDLE hVnicId, CHAR *pcGuid);
BS_STATUS _OS_VNIC_Write (IN HANDLE hVnicId, OUT UCHAR *pucData, IN UINT ulDataLen, OUT UINT *pulWriteLen);
BS_STATUS _OS_VNIC_AddIP(HANDLE hVnicId, UINT ulIp, UINT ulMask);
BS_STATUS _OS_VNIC_GetIP(HANDLE hVnicId, UINT *puiIp, UINT *puiMask);
BS_STATUS _OS_VNIC_AddDns(HANDLE hVnicId, UINT uiDns, UINT uiIndex);
BS_STATUS _OS_VNIC_Signale(IN HANDLE hVnicId);
BS_STATUS _OS_VNIC_SetDns(HANDLE hVnicId, UINT uiDns);
BS_STATUS _OS_VNIC_Ioctl
(
    IN HANDLE hVnicId,
    IN UINT ulCmd,
    IN UCHAR *pucInBuf,
    IN UINT ulInBufLen,
    OUT UCHAR *pucOutBuf,
    IN UINT ulOutBufLen,
    OUT UINT *pulOutBufLen
);
BS_STATUS _OS_VNIC_Read 
(
    IN  HANDLE    hVnicId,
    IN  BS_WAIT_E eMode, 
    IN  UINT     ulTime,
    OUT UCHAR     *pucData,
    IN  UINT     ulDataLen,
    OUT UINT     *pulReadLen
);
BS_STATUS _OS_VNIC_GetAdapterIndex
(
    IN HANDLE hVnicId,
    OUT UINT *puiIndex
);
BS_STATUS _OS_VNIC_DelDns
(
    IN HANDLE hVnicId,
    IN UINT uiDns
);
CHAR * _OS_VNIC_GetAdapterGuid(IN HANDLE hVnicId);

#ifdef __cplusplus
}
#endif
#endif //VNIC_INNER_H_
