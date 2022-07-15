/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2014-2-25
* Description: 
* History:     
******************************************************************************/

#define _VS_FD_MAX_NUM 1024

static HANDLE g_hVsFdList;
static MUTEX_S g_stVsFdMutex;

BS_STATUS VS_FD_Init()
{
    g_hVsFdList = NAP_Create(NAP_TYPE_HASH, _VS_FD_MAX_NUM, sizeof(VS_FD_S), 0);
    if (NULL == g_hVsFdList)
    {
        return BS_NO_MEMORY;
    }

    MUTEX_Init(&g_stVsFdMutex);

    return BS_OK;
}

static INT vs_fd_Alloc(OUT VS_FD_S **ppstFp)
{
    VS_FD_S *pstFd;
    
    pstFd = NAP_ZAlloc(g_hVsFdList);
    if (NULL == pstFd)
    {
        return -1;
    }

    *ppstFp = pstFd;

    return NAP_GetIDByNode(g_hVsFdList, pstFd);
}

static VOID vs_fd_Close(IN VS_FD_S *pstFp)
{
    NAP_Free(g_hVsFdList, pstFp);
}

INT VS_FD_Alloc(OUT VS_FD_S **ppstFp)
{
    INT iFd;
    
    MUTEX_P(&g_stVsFdMutex);
    iFd = vs_fd_Alloc(ppstFp);
    MUTEX_V(&g_stVsFdMutex);

    return iFd;
}

VOID VS_FD_Close(IN VS_FD_S *pstFp)
{
    MUTEX_P(&g_stVsFdMutex);
    vs_fd_Close(pstFp);
    MUTEX_V(&g_stVsFdMutex);
}

VOID VS_FD_Attach(IN VS_FD_S *pstFp, IN VOID *pSocket)
{
    VS_SOCKET_S *pstSocket = pSocket;

    pstFp->pstSocket = pstSocket;
    pstSocket->pstFile = pstFp;
}

VOID * VS_FD_GetSocket(IN VS_FD_S *pstFp)
{
    return pstFp->pstSocket;
}

VOID * VS_FD_GetFp(IN INT iFd)
{
    VS_FD_S *pstFp;

    pstFp = NAP_GetNodeByID(g_hVsFdList, iFd);
    if (NULL == pstFp)
    {
        return NULL;
    }

    return VS_FD_GetSocket(pstFp);
}
