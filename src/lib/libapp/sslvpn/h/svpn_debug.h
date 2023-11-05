/******************************************************************************
* Copyright (C), 2000-2006,  Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2016-9-21
* Description: 
* History:     
******************************************************************************/

#ifndef __SVPN_DEBUG_H_
#define __SVPN_DEBUG_H_

#include "utl/dbg_utl.h"

#ifdef __cplusplus
    extern "C" {
#endif 

typedef enum
{
    SVPN_DBG_ID_IP_TUN = 0,
    SVPN_DBG_ID_TCP_RELAY,
    SVPN_DBG_ID_WEB_PROXY,

    SVPN_DBG_ID_MAX
}SVPN_DBG_ID_E;


#define SVPN_DBG_FLAG_IPTUN_PACKET   0x1
#define SVPN_DBG_FLAG_IPTUN_ERROR    0x2
#define SVPN_DBG_FLAG_IPTUN_HSK      0x4
#define SVPN_DBG_FLAG_IPTUN_EVENT    0x8
#define SVPN_DBG_FLAG_IPTUN_PROCESS  0x10

#define SVPN_DBG_FLAG_TR_PACKET 0x1
#define SVPN_DBG_FLAG_TR_EVENT  0x2
#define SVPN_DBG_FLAG_TR_PROCESS  0x4
#define SVPN_DBG_FLAG_TR_ERROR  0x8


#define SVPN_DBG_FLAG_WEBPROXY_PACKET 0x1
#define SVPN_DBG_FLAG_WEBPROXY_ERROR  0x2


extern DBG_UTL_CTRL_S g_stSvpnDbgCtrl;

#define SVPN_DBG_IS_SWITCH_ON(_DbgID, _DbgFlag) DBG_UTL_IS_SWITCH_ON(&g_stSvpnDbgCtrl, _DbgID, _DbgFlag)

#define SVPN_DBG_OUTPUT(_DbgID, _DbgFlag, _fmt, ...) DBG_UTL_OUTPUT(&g_stSvpnDbgCtrl, _DbgID, _DbgFlag, _fmt, ##__VA_ARGS__)


#ifdef __cplusplus
    }
#endif 

#endif 


