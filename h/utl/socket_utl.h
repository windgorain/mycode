/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2009-6-16
* Description: 
* History:     
******************************************************************************/

#ifndef __SOCKET_UTL_H_
#define __SOCKET_UTL_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

#include "utl/net.h"
#include "utl/socket_in.h"

/* 定义常见错误码 */
#define SOCKET_E_READ_PEER_CLOSE 0
#define SOCKET_E_ERR        -1
#define SOCKET_E_CREATE_ERR -2
#define SOCKET_E_SEND_ERR -3
#define SOCKET_E_RECV_ERR -4

#ifdef IN_WINDOWS
#define SOCKET_E_AGAIN      -WSAEWOULDBLOCK
#endif

#ifdef IN_UNIXLIKE
#define SOCKET_E_AGAIN      -EAGAIN
#endif

extern int _Socket_Create(int iFamily, UINT ulType, char *filename, int line);
extern BS_STATUS Socket_Close(IN INT iSocketId);
extern int Socket_Connect(IN INT iSocketId, IN UINT ulIp/* 主机序 */, IN USHORT usPort/* 主机序 */);
extern int Socket_UDPClient(UINT ip/* 主机序 */, USHORT port/* 主机序 */);
extern int Socket_UnixSocketClient(char *path, int type, int flags);
extern BOOL_T Socket_IsIPv4(IN CHAR *pcIpOrName);
extern BOOL_T Socket_N_IsIPv4(IN CHAR *pcName, IN UINT uiLen);
extern BS_STATUS Socket_Ioctl(IN INT iSocketId, INT lCmd, void *argp);
/* 将字符串形式的IP 转换成网络序IP地址 */
extern UINT Socket_Ipsz2IpNetWitchCheck(IN CHAR *pcIP);
extern UINT Socket_Ipsz2IpNet(char *pcIP);
extern UINT Socket_Ipsz2IpHost(IN CHAR *pcIP);
/* 将主机名或字符串形式的IP 转换成网络序IP地址 */
extern UINT Socket_NameToIpNet(IN CHAR *szIpOrHostName);
/* 将主机名或字符串形式的IP 转换成主机序IP地址 */
extern UINT Socket_NameToIpHost (IN CHAR *szIpOrHostName);
/* 主机序IP转换成字符串 */
extern CHAR * Socket_IpToName (IN UINT ulIp);
CHAR * Socket_Ip2Name(IN UINT ip/*net order*/, OUT char *buf, IN int buf_size);
/* 返回主机序IP和Port */
extern BS_STATUS Socket_GetLocalIpPort(IN INT iSocketId, OUT UINT *pulIp, OUT USHORT *pusPort);
extern USHORT Socket_GetHostPort(IN INT iSocketId);
/* 返回主机序IP和Port */
extern BS_STATUS Socket_GetPeerIpPort(IN INT iSocketId, OUT UINT *pulIp, OUT USHORT *pusPort);
extern UINT Socket_GetFamily(IN INT iSocketId);
extern BS_STATUS Socket_Bind(IN INT iSocketId, IN UINT ulIp/* 网络序 */, IN USHORT usPort/* 网络序 */);
/*ip/port:网络序*/
extern BS_STATUS Socket_Listen(IN INT iSocketId, UINT ulLocalIp, IN USHORT usPort, IN UINT uiBacklog);
extern int _Socket_Accept(int fd, OUT struct sockaddr *pstAddr, INOUT INT *piLen, char *filename, int line);
#define Socket_Accept(a,b,c) _Socket_Accept(a,b,c,__FILE__,__LINE__)
/* 返回值: >=0: 发送的字节数. <0 : 错误 */
extern INT Socket_Write(IN INT iSocketId, IN VOID *data, IN UINT ulLen, IN UINT ulFlag);
extern int Socket_WriteString(int fd, char *buf, unsigned int flag);

extern BS_STATUS Socket_WriteUntilFinish(IN INT iSocketId, IN UCHAR *pucBuf, IN UINT ulLen, IN UINT ulFlag);

/*
  >  0: 成功
  <= 0: SOCKET_E_XXX
*/
extern INT Socket_Read(IN INT iSocketId, OUT void *buf, IN UINT uiBufLen, IN UINT uiFlag);

/*
   OK: 成功;  如果ReadLen为0表示无数据
   PEER_CLOSED: 对端关闭
*/
extern BS_STATUS Socket_Read2
(
	IN INT iSocketId,
	OUT void *buf,
	IN UINT uiLen,
	OUT UINT *puiReadLen,
	IN UINT ulFlag
);

extern BS_STATUS Socket_SendTo
(
    IN INT iSocketId,
    IN VOID *pBuf,
    IN UINT ulBufLen,
    IN UINT ulToIp/* 网络序 */,
    IN USHORT usToPort/* 网络序 */
);
extern BS_STATUS Socket_RecvFrom
(
    IN INT iSocketId,
    OUT VOID *pBuf,
    IN UINT ulBufLen,
    OUT UINT *pulRecvLen,
    OUT UINT *pulFromIp/* 网络序 */,
    OUT USHORT *pusFromPort/* 网络序 */
);
extern BS_STATUS Socket_SetRecvBufSize(IN INT iSocketId, IN UINT ulBufLen);
extern BS_STATUS Socket_SetSendBufSize(IN INT iSocketId, IN UINT ulBufLen);
extern BS_STATUS _Socket_Pair(UINT uiType, OUT INT aiFd[2], char *filename, int line);
extern BS_STATUS Socket_SetNoBlock(IN INT iSocketID, IN BOOL_T bNoBlock);
extern BS_STATUS Socket_SetNoDelay(IN INT iSocketID, IN BOOL_T bNoDelay);
extern int Socket_SetReuseAddr(int fd, int reuse);

extern int Socket_ConnectUnixSocket(int fd, char *path);

extern INT Socket_Dup(IN INT iFd);

/* 创建一个可继承的FD. linux本身就可以继承,返回原id即可,windows要复制一份,然后关闭原fd. 失败不关闭原fd */
extern INT Socket_Inheritable(IN INT iFd);

int _Socket_OpenUdp(UINT ip/*net order*/, USHORT port/*net order*/, char *file, int line);
int _Socket_UdpClient(UINT ip/*网络序*/, USHORT port/*网络序*/, char *file, int line);
int _Socket_TcpServer(UINT ip/* 网络序 */, USHORT port/* 网络序 */, char *file, int line);
int _Socket_UnixServer(char *path, int type, char *file, int line);
int _Socket_UnixClient(char *path, int type, int no_block, char *file, int line);

#define Socket_Create(a,b) _Socket_Create(a,b,__FILE__,__LINE__)
#define Socket_Pair(a,b) _Socket_Pair(a,b,__FILE__,__LINE__)
#define Socket_OpenUdp(ip,port) _Socket_OpenUdp((ip),(port),__FILE__,__LINE__)
#define Socket_UdpClient(ip,port) _Socket_UdpClient((ip),(port),__FILE__,__LINE__)
#define Socket_TcpServer(ip,port) _Socket_TcpServer((ip),(port),__FILE__,__LINE__)
#define Socket_UnixServer(path,type) _Socket_UnixServer((path),(type),__FILE__,__LINE__)
#define Socket_UnixClient(path,type,no_block) _Socket_UnixClient((path),(type),(no_block),__FILE__,__LINE__)

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__SOCKET_UTL_H_*/

