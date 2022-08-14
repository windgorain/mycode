/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2015-12-5
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "comp/comp_wsapp.h"

#include "webproxyapp_main.h"

static WS_DELIVER_TBL_HANDLE g_hWebProxyAppDeliverTbl = NULL;

BS_STATUS WebProxyApp_Deliver_Init()
{
    g_hWebProxyAppDeliverTbl = WS_Deliver_Create();
    if (NULL == g_hWebProxyAppDeliverTbl)
    {
        return BS_NO_MEMORY;
    }

    WS_Deliver_Reg(g_hWebProxyAppDeliverTbl, 100, WS_DELIVER_TYPE_FILE,
        "/_proxy/auth_digest.cgi", WebProxyApp_Main_AuthIn, WS_DELIVER_FLAG_PARSE_BODY_AS_MIME);

    WS_Deliver_Reg(g_hWebProxyAppDeliverTbl, 200, WS_DELIVER_TYPE_PATH,
        "/_proxy/", NULL, 0);

    WS_Deliver_Reg(g_hWebProxyAppDeliverTbl, 300, WS_DELIVER_TYPE_PATH,
        "/_proxy_/", WebProxyApp_Main_RequestIn, WS_DELIVER_FLAG_DELIVER_BODY);

    WS_Deliver_Reg(g_hWebProxyAppDeliverTbl, 400, WS_DELIVER_TYPE_REFER_PATH,
        "/_proxy_/", WebProxyApp_Main_ReferIn, WS_DELIVER_FLAG_DELIVER_BODY);

    return BS_OK;
}

VOID WebProxyApp_Deliver_BindService(IN CHAR *pcWsService)
{
    WSAPP_SetDeliverTbl(pcWsService, g_hWebProxyAppDeliverTbl);
}


