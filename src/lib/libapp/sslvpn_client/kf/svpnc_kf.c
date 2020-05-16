/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-6-22
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/cff_utl.h"
#include "utl/txt_utl.h"
#include "utl/json_utl.h"
#include "comp/comp_kfapp.h"

#include "../h/svpnc_conf.h"
#include "../h/svpnc_func.h"

static BS_STATUS svpnc_kf_SetServerAddress(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN KFAPP_PARAM_S *pstParam)
{
    CHAR *pcServerAddress;

    pcServerAddress = MIME_GetKeyValue(hMime, "ServerAddress");
    SVPNC_SetServer(pcServerAddress);

    JSON_SetSuccess(pstParam->pstJson);

    return BS_OK;
}

static BS_STATUS svpnc_kf_SetUserName(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN KFAPP_PARAM_S *pstParam)
{
    CHAR *pcUserName;

    pcUserName = MIME_GetKeyValue(hMime, "UserName");
    SVPNC_SetUserName(pcUserName);

    JSON_SetSuccess(pstParam->pstJson);

    return BS_OK;
}

static BS_STATUS svpnc_kf_SetUserPassword(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN KFAPP_PARAM_S *pstParam)
{
    CHAR *pcPassword;

    pcPassword = MIME_GetKeyValue(hMime, "Password");
    SVPNC_SetUserPasswdSimple(pcPassword);

    JSON_SetSuccess(pstParam->pstJson);

    return BS_OK;
}

static BS_STATUS svpnc_kf_Login(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN KFAPP_PARAM_S *pstParam)
{
    SVPNC_Login();

    JSON_SetSuccess(pstParam->pstJson);

    return BS_OK;
}

static BS_STATUS svpnc_kf_SetCookie(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN KFAPP_PARAM_S *pstParam)
{
    CHAR *pcTmp;

    pcTmp = MIME_GetKeyValue(hMime, "Cookie");
    if (NULL == pcTmp)
    {
        pcTmp = "";
    }

    SVPNC_SetCookie(pcTmp);

    JSON_SetSuccess(pstParam->pstJson);

    return BS_OK;
}

static BS_STATUS svpnc_kf_TcpRelayStart(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN KFAPP_PARAM_S *pstParam)
{
    SVPNC_TcpRelay_Start();

    JSON_SetSuccess(pstParam->pstJson);

    return BS_OK;
}

static BS_STATUS svpnc_kf_IpTunnelStart(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN KFAPP_PARAM_S *pstParam)
{
    SVPNC_IpTunnel_Start();

    JSON_SetSuccess(pstParam->pstJson);

    return BS_OK;
}


BS_STATUS SVPNC_KF_Init()
{
    COMP_KFAPP_Init();

    COMP_KFAPP_RegFunc("svpnc.SetServerAddress", svpnc_kf_SetServerAddress, NULL);
    COMP_KFAPP_RegFunc("svpnc.SetUserName", svpnc_kf_SetUserName, NULL);
    COMP_KFAPP_RegFunc("svpnc.SetPassword", svpnc_kf_SetUserPassword, NULL);
    COMP_KFAPP_RegFunc("svpnc.Login", svpnc_kf_Login, NULL);
    COMP_KFAPP_RegFunc("svpnc.SetCookie", svpnc_kf_SetCookie, NULL);
    COMP_KFAPP_RegFunc("svpnc.TcpRelayStart", svpnc_kf_TcpRelayStart, NULL);
    COMP_KFAPP_RegFunc("svpnc.IpTunnelStart", svpnc_kf_IpTunnelStart, NULL);

	return BS_OK;
}

