/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-6-30
* Description: 配置参数
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/txt_utl.h"
#include "utl/dns_utl.h"
#include "utl/ulm_utl.h"
#include "utl/ssl_utl.h"
#include "utl/http_lib.h"

#include "../h/svpnc_conf.h"


static CHAR g_szSvpncUserName[ULM_MAX_USER_NAME_LEN + 1] = "";
static CHAR g_szSvpncUserPassword[128] = "";
static CHAR g_szSvpncServer[DNS_MAX_DNS_NAME_SIZE] = "";
static CHAR g_szSvpncCookie[256] = "";
static USHORT g_usSvpncServerPort = 443;
static VOID *g_pSvpncSslCtx = NULL;
static UINT g_uiSvpncServerIP = 0; 
static SVPNC_CONN_TYPE_E g_enSpncConnType = SVPNC_CONN_TYPE_SSL;

BS_STATUS SVPNC_SetServer(IN CHAR *pcServer)
{
    TXT_Strlcpy(g_szSvpncServer, pcServer, sizeof(g_szSvpncServer));

    g_uiSvpncServerIP = Socket_NameToIpHost(g_szSvpncServer);

    return BS_OK;
}

BS_STATUS SVPNC_SetPort(IN CHAR *pcPort)
{
    UINT uiPort = 0;

    TXT_Atoui(pcPort, &uiPort);

    if (uiPort == 0)
    {
        return BS_ERR;
    }

    g_usSvpncServerPort = uiPort;

    return BS_OK;
}

BS_STATUS SVPNC_SetConnType(IN CHAR *pcType)
{
    if (strcmp(pcType, "tcp") == 0)
    {
        g_enSpncConnType = SVPNC_CONN_TYPE_TCP;
    }
    else
    {
        g_enSpncConnType = SVPNC_CONN_TYPE_SSL;
    }

    return BS_OK;
}

SVPNC_CONN_TYPE_E SVPNC_GetConnType()
{
    return g_enSpncConnType;
}

CHAR * SVPNC_GetServer()
{
    return g_szSvpncServer;
}


UINT SVPNC_GetServerIP()
{
    return g_uiSvpncServerIP;
}

USHORT SVPNC_GetServerPort()
{
    return g_usSvpncServerPort;
}

BS_STATUS SVPNC_SetUserName(IN CHAR *pcUserName)
{
    TXT_Strlcpy(g_szSvpncUserName, pcUserName, sizeof(g_szSvpncUserName));

    return BS_OK;
}

CHAR * SVPNC_GetUserName()
{
    return g_szSvpncUserName;
}

BS_STATUS SVPNC_SetUserPasswdSimple(IN CHAR *pcPassword)
{
    TXT_Strlcpy(g_szSvpncUserPassword, pcPassword, sizeof(g_szSvpncUserPassword));
    
    return BS_OK;
}

CHAR * SVPNC_GetUserPasswd()
{
    return g_szSvpncUserPassword;
}

VOID SVPNC_SetCookie(IN CHAR *pcCookie)
{
    TXT_Strlcpy(g_szSvpncCookie, pcCookie, sizeof(g_szSvpncCookie));
}

CHAR * SVPNC_GetCookie()
{
    return g_szSvpncCookie;
}

VOID * SVPNC_GetSslCtx()
{
    return g_pSvpncSslCtx;
}

BS_STATUS SVPNC_SslCtx_Init()
{
    BS_STATUS eRet = BS_OK;
    VOID *pstSslCtx;

    pstSslCtx = SSL_UTL_Ctx_Create(0, 0);
    if (NULL == pstSslCtx) {
        return BS_ERR;
    }

    g_pSvpncSslCtx = pstSslCtx;

    return BS_OK;
}

