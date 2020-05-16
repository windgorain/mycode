/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2015-12-12
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/ws_utl.h"
#include "comp/comp_kfapp.h"

#include "webcenter_inner.h"

BS_STATUS webcenter_kf_Run(IN WS_TRANS_HANDLE hWsTrans, IN KFAPP_PARAM_S *pstParam)
{
    MIME_HANDLE hMime;

    hMime = WS_Trans_GetBodyMime(hWsTrans);
    if (NULL == hMime)
    {
        hMime = WS_Trans_GetQueryMime(hWsTrans);
    }

    if (NULL == hMime)
    {
        return BS_OK;
    }

    return COMP_KFAPP_RunMime(hMime, pstParam);
}

static BS_STATUS webcenter_RecvBodyOK(IN WS_TRANS_HANDLE hWsTrans)
{
    HTTP_HEAD_PARSER hEncap;
    KFAPP_PARAM_S stKfappParam;
    BS_STATUS eRet;

    hEncap = WS_Trans_GetHttpEncap(hWsTrans);

    /* 权限检查 */
    if (! WebCenter_IsPermit(hWsTrans))
    {
        WS_Trans_Redirect(hWsTrans, "/index.htm");
        return BS_OK;
    }

    if (BS_OK != COMP_KFAPP_ParamInit(&stKfappParam))
    {
        return BS_NO_MEMORY;
    }

    eRet = webcenter_kf_Run(hWsTrans, &stKfappParam);
    if (eRet != BS_OK)
    {
        COMP_KFAPP_ParamFini(&stKfappParam);
        return BS_ERR;
    }

    if (NULL == COMP_KFAPP_BuildParamString(&stKfappParam))
    {
        COMP_KFAPP_ParamFini(&stKfappParam);
        return BS_NO_MEMORY;
    }

    HTTP_SetStatusCode(hEncap, HTTP_STATUS_OK);
    HTTP_SetContentLen(hEncap, stKfappParam.uiStringLen);
    HTTP_SetNoCache(hEncap);
    WS_Trans_SetHeadFieldFinish(hWsTrans);

    WS_Trans_AddReplyBodyByBuf(hWsTrans, stKfappParam.pcString, stKfappParam.uiStringLen);
    WS_Trans_ReplyBodyFinish(hWsTrans);

    COMP_KFAPP_ParamFini(&stKfappParam);

    return BS_OK;
}

WS_DELIVER_RET_E WebCenter_RequestIn(IN WS_TRANS_HANDLE hTrans, IN UINT uiEvent)
{
    BS_STATUS eRet = BS_OK;

    switch (uiEvent)
    {
        case WS_TRANS_EVENT_RECV_BODY_OK:
        {
            eRet = webcenter_RecvBodyOK(hTrans);
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

