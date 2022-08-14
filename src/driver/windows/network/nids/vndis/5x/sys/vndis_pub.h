/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2012-3-29
* Description:
* History:
******************************************************************************/

#ifndef __VNDIS_PUB_H_
#define __VNDIS_PUB_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

typedef struct
{
    UINT uiTxLow;
    UINT uiTxHigh;
    UINT uiRxLow;
    UINT uiRxHigh;
    UINT uiTxErr;
    UINT uiRxErr;
}VNDIS_PUB_STATUS_S;

typedef struct
{
    UINT uiMajorVer;
    UINT uiMinorVer;
    UINT uiDbg;
}VNDIS_PUB_VERSION_S;

#define VNDIS_PUB_CONTROL_CODE(request,method) CTL_CODE(FILE_DEVICE_UNKNOWN, request, method, FILE_ANY_ACCESS)

/* 这几个和OpenVpn的Tap定义保持一致 */
#define VNDIS_PUB_IOCTL_GET_MAC             VNDIS_PUB_CONTROL_CODE(1, METHOD_BUFFERED)
#define VNDIS_PUB_IOCTL_GET_VERSION         VNDIS_PUB_CONTROL_CODE(2, METHOD_BUFFERED)
#define VNDIS_PUB_IOCTL_GET_MTU             VNDIS_PUB_CONTROL_CODE(3, METHOD_BUFFERED)
#define VNDIS_PUB_IOCTL_SET_MEDIA_STATUS    VNDIS_PUB_CONTROL_CODE(6, METHOD_BUFFERED)


/* 下面这几个是私有的,和Openvpn的tap不一致 */
#define VNDIS_PUB_IOCTL_GET_MEDIA_STATUS    VNDIS_PUB_CONTROL_CODE(30, METHOD_BUFFERED)
#define VNDIS_PUB_IOCTL_GET_STATUS          VNDIS_PUB_CONTROL_CODE(31, METHOD_BUFFERED)  /* VNDIS_PUB_STATUS_S */
#define VNDIS_PUB_IOCTL_GET_ADAPTER_NAME    VNDIS_PUB_CONTROL_CODE(32, METHOD_BUFFERED)

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__VNDIS_PUB_H_*/


