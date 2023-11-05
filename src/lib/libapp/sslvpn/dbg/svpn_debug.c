/******************************************************************************
* Copyright (C), 2000-2006,  LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2016-9-21
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "../h/svpn_debug.h"

static UINT g_auiSvpnDbgFlag[SVPN_DBG_ID_MAX] = {0};

static DBG_UTL_DEF_S g_astSvpnDbgDef[] = 
{
    {"ip-tunnel", "packet", SVPN_DBG_ID_IP_TUN, SVPN_DBG_FLAG_IPTUN_PACKET},
    {"ip-tunnel", "error", SVPN_DBG_ID_IP_TUN, SVPN_DBG_FLAG_IPTUN_ERROR},
    {"ip-tunnel", "handshake", SVPN_DBG_ID_IP_TUN, SVPN_DBG_FLAG_IPTUN_HSK},
    {"ip-tunnel", "event", SVPN_DBG_ID_IP_TUN, SVPN_DBG_FLAG_IPTUN_EVENT},
    {"ip-tunnel", "process", SVPN_DBG_ID_IP_TUN, SVPN_DBG_FLAG_IPTUN_PROCESS},

    {"tcp-relay", "packet", SVPN_DBG_ID_TCP_RELAY, SVPN_DBG_FLAG_TR_PACKET},
    {"tcp-relay", "event", SVPN_DBG_ID_TCP_RELAY, SVPN_DBG_FLAG_TR_EVENT},
    {"tcp-relay", "process", SVPN_DBG_ID_TCP_RELAY, SVPN_DBG_FLAG_TR_PROCESS},
    {"tcp-relay", "error", SVPN_DBG_ID_TCP_RELAY, SVPN_DBG_FLAG_TR_ERROR},

    {"web-proxy", "packet", SVPN_DBG_ID_WEB_PROXY, SVPN_DBG_FLAG_WEBPROXY_PACKET},
    {"web-proxy", "error", SVPN_DBG_ID_WEB_PROXY, SVPN_DBG_FLAG_WEBPROXY_ERROR},

    {0}
};

DBG_UTL_CTRL_S g_stSvpnDbgCtrl
    = DBG_INIT_VALUE("SVPN",g_auiSvpnDbgFlag, g_astSvpnDbgDef, SVPN_DBG_ID_MAX);


PLUG_API VOID SVPN_Debug_Cmd(IN UINT ulArgc, IN CHAR **argv)
{
    DBG_UTL_DebugCmd(&g_stSvpnDbgCtrl, argv[1], argv[2]);
}


PLUG_API VOID SVPN_NoDebug_Cmd(IN UINT ulArgc, IN CHAR **argv)
{
    DBG_UTL_NoDebugCmd(&g_stSvpnDbgCtrl, argv[2], argv[3]);
}


