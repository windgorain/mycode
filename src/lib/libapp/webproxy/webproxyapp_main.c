/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2015-12-5
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/ws_utl.h"
#include "utl/ssl_utl.h"
#include "utl/conn_utl.h"
#include "utl/file_utl.h"
#include "utl/web_proxy.h"

#include "webproxyapp_main.h"

#define WEB_PROXY_APP_CTX_WORKER_NUM 4  /* 必须是2的指数方次, 下面的mask才有意义 */
#define WEB_PROXY_APP_SSL_CTX_WORKER_MASK (WEB_PROXY_APP_CTX_WORKER_NUM - 1)

static WEB_PROXY_HANDLE g_hWebProxyApp = NULL;
static void * g_astWebProxyAppSslCtx[WEB_PROXY_APP_CTX_WORKER_NUM];

static BS_STATUS webproxyapp_main_InitSslOne(IN UINT uiIndex)
{
    VOID *pstSslCtx;

    pstSslCtx = SSL_UTL_Ctx_Create(0, 0);
    if (NULL == pstSslCtx) {
        return BS_ERR;
    }

    g_astWebProxyAppSslCtx[uiIndex] = pstSslCtx;

    return BS_OK;
}

static BS_STATUS webproxyapp_main_InitWorkerInfo()
{
    UINT i;
    BS_STATUS eRet = BS_OK;
    
    for (i=0; i<WEB_PROXY_APP_CTX_WORKER_NUM; i++)
    {
        eRet = webproxyapp_main_InitSslOne(i);
        if (eRet != BS_OK)
        {
            break;
        }
    }

    return eRet;
}

BS_STATUS WebProxyApp_Main_Init()
{
    g_hWebProxyApp = WEB_Proxy_Create(0);
    if (NULL == g_hWebProxyApp)
    {
        BS_DBGASSERT(0);
        return BS_NO_MEMORY;
    }
    
    if (BS_OK != webproxyapp_main_InitWorkerInfo())
    {
        WEB_Proxy_Destroy(g_hWebProxyApp);
        g_hWebProxyApp = NULL;
        return BS_ERR;
    }

    return BS_OK;
}

WS_DELIVER_RET_E WebProxyApp_Main_RequestIn(IN WS_TRANS_HANDLE hTrans, IN UINT uiEvent)
{
    CONN_HANDLE hDownConn;
    UINT uiWorkerID;
    void *sslctx;

    hDownConn = WS_Conn_GetRawConn(WS_Trans_GetConn(hTrans));
    if (NULL == hDownConn)
    {
        return WS_DELIVER_RET_ERR;
    }

    uiWorkerID = (UINT)(ULONG)CONN_GetUserData(hDownConn, CONN_USER_DATA_INDEX_0);
    sslctx = &g_astWebProxyAppSslCtx[uiWorkerID & WEB_PROXY_APP_SSL_CTX_WORKER_MASK];

    if (BS_OK != WEB_Proxy_RequestIn(g_hWebProxyApp, CONN_GetPoller(hDownConn), sslctx, hTrans, uiEvent))
    {
        return WS_DELIVER_RET_ERR;
    }

    return WS_DELIVER_RET_OK;
}

WS_DELIVER_RET_E WebProxyApp_Main_ReferIn(IN WS_TRANS_HANDLE hTrans, IN UINT uiEvent)
{
    if (BS_OK != WEB_Proxy_ReferInput(g_hWebProxyApp, hTrans, uiEvent))
    {
        return WS_DELIVER_RET_ERR;
    }

    return WS_DELIVER_RET_OK;
}

WS_DELIVER_RET_E WebProxyApp_Main_AuthIn(IN WS_TRANS_HANDLE hTrans, IN UINT uiEvent)
{
    if (BS_OK != WEB_Proxy_AuthAgentInput(g_hWebProxyApp, hTrans, uiEvent))
    {
        return WS_DELIVER_RET_ERR;
    }

    return WS_DELIVER_RET_OK;
}

/* debug web-proxy packet */
PLUG_API BS_STATUS WebProxyApp_Main_DebugPacket(IN UINT ulArgc, IN UCHAR **argv, IN VOID *pEnv)
{
    WEB_Proxy_SetDgbFlag(g_hWebProxyApp, WEB_PROXY_DBG_FLAG_PACKET);
    return BS_OK;
}

/* no debug web-proxy packet */
PLUG_API BS_STATUS WebProxyApp_Main_NoDebugPacket(IN UINT ulArgc, IN UCHAR **argv, IN VOID *pEnv)
{
    WEB_Proxy_ClrDgbFlag(g_hWebProxyApp, WEB_PROXY_DBG_FLAG_PACKET);
    return BS_OK;
}

/* debug web-proxy fsm */
PLUG_API BS_STATUS WebProxyApp_Main_DebugFsm(IN UINT ulArgc, IN UCHAR **argv, IN VOID *pEnv)
{
    WEB_Proxy_SetDgbFlag(g_hWebProxyApp, WEB_PROXY_DBG_FLAG_FSM);
    return BS_OK;
}

/* no debug web-proxy fsm */
PLUG_API BS_STATUS WebProxyApp_Main_NoDebugFsm(IN UINT ulArgc, IN UCHAR **argv, IN VOID *pEnv)
{
    WEB_Proxy_ClrDgbFlag(g_hWebProxyApp, WEB_PROXY_DBG_FLAG_FSM);
    return BS_OK;
}


