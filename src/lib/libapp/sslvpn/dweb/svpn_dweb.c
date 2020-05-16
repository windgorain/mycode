/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2014-8-29
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/file_utl.h"
#include "utl/ws_utl.h"

#include "../h/svpn_def.h"

#include "../h/svpn_context.h"
#include "../h/svpn_local_user.h"
#include "../h/svpn_dweb.h"
#include "../h/svpn_ulm.h"
#include "../h/svpn_ctxdata.h"
#include "../h/svpn_mf.h"

static SVPN_DWEB_S * svpn_dweb_InitTrans(IN WS_TRANS_HANDLE hWsTrans)
{
    SVPN_DWEB_S *pstDWeb;
    MIME_HANDLE hMime;
    CHAR *pcOnlineUserCookie;
    SVPN_CONTEXT_HANDLE hSvpnContext;

    hSvpnContext = SVPN_Context_GetContextByWsTrans(hWsTrans);
    if (NULL == hSvpnContext)
    {
        return NULL;
    }

    pstDWeb = MEM_ZMalloc(sizeof(SVPN_DWEB_S));
    if (NULL == pstDWeb)
    {
        return NULL;
    }

    pstDWeb->pstJson = cJSON_CreateObject();
    if (NULL == pstDWeb->pstJson)
    {
        MEM_Free(pstDWeb);
        return NULL;
    }

    hMime = WS_Trans_GetCookieMime(hWsTrans);
    if (NULL != hMime)
    {
        pcOnlineUserCookie = MIME_GetKeyValue(hMime, "svpnuid");
        if (NULL != pcOnlineUserCookie)
        {
            pstDWeb->uiOnlineUserID = SVPN_ULM_GetUserIDByCookie(hSvpnContext, pcOnlineUserCookie);
        }
    }

    pstDWeb->hWsTrans = hWsTrans;
    pstDWeb->hSvpnContext = hSvpnContext;

    WS_Trans_SetUserHandle(hWsTrans, "SvpnDWeb", pstDWeb);

    return pstDWeb;
}

static BS_STATUS svpn_dweb_RecvBodyOK(IN WS_TRANS_HANDLE hWsTrans)
{
    HTTP_HEAD_PARSER hEncap;
    SVPN_DWEB_S *pstDweb;
    CHAR *pcJson;
    UINT uiBodyLen = 0;

    pstDweb = svpn_dweb_InitTrans(hWsTrans);
    if (pstDweb == NULL)
    {
        return BS_ERR;
    }

    SVPN_MF_Run(pstDweb);

    if (NULL != pstDweb->pstJson)
    {
        pcJson = cJSON_Print(pstDweb->pstJson);
        if (NULL != pcJson)
        {
            uiBodyLen = strlen(pcJson);
            WS_Trans_AddReplyBodyByBuf(hWsTrans, pcJson, uiBodyLen);
            WS_Trans_ReplyBodyFinish(hWsTrans);
            free(pcJson);
        }
    }

    hEncap = WS_Trans_GetHttpEncap(hWsTrans);

    HTTP_SetStatusCode(hEncap, HTTP_STATUS_OK);
    HTTP_SetContentLen(hEncap, uiBodyLen);
    HTTP_SetNoCache(hEncap);

    WS_Trans_SetHeadFieldFinish(hWsTrans);

    return BS_OK;
}

static VOID svpn_dweb_Destory(IN WS_TRANS_HANDLE hTrans)
{
    SVPN_DWEB_S *pstDweb;

    pstDweb = WS_Trans_GetUserHandle(hTrans, "SvpnDWeb");
    if (NULL == pstDweb)
    {
        return;
    }

    WS_Trans_SetUserHandle(hTrans, "SvpnDWeb", NULL);

    if (NULL != pstDweb->pstJson)
    {
        cJSON_Delete(pstDweb->pstJson);
    }

    MEM_Free(pstDweb);

    return;
}

BS_STATUS SVPN_DWEB_Init()
{
    return BS_OK;
}

WS_DELIVER_RET_E SVPN_DWeb_RequestIn(IN WS_TRANS_HANDLE hTrans, IN UINT uiEvent)
{
    BS_STATUS eRet = BS_OK;

    switch (uiEvent)
    {
        case WS_TRANS_EVENT_RECV_BODY_OK:
        {
            eRet = svpn_dweb_RecvBodyOK(hTrans);
            break;
        }

        case WS_TRANS_EVENT_DESTORY:
        {
            svpn_dweb_Destory(hTrans);
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

