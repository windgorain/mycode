/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2009-7-7
* Description:  使用windows 窗口的异步TCP
* History:     
******************************************************************************/


#define RETCODE_FILE_NUM RETCODE_FILE_NUM_PLUG_BASE

#include "bs.h"

#include "utl/local_info.h"

#ifdef IN_WINDOWS

#define WM_SOCKET (WM_USER + 11)

typedef struct
{
    PF_SSLTCP_INNER_CB_FUNC pfCallBackFunc;
    USER_HANDLE_S stUserHandle;
}_SSLTCP_WSAASYN_ASYN_SOCKET_CB_S;

static BOOL_T g_bSsltcpWsaasynIsWindowOk = FALSE;
static HWND g_hSsltcpWsaasynWindow = NULL;
static _SSLTCP_WSAASYN_ASYN_SOCKET_CB_S g_astSsltcpWsaAsynSocketCbs[SSLTCP_MAX_SSLTCP_NUM];

static LRESULT CALLBACK SSLTCP_WsaAsyn_WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    _SSLTCP_WSAASYN_ASYN_SOCKET_CB_S stSocketCb;
    UINT ulEvent = 0;

    if (uMsg == WM_SOCKET)
    {
        SPLX_P();
        stSocketCb = g_astSsltcpWsaAsynSocketCbs[wParam - 1];
        SPLX_V();

        if (stSocketCb.pfCallBackFunc == NULL)
        {
            return 0;
        }

        if (WSAGETSELECTERROR(lParam))
        {
            ulEvent = SSLTCP_EVENT_WRITE | SSLTCP_EVENT_READ;
        }
        else
        {
            switch(WSAGETSELECTEVENT(lParam))
            {
            case FD_ACCEPT:
                ulEvent = SSLTCP_EVENT_READ;
            break;

            case FD_CONNECT:
                ulEvent = SSLTCP_EVENT_WRITE | SSLTCP_EVENT_READ;
                WSAAsyncSelect(lParam, g_hSsltcpWsaasynWindow, WM_SOCKET, FD_READ | FD_WRITE | FD_CLOSE);
                break;

            case FD_READ:
                ulEvent = SSLTCP_EVENT_READ;
            break;

            case FD_WRITE:
                ulEvent = SSLTCP_EVENT_WRITE;
            break;

            case FD_CLOSE:
                ulEvent = SSLTCP_EVENT_WRITE | SSLTCP_EVENT_READ;
            break;

            default:
                return 0;
                break;
            }
        }

        stSocketCb.pfCallBackFunc (UINT_HANDLE(wParam), ulEvent, &stSocketCb.stUserHandle);

        return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

static HWND _SSLTCP_WsaAsyn_MakeWorkerWindow()
{
   WNDCLASS wndclass;
   CHAR *ProviderClass = "AsyncSelect";
   HWND Window;

   wndclass.style = CS_HREDRAW | CS_VREDRAW;
   wndclass.lpfnWndProc = (WNDPROC)SSLTCP_WsaAsyn_WindowProc;
   wndclass.cbClsExtra = 0;
   wndclass.cbWndExtra = 0;
   wndclass.hInstance = NULL;
   wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
   wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
   wndclass.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
   wndclass.lpszMenuName = NULL;
   wndclass.lpszClassName = ProviderClass;

   if (RegisterClass(&wndclass) == 0)
   {
      BS_WARNNING(("RegisterClass() failed with error %d\n", GetLastError()));
      return NULL;
   }

   

   if ((Window = CreateWindow(
      ProviderClass,
      "",
      WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT,
      CW_USEDEFAULT,
      CW_USEDEFAULT,
      CW_USEDEFAULT,
      NULL,
      NULL,
      NULL,
      NULL)) == NULL)
   {
      BS_WARNNING(("CreateWindow() failed with error %d\n", GetLastError()));
      return NULL;
   }

   return Window;
}

static BS_STATUS _SSLTCP_WsaAsyn_InitWindow(IN USER_HANDLE_S *pstUserHandle)
{
    DWORD Ret;
    MSG msg;

    if ((g_hSsltcpWsaasynWindow = _SSLTCP_WsaAsyn_MakeWorkerWindow()) == NULL)
    {
        RETURN(BS_ERR);
    }

    g_bSsltcpWsaasynIsWindowOk = TRUE;

    while(Ret = GetMessage(&msg, NULL, 0, 0))
    {
       if (Ret == -1)
       {
          printf("GetMessage() failed with error %d\n", GetLastError());
          RETURN(BS_ERR);
       }
    
       TranslateMessage(&msg);
       DispatchMessage(&msg);
    }

    return BS_OK;
}

static BS_STATUS _SSLTCP_WsaAsyn_CreateTcp(IN UINT ulFamily, IN VOID *pParam, OUT HANDLE *hSslTcpId)
{
    INT iSocketID;

    if ((iSocketID = Socket_Create (AF_INET, SOCK_STREAM)) < 0)
    {
        RETURN(BS_ERR);
    }

    *hSslTcpId = UINT_HANDLE(iSocketID);

    return BS_OK;
}

static BS_STATUS _SSLTCP_WsaAsyn_Listen(IN HANDLE hFileHandle, UINT ulLocalIp, IN USHORT usPort, IN USHORT usBacklog)
{
    return Socket_Listen(HANDLE_UINT(hFileHandle), htonl(ulLocalIp), htons(usPort), usBacklog);
}

static BS_STATUS _SSLTCP_WsaAsyn_Connect(IN HANDLE hFileHandle, IN UINT ulIp, IN USHORT usPort)
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

static BS_STATUS _SSLTCP_WsaAsyn_SetAsyn
(
    IN HANDLE hFileHandle,
    IN PF_SSLTCP_INNER_CB_FUNC pfFunc,
    IN USER_HANDLE_S *pstUserHandle
)
{
    UINT ulSocketId = HANDLE_UINT(hFileHandle);
    UINT ulEvent;
    UINT ulRIp;
    USHORT usRPort;

    if (NULL == g_hSsltcpWsaasynWindow)
    {
        BS_WARNNING(("SSLTCP WsaAsyn Not Init!"));
        RETURN(BS_NOT_INIT);
    }

    if (ulSocketId > SSLTCP_MAX_SSLTCP_NUM)
    {
        BS_WARNNING(("The socket id %d is out of range!", ulSocketId));
        BS_DBGASSERT(0);
        RETURN(BS_OUT_OF_RANGE);
    }

    SPLX_P();
    g_astSsltcpWsaAsynSocketCbs[ulSocketId - 1].pfCallBackFunc = pfFunc;
    g_astSsltcpWsaAsynSocketCbs[ulSocketId - 1].stUserHandle = *pstUserHandle;
    SPLX_V();

    
    if ((BS_OK != Socket_GetPeerIpPort(HANDLE_UINT(hFileHandle), &ulRIp, &usRPort))
        || (ulRIp == 0) || (usRPort == 0))
    {
        ulEvent = FD_ACCEPT | FD_CLOSE | FD_CONNECT;
    }
    else
    {
        ulEvent = FD_READ | FD_WRITE | FD_ACCEPT | FD_CLOSE | FD_CONNECT;
    }

    WSAAsyncSelect(HANDLE_UINT(hFileHandle), g_hSsltcpWsaasynWindow, WM_SOCKET, ulEvent);

    return BS_OK;
}

static BS_STATUS _SSLTCP_WsaAsyn_UnSetAsyn(IN HANDLE hFileHandle)
{
    UINT ulSocketId = HANDLE_UINT(hFileHandle);
    UINT ulIoMode = 0;

    if (NULL == g_hSsltcpWsaasynWindow)
    {
        RETURN(BS_NOT_INIT);
    }

    if (ulSocketId <= SSLTCP_MAX_SSLTCP_NUM)
    {
        g_astSsltcpWsaAsynSocketCbs[ulSocketId - 1].pfCallBackFunc = NULL;
    }

    WSAAsyncSelect(HANDLE_UINT(hFileHandle), g_hSsltcpWsaasynWindow, WM_SOCKET, 0);
    Socket_Ioctl(ulSocketId, (INT) FIONBIO, &ulIoMode);

    return BS_OK;
}

static BS_STATUS _SSLTCP_WsaAsyn_Accept(IN HANDLE hListenSocket, OUT HANDLE *phAcceptSocket)
{
    INT iAcceptId;
    INT ulListhenId = HANDLE_UINT(hListenSocket);

    iAcceptId = Socket_Accept(ulListhenId, NULL, NULL);

    if ((ulListhenId <= SSLTCP_MAX_SSLTCP_NUM)
        && (g_astSsltcpWsaAsynSocketCbs[ulListhenId - 1].pfCallBackFunc != NULL))
    {
        _SSLTCP_WsaAsyn_UnSetAsyn(UINT_HANDLE(iAcceptId));
    }

    if (iAcceptId < 0)
    {
        return BS_ERR;
    }

    *phAcceptSocket = UINT_HANDLE(iAcceptId);

    return BS_OK;    
}

static INT _SSLTCP_WsaAsyn_Write(IN HANDLE hFileHandle, IN UCHAR *pucBuf, IN UINT ulLen, IN UINT ulFlag)
{
    return Socket_Write(HANDLE_UINT(hFileHandle), pucBuf, ulLen, ulFlag);
}

static BS_STATUS _SSLTCP_WsaAsyn_Read
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

static BS_STATUS _SSLTCP_WsaAsyn_Close(IN HANDLE hFileHandle)
{
    _SSLTCP_WsaAsyn_UnSetAsyn(hFileHandle);
    return Socket_Close(HANDLE_UINT(hFileHandle));
}

static BS_STATUS _SSLTCP_WsaAsyn_GetHostIpPort(IN HANDLE hFileHandle, OUT UINT *pulIp, OUT USHORT *pusPort)
{
    return Socket_GetLocalIpPort(HANDLE_UINT(hFileHandle), pulIp, pusPort);
}

static BS_STATUS _SSLTCP_WsaAsyn_GetPeerIpPort(IN HANDLE hFileHandle, OUT UINT *pulIp, OUT USHORT *pusPort)
{
    return Socket_GetPeerIpPort(HANDLE_UINT(hFileHandle), pulIp, pusPort);
}

static BS_STATUS _SSLTCP_WsaAsyn_Init()
{
    SSLTCP_PROTO_S stProto;
    UINT ulTid;

    Mem_Zero(g_astSsltcpWsaAsynSocketCbs, sizeof(g_astSsltcpWsaAsynSocketCbs));

    ulTid = THREAD_Create("wsaasynTcp", NULL, _SSLTCP_WsaAsyn_InitWindow, NULL);
    if (THREAD_ID_INVALID == ulTid) {
        RETURN(BS_ERR);
    }

    while (g_bSsltcpWsaasynIsWindowOk == FALSE)
    {
        Sleep(100);
    }

    Mem_Zero(&stProto, sizeof(SSLTCP_PROTO_S));

    TXT_Strlcpy(stProto.szProtoName, "tcp", sizeof(stProto.szProtoName));
    stProto.pfCreate = _SSLTCP_WsaAsyn_CreateTcp;
    stProto.pfListen = _SSLTCP_WsaAsyn_Listen;
    stProto.pfConnect = _SSLTCP_WsaAsyn_Connect;
    stProto.pfAccept = _SSLTCP_WsaAsyn_Accept;
    stProto.pfWrite = _SSLTCP_WsaAsyn_Write;
    stProto.pfRead = _SSLTCP_WsaAsyn_Read;
    stProto.pfClose = _SSLTCP_WsaAsyn_Close;
    stProto.pfSetAsyn = _SSLTCP_WsaAsyn_SetAsyn;
    stProto.pfUnSetAsyn = _SSLTCP_WsaAsyn_UnSetAsyn;
    stProto.pfGetHostIpPort = _SSLTCP_WsaAsyn_GetHostIpPort;
    stProto.pfGetPeerIpPort = _SSLTCP_WsaAsyn_GetPeerIpPort;

    return SSLTCP_RegProto(&stProto);
}

static int _plug_init()
{
    BS_STATUS eRet;

    if (BS_OK != (eRet = _SSLTCP_WsaAsyn_Init())) {
        EXEC_OutString(" Can't init ssltcp wsaasyn plug\r\n");
    }

    return eRet;
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

#endif

