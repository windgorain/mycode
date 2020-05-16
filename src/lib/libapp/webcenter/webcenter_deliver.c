/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2015-12-12
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "comp/comp_wsapp.h"

#include "webcenter_inner.h"

static WS_DELIVER_TBL_HANDLE g_hWebCenterDeliverTbl = NULL;


static BS_STATUS webcenter_CheckServiceOnRecvBodyOK(IN WS_TRANS_HANDLE hWsTrans)
{
    HTTP_HEAD_PARSER hEncap;
    MIME_HANDLE hMime;
    CHAR *pcFuncName;
    CHAR *pcRet = "({\"result\":1})";

    hEncap = WS_Trans_GetHttpEncap(hWsTrans);

    hMime = WS_Trans_GetQueryMime(hWsTrans);
    if (hMime == NULL)
    {
        return BS_OK;
    }

    pcFuncName = MIME_GetKeyValue(hMime, "callback");
    if (NULL == pcFuncName)
    {
        return BS_OK;
    }

    HTTP_SetStatusCode(hEncap, HTTP_STATUS_OK);
    HTTP_SetNoCache(hEncap);
    WS_Trans_SetHeadFieldFinish(hWsTrans);

    WS_Trans_AddReplyBodyByBuf(hWsTrans, pcFuncName, strlen(pcFuncName));
    WS_Trans_AddReplyBodyByBuf(hWsTrans, pcRet, strlen(pcRet));
    WS_Trans_ReplyBodyFinish(hWsTrans);

    return BS_OK;
}

static WS_DELIVER_RET_E webcenter_CheckServiceOn(IN WS_TRANS_HANDLE hTrans, IN UINT uiEvent)
{
    BS_STATUS eRet = BS_OK;

    switch (uiEvent)
    {
        case WS_TRANS_EVENT_RECV_BODY_OK:
        {
            eRet = webcenter_CheckServiceOnRecvBodyOK(hTrans);
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

BS_STATUS WebCenter_Deliver_Init()
{
    g_hWebCenterDeliverTbl = WS_Deliver_Create();
    if (NULL == g_hWebCenterDeliverTbl)
    {
        return BS_NO_MEMORY;
    }

    WS_Deliver_Reg(g_hWebCenterDeliverTbl, 100, WS_DELIVER_TYPE_FILE,
        "/login.cgi", WebCenter_Login, WS_DELIVER_FLAG_PARSE_BODY_AS_MIME);

    WS_Deliver_Reg(g_hWebCenterDeliverTbl, 100, WS_DELIVER_TYPE_FILE,
        "/logout.cgi", WebCenter_Logout, WS_DELIVER_FLAG_DROP_BODY);

    WS_Deliver_Reg(g_hWebCenterDeliverTbl, 100, WS_DELIVER_TYPE_FILE,
        "/checkonline.cgi", WebCenter_CheckOnline, WS_DELIVER_FLAG_DROP_BODY);

    WS_Deliver_Reg(g_hWebCenterDeliverTbl, 100, WS_DELIVER_TYPE_FILE,
        "/request.cgi", WebCenter_RequestIn, WS_DELIVER_FLAG_PARSE_BODY_AS_MIME);

    WS_Deliver_Reg(g_hWebCenterDeliverTbl, 100, WS_DELIVER_TYPE_FILE,
        "/checkserviceon.cgi", webcenter_CheckServiceOn, WS_DELIVER_FLAG_PARSE_BODY_AS_MIME);

    return BS_OK;
}

VOID WebCenter_Deliver_BindService(IN CHAR *pcWsService)
{
    COMP_WSAPP_SetDeliverTbl(pcWsService, g_hWebCenterDeliverTbl);
}

