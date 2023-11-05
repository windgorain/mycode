/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2009-8-15
* Description: 
* History:     
******************************************************************************/

#define RETCODE_FILE_NUM RETCODE_FILE_NUM_PLUG_BASE
    
#include "bs.h"

#include "utl/local_info.h"

typedef struct
{
    PF_SSLTCP_INNER_CB_FUNC pfCallBackFunc;
    USER_HANDLE_S stUserHandle;
}_SSLTCP_EPOLL_CB_S;


static int g_lSsltcpEpollId = 0;
static struct epoll_event g_astSsltcpEpollEvents[SSLTCP_MAX_SSLTCP_NUM];
static _SSLTCP_EPOLL_CB_S g_astSsltcpEpollCbs[SSLTCP_MAX_SSLTCP_NUM];

static BS_STATUS _SSLTCP_Epoll_Main(IN USER_HANDLE_S *pstUserHandle)
{
    int lNfds;
    struct epoll_event stEv;
    int i;
    int iSocketId;
    _SSLTCP_EPOLL_CB_S stEpollNode;
    UINT ulEvent;
    int what;

    for (;;)
    {
        lNfds = epoll_wait(g_lSsltcpEpollId, &g_astSsltcpEpollEvents, SSLTCP_MAX_SSLTCP_NUM, -1);
        for (i=0; i<lNfds; i++)
        {
            iSocketId = g_astSsltcpEpollEvents[i].datea.fd;

            if ((iSocketId <= 0) || (iSocketId > SSLTCP_MAX_SSLTCP_NUM))
            {
                continue;
            }

            SPLX_P();
            stEpollNode = g_astSsltcpEpollCbs[iSocketId - 1];
            SPLX_V();

            if (stEpollNode.pfCallBackFunc == NULL)
            {
                continue;
            }

            ulEvent = 0;
            what = g_astSsltcpEpollEvents[i].events;

            if (what & (EPOLLHUP|EPOLLERR))
            {
                ulEvent = SSLTCP_EVENT_EXECPT | SSLTCP_EVENT_READ | SSLTCP_EVENT_WRITE;
            }
            else
            {
                if (what & EPOLLIN)
                {
                    ulEvent |= SSLTCP_EVENT_READ;
                }

                if (what & EPOLLOUT)
                {
                    ulEvent |= SSLTCP_EVENT_WRITE;
                }
            }

            stEpollNode.pfCallBackFunc (UINT_HANDLE(iSocketId), ulEvent, &stEpollNode.stUserHandle);
        }
    }
}

static BS_STATUS _SSLTCP_Epll_AsynSocketInit()
{
    Mem_Zero(g_astSsltcpEpollCbs, sizeof(g_astSsltcpEpollCbs));
    
    g_lSsltcpEpollId = epoll_create(SSLTCP_MAX_SSLTCP_NUM);
    if (-1 == g_lSsltcpEpollId)
    {
        BS_WARNNING(("Can't craete epoll!"));
        RETURN(BS_ERR);
    }

    if (THREAD_ID_INVALID == THREAD_Create("Epoll", NULL, _SSLTCP_Epoll_Main, NULL)) {
        RETURN(BS_ERR);
    }

    return BS_OK;
}

static inline BOOL_T _SSLTCP_Epoll_IsAsyn(IN UINT ulSocketId)
{
    if (g_astSsltcpEpollCbs[ulSocketId - 1].pfCallBackFunc != NULL)
    {
        return TRUE;
    }

    return FALSE;
}

static BS_STATUS _SSLTCP_Epoll_SetAsyn
(
    IN HANDLE hFileHandle,
    IN PF_SSLTCP_INNER_CB_FUNC pfFunc,
    IN USER_HANDLE_S *pstUserHandle
)
{
    UINT ulSocketId = HANDLE_UINT(hFileHandle);
    struct epoll_event epev = {0, {0}};

    if (ulSocketId > SSLTCP_MAX_SSLTCP_NUM)
    {
        RETURN(BS_OUT_OF_RANGE);
    }

    if (TRUE == _SSLTCP_Epoll_IsAsyn(ulSocketId))
    {
        SPLX_P();
        g_astSsltcpEpollCbs[ulSocketId - 1].pfCallBackFunc = pfFunc;
        g_astSsltcpEpollCbs[ulSocketId - 1].stUserHandle = *pstUserHandle;
        SPLX_V();
        return BS_OK;
    }

    epev.data.fd = ulSocketId;
    epev.events = EPOLLIN | EPOLLOUT;

    SPLX_P();
    g_astSsltcpEpollCbs[ulSocketId - 1].pfCallBackFunc = pfFunc;
    g_astSsltcpEpollCbs[ulSocketId - 1].stUserHandle = *pstUserHandle;
    SPLX_V();

    if (epoll_ctl(g_lSsltcpEpollId, EPOLL_CTL_ADD, ulSocketId, &epev) == -1)
    {
	    RETURN(BS_ERR);
    }

    return BS_OK;
}

static BS_STATUS _SSLTCP_Epoll_UnSetAsyn(IN HANDLE hFileHandle)
{
    UINT ulSocketId = HANDLE_UINT(hFileHandle);
    struct epoll_event epev = {0, {0}};

    if (ulSocketId > SSLTCP_MAX_SSLTCP_NUM)
    {
        RETURN(BS_OUT_OF_RANGE);
    }

    if (FALSE == _SSLTCP_Epoll_IsAsyn(ulSocketId))
    {
        return BS_OK;
    }

    epev.data.fd = ulSocketId;
    epev.events = EPOLLIN | EPOLLOUT;

    if (epoll_ctl(g_lSsltcpEpollId, EPOLL_CTL_DEL, ulSocketId, &epev) == -1)
    {
	    RETURN(BS_ERR);
    }

    SPLX_P();
    g_astSsltcpEpollCbs[ulSocketId - 1].pfCallBackFunc = NULL;
    SPLX_V();

    return BS_OK;
}

static BS_STATUS _SSLTCP_Epoll_Accept(IN HANDLE hListenSocket, OUT HANDLE *phAcceptSocket)
{
    INT iAcceptId;
    
    iAcceptId = Socket_Accept(HANDLE_UINT(hListenSocket), NULL, NULL);
    if (iAcceptId < 0)
    {
        return BS_ERR;
    }

    *phAcceptSocket = UINT_HANDLE(iAcceptId);

    return BS_OK;    
}

static BS_STATUS _SSLTCP_Epoll_Read
(
    IN HANDLE hFileHandle,
    OUT UCHAR *pucBuf,
    IN UINT ulLen,
    OUT UINT *puiReadLen,
    IN UINT ulFlag
)
{
    return Socket_Read2(HANDLE_UINT(hFileHandle), pucBuf, ulLen, puiReadLen, ulFlag);
}

static INT _SSLTCP_Epoll_Write(IN HANDLE hFileHandle, IN UCHAR *pucBuf, IN UINT ulLen, IN UINT ulFlag)
{
    return Socket_Write(HANDLE_UINT(hFileHandle), pucBuf, ulLen, ulFlag);
}

static BS_STATUS _SSLTCP_Epoll_Close(IN HANDLE hFileHandle)
{
    _SSLTCP_Epoll_UnSetAsyn(hFileHandle);
    return Socket_Close(HANDLE_UINT(hFileHandle));
}

static BS_STATUS _SSLTCP_Epoll_Listen(IN HANDLE hFileHandle, UINT ulLocalIp, IN USHORT usPort, IN USHORT usBacklog)
{
    return Socket_Listen(HANDLE_UINT(hFileHandle), htonl(ulLocalIp), htons(usPort), usBacklog);
}

static BS_STATUS _SSLTCP_Epoll_Connect(IN HANDLE hFileHandle, IN UINT ulIp, IN USHORT usPort)
{
    int ret;
    ret = Socket_Connect(HANDLE_UINT(hFileHandle), ulIp, usPort);
    if (ret < 0) {
        if (ret == SOCKET_E_AGAIN) {
            return BS_AGAIN;
        }
        return BS_ERR;
    }

    return ret;
    
}

static BS_STATUS _SSLTCP_Epoll_GetHostIpPort(IN HANDLE hFileHandle, OUT UINT *pulIp, OUT USHORT *pusPort)
{
    return Socket_GetLocalIpPort(HANDLE_UINT(hFileHandle), pulIp, pusPort);
}

static BS_STATUS _SSLTCP_Epoll_GetPeerIpPort(IN HANDLE hFileHandle, OUT UINT *pulIp, OUT USHORT *pusPort)
{
    return Socket_GetPeerIpPort(HANDLE_UINT(hFileHandle), pulIp, pusPort);
}

static BS_STATUS _SSLTCP_Epoll_CreateTcp(IN UINT ulFamily, IN VOID *pParam, OUT HANDLE *hSslTcpId)
{
    UINT  ulIndexFrom1 = 0;
    INT  iSocketID;

    if ((iSocketID = Socket_Create (AF_INET, SOCK_STREAM)) < 0)
    {
        RETURN(BS_ERR);
    }

    *hSslTcpId = UINT_HANDLE(iSocketID);

    return BS_OK;
}

static BS_STATUS _SSLTCP_Epoll_Init()
{
    SSLTCP_PROTO_S stProto;

    _SSLTCP_Epll_AsynSocketInit();

    Mem_Zero(&stProto, sizeof(SSLTCP_PROTO_S));

    TXT_Strlcpy(stProto.szProtoName, "tcp", sizeof(stProto.szProtoName));
    stProto.pfCreate =  _SSLTCP_Epoll_CreateTcp;
    stProto.pfListen =  _SSLTCP_Epoll_Listen;
    stProto.pfConnect =  _SSLTCP_Epoll_Connect;
    stProto.pfAccept =  _SSLTCP_Epoll_Accept;
    stProto.pfWrite =  _SSLTCP_Epoll_Write;
    stProto.pfRead =  _SSLTCP_Epoll_Read;
    stProto.pfClose =  _SSLTCP_Epoll_Close;
    stProto.pfSetAsyn =  _SSLTCP_Epoll_SetAsyn;
    stProto.pfUnSetAsyn =  _SSLTCP_Epoll_UnSetAsyn;
    stProto.pfGetHostIpPort =  _SSLTCP_Epoll_GetHostIpPort;
    stProto.pfGetPeerIpPort =  _SSLTCP_Epoll_GetPeerIpPort;

    return SSLTCP_RegProto(&stProto);
}

static int _plug_init(IN CHAR *pszPlugFileName)
{
    if (BS_OK != _SSLTCP_Epoll_Init())
    {
        EXEC_OutString(" Can't init ssltcp tcp plug.\r\n");
        return BS_ERR;
    }

    return BS_OK;
}

PLUG_API int Plug_Stage(int stage)
{
    switch (stage) {
        case PLUG_STAGE_PLUG_LOAD:
            return _plug_init();
        default:
            break;
    }

    return 0;
}

PLUG_MAIN

