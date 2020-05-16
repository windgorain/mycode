/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-3-28
* Description: key func
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/sprintf_utl.h"
#include "utl/cff_utl.h"
#include "utl/txt_utl.h"
#include "utl/json_utl.h"
#include "comp/comp_kfapp.h"

#include "../inc/vnetc_conf.h"

#include "../inc/vnetc_kf.h"
#include "../inc/vnetc_node.h"
#include "../inc/vnetc_user_status.h"
#include "../inc/vnetc_addr_monitor.h"


#define _VNETC_CONF_FILE_NAME "vnetclient.ini"


static BS_STATUS vnetc_kf_SetServerAddress(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN KFAPP_PARAM_S *pstParam)
{
    CHAR *pcServerAddress;

    pcServerAddress = MIME_GetKeyValue(hMime, "ServerAddress");
    VNETC_SetServer(pcServerAddress);

    JSON_SetSuccess(pstParam->pstJson);

    return BS_OK;
}

static BS_STATUS vnetc_kf_SetUserName(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN KFAPP_PARAM_S *pstParam)
{
    CHAR *pcUserName;

    pcUserName = MIME_GetKeyValue(hMime, "UserName");
    VNETC_SetUserName(pcUserName);

    JSON_SetSuccess(pstParam->pstJson);

    return BS_OK;
}

static BS_STATUS vnetc_kf_SetPassword(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN KFAPP_PARAM_S *pstParam)
{
    CHAR *pcPassword;

    pcPassword = MIME_GetKeyValue(hMime, "Password");
    VNETC_SetUserPasswdSimple(pcPassword);

    JSON_SetSuccess(pstParam->pstJson);

    return BS_OK;
}

static BS_STATUS vnetc_kf_SavePassword(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN KFAPP_PARAM_S *pstParam)
{
    CFF_HANDLE hIniHandle;

    hIniHandle = CFF_INI_Open(_VNETC_CONF_FILE_NAME, CFF_FLAG_CREATE_IF_NOT_EXIST);
    if (NULL == hIniHandle)
    {
        JSON_SetFailed(pstParam->pstJson, "CanNotOpenFile");
        return BS_OK;
    }

    CFF_SetPropAsString(hIniHandle, "config", "password", VNETC_GetUserPasswd());
    CFF_Save(hIniHandle);
    CFF_Close(hIniHandle);

    JSON_SetSuccess(pstParam->pstJson);

    return BS_OK;
}

static BS_STATUS vnetc_kf_IsHavePassword(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN KFAPP_PARAM_S *pstParam)
{
    CFF_HANDLE hIniHandle;
    CHAR *pcPassword;
    CHAR *pcRet = "false";

    hIniHandle = CFF_INI_Open(_VNETC_CONF_FILE_NAME, CFF_FLAG_CREATE_IF_NOT_EXIST);
    if (NULL != hIniHandle)
    {
        if ((BS_OK == CFF_GetPropAsString(hIniHandle, "config", "password", &pcPassword))
            && (pcPassword != NULL) && (pcPassword[0] != '\0'))
        {
            pcRet = "true";
        }
    }
 
    CFF_Close(hIniHandle);

    JSON_AddString(pstParam->pstJson, "IsHavePassword", pcRet);

    JSON_SetSuccess(pstParam->pstJson);

    return BS_OK;
}

static BS_STATUS vnetc_kf_Start(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN KFAPP_PARAM_S *pstParam)
{
    if (BS_OK == VNETC_Start())
    {
        JSON_SetSuccess(pstParam->pstJson);
        return BS_OK;
    }

    JSON_SetFailed(pstParam->pstJson, "Can't start");

    return BS_ERR;
}

static BS_STATUS vnetc_kf_GetUserStatus(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN KFAPP_PARAM_S *pstParam)
{
    VNET_USER_STATUS_E eStatus;
    CHAR *pcRet = "Logining";
    
    eStatus = VNETC_User_GetStatus();
    if (VNET_USER_STATUS_ONLINE == eStatus)
    {
        JSON_AddString(pstParam->pstJson, "Cookie", VNETC_NODE_GetSelfCookieString());
        pcRet = "Online";
    }
    else if ((VNET_USER_STATUS_OFFLINE == eStatus) || (VNET_USER_STATUS_INIT == eStatus))
    {
        pcRet = "Offline";
    }

    JSON_AddString(pstParam->pstJson, "Status", pcRet);
    JSON_SetSuccess(pstParam->pstJson);

    return BS_OK;
}

static BS_STATUS vnetc_kf_GetAddressInfo(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN KFAPP_PARAM_S *pstParam)
{
    UINT uiIP;
    UINT uiMask;
    CHAR szTmp[32];

    uiIP = VNETC_AddrMonitor_GetIP();
    uiMask = VNETC_AddrMonitor_GetMask();

    BS_Snprintf(szTmp, sizeof(szTmp), "%pI4", &uiIP);
    JSON_AddString(pstParam->pstJson, "Address", szTmp);
    BS_Snprintf(szTmp, sizeof(szTmp), "%pI4", &uiMask);
    JSON_AddString(pstParam->pstJson, "Mask", szTmp);

    JSON_SetSuccess(pstParam->pstJson);

    return BS_OK;
}

static BS_STATUS vnetc_kf_SetDescription(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN KFAPP_PARAM_S *pstParam)
{
    CHAR *pcDesc;

    pcDesc = MIME_GetKeyValue(hMime, "Description");
    VNETC_SetDescription(pcDesc);

    JSON_SetSuccess(pstParam->pstJson);

    return BS_OK;
}

static BS_STATUS vnetc_kf_Logout(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN KFAPP_PARAM_S *pstParam)
{
    VNETC_Stop();

    JSON_SetSuccess(pstParam->pstJson);
    
    return BS_OK;
}

BS_STATUS VNETC_KF_Init()
{
    CFF_HANDLE hIniHandle;
    CHAR *pcPassword;

    COMP_KFAPP_Init();

    COMP_KFAPP_RegFunc("vnetc.SetServerAddress", vnetc_kf_SetServerAddress, NULL);
    COMP_KFAPP_RegFunc("vnetc.SetUserName", vnetc_kf_SetUserName, NULL);
    COMP_KFAPP_RegFunc("vnetc.SetPassword", vnetc_kf_SetPassword, NULL);
    COMP_KFAPP_RegFunc("vnetc.SavePassword", vnetc_kf_SavePassword, NULL);
    COMP_KFAPP_RegFunc("vnetc.IsHavePassword", vnetc_kf_IsHavePassword, NULL);
    COMP_KFAPP_RegFunc("vnetc.Start", vnetc_kf_Start, NULL);
    COMP_KFAPP_RegFunc("vnetc.GetUserStatus", vnetc_kf_GetUserStatus, NULL);
    COMP_KFAPP_RegFunc("vnetc.GetAddressInfo", vnetc_kf_GetAddressInfo, NULL);
    COMP_KFAPP_RegFunc("vnetc.SetDescription", vnetc_kf_SetDescription, NULL);
    COMP_KFAPP_RegFunc("vnetc.Logout", vnetc_kf_Logout, NULL);
    
    hIniHandle = CFF_INI_Open(_VNETC_CONF_FILE_NAME, CFF_FLAG_READ_ONLY);
    if (NULL != hIniHandle)
    {
        if (BS_OK == CFF_GetPropAsString(hIniHandle, "config", "password", &pcPassword))
        {
            VNETC_SetUserPasswdCipher(pcPassword);
        }
        CFF_Close(hIniHandle);
    }
    
    return BS_OK;
}


