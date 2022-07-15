/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2008-4-9
* Description: 
* History:     
******************************************************************************/
#include "bs.h"
#include "utl/hookapi.h"

INT WSAAPI _Tool_Connlog_ConnectHook(IN INT lSocketId, IN struct sockaddr_in *pstName, IN INT lNameLen);
INT WSAAPI _Tool_Connlog_WSAConnectHook
(
    SOCKET s,
    const struct sockaddr FAR * name,
    int namelen,
    LPWSABUF lpCallerData,
    LPWSABUF lpCalleeData,
    LPQOS lpSQOS,
    LPQOS lpGQOS
);

/* 重定向函数表 */
static HOOKAPI_ENTRY_TBL_S g_astToolConnLogEntryTbl[]  =
{
    {"WS2_32.dll", "connect", 0, (UINT)_Tool_Connlog_ConnectHook},
    {"wsock32.dll", "connect", 0, (UINT)_Tool_Connlog_ConnectHook},
    {"WS2_32.dll", "WSAConnect", 0, (UINT)_Tool_Connlog_WSAConnectHook},
};

static UINT g_ulConnLogLogId = 0;



INT WSAAPI _Tool_Connlog_ConnectHook(IN INT lSocketId, IN struct sockaddr_in *pstName, IN INT lNameLen)
{
    LogFile_OutString(g_ulConnLogLogId, "\r\nConntect %s", Socket_IpToName(htonl(pstName->sin_addr.s_addr)));

    return APITBL_Connect(lSocketId, pstName, lNameLen);
}

INT WSAAPI _Tool_Connlog_WSAConnectHook
(
    SOCKET s,
    IN struct sockaddr_in *pstName,
    int namelen,
    LPWSABUF lpCallerData,
    LPWSABUF lpCalleeData,
    LPQOS lpSQOS,
    LPQOS lpGQOS
)
{
    LogFile_OutString(g_ulConnLogLogId, "\r\nConntect %s", Socket_IpToName(htonl(pstName->sin_addr.s_addr)));

    return APITBL_WSAConnect(s, pstName, namelen, lpCallerData, lpCalleeData, lpSQOS, lpGQOS);
}

BS_STATUS Tool_Connlog_Init()
{
    static BOOL_T bIsInit = FALSE;

    if (bIsInit == FALSE)
    {
        bIsInit = TRUE;
        LoadBs_Init();
        g_ulConnLogLogId = LogFile_Open("connlog.txt");
        HOOKAPI_ReplaceIATByTable(g_astToolConnLogEntryTbl, sizeof(g_astToolConnLogEntryTbl)/sizeof(HOOKAPI_ENTRY_TBL_S));
    }

    return BS_OK;
}

