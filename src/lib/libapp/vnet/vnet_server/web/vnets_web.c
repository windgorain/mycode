/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-2-16
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "comp/comp_wsapp.h"

#include "../inc/vnets_conf.h"
#include "../inc/vnets_protocol.h"
#include "../inc/vnets_domain.h"
#include "../inc/vnets_ses.h"
#include "../inc/vnets_dc.h"
#include "../inc/vnets_web.h"
#include "../inc/vnets_node.h"

#include "vnets_web_inner.h"
#include "vnets_web_ulm.h"

static WS_DELIVER_TBL_HANDLE g_hVnetsWebDeliverTbl = NULL;

static BS_STATUS vnets_web_GetUserID(IN WS_TRANS_HANDLE hWsTrans, OUT VNETS_WEB_USER_S *pstWebUser)
{
    MIME_HANDLE hCookieMime;
    CHAR *pcOnlineUserCookie;
    CHAR *pcNodeCookie;
    UINT uiUserID = 0;

    hCookieMime = WS_Trans_GetCookieMime(hWsTrans);
    if (NULL == hCookieMime)
    {
        return BS_ERR;
    }

    pcOnlineUserCookie = MIME_GetKeyValue(hCookieMime, "vnet_userid");
    if (NULL != pcOnlineUserCookie)
    {
        uiUserID = VNETS_WebUlm_GetUserIDByCookie(pcOnlineUserCookie);
        if (uiUserID != 0)
        {
            pstWebUser->ucType = VNETS_WEB_USER_TYPE_WEB;
            pstWebUser->uiOnlineUserID = uiUserID;

            return BS_OK;
        }
    }

    pcNodeCookie = MIME_GetKeyValue(hCookieMime, "vnet_nodecookie");
    if (NULL != pcNodeCookie)
    {
        uiUserID = VNETS_NODE_GetNodeIdByCookieString(pcNodeCookie);
        if (uiUserID != 0)
        {
            pstWebUser->ucType = VNETS_WEB_USER_TYPE_CLIENT;
            pstWebUser->uiOnlineUserID = uiUserID;

            return BS_OK;
        }
    }

    return BS_ERR;
}

static BS_STATUS vnets_web_RecvBodyOK(IN WS_TRANS_HANDLE hWsTrans)
{
    HTTP_HEAD_PARSER hEncap;
    BS_STATUS eRet;
    cJSON *pstJson;
    CHAR *pcJson;
    UINT uiBodyLen;
    VNETS_WEB_S stDWeb = {0};

    pstJson = cJSON_CreateObject();
    if (NULL == pstJson)
    {
        return BS_NO_MEMORY;
    }

    stDWeb.hWsTrans = hWsTrans;
    stDWeb.pstJson = pstJson;
    vnets_web_GetUserID(hWsTrans, &stDWeb.stOnlineUser);

    eRet = VNETS_WebKf_Run(&stDWeb);
    if (eRet != BS_OK)
    {
        cJSON_Delete(pstJson);
        return BS_ERR;
    }

    if (stDWeb.uiFlag & VNETS_WEB_FLAG_EMPTY_JSON)
    {
        cJSON_Delete(pstJson);
        return BS_OK;
    }

    pcJson = cJSON_Print(pstJson);
    if (NULL == pcJson)
    {
        cJSON_Delete(pstJson);
        return BS_ERR;
    }

    uiBodyLen = strlen(pcJson);

    hEncap = WS_Trans_GetHttpEncap(hWsTrans);
    HTTP_SetStatusCode(hEncap, HTTP_STATUS_OK);
    HTTP_SetContentLen(hEncap, uiBodyLen);
    HTTP_SetNoCache(hEncap);
    WS_Trans_SetHeadFieldFinish(hWsTrans);
    WS_Trans_AddReplyBodyByBuf(hWsTrans, pcJson, uiBodyLen);
    WS_Trans_ReplyBodyFinish(hWsTrans);

    free(pcJson);
    cJSON_Delete(pstJson);

    return BS_OK;
}

static WS_DELIVER_RET_E vnets_web_RequestIn(IN WS_TRANS_HANDLE hTrans, IN UINT uiEvent)
{
    BS_STATUS eRet = BS_OK;

    switch (uiEvent)
    {
        case WS_TRANS_EVENT_RECV_BODY_OK:
        {
            eRet = vnets_web_RecvBodyOK(hTrans);
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

BS_STATUS VNETS_Web_Init()
{
    g_hVnetsWebDeliverTbl = WS_Deliver_Create();
    if (NULL == g_hVnetsWebDeliverTbl)
    {
        return BS_NO_MEMORY;
    }

    WS_Deliver_Reg(g_hVnetsWebDeliverTbl, 100, WS_DELIVER_TYPE_FILE,
        "/request.cgi", vnets_web_RequestIn, WS_DELIVER_FLAG_PARSE_BODY_AS_MIME);

    return BS_OK;
}

VOID VNETS_Web_BindService(IN CHAR *pcWsService)
{
    COMP_WSAPP_SetDeliverTbl(pcWsService, g_hVnetsWebDeliverTbl);
}


