/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2009-6-16
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/num_utl.h"
#include "utl/bit_opt.h"
#include "utl/mem_utl.h"
#include "utl/socket_utl.h"

static VOID socket_WindowInit()
{
#ifdef IN_WINDOWS
    WORD wVersionRequested;
    WSADATA wsaData;
    int err;
    static BOOL_T bIsInit = 0;

    if (bIsInit == 0)
    {
        bIsInit = 1;

        wVersionRequested = MAKEWORD (2, 2);
        err = WSAStartup (wVersionRequested, &wsaData);
        if (err != 0)
        {
            return;
        }
    }
#endif
}

static INT socket_GetLastErrno()
{
    INT iErr = SOCKET_E_ERR;

#ifdef IN_WINDOWS
    iErr = WSAGetLastError();
#else
    iErr = errno;
    if ((EINPROGRESS == iErr) || (EINTR == iErr))
    {
        iErr = EAGAIN;
    }
#endif

    if (iErr > 0)
    {
        iErr = -iErr;
    }

    return iErr;
}

/*
  >  0: 成功
  <= 0: SOCKET_E_XXX
*/
INT Socket_Read(IN INT iSocketId, OUT void *buf, IN UINT uiBufLen, IN UINT uiFlag)
{
    INT iReadLen;
    INT iRet;

    iReadLen = recv(iSocketId, buf, (INT)uiBufLen, (INT)uiFlag);

    if (iReadLen == 0)
    {
        iRet = 0;
    }
    else if (iReadLen < 0)
    {
        iRet = socket_GetLastErrno();
    }
    else
    {
        iRet = iReadLen;
    }

	return iRet;
}

/*
   BS_OK: 成功;
   BS_PEER_CLOSED: 对端关闭
*/
BS_STATUS Socket_Read2
(
	IN INT iSocketId,
	OUT void *buf,
	IN UINT uiLen,
	OUT UINT *puiReadLen,
	IN UINT ulFlag
)
{
    INT iLen;
    BS_STATUS eRet = BS_ERR;

	*puiReadLen = 0;

    iLen = Socket_Read(iSocketId, buf, uiLen, ulFlag);

	if (iLen > 0)
	{
        eRet = BS_OK;
		*puiReadLen = iLen;
	}
    else
    {
        if (iLen == 0)
        {
            eRet = BS_PEER_CLOSED;
        }
        else if (iLen == SOCKET_E_AGAIN)
        {
            eRet = BS_OK;
        }
    }

    return eRet;
}

INT Socket_Accept(IN INT iListenSocketId, OUT struct sockaddr *pstAddr/* 可以为NULL */, INOUT INT *piLen/* 可以为NULL */)
{
    /*接受连接*/
    struct sockaddr server_addr;
    INT s;
    INT iRet;
    int length = sizeof(server_addr);
    struct sockaddr *pstAddrTmp = pstAddr;
    INT *piLenTmp = piLen;

    if (NULL == pstAddrTmp)
    {
        pstAddrTmp = &server_addr;
        piLenTmp = &length;
    }

    s = (INT)accept(iListenSocketId, pstAddrTmp, (UINT*)piLenTmp);

    if (s >= 0)
    {
        iRet = s;
        SSHOW_Add(s);
    }
    else
    {
        iRet = socket_GetLastErrno();
    }

    return iRet;
}

/* 返回值: >=0: 发送的字节数. <0 : 错误 */
INT Socket_Write(IN INT iSocketId, IN VOID *data, IN UINT ulLen, IN UINT ulFlag)
{
    INT iSendLen;
    INT iRet;

    iSendLen = send(iSocketId, data, (INT)ulLen, (INT)ulFlag);
    if (iSendLen >= 0)
    {
        iRet = iSendLen;
    }
    else
    {
        iRet = socket_GetLastErrno();
    }

    return iRet;
}

int Socket_WriteString(int fd, char *buf, unsigned int flag)
{
    return Socket_Write(fd, buf, strlen(buf), flag);
}

BS_STATUS Socket_WriteUntilFinish(IN INT iSocketId, IN UCHAR *pucBuf, IN UINT ulLen, IN UINT ulFlag)
{
    INT iLen;
    UINT uiSendLen = 0;

    while (uiSendLen < ulLen)
    {
        iLen = Socket_Write(iSocketId, pucBuf + uiSendLen, ulLen - uiSendLen, ulFlag);
        if (iLen <= 0)
        {
            RETURN(BS_ERR);
        }
        uiSendLen += (UINT)iLen;
    }

    return BS_OK;
}

/*
  >=  0: 成功
  < 0: SOCKET_E_XXX
*/
int Socket_Connect(IN INT iSocketID, IN UINT ulIp/* 主机序 */, IN USHORT usPort/* 主机序 */)
{
    struct sockaddr_in server_addr;     /* 服务器地址结构   */

    if ((ulIp == 0) || (usPort == 0))
    {
        return BS_ERR;
    }

    Mem_Zero (&server_addr, sizeof(struct sockaddr_in));
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(ulIp);
    server_addr.sin_port = htons(usPort);

    if (connect(iSocketID, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0 )
    {
        return socket_GetLastErrno();
    }

    return BS_OK;
}

BOOL_T Socket_IsIPv4(IN CHAR *pcIpOrName)
{
    CHAR *pcTmp = pcIpOrName;
    CHAR *pcSplit;
    UINT ulLen, i, j;

    for (j=0; j<4; j++)
    {
        /* 最后一节是没有'.' 的，前三节都有'.' */
        if (j < 3)
        {
            pcSplit = strchr(pcTmp, '.');
            if (pcSplit == NULL)
            {
                return FALSE;
            }
        }
        else
        {
            pcSplit = pcTmp + strlen(pcTmp);
        }
        
        ulLen = (UINT)(pcSplit - pcTmp);
        if (ulLen > 3)      /* 长度不能超过3 */
        {
            return FALSE;
        }
        for (i=0; i<ulLen; i++)     /* IP 必须是数字 */
        {
            if (!NUM_IN_RANGE((INT)pcTmp[i], (INT)'0', (INT)'9'))
            {
                return FALSE;
            }
        }
        if (ulLen == 3)     /* 不能大于255 */
        {
            if (pcTmp[0] > '2')
            {
                return FALSE;
            }

            if (pcTmp[0] == '2')
            {
                if (pcTmp[1] > '5')
                {
                    return FALSE;
                }

                if (pcTmp[1] == '5')
                {
                    if (pcTmp[2] > '5')
                    {
                        return FALSE;
                    }
                }
            }

        }
        pcTmp = pcSplit + 1;
    }

    return TRUE;
}

BOOL_T Socket_N_IsIPv4(IN CHAR *pcName, IN UINT uiLen)
{
    CHAR szTmp[INET_ADDRSTRLEN + 1];

    if (uiLen >= sizeof(szTmp))
    {
        return FALSE;
    }

    memcpy(szTmp, pcName, uiLen);
    szTmp[uiLen] = '\0';

    return Socket_IsIPv4(szTmp);
}

UINT Socket_Ipsz2IpNetWitchCheck(IN CHAR *pcIP)
{
    if (FALSE == Socket_IsIPv4(pcIP))
    {
        return 0;
    }

    return inet_addr(pcIP);
}

/* 将字符串形式的IP 转换成网络序IP地址 */
UINT Socket_Ipsz2IpNet(IN CHAR *pcIP)
{
    return inet_addr(pcIP);
}

UINT Socket_Ipsz2IpHost(IN CHAR *pcIP)
{
    UINT uiIp;

    uiIp = Socket_Ipsz2IpNet(pcIP);

    return ntohl(uiIp);
}

/* 将主机名或字符串形式的IP 转换成网络序IP地址 */
UINT Socket_NameToIpNet(IN CHAR *szIpOrHostName)
{
    socket_WindowInit();

    if (NULL == szIpOrHostName)
    {
        return 0;
    }

    if(stricmp(szIpOrHostName, "LOCALHOST") == 0)
    {
        return inet_addr("127.0.0.1");
    }
    else if (! Socket_IsIPv4(szIpOrHostName))
    {
        struct hostent* remoteHost;
        remoteHost = gethostbyname(szIpOrHostName);
        if (remoteHost != NULL)
        {
            return *((UINT*)remoteHost->h_addr_list[0]);
        }
        else
        {
            return 0;
        }
    }
    else
    {
        return inet_addr(szIpOrHostName);
    }
}

/* 将主机名或字符串形式的IP 转换成主机序IP地址 */
UINT Socket_NameToIpHost (IN CHAR *szIpOrHostName)
{
    UINT uiIP;

    uiIP = Socket_NameToIpNet(szIpOrHostName);

    return ntohl(uiIP);
}

/* 主机序IP转换成字符串 */
CHAR * Socket_IpToName (IN UINT ulIp)
{
    struct in_addr stAddr;

#ifdef IN_WINDOWS
    stAddr.S_un.S_addr = htonl(ulIp);
#endif
#ifdef IN_UNIXLIKE
    stAddr.s_addr = htonl(ulIp);
#endif
    return inet_ntoa (stAddr);
}

CHAR * Socket_Ip2Name(IN UINT ip/*net order*/, OUT char *buf, IN int buf_size)
{
#ifdef IN_UNIXLIKE
    inet_ntop(AF_INET, &ip, buf, buf_size);
    return buf;
#endif
#ifdef IN_WINDOWS
    strlcpy(buf, Socket_IpToName(ntohl(ip), buf_size));
    return buf;
#endif
}

BS_STATUS Socket_SetSockOpt(IN INT iSocketId, IN INT iLevel, IN INT iOpt, IN VOID *pOpt, IN UINT uiOptLen)
{
    if (setsockopt(iSocketId, iLevel, iOpt, pOpt, uiOptLen) < 0)
    {
        return BS_ERR;
    }

    return BS_OK;
}

BS_STATUS Socket_Ioctl(INT iSocketId, INT lCmd, void *argp)
{
    INT lRet;
    
#ifdef IN_WINDOWS
    lRet = ioctlsocket(iSocketId, lCmd, argp);
#else
    lRet = ioctl(iSocketId, lCmd, argp);
#endif

    if (lRet != 0)
    {
        RETURN(BS_ERR);
    }

    return BS_OK;
}

/* 返回主机序IP和Port */
BS_STATUS Socket_GetLocalIpPort(IN INT iSocketId, OUT UINT *pulIp, OUT USHORT *pusPort)
{
    socklen_t iAddrLen;
    struct sockaddr_storage stSockAddr;
    struct sockaddr_in *pstSin;

    Mem_Zero(&stSockAddr, sizeof(stSockAddr));
    iAddrLen = sizeof(stSockAddr);
    
    getsockname(iSocketId, (struct sockaddr *)&stSockAddr, &iAddrLen);

    pstSin = (struct sockaddr_in*)&stSockAddr;

    *pusPort = ntohs(pstSin->sin_port);
#ifdef IN_WINDOWS
    *pulIp = ntohl(pstSin->sin_addr.S_un.S_addr);
#endif
#ifdef IN_UNIXLIKE
    *pulIp = ntohl(pstSin->sin_addr.s_addr);
#endif
    return BS_OK;
}

/* 返回主机序IP和Port */
BS_STATUS Socket_GetPeerIpPort(IN INT iSocketId, OUT UINT *pulIp, OUT USHORT *pusPort)
{
    socklen_t iAddrLen;
    struct sockaddr_storage stSockAddr;
    struct sockaddr_in *pstSin;

    Mem_Zero(&stSockAddr, sizeof(stSockAddr));
    iAddrLen = sizeof(stSockAddr);
    
    getpeername(iSocketId, (struct sockaddr *)&stSockAddr, &iAddrLen);

    pstSin = (struct sockaddr_in*)&stSockAddr;

    *pusPort = ntohs(pstSin->sin_port);
#ifdef IN_WINDOWS
    *pulIp = ntohl(pstSin->sin_addr.S_un.S_addr);
#endif
#ifdef IN_UNIXLIKE
    *pulIp = ntohl(pstSin->sin_addr.s_addr);
#endif

    return BS_OK;
}

/* 返回主机序Port */
USHORT Socket_GetHostPort(IN INT iSocketId)
{
    UINT uiIp;
    USHORT usPort;
    if (BS_OK != Socket_GetLocalIpPort(iSocketId, &uiIp, &usPort))
    {
        return 0;
    }

    return usPort;
}

UINT Socket_GetFamily(IN INT iSocketId)
{
    struct sockaddr_storage stSockAddrLocal;
    struct sockaddr_in *pstSinLocal;
    socklen_t iAddrLen;

    Mem_Zero(&stSockAddrLocal, sizeof(stSockAddrLocal));
    iAddrLen = sizeof(stSockAddrLocal);
    getsockname(iSocketId, (struct sockaddr *)&stSockAddrLocal, &iAddrLen);

    pstSinLocal = (struct sockaddr_in*)&stSockAddrLocal;

    return pstSinLocal->sin_family;
}

BS_STATUS Socket_Bind(IN INT iSocketId, IN UINT ulIp/* 网络序 */, IN USHORT usPort/* 网络序 */)
{
    struct sockaddr_in server_addr;

    Mem_Zero (&server_addr, sizeof(struct sockaddr_in));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = usPort;
    server_addr.sin_addr.s_addr= ulIp;

    if (bind(iSocketId, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
    {
        RETURN(BS_ERR);
    }

    return BS_OK;
}

BS_STATUS Socket_SetRecvBufSize(IN INT iSocketId, IN UINT ulBufLen)
{
    int ulLen = sizeof(int);
    
    if (setsockopt(iSocketId, SOL_SOCKET, SO_RCVBUF, (VOID *)&ulBufLen, ulLen) < 0)
    {
        RETURN(BS_ERR);
    }

    return BS_OK;
}

BS_STATUS Socket_SetSendBufSize(IN INT iSocketId, IN UINT ulBufLen)
{
    int ulLen = sizeof(int);
    
    if (setsockopt(iSocketId, SOL_SOCKET, SO_SNDBUF, (VOID *)&ulBufLen, ulLen) < 0)
    {
        RETURN(BS_ERR);
    }

    return BS_OK;
}

BS_STATUS Socket_Close(IN INT iSocketId)
{
    SSHOW_Del(iSocketId);
    closesocket(iSocketId);
    return BS_OK;
}

INT Socket_Create(IN INT iFamily, IN UINT ulType)
{
    INT s;

    socket_WindowInit();

    /* 建立Socket  */
    s = socket (iFamily, (INT)ulType, 0);

    if (s < 0)
    {
        return s;
    }

    SSHOW_Add(s);

    return s;
}

BS_STATUS Socket_Listen(IN INT iSocketID, UINT ulLocalIp/* 网络序 */, IN USHORT usPort/* 网络序 */, IN UINT uiBacklog)
{
    INT iReuse = 1;

#ifndef IN_WINDOWS /* 不是windows,则设置reuseaddr属性 */
    (VOID) setsockopt(iSocketID, SOL_SOCKET, SO_REUSEADDR, (CHAR*)&iReuse, sizeof(INT));
#endif

    if (BS_OK != Socket_Bind(iSocketID, ulLocalIp, usPort))
    {
        RETURN(BS_ERR);
    }

    if(listen(iSocketID, uiBacklog)!=0)
    {
        RETURN(BS_ERR);
    }

    return BS_OK;
}


BS_STATUS Socket_SendTo
(
    IN INT iSocketId,
    IN VOID *pBuf,
    IN UINT ulBufLen,
    IN UINT ulToIp/* 网络序 */,
    IN USHORT usToPort/* 网络序 */
)
{
    struct sockaddr_in stSockAddr;
    INT lSize;

    Mem_Zero(&stSockAddr, sizeof(stSockAddr));

    stSockAddr.sin_family = AF_INET;
    stSockAddr.sin_port   = usToPort;
    stSockAddr.sin_addr.s_addr = ulToIp;

    lSize = sendto(iSocketId, pBuf, ulBufLen, 0, (struct sockaddr*)&stSockAddr, sizeof(stSockAddr));

    if (lSize < 0)
    {
        RETURN(BS_ERR);
    }

    return BS_OK;
}

BS_STATUS Socket_RecvFrom
(
    IN INT iSocketId,
    OUT VOID *pBuf,
    IN UINT ulBufLen,
    OUT UINT *pulRecvLen,
    OUT UINT *pulFromIp/* 网络序 */,
    OUT USHORT *pusFromPort/* 网络序 */
)
{
    struct sockaddr_in stSockAddr;
    socklen_t  ulAddrLen = 0;
    INT lSize;

    *pulRecvLen = 0;

    Mem_Zero(&stSockAddr,sizeof(sizeof(stSockAddr)));
    ulAddrLen = sizeof(stSockAddr);

    lSize = recvfrom(iSocketId, pBuf, ulBufLen, 0, (struct sockaddr*)&stSockAddr, &ulAddrLen);
    if (lSize < 0)
    {
        RETURN(BS_ERR);
    }

    *pulFromIp = stSockAddr.sin_addr.s_addr;
    *pusFromPort = stSockAddr.sin_port;

    *pulRecvLen = lSize;

    return BS_OK;
}

BS_STATUS Socket_Pair(UINT uiType, OUT INT aiFd[2])
{
#ifndef IN_WINDOWS
	int fd[2];

    if (socketpair(AF_UNIX, uiType, 0, fd) < 0)
    {
        RETURN(BS_ERR);
    }

    SSHOW_Add(fd[0]);
    SSHOW_Add(fd[1]);

    aiFd[0] = (UINT)fd[0];
    aiFd[1] = (UINT)fd[1];

    return BS_OK;
#else
    
	INT iListenFd, iAcceptFd, iConnectFd;
    UINT uiIp;
    USHORT usPort;

    if ((iListenFd = Socket_Create(AF_INET, uiType)) < 0)
    {
        RETURN(BS_ERR);
    }

    if ((iConnectFd = Socket_Create(AF_INET, uiType)) < 0)
    {
        Socket_Close(iListenFd);
        RETURN(BS_ERR);
    }

    if (BS_OK != Socket_Listen(iListenFd, htonl(LOOPBACK_IP), 0, 1))
    {
        Socket_Close(iListenFd);
        Socket_Close(iConnectFd);
        RETURN(BS_ERR);
    }

    Socket_GetLocalIpPort(iListenFd, &uiIp, &usPort);
    if (BS_OK != Socket_Connect(iConnectFd, LOOPBACK_IP, usPort))
    {
        Socket_Close(iListenFd);
        Socket_Close(iConnectFd);
        RETURN(BS_ERR);
    }

    iAcceptFd = Socket_Accept(iListenFd, NULL, NULL);
    if (iAcceptFd < 0)
    {
        Socket_Close(iListenFd);
        Socket_Close(iConnectFd);
        RETURN(BS_ERR);
    }

    Socket_Close(iListenFd);

    aiFd[0] = iConnectFd;
    aiFd[1] = iAcceptFd;

    return BS_OK;
#endif
}

BS_STATUS Socket_SetNoBlock(IN INT iSocketID, IN BOOL_T bNoBlock)
{
#ifdef IN_WINDOWS
    int uiValue = (int)bNoBlock;
    return Socket_Ioctl(iSocketID, FIONBIO, &uiValue);
#endif

#ifdef IN_UNIXLIKE
    int flag = -1;
    flag = fcntl(iSocketID,F_GETFL);
    BIT_CLR(flag, O_NONBLOCK);
    if (bNoBlock) {
        flag |= O_NONBLOCK;
    }
    fcntl(iSocketID,F_SETFL,flag);
    return 0;
#endif
}

BS_STATUS Socket_SetNoDelay(IN INT iSocketID, IN BOOL_T bNoDelay)
{
    INT iOn = 0;

    if (bNoDelay)
    {
        iOn = 1;
    }
    
    return Socket_SetSockOpt(iSocketID, IPPROTO_TCP, TCP_NODELAY, &iOn, sizeof(INT));
}

INT Socket_Dup(IN INT iFd)
{
    INT iNewFd;
    
#ifdef IN_UNIXLIKE
    iNewFd = dup(iFd);
#endif

#ifdef IN_WINDOWS
    HANDLE hDupFd;
    HANDLE hCurrentProcess;

    hCurrentProcess = GetCurrentProcess();
    
    if (! DuplicateHandle(hCurrentProcess,(HANDLE)iFd, hCurrentProcess, &hDupFd, 0, TRUE, DUPLICATE_SAME_ACCESS))
    {
        iNewFd = -1;
    }

    iNewFd = (INT)(UINT)hDupFd;
#endif

    if (iNewFd >= 0)
    {
        SSHOW_Add(iNewFd);
    }

    return iNewFd;
}

/* 创建一个可继承的FD. linux本身就可以继承,返回原id即可,windows要复制一份,然后关闭原fd. 失败不关闭原fd */
INT Socket_Inheritable(IN INT iFd)
{
#ifdef IN_UNIXLIKE
    return iFd;
#endif

#ifdef IN_WINDOWS
    INT iNewFd;
    iNewFd = Socket_Dup(iFd);
    if (iNewFd >= 0)
    {
        Socket_Close(iFd);
    }

    return iNewFd;
#endif
}

