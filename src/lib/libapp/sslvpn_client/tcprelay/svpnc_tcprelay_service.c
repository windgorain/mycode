/******************************************************************************
* Copyright (C), 2000-2006,  Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2016-7-3
* Description: TCP监听服务
* History:     
******************************************************************************/
#include "bs.h"
    
#include "utl/sprintf_utl.h"
#include "utl/cjson.h"
#include "utl/txt_utl.h"
#include "utl/conn_utl.h"
#include "utl/ssl_utl.h"
#include "utl/vbuf_utl.h"
#include "utl/kv_utl.h"
#include "utl/mime_utl.h"
#include "utl/http_lib.h"
    
#include "../h/svpnc_conf.h"
#include "../h/svpnc_utl.h"
    
#include "svpnc_tcprelay_inner.h"

#define SVPNC_TCPRELAY_MAX_BUF_CACHED_SIZE 4096

typedef enum
{
    S_INIT = 0,
    S_DHSK,             /* 握手状态 */
    S_CONNECTING,       /* Socket连接状态 */
    S_SSL_CONNECTING,   /* SSL 连接状态 */
    S_HANDSHAKE,        /* TCP Relay握手状态 */
    S_FWD,              /* TCP Relay转发状态 */
}_WEB_PROXY_STATE_E;

typedef enum
{
    E_STEP = 0,
    E_DOWN_IN,
    E_DOWN_OUT,
    E_DOWN_ERR,
    E_UP_IN,
    E_UP_OUT,
    E_UP_ERR,
    E_SSL_CONNECTED,
    E_HANDSHAKE_OK,
}_WEB_PROXY_EVENT_E;

static BS_STATUS _vpnc_tr_DownHashShakeProcess
(
    IN SVPNC_TCPRELAY_NODE_S *pstNode,
    IN CHAR *pcHskType,
    IN CHAR *pcServerIP,
    IN CHAR *pcServerPort
)
{
    UINT uiIP;
    UINT uiPort;

    if (BS_OK != TXT_XAtoui(pcServerIP, &uiIP))
    {
        FSM_PushEvent(&pstNode->stFsm, E_UP_ERR);
        return BS_OK;
    }

    if (BS_OK != TXT_Atoui(pcServerPort, &uiPort))
    {
        FSM_PushEvent(&pstNode->stFsm, E_UP_ERR);
        return BS_OK;
    }

    if (strcmp(pcHskType, "HSK") == 0)
    {
        if (FALSE == SVPNC_TR_IsPermit(uiIP, uiPort))
        {
            FSM_PushEvent(&pstNode->stFsm, E_UP_ERR);
            return BS_OK;
        }

        pstNode->uiServerIP = uiIP;
        pstNode->usServerPort = uiPort;

        FSM_PushEvent(&pstNode->stFsm, E_STEP);

        return BS_OK;
    }

    return BS_OK;
}

static BS_STATUS _svpnc_tr_DownHandshake(IN FSM_S *pstFsm, IN UINT uiEvent)
{
    SVPNC_TCPRELAY_NODE_S *pstNode = FSM_GetPrivateData(pstFsm);
    INT iRet;
    CHAR szBuf[256];
    CHAR *pcData;
    KV_HANDLE hKvHandle;
    LSTR_S stLStr;
    CHAR *pcType;
    CHAR *pcServerIP;
    CHAR *pcPort;
    UINT uiHeadlen;

    while (1)
    {
        iRet = CONN_Read(pstNode->hDownConn, szBuf, sizeof(szBuf));
        if (iRet <= 0)
        {
            if (iRet != SOCKET_E_AGAIN)
            {
                FSM_PushEvent(pstFsm, E_UP_ERR);
            }
            break;
        }
        else
        {
            VBUF_CatFromBuf(&pstNode->stDownVBuf, szBuf, iRet);
        }
    }

    if (VBUF_GetDataLength(&pstNode->stDownVBuf) == 0)
    {
        return BS_OK;
    }

    pcData = VBUF_GetData(&pstNode->stDownVBuf);
    uiHeadLen = HTTP_GetHeadLen(pcData, VBUF_GetDataLength(&pstNode->stDownVBuf));

    if (uiHeadLen == 0) {
        return BS_OK;
    }

    hKvHandle = KV_Create(0);
    if (hKvHandle == NULL) {
        FSM_PushEvent(pstFsm, E_UP_ERR);
        return BS_OK;
    }

    stLStr.pcData = pcData;
    stLStr.uiLen = uiHeadlen - 4;
    KV_Parse(hKvHandle, &stLStr, '\n', '=');

    pcType = KV_GetKeyValue(hKvHandle, "Type");
    pcServerIP = KV_GetKeyValue(hKvHandle, "IP");
    pcPort = KV_GetKeyValue(hKvHandle, "Port");
    if (NULL == pcType)
    {
        KV_Destory(hKvHandle);
        FSM_PushEvent(pstFsm, E_UP_ERR);
        return BS_OK;
    }

    _vpnc_tr_DownHashShakeProcess(pstNode, pcType, pcServerIP, pcPort);

    KV_Destory(hKvHandle);

    VBUF_CutHead(&pstNode->stDownVBuf, uiHeadlen);

    return BS_OK;
}

static BS_WALK_RET_E _svpnc_tr_UpEvent(IN INT iSocketId, IN UINT uiEvent, IN USER_HANDLE_S *pstUserHandle)
{
    SVPNC_TCPRELAY_NODE_S *pstNode = pstUserHandle->ahUserHandle[0];
    BS_STATUS eRet = BS_OK;

    if (uiEvent & MYPOLL_EVENT_IN)
    {
        eRet = FSM_EventHandle(&pstNode->stFsm, E_UP_IN);
    }

    if (eRet == BS_STOP)
    {
        return BS_WALK_CONTINUE;
    }

    if (uiEvent & MYPOLL_EVENT_OUT)
    {
        eRet = FSM_EventHandle(&pstNode->stFsm, E_UP_OUT);
    }

    if (eRet == BS_STOP)
    {
        return BS_WALK_CONTINUE;
    }

    if (uiEvent & MYPOLL_EVENT_ERR)
    {
        FSM_EventHandle(&pstNode->stFsm, E_UP_ERR);
    }

    return BS_WALK_CONTINUE;
}

static CONN_HANDLE _svpnc_tr_CreateUpConn()
{
    CONN_HANDLE hUpConn;

    hUpConn = SVPNC_CreateServerConn();

    Socket_SetNoBlock(CONN_GetFD(hUpConn), TRUE);

    CONN_SetPoller(hUpConn, SVPNC_TR_GetMyPoller());

    return hUpConn;
}

static BS_STATUS _svpnc_tr_Connect(IN FSM_S *pstFsm, IN UINT uiEvent)
{
    SVPNC_TCPRELAY_NODE_S *pstNode = FSM_GetPrivateData(pstFsm);
    BS_STATUS eRet;
    CONN_HANDLE hUpConn;
    USER_HANDLE_S stUserHandle;

    hUpConn = _svpnc_tr_CreateUpConn();
    if (NULL == hUpConn)
    {
        FSM_PushEvent(pstFsm, E_DOWN_ERR);
        return BS_OK;
    }

    pstNode->hUpConn = hUpConn;
    stUserHandle.ahUserHandle[0] = pstNode;
    CONN_SetEvent(pstNode->hUpConn, MYPOLL_EVENT_OUT, _svpnc_tr_UpEvent, &stUserHandle);
    CONN_ClearEvent(pstNode->hDownConn);

    eRet = Socket_Connect(CONN_GetFD(pstNode->hUpConn), SVPNC_GetServerIP(), SVPNC_GetServerPort());

    if ((eRet != BS_OK) && (eRet != SOCKET_E_AGAIN))
    {
        FSM_PushEvent(pstFsm, E_DOWN_ERR);
        return BS_OK;
    }

    return BS_OK;
}

static BS_STATUS _svpnc_tr_StartHandshake(IN FSM_S *pstFsm, IN UINT uiEvent)
{
    SVPNC_TCPRELAY_NODE_S *pstNode = FSM_GetPrivateData(pstFsm);
    CHAR szInfo[512];
    UINT uiIP;

    uiIP = htonl(pstNode->uiServerIP);

    BS_Snprintf(szInfo, sizeof(szInfo),
        "TCP-Proxy / HTTP/1.1\r\n"
        "Host: %s\r\n"
        "server: %pI4:%d\r\n"
        "Cookie: svpnuid=%s\r\n"
        "\r\n",
        SVPNC_GetServer(), &uiIP, pstNode->usServerPort, SVPNC_GetCookie());

    CONN_WriteString(pstNode->hUpConn, szInfo);

    CONN_ModifyEvent(pstNode->hUpConn, MYPOLL_EVENT_IN);

    return BS_OK;
}

static BS_STATUS _svpnc_tr_Handshake(IN FSM_S *pstFsm, IN UINT uiEvent)
{
    SVPNC_TCPRELAY_NODE_S *pstNode = FSM_GetPrivateData(pstFsm);
    CHAR szInfo[2048];
    INT iRet;
    UINT uiHeadLen;
    BS_STATUS eRet;
    CHAR *pcData;

    while (1) /* SSL读取时,需要将SSL缓冲区读空,否则有可能不来Socket IN事件导致SSL缓冲区中数据无机会读取 */
    {
        iRet = CONN_Read(pstNode->hUpConn, szInfo, sizeof(szInfo));
        if (iRet <= 0)
        {
            if (iRet != SOCKET_E_AGAIN)
            {
                FSM_PushEvent(pstFsm, E_UP_ERR);
            }
            break;
        }
        else
        {
            VBUF_CatFromBuf(&pstNode->stUpVBuf, szInfo, iRet);
        }
    }

    if (VBUF_GetDataLength(&pstNode->stUpVBuf) == 0)
    {
        return BS_OK;
    }

    pcData = VBUF_GetData(&pstNode->stUpVBuf);

    uiHeadLen = HTTP_GetHeadLen(pcData, VBUF_GetDataLength(&pstNode->stUpVBuf));
    if (uiHeadLen == 0) {
        return BS_OK;
    }

    if (BS_OK != HTTP_ParseHead(pstNode->hHttpHeadParser,
                pcData, uiHeadLen, HTTP_RESPONSE))
    {
        FSM_PushEvent(pstFsm, E_UP_ERR);
        return BS_OK;
    }

    if (HTTP_STATUS_OK != HTTP_GetStatusCode(pstNode->hHttpHeadParser))
    {
        FSM_PushEvent(pstFsm, E_UP_ERR);
        return BS_OK;
    }

    VBUF_CutHead(&pstNode->stUpVBuf, uiHeadLen);

    FSM_PushEvent(pstFsm, E_HANDSHAKE_OK);

    return BS_OK;
}

static BS_STATUS _svpnc_tr_StartFwd(IN FSM_S *pstFsm, IN UINT uiEvent)
{
    SVPNC_TCPRELAY_NODE_S *pstNode = FSM_GetPrivateData(pstFsm);

    if (VBUF_GetDataLength(&pstNode->stUpVBuf) > 0)
    {
        CONN_ModifyEvent(pstNode->hDownConn, MYPOLL_EVENT_OUT);
    }
    else
    {
        CONN_ModifyEvent(pstNode->hUpConn, MYPOLL_EVENT_IN);
    }

    if (VBUF_GetDataLength(&pstNode->stDownVBuf) > 0)
    {
        CONN_ModifyEvent(pstNode->hUpConn, MYPOLL_EVENT_OUT);
    }
    else
    {
        CONN_ModifyEvent(pstNode->hDownConn, MYPOLL_EVENT_IN);
    }

    return BS_OK;
}

static BS_STATUS _svpnc_tr_FwdUpIn(IN FSM_S *pstFsm, IN UINT uiEvent)
{
    SVPNC_TCPRELAY_NODE_S *pstNode = FSM_GetPrivateData(pstFsm);
    UCHAR aucData[2048];
    INT iRet;

    while (VBUF_GetDataLength(&pstNode->stUpVBuf) < SVPNC_TCPRELAY_MAX_BUF_CACHED_SIZE)
    {
        iRet = CONN_Read(pstNode->hUpConn, aucData, sizeof(aucData));
        if (iRet <= 0)
        {
            if (iRet != SOCKET_E_AGAIN)
            {
                FSM_PushEvent(pstFsm, E_UP_ERR);
            }

            break;
        }
        else
        {
            VBUF_CatFromBuf(&pstNode->stUpVBuf, aucData, iRet);
        }
    }

    if (VBUF_GetDataLength(&pstNode->stUpVBuf) > 0)
    {
        CONN_DelEvent(pstNode->hUpConn, MYPOLL_EVENT_IN);
        CONN_AddEvent(pstNode->hDownConn, MYPOLL_EVENT_OUT);
    }

    return BS_OK;
}

static BS_STATUS _svpnc_tr_FwdUpOut(IN FSM_S *pstFsm, IN UINT uiEvent)
{
    SVPNC_TCPRELAY_NODE_S *pstNode = FSM_GetPrivateData(pstFsm);
    INT iRet = 0;

    if (VBUF_GetDataLength(&pstNode->stDownVBuf) > 0)
    {
        iRet = CONN_Write(pstNode->hUpConn, VBUF_GetData(&pstNode->stDownVBuf), VBUF_GetDataLength(&pstNode->stDownVBuf));
        if ((iRet < 0) && (iRet != SOCKET_E_AGAIN))
        {
            FSM_PushEvent(pstFsm, E_UP_ERR);
            return BS_ERR;
        }

        VBUF_CutHead(&pstNode->stDownVBuf, iRet);
    }

    if (VBUF_GetDataLength(&pstNode->stDownVBuf) == 0)
    {
        CONN_DelEvent(pstNode->hUpConn, MYPOLL_EVENT_OUT);
        CONN_AddEvent(pstNode->hDownConn, MYPOLL_EVENT_IN);
    }

    return BS_OK;
}

static BS_STATUS _svpnc_tr_FwdDownIn(IN FSM_S *pstFsm, IN UINT uiEvent)
{
    SVPNC_TCPRELAY_NODE_S *pstNode = FSM_GetPrivateData(pstFsm);
    UCHAR aucData[2048];
    INT iRet;

    iRet = CONN_Read(pstNode->hDownConn, aucData, sizeof(aucData));
    if (iRet < 0)
    {
        if (iRet == SOCKET_E_AGAIN)
        {
            return BS_OK;
        }

        FSM_PushEvent(pstFsm, E_DOWN_ERR);

        return BS_ERR;
    }
        
    if (iRet == 0)
    {
        FSM_PushEvent(pstFsm, E_DOWN_ERR);
        return BS_OK;
    }

    VBUF_CatFromBuf(&pstNode->stDownVBuf, aucData, iRet);

    CONN_DelEvent(pstNode->hDownConn, MYPOLL_EVENT_IN);
    CONN_AddEvent(pstNode->hUpConn, MYPOLL_EVENT_OUT);

    return BS_OK;
}

static BS_STATUS _svpnc_tr_FwdDownOut(IN FSM_S *pstFsm, IN UINT uiEvent)
{
    SVPNC_TCPRELAY_NODE_S *pstNode = FSM_GetPrivateData(pstFsm);
    INT iRet = 0;

    if (VBUF_GetDataLength(&pstNode->stUpVBuf) > 0)
    {
        iRet = CONN_Write(pstNode->hDownConn, VBUF_GetData(&pstNode->stUpVBuf), VBUF_GetDataLength(&pstNode->stUpVBuf));
        if ((iRet < 0) && (iRet != SOCKET_E_AGAIN))
        {
            FSM_PushEvent(pstFsm, E_DOWN_ERR);
            return BS_ERR;
        }

        VBUF_CutHead(&pstNode->stUpVBuf, iRet);
    }

    if (VBUF_GetDataLength(&pstNode->stUpVBuf) == 0)
    {
        /* 可能ssl中还有部分数据 */
        _svpnc_tr_FwdUpIn(pstFsm, uiEvent);
    }

    if (VBUF_GetDataLength(&pstNode->stUpVBuf) == 0)
    {
        CONN_DelEvent(pstNode->hDownConn, MYPOLL_EVENT_OUT);
        CONN_AddEvent(pstNode->hUpConn, MYPOLL_EVENT_IN);
    }

    return BS_OK;
}

static BS_STATUS _svpnc_tr_err(IN FSM_S *pstFsm, IN UINT uiEvent)
{
    SVPNC_TCPRELAY_NODE_S *pstNode = FSM_GetPrivateData(pstFsm);

    if (uiEvent == E_DOWN_ERR)
    {
        VBUF_Clear(&pstNode->stUpVBuf);
        CONN_Free(pstNode->hDownConn);
        pstNode->hDownConn = NULL;

        if (VBUF_GetDataLength(&pstNode->stDownVBuf) == 0)
        {
            CONN_Free(pstNode->hUpConn);
            pstNode->hUpConn = NULL;
        }
    }

    if (uiEvent == E_UP_ERR)
    {
        VBUF_Clear(&pstNode->stDownVBuf);
        CONN_Free(pstNode->hUpConn);
        pstNode->hUpConn = NULL;

        if (VBUF_GetDataLength(&pstNode->stUpVBuf) == 0)
        {
            CONN_Free(pstNode->hDownConn);
            pstNode->hDownConn = NULL;
        }
    }

    if ((pstNode->hUpConn == NULL) && (pstNode->hDownConn == NULL))
    {
        SVPNC_TRNode_Free(pstNode);
        return BS_STOP;
    }

    return BS_OK;
}

static BS_STATUS _svpnc_tr_SslConnect(IN FSM_S *pstFsm, IN UINT uiEvent)
{
    SVPNC_TCPRELAY_NODE_S *pstNode = FSM_GetPrivateData(pstFsm);
    INT iRet;
    BS_STATUS eRet = BS_OK;

    if (NULL == CONN_GetSsl(pstNode->hUpConn))
    {
        FSM_PushEvent(pstFsm, E_SSL_CONNECTED);
        return BS_OK;
    }

    iRet = SSL_UTL_Connect(CONN_GetSsl(pstNode->hUpConn));
    switch (iRet)
    {
        case SSL_UTL_E_NONE:
        {
            FSM_PushEvent(pstFsm, E_SSL_CONNECTED);
            break;
        }

        case SSL_UTL_E_WANT_READ:
        {
            CONN_ModifyEvent(pstNode->hUpConn, MYPOLL_EVENT_IN);
            break;
        }

        case SSL_UTL_E_WANT_WRITE:
        {
            CONN_ModifyEvent(pstNode->hUpConn, MYPOLL_EVENT_OUT);
            break;
        }

        default:
        {
            FSM_PushEvent(pstFsm, E_UP_ERR);
            break;
        }
    }

    return eRet;
}

static FSM_STATE_MAP_S g_astSvpncTRStateMap[] = 
{
    {"S.Init", S_INIT},
	{"S.DHSK", S_DHSK},
    {"S.Connecting", S_CONNECTING},
    {"S.SSL.Connecting", S_SSL_CONNECTING},
    {"S.Handshake", S_HANDSHAKE},
    {"S.Fwd", S_FWD},
};

static FSM_EVENT_MAP_S g_astSvpncTREventMap[] = 
{
    {"E.Step",              E_STEP},
    {"E.Down.In",           E_DOWN_IN},
    {"E.Down.Out",          E_DOWN_OUT},
    {"E.Down.Err",          E_DOWN_ERR},
    {"E.Up.In",             E_UP_IN},
    {"E.Up.Out",            E_UP_OUT},
    {"E.Up.Err",            E_UP_ERR},
    {"E.SSL.Connected",     E_SSL_CONNECTED},
    {"E.Handshake.OK",      E_HANDSHAKE_OK},
};

static FSM_SWITCH_MAP_S g_astSvpncTRSwitchMap[] =
{
    {"S.Init", "E.Step", "S.DHSK", NULL},
    {"S.DHSK",  "E.Down.In", "S.DHSK", _svpnc_tr_DownHandshake},
    {"S.DHSK",  "E.Step", "S.Connecting", _svpnc_tr_Connect},
    {"S.Connecting,S.SSL.Connecting", "E.Up.In,E.Up.Out", "S.SSL.Connecting", _svpnc_tr_SslConnect},
    {"S.SSL.Connecting", "E.SSL.Connected", "S.Handshake", _svpnc_tr_StartHandshake},
    {"S.Handshake", "E.Up.In", "@", _svpnc_tr_Handshake},
    {"S.Handshake", "E.Handshake.OK", "S.Fwd", _svpnc_tr_StartFwd},
    {"S.Fwd", "E.Down.In", "@", _svpnc_tr_FwdDownIn},
    {"S.Fwd", "E.Down.Out", "@", _svpnc_tr_FwdDownOut},
    {"S.Fwd", "E.Up.In", "@", _svpnc_tr_FwdUpIn},
    {"S.Fwd", "E.Up.Out", "@", _svpnc_tr_FwdUpOut},
    {"*", "E.Down.Err,E.Up.Err", "@", _svpnc_tr_err}
};

static FSM_SWITCH_TBL g_hSvpncTRSwitchTbl = NULL;

static BS_WALK_RET_E _svpnc_tr_DownEvent(IN INT iSocketId, IN UINT uiEvent, IN USER_HANDLE_S *pstUserHandle)
{
    SVPNC_TCPRELAY_NODE_S *pstNode = pstUserHandle->ahUserHandle[0];
    BS_STATUS eRet = BS_OK;

    if (uiEvent & MYPOLL_EVENT_IN)
    {
        eRet = FSM_EventHandle(&pstNode->stFsm, E_DOWN_IN);
    }

    if (eRet == BS_STOP)
    {
        return BS_WALK_CONTINUE;
    }

    if (uiEvent & MYPOLL_EVENT_OUT)
    {
        eRet = FSM_EventHandle(&pstNode->stFsm, E_DOWN_OUT);
    }

    if (eRet == BS_STOP)
    {
        return BS_WALK_CONTINUE;
    }

    if (uiEvent & MYPOLL_EVENT_ERR)
    {
        eRet = FSM_EventHandle(&pstNode->stFsm, E_DOWN_ERR);
    }

    return BS_WALK_CONTINUE;
}

static BS_WALK_RET_E _svpnc_tr_AcceptEventNotify(IN INT iSocketId, IN UINT uiEvent, IN USER_HANDLE_S *pstUserHandle)
{
    INT iAcceptSocketId;
    CONN_HANDLE hDownConn;
    SVPNC_TCPRELAY_NODE_S *pstNode;
    USER_HANDLE_S stUserHandle;

    iAcceptSocketId = Socket_Accept(iSocketId, NULL, NULL);
    if (iAcceptSocketId < 0)
    {
        return BS_WALK_CONTINUE;
    }

    Socket_SetNoBlock(iAcceptSocketId, TRUE);

    hDownConn = CONN_New(iAcceptSocketId);
    if (hDownConn == NULL)
    {
        Socket_Close(iAcceptSocketId);
        return BS_WALK_CONTINUE;
    }

    CONN_SetPoller(hDownConn, SVPNC_TR_GetMyPoller());

    pstNode = SVPNC_TRNode_New();
    if (NULL == pstNode)
    {
        CONN_Free(hDownConn);
        return BS_WALK_CONTINUE;
    }

    pstNode->hDownConn = hDownConn;

    stUserHandle.ahUserHandle[0] = pstNode;

    CONN_SetEvent(pstNode->hDownConn, MYPOLL_EVENT_IN, _svpnc_tr_DownEvent, &stUserHandle);

    FSM_Init(&pstNode->stFsm, g_hSvpncTRSwitchTbl);
    FSM_SetPrivateData(&pstNode->stFsm, pstNode);
    FSM_InitState(&pstNode->stFsm, S_INIT);

    if (BS_OK != FSM_InitEventQue(&pstNode->stFsm, 10))
    {
        SVPNC_TRNode_Free(pstNode);
        return BS_WALK_CONTINUE;
    }

    FSM_EventHandle(&pstNode->stFsm, E_STEP);

    return BS_WALK_CONTINUE;
}

static BS_STATUS _svpnc_tr_StartService()
{
    INT iSocket;

    iSocket = Socket_Create(AF_INET, SOCK_STREAM);
    if (iSocket < 0)
    {
        return BS_ERR;
    }

    if (BS_OK != Socket_Listen(iSocket, htonl(0x7f000001), htons(60002), 5))
    {
        Socket_Close(iSocket);
        return BS_ERR;
    }

    Socket_SetNoBlock(iSocket, TRUE);

    MyPoll_SetEvent(SVPNC_TR_GetMyPoller(), iSocket, MYPOLL_EVENT_IN, _svpnc_tr_AcceptEventNotify, NULL);

    return BS_OK;
}

BS_STATUS SVPNC_TRService_Init()
{
    if (NULL == g_hSvpncTRSwitchTbl)
    {
        g_hSvpncTRSwitchTbl = FSM_CREATE_SWITCH_TBL(g_astSvpncTRStateMap,
                    g_astSvpncTREventMap, g_astSvpncTRSwitchMap);
    }

    if (NULL == g_hSvpncTRSwitchTbl)
    {
        return BS_NO_MEMORY;
    }
    
    _svpnc_tr_StartService();

    return BS_OK;
}

