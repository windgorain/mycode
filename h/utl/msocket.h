/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2010-4-19
* Description: 
* History:     
******************************************************************************/

#ifndef __MSOCKET_H_
#define __MSOCKET_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

typedef int MSOCKET_ID;

BS_STATUS MSocket_Init();
BS_STATUS MSocket_Create(IN INT iFamily, IN UINT uiType, OUT MSOCKET_ID *puiSocketID);
BS_STATUS MSocket_Close(IN MSOCKET_ID uiSocketId);
BS_STATUS MSocket_Read
(
    IN MSOCKET_ID uiSocketId,
    OUT UCHAR *pucBuf,
    IN UINT uiLen,
    OUT UINT *puiReadLen,
    IN UINT uiFlag
);
BS_STATUS MSocket_Accept(IN MSOCKET_ID uilistenSocketId, OUT MSOCKET_ID *pAcceptSocketId);
/* 返回值: >=0: 发送的字节数. <0 : 错误 */
INT MSocket_Write(IN MSOCKET_ID uiSocketId, IN UCHAR *pucBuf, IN UINT uiLen, IN UINT uiFlag);
int MSocket_Connect(IN MSOCKET_ID uiSocketID, IN UINT uiIp/* 主机序 */, IN USHORT usPort/* 主机序 */);
BS_STATUS MSocket_Ioctl(MSOCKET_ID uiSocketId, INT lCmd, UINT *argp);
BS_STATUS MSocket_GetHostIpPort(IN MSOCKET_ID uiSocketId, OUT UINT *puiIp, OUT USHORT *pusPort);
BS_STATUS MSocket_GetPeerIpPort(IN MSOCKET_ID uiSocketId, OUT UINT *pulIp, OUT USHORT *pusPort);
BS_STATUS MSocket_Bind(IN MSOCKET_ID uiSocketId, IN UINT uiIp/* 网络序 */, IN USHORT usPort/* 网络序 */);
BS_STATUS MSocket_SetRecvBufSize(IN MSOCKET_ID uiSocketId, IN UINT uiBufLen);
BS_STATUS MSocket_SetSendBufSize(IN MSOCKET_ID uiSocketId, IN UINT uiBufLen);
BS_STATUS MSocket_Listen(IN MSOCKET_ID uiSocketId, UINT uiLocalIp/* 网络序 */, IN USHORT usPort/* 网络序 */, IN UINT uiBacklog);
BS_STATUS MSocket_SendTo
(
    IN MSOCKET_ID uiSocketId,
    IN VOID *pBuf,
    IN UINT uiBufLen,
    IN UINT uiToIp/* 网络序 */,
    IN USHORT usToPort/* 网络序 */
);
BS_STATUS MSocket_RecvFrom
(
    IN MSOCKET_ID uiSocketId,
    OUT VOID *pBuf,
    IN UINT uiBufLen,
    OUT UINT *puiRecvLen,
    OUT UINT *puiFromIp/* 网络序 */,
    OUT USHORT *pusFromPort/* 网络序 */
);
BS_STATUS MSocket_Pair(UINT uiType, OUT MSOCKET_ID auiFd[2]);
VOID MSocket_FdSet(IN MSOCKET_ID uiSocketId, IN fd_set *pstFdSet);
BOOL_T MSocket_FdIsSet(IN MSOCKET_ID uiSocketId, IN fd_set *pstFdSet);
VOID MSocket_FdClr(IN MSOCKET_ID uiSocketId, IN fd_set *pstFdSet);
UINT MSocket_GetSocketId(IN MSOCKET_ID uiSocketId);
BS_STATUS MSocket_SetUserHandle(IN MSOCKET_ID uiSocketId, IN HANDLE hUserHandle);
BS_STATUS MSocket_GetUserHandle(IN MSOCKET_ID uiSocketId, OUT HANDLE *phUserHandle);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__MSOCKET_H_*/


