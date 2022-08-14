/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2010-4-19
* Description: managed socket:可管理的Socket.对Socket做了一层封装.使用它的ID
*                  代替Socket ID, 它的控制结构中可以放置一些自己的东西.
*                  比如UserHandle等
* History:     
******************************************************************************/
/* retcode所需要的宏 */
#define RETCODE_FILE_NUM RETCODE_FILE_NUM_MSOCKET

#include "bs.h"

#include "utl/socket_utl.h"
#include "utl/nap_utl.h"
#include "utl/ssltcp_utl.h"
#include "utl/mutex_utl.h"
#include "utl/msocket.h"


typedef struct
{
    INT iSocketId;
    HANDLE hUserHandle;
}_MSOCKET_CTRL_S;

static HANDLE g_hMSocketNap = NULL;
static MUTEX_S g_stMSocketMutex;

BS_STATUS MSocket_Init()
{
    NAP_PARAM_S param = {0};

    if (NULL == g_hMSocketNap)
    {
        param.enType = NAP_TYPE_PTR_ARRAY;
        param.uiMaxNum = SSLTCP_MAX_SSLTCP_NUM;
        param.uiNodeSize = sizeof(_MSOCKET_CTRL_S);

        g_hMSocketNap = NAP_Create(&param);

        if (g_hMSocketNap == NULL)
        {
            RETURN(BS_ERR);
        }

        NAP_EnableSeq(g_hMSocketNap, 0xffff0000ULL, SSLTCP_MAX_SSLTCP_NUM);
    }

    MUTEX_Init(&g_stMSocketMutex);

    return BS_OK;
}

BS_STATUS MSocket_Create(IN INT iFamily, IN UINT uiType, OUT MSOCKET_ID *puiSocketID)
{
    BS_STATUS eRet = BS_OK;
    _MSOCKET_CTRL_S *pstMSocket = NULL;

    MUTEX_P(&g_stMSocketMutex);
    pstMSocket = NAP_ZAlloc(g_hMSocketNap);
    MUTEX_V(&g_stMSocketMutex);

    if (NULL == pstMSocket)
    {
        RETURN(BS_NO_RESOURCE);
    }

    pstMSocket->iSocketId = Socket_Create(iFamily, uiType);
    if (pstMSocket->iSocketId < 0)
    {
        MUTEX_P(&g_stMSocketMutex);
        NAP_Free(g_hMSocketNap, pstMSocket);
        MUTEX_V(&g_stMSocketMutex);
        return eRet;
    }

    *puiSocketID = (UINT)NAP_GetIDByNode(g_hMSocketNap, pstMSocket);

    return BS_OK;
}

BS_STATUS MSocket_Close(IN MSOCKET_ID uiSocketId)
{
    _MSOCKET_CTRL_S *pstMSocket;

    pstMSocket = NAP_GetNodeByID(g_hMSocketNap, uiSocketId);
    if (NULL == pstMSocket)
    {
        RETURN(BS_NO_SUCH);
    }

    Socket_Close(pstMSocket->iSocketId);

    MUTEX_P(&g_stMSocketMutex);
    NAP_Free(g_hMSocketNap, pstMSocket);
    MUTEX_V(&g_stMSocketMutex);

    return BS_OK;
}

BS_STATUS MSocket_Read
(
    IN MSOCKET_ID uiSocketId,
    OUT UCHAR *pucBuf,
    IN UINT uiLen,
    OUT UINT *puiReadLen,
    IN UINT uiFlag
)
{
    _MSOCKET_CTRL_S *pstMSocket;

    pstMSocket = NAP_GetNodeByID(g_hMSocketNap, uiSocketId);
    if (NULL == pstMSocket)
    {
        return BS_ERR;
    }

    return Socket_Read2(pstMSocket->iSocketId, pucBuf, uiLen, puiReadLen, uiFlag);
}

BS_STATUS MSocket_Accept(IN MSOCKET_ID uilistenSocketId, OUT MSOCKET_ID *pAcceptSocketId)
{
    _MSOCKET_CTRL_S *pstMSocket;
    _MSOCKET_CTRL_S *pstMSocketAccept;

    pstMSocket = NAP_GetNodeByID(g_hMSocketNap, uilistenSocketId);
    if (NULL == pstMSocket)
    {
        RETURN(BS_NO_SUCH);
    }

    MUTEX_P(&g_stMSocketMutex);
    pstMSocketAccept = NAP_ZAlloc(g_hMSocketNap);
    MUTEX_V(&g_stMSocketMutex);
    if (NULL == pstMSocketAccept)
    {
        RETURN(BS_NO_RESOURCE);
    }

    pstMSocketAccept->iSocketId = Socket_Accept(pstMSocket->iSocketId, NULL, NULL);
    if (pstMSocketAccept->iSocketId < 0)
    {
        MUTEX_P(&g_stMSocketMutex);
        NAP_Free(g_hMSocketNap, pstMSocketAccept);
        MUTEX_V(&g_stMSocketMutex);
        return BS_ERR;
    }

    *pAcceptSocketId = (UINT)NAP_GetIDByNode(g_hMSocketNap, pstMSocketAccept);
    return BS_OK;
}

/* 返回值: >=0: 发送的字节数. <0 : 错误 */
INT MSocket_Write(IN MSOCKET_ID uiSocketId, IN UCHAR *pucBuf, IN UINT uiLen, IN UINT uiFlag)
{
    _MSOCKET_CTRL_S *pstMSocket;

    pstMSocket = NAP_GetNodeByID(g_hMSocketNap, uiSocketId);
    if (NULL == pstMSocket)
    {
        return -1;
    }

    return Socket_Write(pstMSocket->iSocketId, pucBuf, uiLen, uiFlag);
}

int MSocket_Connect(IN MSOCKET_ID uiSocketId, IN UINT uiIp/* 主机序 */, IN USHORT usPort/* 主机序 */)
{
    _MSOCKET_CTRL_S *pstMSocket;

    pstMSocket = NAP_GetNodeByID(g_hMSocketNap, uiSocketId);
    if (NULL == pstMSocket)
    {
        return BS_ERR;
    }

    return Socket_Connect(pstMSocket->iSocketId, uiIp, usPort);
}

BS_STATUS MSocket_Ioctl(IN MSOCKET_ID uiSocketId, IN INT lCmd, IN UINT *argp)
{
    _MSOCKET_CTRL_S *pstMSocket;

    pstMSocket = NAP_GetNodeByID(g_hMSocketNap, uiSocketId);
    if (NULL == pstMSocket)
    {
        RETURN(BS_NO_SUCH);
    }

    return Socket_Ioctl(pstMSocket->iSocketId, lCmd, argp);
}

/* 返回主机序IP和Port */
BS_STATUS MSocket_GetHostIpPort(IN MSOCKET_ID uiSocketId, OUT UINT *puiIp, OUT USHORT *pusPort)
{
    _MSOCKET_CTRL_S *pstMSocket;

    pstMSocket = NAP_GetNodeByID(g_hMSocketNap, uiSocketId);
    if (NULL == pstMSocket)
    {
        RETURN(BS_NO_SUCH);
    }

    return Socket_GetLocalIpPort(pstMSocket->iSocketId, puiIp, pusPort);
}

/* 返回主机序IP和Port */
BS_STATUS MSocket_GetPeerIpPort(IN MSOCKET_ID uiSocketId, OUT UINT *pulIp, OUT USHORT *pusPort)
{
    _MSOCKET_CTRL_S *pstMSocket;

    pstMSocket = NAP_GetNodeByID(g_hMSocketNap, uiSocketId);
    if (NULL == pstMSocket)
    {
        RETURN(BS_NO_SUCH);
    }

    return Socket_GetPeerIpPort(pstMSocket->iSocketId, pulIp, pusPort);
}

BS_STATUS MSocket_Bind(IN MSOCKET_ID uiSocketId, IN UINT uiIp/* 网络序 */, IN USHORT usPort/* 网络序 */)
{
    _MSOCKET_CTRL_S *pstMSocket;

    pstMSocket = NAP_GetNodeByID(g_hMSocketNap, uiSocketId);
    if (NULL == pstMSocket)
    {
        RETURN(BS_NO_SUCH);
    }

    return Socket_Bind(pstMSocket->iSocketId, uiIp, usPort);
}

BS_STATUS MSocket_SetRecvBufSize(IN MSOCKET_ID uiSocketId, IN UINT uiBufLen)
{
    _MSOCKET_CTRL_S *pstMSocket;

    pstMSocket = NAP_GetNodeByID(g_hMSocketNap, uiSocketId);
    if (NULL == pstMSocket)
    {
        RETURN(BS_NO_SUCH);
    }

    return Socket_SetRecvBufSize(pstMSocket->iSocketId, uiBufLen);
}

BS_STATUS MSocket_SetSendBufSize(IN MSOCKET_ID uiSocketId, IN UINT uiBufLen)
{
    _MSOCKET_CTRL_S *pstMSocket;

    pstMSocket = NAP_GetNodeByID(g_hMSocketNap, uiSocketId);
    if (NULL == pstMSocket)
    {
        RETURN(BS_NO_SUCH);
    }

    return Socket_SetSendBufSize(pstMSocket->iSocketId, uiBufLen);
}

/*ip/port:网络序*/
BS_STATUS MSocket_Listen(IN MSOCKET_ID uiSocketId, UINT uiLocalIp, IN USHORT usPort, IN UINT uiBacklog)
{
    _MSOCKET_CTRL_S *pstMSocket;

    pstMSocket = NAP_GetNodeByID(g_hMSocketNap, uiSocketId);
    if (NULL == pstMSocket)
    {
        RETURN(BS_NO_SUCH);
    }

    return Socket_Listen(pstMSocket->iSocketId, uiLocalIp, usPort, uiBacklog);
}

BS_STATUS MSocket_SendTo
(
    IN MSOCKET_ID uiSocketId,
    IN VOID *pBuf,
    IN UINT uiBufLen,
    IN UINT uiToIp/* 网络序 */,
    IN USHORT usToPort/* 网络序 */
)
{
    _MSOCKET_CTRL_S *pstMSocket;

    pstMSocket = NAP_GetNodeByID(g_hMSocketNap, uiSocketId);
    if (NULL == pstMSocket)
    {
        RETURN(BS_NO_SUCH);
    }

    return Socket_SendTo(pstMSocket->iSocketId, pBuf, uiBufLen, uiToIp, usToPort);
}

BS_STATUS MSocket_RecvFrom
(
    IN MSOCKET_ID uiSocketId,
    OUT VOID *pBuf,
    IN UINT uiBufLen,
    OUT UINT *puiRecvLen,
    OUT UINT *puiFromIp/* 网络序 */,
    OUT USHORT *pusFromPort/* 网络序 */
)
{
    _MSOCKET_CTRL_S *pstMSocket;

    pstMSocket = NAP_GetNodeByID(g_hMSocketNap, uiSocketId);
    if (NULL == pstMSocket)
    {
        RETURN(BS_NO_SUCH);
    }

    return Socket_RecvFrom(pstMSocket->iSocketId, pBuf, uiBufLen, puiRecvLen, puiFromIp, pusFromPort);
}

BS_STATUS MSocket_Pair(IN UINT uiType, OUT MSOCKET_ID auiFd[2])
{
    _MSOCKET_CTRL_S *pstMSocket1, *pstMSocket2;
    INT aiFdTmp [2];
    BS_STATUS eRet;

    MUTEX_P(&g_stMSocketMutex);
    pstMSocket1 = NAP_ZAlloc(g_hMSocketNap);
    pstMSocket2 = NAP_ZAlloc(g_hMSocketNap);    
    MUTEX_V(&g_stMSocketMutex);
    if ((NULL == pstMSocket1) || (NULL == pstMSocket2))
    {
        if (NULL != pstMSocket1)
        {
            MUTEX_P(&g_stMSocketMutex);
            NAP_Free(g_hMSocketNap, pstMSocket1);
            MUTEX_V(&g_stMSocketMutex);
        }

        if (NULL != pstMSocket2)
        {
            MUTEX_P(&g_stMSocketMutex);
            NAP_Free(g_hMSocketNap, pstMSocket2);
            MUTEX_V(&g_stMSocketMutex);
        }
        
        RETURN(BS_NO_RESOURCE);
    }

    eRet = Socket_Pair(uiType, aiFdTmp);
    if (BS_OK != eRet)
    {
        MUTEX_P(&g_stMSocketMutex);
        NAP_Free(g_hMSocketNap, pstMSocket1);
        NAP_Free(g_hMSocketNap, pstMSocket2);
        MUTEX_V(&g_stMSocketMutex);
        return eRet;
    }

    pstMSocket1->iSocketId = aiFdTmp[0];
    pstMSocket2->iSocketId = aiFdTmp[1];

    auiFd[0] = (UINT)NAP_GetIDByNode(g_hMSocketNap, pstMSocket1);
    auiFd[1] = (UINT)NAP_GetIDByNode(g_hMSocketNap, pstMSocket2);

    return BS_OK;
}

VOID MSocket_FdSet(IN MSOCKET_ID uiSocketId, IN fd_set *pstFdSet)
{
    _MSOCKET_CTRL_S *pstMSocket;

    pstMSocket = NAP_GetNodeByID(g_hMSocketNap, uiSocketId);
    if (NULL == pstMSocket)
    {
        return;
    }

    FD_SET((UINT)(pstMSocket->iSocketId), pstFdSet);
}

BOOL_T MSocket_FdIsSet(IN MSOCKET_ID uiSocketId, IN fd_set *pstFdSet)
{
    _MSOCKET_CTRL_S *pstMSocket;

    pstMSocket = NAP_GetNodeByID(g_hMSocketNap, uiSocketId);
    if (NULL == pstMSocket)
    {
        return FALSE;
    }

    return FD_ISSET(pstMSocket->iSocketId, pstFdSet);
}

VOID MSocket_FdClr(IN MSOCKET_ID uiSocketId, IN fd_set *pstFdSet)
{
    _MSOCKET_CTRL_S *pstMSocket;

    pstMSocket = NAP_GetNodeByID(g_hMSocketNap, uiSocketId);
    if (NULL == pstMSocket)
    {
        return;
    }

    FD_CLR((UINT)(pstMSocket->iSocketId), pstFdSet);
}

UINT MSocket_GetSocketId(IN MSOCKET_ID uiSocketId)
{
    _MSOCKET_CTRL_S *pstMSocket;

    pstMSocket = NAP_GetNodeByID(g_hMSocketNap, uiSocketId);
    if (NULL == pstMSocket)
    {
        return 0;
    }

    return pstMSocket->iSocketId;
}

/* 以下是MSOCKET的特有接口 */
BS_STATUS MSocket_SetUserHandle(IN MSOCKET_ID uiSocketId, IN HANDLE hUserHandle)
{
    _MSOCKET_CTRL_S *pstMSocket;

    pstMSocket = NAP_GetNodeByID(g_hMSocketNap, uiSocketId);
    if (NULL == pstMSocket)
    {
        RETURN(BS_NO_SUCH);
    }

    pstMSocket->hUserHandle = hUserHandle;

    return BS_OK;
}

BS_STATUS MSocket_GetUserHandle(IN MSOCKET_ID uiSocketId, OUT HANDLE *phUserHandle)
{
    _MSOCKET_CTRL_S *pstMSocket;

    pstMSocket = NAP_GetNodeByID(g_hMSocketNap, uiSocketId);
    if (NULL == pstMSocket)
    {
        RETURN(BS_NO_SUCH);
    }

    *phUserHandle = pstMSocket->hUserHandle;

    return BS_OK;
}

