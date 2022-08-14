/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2015-6-8
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/ws_utl.h"
#include "comp/comp_wsapp.h"

#include "../h/svpn_def.h"

#include "../h/svpn_context.h"
#include "../h/svpn_dweb.h"
#include "../h/svpn_tcprelay.h"
#include "../h/svpn_iptunnel.h"
#include "../h/svpn_web_proxy.h"
#include "../h/svpn_cfglock.h"


static WS_DELIVER_TBL_HANDLE g_hSvpnDeliverTbl = NULL;

BS_STATUS SVPN_Deliver_Init()
{
    g_hSvpnDeliverTbl = WS_Deliver_Create();
    if (NULL == g_hSvpnDeliverTbl)
    {
        return BS_NO_MEMORY;
    }

    WS_Deliver_Reg(g_hSvpnDeliverTbl, 0, WS_DELIVER_TYPE_METHOD,
        "TCP-Proxy", SVPN_TcpRelay_RequestIn, WS_DELIVER_FLAG_DELIVER_BODY);

    WS_Deliver_Reg(g_hSvpnDeliverTbl, 0, WS_DELIVER_TYPE_METHOD,
        "IP-Tunnel", SVPN_IpTunnel_RequestIn, WS_DELIVER_FLAG_DELIVER_BODY);

    WS_Deliver_Reg(g_hSvpnDeliverTbl, 100, WS_DELIVER_TYPE_FILE,
        "/_proxy/auth_digest.cgi", SVPN_WebProxy_AuthIn, WS_DELIVER_FLAG_PARSE_BODY_AS_MIME);

    WS_Deliver_Reg(g_hSvpnDeliverTbl, 200, WS_DELIVER_TYPE_PATH,
        "/_proxy/", NULL, 0);
    
    WS_Deliver_Reg(g_hSvpnDeliverTbl, 200, WS_DELIVER_TYPE_PATH,
        "/_base/_proxy/", NULL, 0);

    WS_Deliver_Reg(g_hSvpnDeliverTbl, 300, WS_DELIVER_TYPE_PATH,
        "/_proxy_/", SVPN_WebProxy_RequestIn, WS_DELIVER_FLAG_DELIVER_BODY);

    WS_Deliver_Reg(g_hSvpnDeliverTbl, 400, WS_DELIVER_TYPE_REFER_PATH,
        "/_proxy_/", SVPN_WebProxy_ReferIn, WS_DELIVER_FLAG_DELIVER_BODY);

    WS_Deliver_Reg(g_hSvpnDeliverTbl, 500, WS_DELIVER_TYPE_FILE,
        "/request.cgi", SVPN_DWeb_RequestIn, WS_DELIVER_FLAG_PARSE_BODY_AS_MIME);

    return BS_OK;
}

VOID SVPN_Deliver_BindContext(IN CHAR *pcWsService)
{
    WSAPP_SetDeliverTbl(pcWsService, g_hSvpnDeliverTbl);
}

