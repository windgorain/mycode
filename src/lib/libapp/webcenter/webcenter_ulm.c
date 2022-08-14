/******************************************************************************
* Copyright (C), 2000-2006,  LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2016-10-27
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/conn_utl.h"
#include "utl/socket_utl.h"
#include "utl/ws_utl.h"
#include "utl/mutex_utl.h"
#include "utl/ulm_utl.h"
#include "comp/comp_localuser.h"

static MUTEX_S g_stWebCenterMutex;
static ULM_HANDLE g_hWebCenterUlm;

static BS_STATUS _webcenter_ulm_AddUser(IN CHAR *pcUserName)
{
    UINT uiUserID;

    MUTEX_P(&g_stWebCenterMutex);
    uiUserID = ULM_AddUser(g_hWebCenterUlm, pcUserName);
    MUTEX_V(&g_stWebCenterMutex);

    return uiUserID;
}

static UINT _webcenter_ulm_GetUserIDByCookie(IN CHAR *pcCookie)
{
    UINT uiUserID;

    if (NULL == pcCookie)
    {
        return 0;
    }

    MUTEX_P(&g_stWebCenterMutex);
    uiUserID = ULM_GetUserIDByCookie(g_hWebCenterUlm, pcCookie);
    MUTEX_V(&g_stWebCenterMutex);

    return uiUserID;
}

static VOID _webcenter_ulm_LogoutByCookie(IN CHAR *pcCookie)
{
    UINT uiUserID;

    if (NULL == pcCookie)
    {
        return;
    }

    MUTEX_P(&g_stWebCenterMutex);
    uiUserID = ULM_GetUserIDByCookie(g_hWebCenterUlm, pcCookie);
    if (uiUserID != 0)
    {
        ULM_DelUser(g_hWebCenterUlm, uiUserID);
    }
    MUTEX_V(&g_stWebCenterMutex);

    return;
}

BS_STATUS WebCenter_ULM_Init()
{
    MUTEX_Init(&g_stWebCenterMutex);
    g_hWebCenterUlm = ULM_CreateInstance(10);

    return BS_OK;
}

static BS_STATUS _webcenter_Login(IN WS_TRANS_HANDLE hWsTrans)
{
    HTTP_HEAD_PARSER hEncap;
    MIME_HANDLE hMime;
    CHAR *pcUserName;
    CHAR *pcPassword;
    UINT uiUserID;
    CHAR *pcCookie;
    CHAR szCookie[256];
    CHAR *pcRet;
    LOCALUSER_RET_E eLoginRet;

    hMime = WS_Trans_GetBodyMime(hWsTrans);
    if (NULL == hMime)
    {
        hMime = WS_Trans_GetQueryMime(hWsTrans);
    }

    if (NULL == hMime)
    {
        return BS_ERR;
    }

    pcUserName = MIME_GetKeyValue(hMime, "UserName");
    pcPassword = MIME_GetKeyValue(hMime, "Password");

    if ((NULL == pcUserName) || (NULL == pcPassword) || (pcUserName[0] == '\0'))
    {
        return BS_ERR;
    }

    hEncap = WS_Trans_GetHttpEncap(hWsTrans);

    eLoginRet = LocalUser_Login(CMD_EXP_RUNNER_TYPE_WEB, pcUserName, pcPassword);
    if (eLoginRet == LOCALUSER_OK)
    {
        uiUserID = _webcenter_ulm_AddUser(pcUserName);
        if (0 == uiUserID)
        {
            pcRet = "{\"error\":\"Fail\", \"reason\":\"Reach the max user number\"}";
        }
        else
        {
            pcCookie = ULM_GetUserCookie(g_hWebCenterUlm, uiUserID);
            snprintf(szCookie, sizeof(szCookie), "userid=%s; path=/", pcCookie);
            HTTP_SetHeadField(hEncap, HTTP_FIELD_SET_COOKIE, szCookie);
            pcRet = "{\"error\":\"Success\"}";
        }
    }
    else if (eLoginRet == LOCALUSER_COMP_NOT_INIT)
    {
        pcRet = "{\"error\":\"Fail\", \"reason\":\"Not init\"}";
    }
    else
    {
        pcRet = "{\"error\":\"Fail\", \"reason\":\"Auth failed\"}";
    }

    HTTP_SetStatusCode(hEncap, HTTP_STATUS_OK);
    HTTP_SetContentLen(hEncap, strlen(pcRet));
    HTTP_SetNoCache(hEncap);
    WS_Trans_SetHeadFieldFinish(hWsTrans);

    WS_Trans_AddReplyBodyByBuf(hWsTrans, pcRet, strlen(pcRet));
    WS_Trans_ReplyBodyFinish(hWsTrans);

    return BS_OK;
}

WS_DELIVER_RET_E WebCenter_Login(IN WS_TRANS_HANDLE hTrans, IN UINT uiEvent)
{
    BS_STATUS eRet = BS_OK;

    switch (uiEvent)
    {
        case WS_TRANS_EVENT_RECV_BODY_OK:
        {
            eRet = _webcenter_Login(hTrans);
            break;
        }

        default:
        {
            break;
        }
    }

    if (eRet != BS_OK)
    {
        return WS_DELIVER_RET_ERR;
    }

    return WS_DELIVER_RET_OK;
}


static BS_STATUS _webcenter_Logout(IN WS_TRANS_HANDLE hWsTrans)
{
    HTTP_HEAD_PARSER hEncap;
    CHAR *pcRet = "{\"result\":\"Success\"}";
    MIME_HANDLE hMime;

    hMime = WS_Trans_GetCookieMime(hWsTrans);
    if (NULL != hMime)
    {
        _webcenter_ulm_LogoutByCookie(MIME_GetKeyValue(hMime, "userid"));
    }

    hEncap = WS_Trans_GetHttpEncap(hWsTrans);

    HTTP_SetStatusCode(hEncap, HTTP_STATUS_OK);
    HTTP_SetContentLen(hEncap, strlen(pcRet));
    HTTP_SetNoCache(hEncap);
    WS_Trans_SetHeadFieldFinish(hWsTrans);

    WS_Trans_AddReplyBodyByBuf(hWsTrans, pcRet, strlen(pcRet));
    WS_Trans_ReplyBodyFinish(hWsTrans);

    return BS_OK;
}

WS_DELIVER_RET_E WebCenter_Logout(IN WS_TRANS_HANDLE hTrans, IN UINT uiEvent)
{
    BS_STATUS eRet = BS_OK;

    switch (uiEvent)
    {
        case WS_TRANS_EVENT_RECV_BODY_OK:
        {
            eRet = _webcenter_Logout(hTrans);
            break;
        }

        default:
        {
            break;
        }
    }

    if (eRet != BS_OK)
    {
        return WS_DELIVER_RET_ERR;
    }

    return WS_DELIVER_RET_OK;
}


UINT WebCenter_GetOnlineUserID(IN WS_TRANS_HANDLE hWsTrans)
{
    MIME_HANDLE hMime;
    CHAR *pcCookie;

    hMime = WS_Trans_GetCookieMime(hWsTrans);
    if (NULL == hMime)
    {
        return 0;
    }

    pcCookie = MIME_GetKeyValue(hMime, "userid");

    return _webcenter_ulm_GetUserIDByCookie(pcCookie);
}

BOOL_T WebCenter_IsLoopbackAddr(IN WS_TRANS_HANDLE hWsTrans)
{
    INT iFd;
    UINT uiIP = 0;
    USHORT usPort;
	CONN_HANDLE hConn;
    
    hConn = WS_Conn_GetRawConn(WS_Trans_GetConn(hWsTrans));
    iFd = CONN_GetFD(hConn);

    Socket_GetLocalIpPort(iFd, &uiIP, &usPort);

    if (uiIP == INADDR_LOOPBACK)
    {
        return TRUE;
    }

    return FALSE;
}

BOOL_T WebCenter_IsPermit(IN WS_TRANS_HANDLE hWsTrans)
{
    if (WebCenter_IsLoopbackAddr(hWsTrans))
    {
        return TRUE;
    }
    
    if (WebCenter_GetOnlineUserID(hWsTrans) == 0)
    {
        return FALSE;
    }

    return TRUE;
}

static BS_STATUS _webcenter_CheckOnline(IN WS_TRANS_HANDLE hWsTrans)
{
    HTTP_HEAD_PARSER hEncap;
    CHAR *pcRet;

    if (WebCenter_IsPermit(hWsTrans))
    {
        pcRet = "{\"result\":\"Success\"}";
    }
    else
    {
        pcRet = "{\"error\":\"Failed\"}";
    }

    hEncap = WS_Trans_GetHttpEncap(hWsTrans);

    HTTP_SetStatusCode(hEncap, HTTP_STATUS_OK);
    HTTP_SetContentLen(hEncap, strlen(pcRet));
    HTTP_SetNoCache(hEncap);
    WS_Trans_SetHeadFieldFinish(hWsTrans);

    WS_Trans_AddReplyBodyByBuf(hWsTrans, pcRet, strlen(pcRet));
    WS_Trans_ReplyBodyFinish(hWsTrans);

    return BS_OK;
}

WS_DELIVER_RET_E WebCenter_CheckOnline(IN WS_TRANS_HANDLE hTrans, IN UINT uiEvent)
{
    BS_STATUS eRet = BS_OK;

    switch (uiEvent)
    {
        case WS_TRANS_EVENT_RECV_BODY_OK:
        {
            eRet = _webcenter_CheckOnline(hTrans);
            break;
        }

        default:
        {
            break;
        }
    }

    if (eRet != BS_OK)
    {
        return WS_DELIVER_RET_ERR;
    }

    return WS_DELIVER_RET_OK;
}
