/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2015-8-31
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/ws_utl.h"
#include "utl/ssl_utl.h"
#include "utl/conn_utl.h"
#include "utl/socket_utl.h"
#include "utl/file_utl.h"
#include "utl/web_proxy.h"
#include "utl/string_utl.h"
#include "utl/txt_utl.h"
#include "utl/lstr_utl.h"
#include "utl/uri_acl.h"

#include "../h/svpn_def.h"
#include "../h/svpn_context.h"
#include "../h/svpn_trans.h"
#include "../h/svpn_role.h"
#include "../h/svpn_ulm.h"
#include "../h/svpn_acl.h"

#define SVPN_SSL_CTX_WORKER_NUM 4  
#define SVPN_SSL_CTX_WORKER_MASK (SVPN_SSL_CTX_WORKER_NUM - 1)

static WEB_PROXY_HANDLE g_hSvpnWebProxy = NULL;
static void *g_astWebProxySslCtx[SVPN_SSL_CTX_WORKER_NUM];

static BS_STATUS svpn_webproxy_InitSslOne(IN UINT uiIndex)
{
    VOID *pstSslCtx;

    pstSslCtx = SSL_UTL_Ctx_Create(0, 0);
    if (NULL == pstSslCtx)
    {
        return BS_ERR;
    }

    g_astWebProxySslCtx[uiIndex] = pstSslCtx;

    return BS_OK;
}

static BS_STATUS svpn_webproxy_InitWorkerInfo()
{
    UINT i;
    BS_STATUS eRet = BS_OK;
    
    for (i=0; i<SVPN_SSL_CTX_WORKER_NUM; i++)
    {
        eRet = svpn_webproxy_InitSslOne(i);
        if (eRet != BS_OK)
        {
            break;
        }
    }

    return eRet;
}

static BOOL_T svpn_webproxy_CheckAclPermit
(
    IN SVPN_CONTEXT_HANDLE hSvpnContext,
    IN CHAR *pcAclList,
    IN URI_ACL_MATCH_INFO_S *pstMatchInfo
)
{
    BOOL_T bPertmit = FALSE;

    SVPN_ACL_Match(hSvpnContext, pcAclList, pstMatchInfo, &bPertmit);

    return bPertmit;
}

static BOOL_T svpn_webproxy_CheckRolePermit
(
    IN SVPN_CONTEXT_HANDLE hSvpnContext,
    IN CHAR *pcRole,
    IN URI_ACL_MATCH_INFO_S *pstMatchInfo
)
{
    HSTRING hString;
    CHAR *pcACLs;
    CHAR *pcACL;
    BOOL_T bPermit = FALSE;

    hString = SVPN_Role_GetACL(hSvpnContext, pcRole);
    if (NULL == hString)
    {
        return FALSE;
    }

    pcACLs = STRING_GetBuf(hString);

    TXT_SCAN_ELEMENT_BEGIN(pcACLs, ',', pcACL)
    {
        if (TRUE == svpn_webproxy_CheckAclPermit(hSvpnContext, pcACL, pstMatchInfo))
        {
            TXT_SCAN_ELEMENT_STOP();
            bPermit = TRUE;
            break;
        }
    }TXT_SCAN_ELEMENT_END();

    STRING_Delete(hString);

    return bPermit;
}


static BS_STATUS svpn_webproxy_BuildMatchInfo(IN CHAR *pcUrl, OUT URI_ACL_MATCH_INFO_S *pstMatchInfo)
{
    LSTR_S stProtocol;
    LSTR_S stHost;
    LSTR_S stPort;
    LSTR_S stPath;
    BS_STATUS eRet = BS_OK;
    UINT uiCopyLen;
    UINT uiPort;

    if (BS_OK != WEB_Proxy_ParseServerUrl(pcUrl, NULL, &stProtocol, &stHost, &stPort, &stPath))
    {
        return BS_ERR;
    }

    if ((stProtocol.uiLen == STR_LEN("http"))
        && (0 == strnicmp(stProtocol.pcData, "http", stProtocol.uiLen)))
    {
        pstMatchInfo->enProtocol = URI_ACL_PROTOCOL_HTTP;
    }
    else if ((stProtocol.uiLen == STR_LEN("https"))
        && (0 == strnicmp(stProtocol.pcData, "https", stProtocol.uiLen)))
    {
        pstMatchInfo->enProtocol = URI_ACL_PROTOCOL_HTTPS;
    }
    else
    {
        return BS_NOT_SUPPORT;
    }

    pstMatchInfo->uiFlag |= URI_ACL_KEY_PROTOCOL;

    if ((stHost.uiLen > 2) && (stHost.pcData[0] == '['))
    {
        eRet = INET_ADDR_N_Str2IP(AF_INET6, stHost.pcData + 1, stHost.uiLen - 2, &pstMatchInfo->stAddr);
        pstMatchInfo->uiFlag |= URI_ACL_KEY_IPADDR;
    }
    else if (TRUE == Socket_N_IsIPv4(stHost.pcData, stHost.uiLen))
    {
        eRet = INET_ADDR_N_Str2IP(AF_INET, stHost.pcData, stHost.uiLen, &pstMatchInfo->stAddr);
        pstMatchInfo->uiFlag |= URI_ACL_KEY_IPADDR;
    }
    else
    {
        uiCopyLen = MIN(URI_ACL_MAX_DOMAIN_LEN, stHost.uiLen);
        memcpy(pstMatchInfo->szDomain, stHost.pcData, uiCopyLen);
        pstMatchInfo->szDomain[uiCopyLen] = '\0';
        pstMatchInfo->uiFlag |= URI_ACL_KEY_DOMAIN;
    }

    if (BS_OK != eRet)
    {
        return BS_ERR;
    }

    if (stPort.uiLen > 0)
    {
        uiPort = 0;
        LSTR_Atoui(&stPort, &uiPort);
        pstMatchInfo->usPort = uiPort;
        pstMatchInfo->uiFlag |= URI_ACL_KEY_PORT;
    }

    if (stPath.uiLen > 0)
    {
        uiCopyLen = MIN(URI_ACL_MAX_PATH_LEN, stPath.uiLen);
        memcpy(pstMatchInfo->szPath, stPath.pcData, uiCopyLen);
        pstMatchInfo->szPath[uiCopyLen] = '\0';
        pstMatchInfo->uiFlag |= URI_ACL_KEY_PATH;
    }

    return BS_OK;
}

static BOOL_T svpn_webproxy_CheckPermit(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN UINT uiOnlineID, IN CHAR *pcUrl)
{
    HSTRING hString;
    CHAR *pcRoles;
    CHAR *pcRole;
    BOOL_T bPermit = FALSE;
    URI_ACL_MATCH_INFO_S stMatchInfo = {0};

    if (BS_OK != svpn_webproxy_BuildMatchInfo(pcUrl, &stMatchInfo))
    {
        return FALSE;
    }

    hString = SVPN_ULM_GetUserRoles(hSvpnContext, uiOnlineID);
    if (NULL == hString)
    {
        return FALSE;
    }

    pcRoles = STRING_GetBuf(hString);

    TXT_SCAN_ELEMENT_BEGIN(pcRoles, ',', pcRole)
    {
        if (TRUE == svpn_webproxy_CheckRolePermit(hSvpnContext, pcRole, &stMatchInfo))
        {
            TXT_SCAN_ELEMENT_STOP();
            bPermit = TRUE;
            break;
        }
    }TXT_SCAN_ELEMENT_END();

    STRING_Delete(hString);

    return bPermit;
}

BS_STATUS SVPN_WebProxy_Init()
{
    g_hSvpnWebProxy = WEB_Proxy_Create(0);
    if (NULL == g_hSvpnWebProxy)
    {
        BS_DBGASSERT(0);
        return BS_NO_MEMORY;
    }
    
    if (BS_OK != svpn_webproxy_InitWorkerInfo())
    {
        WEB_Proxy_Destroy(g_hSvpnWebProxy);
        g_hSvpnWebProxy = NULL;
        return BS_ERR;
    }

    return BS_OK;
}

WS_DELIVER_RET_E SVPN_WebProxy_RequestIn(IN WS_TRANS_HANDLE hWsTrans, IN UINT uiEvent)
{
    CONN_HANDLE hDownConn;
    UINT uiWorkerID;
    void *pstSslCtx;
    SVPN_TRANS_S *pstSvpnTrans;
    HTTP_HEAD_PARSER hHeadParser;
    CHAR *pcUrl;

    hDownConn = WS_Conn_GetRawConn(WS_Trans_GetConn(hWsTrans));
    if (NULL == hDownConn)
    {
        return WS_DELIVER_RET_ERR;
    }

    if (WS_TRANS_EVENT_RECV_HEAD_OK == uiEvent)
    {
        pstSvpnTrans = SVPN_Trans_Create(hWsTrans);
        if (NULL == pstSvpnTrans)
        {
            return WS_DELIVER_RET_ERR;
        }

        hHeadParser = WS_Trans_GetHttpRequestParser(hWsTrans);
        pcUrl = HTTP_GetUriAbsPath(hHeadParser);

        
        if (TRUE != svpn_webproxy_CheckPermit(pstSvpnTrans->hSvpnContext, pstSvpnTrans->uiOnlineUserID, pcUrl))
        {
            WS_Trans_AddReplyBodyByString(hWsTrans,
                    (UCHAR*)"Not permit to access");
            WS_Trans_ReplyBodyFinish(hWsTrans);
            WS_Trans_Reply(hWsTrans, HTTP_STATUS_OK, WS_TRANS_REPLY_FLAG_IMMEDIATELY);
            return WS_DELIVER_RET_OK;
        }
    }

    if (WS_TRANS_EVENT_DESTORY == uiEvent)
    {
        SVPN_Trans_Destory(hWsTrans);
    }

    uiWorkerID = (UINT)(ULONG)CONN_GetUserData(hDownConn, CONN_USER_DATA_INDEX_0);
    pstSslCtx = g_astWebProxySslCtx[uiWorkerID & SVPN_SSL_CTX_WORKER_MASK];

    if (BS_OK != WEB_Proxy_RequestIn(g_hSvpnWebProxy, CONN_GetPoller(hDownConn), pstSslCtx, hWsTrans, uiEvent))
    {
        return WS_DELIVER_RET_ERR;
    }

    return WS_DELIVER_RET_OK;
}

WS_DELIVER_RET_E SVPN_WebProxy_ReferIn(IN WS_TRANS_HANDLE hTrans, IN UINT uiEvent)
{
    if (BS_OK != WEB_Proxy_ReferInput(g_hSvpnWebProxy, hTrans, uiEvent))
    {
        return WS_DELIVER_RET_ERR;
    }

    return WS_DELIVER_RET_OK;
}

WS_DELIVER_RET_E SVPN_WebProxy_AuthIn(IN WS_TRANS_HANDLE hTrans, IN UINT uiEvent)
{
    if (BS_OK != WEB_Proxy_AuthAgentInput(g_hSvpnWebProxy, hTrans, uiEvent))
    {
        return WS_DELIVER_RET_ERR;
    }

    return WS_DELIVER_RET_OK;
}


PLUG_API BS_STATUS SVPN_WebProxy_DebugPacket(IN UINT ulArgc, IN UCHAR **argv, IN VOID *pEnv)
{
    WEB_Proxy_SetDgbFlag(g_hSvpnWebProxy, WEB_PROXY_DBG_FLAG_PACKET);
    return BS_OK;
}


PLUG_API BS_STATUS SVPN_WebProxy_NoDebugPacket(IN UINT ulArgc, IN UCHAR **argv, IN VOID *pEnv)
{
    WEB_Proxy_ClrDgbFlag(g_hSvpnWebProxy, WEB_PROXY_DBG_FLAG_PACKET);
    return BS_OK;
}


PLUG_API BS_STATUS SVPN_WebProxy_DebugFsm(IN UINT ulArgc, IN UCHAR **argv, IN VOID *pEnv)
{
    WEB_Proxy_SetDgbFlag(g_hSvpnWebProxy, WEB_PROXY_DBG_FLAG_FSM);
    return BS_OK;
}


PLUG_API BS_STATUS SVPN_WebProxy_NoDebugFsm(IN UINT ulArgc, IN UCHAR **argv, IN VOID *pEnv)
{
    WEB_Proxy_ClrDgbFlag(g_hSvpnWebProxy, WEB_PROXY_DBG_FLAG_FSM);
    return BS_OK;
}


