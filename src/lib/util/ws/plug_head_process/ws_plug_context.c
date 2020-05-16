/******************************************************************************
* Copyright (C),    LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2014-7-21
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/data2hex_utl.h"
#include "utl/ws_utl.h"

#include "../ws_def.h"
#include "../ws_conn.h"
#include "../ws_trans.h"
#include "../ws_event.h"
#include "../ws_context.h"

#define WS_PLUG_CONTEXT_JUMP_URL_LEN 1023

static VOID ws_plugcontext_BuildJumpUrl(IN WS_TRANS_S *pstTrans, OUT CHAR *pcJumpUrl, IN UINT uiSize)
{
    CHAR *pcUrl;
    UINT uiLen;
    CHAR szTmp[WS_PLUG_CONTEXT_JUMP_URL_LEN + 1];

    pcUrl = HTTP_GetFullUri(pstTrans->hHttpHeadRequest);
    if (NULL == pcUrl)
    {
        pcUrl = "/";
    }

    uiLen = strlen(pcUrl);
    if ((uiLen * 2 > WS_PLUG_CONTEXT_JUMP_URL_LEN)
        || (uiLen * 2 + STR_LEN("/index.cgi?url=") >= uiSize))
    {
        pcUrl = "/";
        uiLen = 1;
    }

    DH_Data2HexString((UCHAR*)pcUrl, uiLen, szTmp);

    snprintf(pcJumpUrl, uiSize + 1, "/index.cgi?url=%s", szTmp);

    return;
}

WS_EV_RET_E ws_plugcontext_Redirect2Default(IN WS_TRANS_S *pstTrans)
{
    CHAR szCookieValue[WS_DOMAIN_MAX_LEN + 25];
    CHAR szTmp[WS_PLUG_CONTEXT_JUMP_URL_LEN + 1];

    snprintf(szCookieValue, sizeof(szCookieValue), "domain=; path=/");

    HTTP_SetHeadField(pstTrans->hHttpHeadReply, HTTP_FIELD_SET_COOKIE, szCookieValue);

    ws_plugcontext_BuildJumpUrl(pstTrans, szTmp, sizeof(szTmp));

    /* 回应重定向,重定向到Context */
    if (BS_OK != WS_Trans_Redirect(pstTrans, szTmp))
    {
        return WS_EV_RET_ERR;
    }

    return WS_EV_RET_STOP;
}

WS_EV_RET_E _WS_PlugContext_EventProcess(IN WS_TRANS_S *pstTrans, IN UINT uiEvent)
{
    HTTP_HEAD_PARSER hReqParser;
    CHAR *pcHost;
    CHAR *pcContext = NULL;
    WS_CONTEXT_HANDLE hContext;
    WS_VHOST_HANDLE hVHost;
    CHAR *pcSplit;
    UINT uiVHostLen;

    hReqParser = pstTrans->hHttpHeadRequest;

    pcHost = HTTP_GetHeadField(hReqParser, HTTP_FIELD_HOST);
    if (NULL == pcHost)
    {
        pcHost = "";
    }

    pcSplit = strchr(pcHost, ':');
    if (pcSplit != NULL)
    {
        uiVHostLen = pcSplit - pcHost;
    }
    else
    {
        uiVHostLen = strlen(pcHost);
    }

    hVHost = WS_VHost_Match(pstTrans->pstWs, pcHost, uiVHostLen);
    if (NULL == hVHost)
    {
        if (BS_OK != WS_Trans_Reply(pstTrans, HTTP_STATUS_NOT_FOUND, WS_TRANS_REPLY_FLAG_WITHOUT_BODY))
        {
            return WS_EV_RET_ERR;
        }

        return WS_EV_RET_STOP;
    }

    pstTrans->hVHost = hVHost;

    if (pstTrans->hCookie != NULL)
    {
        pcContext = MIME_GetKeyValue(pstTrans->hCookie, "domain");
    }

    hContext = _WS_Context_Match(hVHost, pcContext);
    if (hContext == NULL)
    {
        if ((pcContext != NULL) && (pcContext[0] == '\0'))
        {
            hContext = WS_Context_GetDftContext(hVHost);
        }
        else
        {
            return ws_plugcontext_Redirect2Default(pstTrans);
        }
    }

    pstTrans->hContext = hContext;

    return WS_EV_RET_CONTINUE;
}


