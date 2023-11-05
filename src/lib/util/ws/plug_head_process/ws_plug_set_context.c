/******************************************************************************
* Copyright (C),    LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2014-7-22
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

static VOID ws_plugsetcontext_GetJump2Url(IN WS_TRANS_S *pstTrans, OUT CHAR *pcJump2Url, IN UINT uiSize)
{
    CHAR *pcUrl;
    CHAR szTmp[1024];
    UINT uiLen;

    pcUrl = MIME_GetKeyValue(pstTrans->hQuery, "url");
    if (NULL == pcUrl)
    {
        snprintf(pcJump2Url, uiSize, "/");
        return;
    }

    uiLen = strlen(pcUrl);

    if (uiLen > (uiSize - 1) * 2)
    {
        snprintf(pcJump2Url, uiSize, "/");
        return;
    }

    if (BS_OK != DH_HexString2Data(pcUrl, szTmp))
    {
        snprintf(pcJump2Url, uiSize, "/");
        return;
    }

    szTmp[uiLen/2] = '\0';

    snprintf(pcJump2Url, uiSize, "%s", szTmp);
    return;
}

WS_EV_RET_E ws_plugsetcontext_Redirect(IN WS_TRANS_S *pstTrans, IN CHAR *pcContext)
{
    CHAR szCookieValue[WS_DOMAIN_MAX_LEN + 25];
    CHAR szTmp[1024];

    snprintf(szCookieValue, sizeof(szCookieValue), "domain=%s; path=/", pcContext);

    HTTP_SetHeadField(pstTrans->hHttpHeadReply, HTTP_FIELD_SET_COOKIE, szCookieValue);

    ws_plugsetcontext_GetJump2Url(pstTrans, szTmp, sizeof(szTmp));

    
    if (BS_OK != WS_Trans_Redirect(pstTrans, szTmp))
    {
        return WS_EV_RET_ERR;
    }

    return WS_EV_RET_STOP;
}

CHAR * ws_plugsetcontext_GetSetContext(IN WS_TRANS_S *pstTrans)
{
    CHAR *pcRequestFile;

    pcRequestFile = pstTrans->pcRequestFile;
    if (NULL == pcRequestFile)
    {
        return NULL;
    }

    if (strcmp(pcRequestFile, "/@") == 0)
    {
        return "";
    }

    if (strcmp(pcRequestFile, "/") != 0)
    {
        return NULL;
    }

    if (pstTrans->hQuery == NULL)
    {
        return NULL;
    }

    return MIME_GetKeyValue(pstTrans->hQuery, "domain");
}

WS_EV_RET_E _WS_PlugSetContext_EventProcess(IN WS_TRANS_S *pstTrans, IN UINT uiEvent)
{
    CHAR *pcSetContext;

    pcSetContext = ws_plugsetcontext_GetSetContext(pstTrans);
    if (NULL == pcSetContext)
    {
        return WS_EV_RET_CONTINUE;
    }

    return ws_plugsetcontext_Redirect(pstTrans, pcSetContext);
}

