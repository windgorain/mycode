/******************************************************************************
* Copyright (C), 2000-2006,  LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2017-4-27
* Description: 
* History:     
******************************************************************************/
#include "bs.h"
   
 
#include "utl/log_utl.h"

static HANDLE g_hSvpncIpTunLog = NULL;

BS_STATUS SVPNC_Log_Init()
{
    g_hSvpncIpTunLog = LOG_Open("svpnc_iptunnel.log");

    return BS_OK;
}

VOID SVPNC_Log(IN CHAR *pszLogFmt, ...)
{
    va_list args;

    va_start(args, pszLogFmt);
    LOG_OutStringByValist(g_hSvpncIpTunLog, pszLogFmt, args);
    va_end(args);
}

