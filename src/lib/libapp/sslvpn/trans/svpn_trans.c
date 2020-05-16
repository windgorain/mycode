/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2015-6-8
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/ws_utl.h"

#include "../h/svpn_context.h"
#include "../h/svpn_ulm.h"
#include "../h/svpn_trans.h"

SVPN_TRANS_S * SVPN_Trans_Create(IN WS_TRANS_HANDLE hWsTrans)
{
    SVPN_TRANS_S *pstTrans;
    MIME_HANDLE hMime;
    CHAR *pcOnlineUserCookie;
    SVPN_CONTEXT_HANDLE hSvpnContext;

    hSvpnContext = SVPN_Context_GetContextByWsTrans(hWsTrans);
    if (NULL == hSvpnContext)
    {
        return NULL;
    }

    pstTrans = WS_TransMemPool_ZAlloc(hWsTrans, sizeof(SVPN_TRANS_S));
    if (NULL == pstTrans)
    {
        return NULL;
    }

    hMime = WS_Trans_GetCookieMime(hWsTrans);
    if (NULL != hMime)
    {
        pcOnlineUserCookie = MIME_GetKeyValue(hMime, "svpnuid");
        if (NULL != pcOnlineUserCookie)
        {
            pstTrans->uiOnlineUserID = SVPN_ULM_GetUserIDByCookie(hSvpnContext, pcOnlineUserCookie);
        }
    }

    pstTrans->hWsTrans = hWsTrans;
    pstTrans->hSvpnContext = hSvpnContext;

    WS_Trans_SetUserHandle(hWsTrans, "SvpnTrans", pstTrans);

    return pstTrans;
}

VOID SVPN_Trans_Destory(IN WS_TRANS_HANDLE hWsTrans)
{
    SVPN_TRANS_S *pstTrans;

    pstTrans = WS_Trans_GetUserHandle(hWsTrans, "SvpnTrans");
    if (NULL != pstTrans)
    {
        WS_TransMemPool_Free(hWsTrans, pstTrans);
    }
}

