/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2008-3-20
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/hookapi.h"
#include "utl/sock_rdt.h"

typedef struct tagSOCK_RDT_EVENT_CALL_BACK_S
{
    DLL_NODE(tagSOCK_RDT_EVENT_CALL_BACK_S) stDllNode;
    UINT ulEvent;
    PF_SOCK_RDT_CALL_BACK pfFunc;
    UINT ulUserHandle;
}SOCK_EVENT_CALL_BACK_S;


PLUG_API INT _SockRDT_ConnectHook(IN INT lSocketId, IN struct sockaddr_in *pstName, IN INT lNameLen);


static CHAR * g_pszSockRdtConnectType = "tcp";
static UINT g_ulSockRdtServerIp = 0;
static USHORT g_usSockRdtServerPort = 0;
static DLL_HEAD_S g_stEventCbList = DLL_HEAD_INIT_VALUE(&g_stEventCbList);


static HOOKAPI_ENTRY_TBL_S g_astSockRdtEntryTbl[]  =
{
    {"WS2_32.dll", "connect", 0, (HANDLE)_SockRDT_ConnectHook},
    {"wsock32.dll", "connect", 0, (HANDLE)_SockRDT_ConnectHook},
};




PLUG_API INT _SockRDT_ConnectHook(IN INT lSocketId, IN struct sockaddr_in *pstName, IN INT lNameLen)
{
    struct sockaddr_in server_addr;
    SOCK_EVENT_CALL_BACK_S *pstNode;
    

    if (pstName->sin_family != AF_INET)
    {
        BS_WARNNING(("Not support yet!"));
        return -1;
    }

    Mem_Zero (&server_addr, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(g_ulSockRdtServerIp);
    server_addr.sin_port = htons(g_usSockRdtServerPort);

    DLL_SCAN(&g_stEventCbList, pstNode)
    {
        if (pstNode->ulEvent & SOCK_RDT_EVENT_BEFORE_CONNTION)
        {
            pstNode->pfFunc(SOCK_RDT_EVENT_BEFORE_CONNTION, (UINT)lSocketId, pstName, lNameLen, pstNode->ulUserHandle);
        }
    }

    if (APITBL_Connect(lSocketId, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        return -1;
    }

    DLL_SCAN(&g_stEventCbList, pstNode)
    {
        if (pstNode->ulEvent & SOCK_RDT_EVENT_AFTER_CONNTION)
        {
            pstNode->pfFunc(SOCK_RDT_EVENT_AFTER_CONNTION, (UINT)lSocketId, pstName, lNameLen, pstNode->ulUserHandle);
        }
    }


    return 0;
}


BS_STATUS SockRDT_RedirectTo(IN CHAR *pszProtocolName, IN UINT ulRdtIp, IN USHORT usRdtPort)
{
    g_pszSockRdtConnectType = pszProtocolName;
    g_ulSockRdtServerIp = ulRdtIp;
    g_usSockRdtServerPort = usRdtPort;

    HOOKAPI_ReplaceIATByTable(g_astSockRdtEntryTbl, sizeof(g_astSockRdtEntryTbl)/sizeof(HOOKAPI_ENTRY_TBL_S));

    return BS_OK;
}

BS_STATUS SockRDT_RegEvent(IN UINT ulEvent, IN PF_SOCK_RDT_CALL_BACK pfFunc, IN UINT ulUserHandle)
{
    SOCK_EVENT_CALL_BACK_S *pstNode;

    pstNode = MEM_ZMalloc(sizeof(SOCK_EVENT_CALL_BACK_S));
    if (NULL == pstNode)
    {
        RETURN(BS_NO_MEMORY);
    }

    pstNode->ulEvent = ulEvent;
    pstNode->pfFunc = pfFunc;
    pstNode->ulUserHandle = ulUserHandle;

    DLL_ADD((DLL_HEAD_S*)&g_stEventCbList, pstNode);

    return BS_OK;
}


