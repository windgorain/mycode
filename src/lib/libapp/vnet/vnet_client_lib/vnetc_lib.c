/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2012-6-3
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/cff_utl.h"
#include "utl/file_utl.h"
#include "utl/txt_utl.h"
#include "utl/ob_chain.h"

#include "../inc/vnet_conf.h"
#include "../inc/vnet_user_status.h"

#include "vnetc_lib.h"

#define VNETC_LIB_DLL_PATH "plug/vnetclient/vnetclient.dll"

#define VNETC_LIB_INI_FILENAME "vnetconfig.ini"

static VNETC_API_TBL_S * g_pstVnetcLibApiTbl;
static CFF_HANDLE g_hVnetcLibCffHandle;

static VOID vnetc_lib_Conf2Vnet()
{
    g_pstVnetcLibApiTbl->pfSetDomain(VNETC_LIB_GetDomain());
    g_pstVnetcLibApiTbl->pfSetUserName(VNETC_LIB_GetUserName());
    g_pstVnetcLibApiTbl->pfSetPassword(VNETC_LIB_GetPassword());
    g_pstVnetcLibApiTbl->pfSetDescription(VNETC_LIB_GetDescription());
}

BS_STATUS VNETC_LIB_Init()
{
    CHAR szFilePath[FILE_MAX_PATH_LEN + 1];

    TXT_Strlcpy(szFilePath, SYSINFO_GetExePath(), sizeof(szFilePath));
    TXT_Strlcat(szFilePath, "\\", sizeof(szFilePath));
    TXT_Strlcat(szFilePath, VNETC_LIB_DLL_PATH, sizeof(szFilePath));
    
    g_pstVnetcLibApiTbl = COMP_Ref(VNETC_COMP_NAME);
    if (NULL == g_pstVnetcLibApiTbl) {
        return BS_NOT_FOUND;
    }

    TXT_Strlcpy(szFilePath, SYSINFO_GetExePath(), sizeof(szFilePath));
    TXT_Strlcat(szFilePath, "\\", sizeof(szFilePath));
    TXT_Strlcat(szFilePath, VNETC_LIB_INI_FILENAME, sizeof(szFilePath));

    g_hVnetcLibCffHandle = CFF_INI_Open(szFilePath, CFF_FLAG_CREATE_IF_NOT_EXIST);
    if (NULL == g_hVnetcLibCffHandle)
    {
        return BS_CAN_NOT_OPEN;
    }

    vnetc_lib_Conf2Vnet();

    return BS_OK;
}

CHAR * VNETC_LIB_GetVersion()
{
    return g_pstVnetcLibApiTbl->pfGetVersion();
}

BS_STATUS VNETC_LIB_SetServerAddress(IN CHAR *pcServer)
{
    CFF_SetPropAsString(g_hVnetcLibCffHandle, "Server", "Server", pcServer);
    return g_pstVnetcLibApiTbl->pfSetServer(pcServer);
}

BS_STATUS VNETC_LIB_SetDomain(IN CHAR *pcDomain)
{
    CFF_SetPropAsString(g_hVnetcLibCffHandle, "Config", "Domain", pcDomain);

    return g_pstVnetcLibApiTbl->pfSetDomain(pcDomain);
}

CHAR * VNETC_LIB_GetDomain()
{
    CHAR *pcDomain = "";
    
    CFF_GetPropAsString(g_hVnetcLibCffHandle, "Config", "Domain", &pcDomain);

    return pcDomain;
}

BS_STATUS VNETC_LIB_SetUser(IN CHAR *pcUserName, IN CHAR *pcPassword)
{
    BS_STATUS eRet;
    UINT uiSave = 0;
    CHAR szEncPassWord[VNET_CONF_MAX_USER_ENC_PASSWD_LEN + 1];

    if ((NULL == pcUserName)  || (NULL == pcPassword))
    {
        return BS_NULL_PARA;
    }

    CFF_SetPropAsString(g_hVnetcLibCffHandle, "Config", "UserName", pcUserName);
    if (VNETC_LIB_GetIfSavePassword() && (pcPassword[0] != '\0'))
    {
        PW_Base64Encrypt(pcPassword, szEncPassWord, sizeof(szEncPassWord));
        CFF_SetPropAsString(g_hVnetcLibCffHandle, "Config", "PassWord", szEncPassWord);
    }
    else
    {
        CFF_SetPropAsString(g_hVnetcLibCffHandle, "Config", "PassWord", "");
    }

    eRet = g_pstVnetcLibApiTbl->pfSetUserName(pcUserName);
    if (BS_OK != eRet)
    {
        return eRet;
    }

    eRet = g_pstVnetcLibApiTbl->pfSetPassword(pcPassword);
    if (BS_OK != eRet)
    {
        return eRet;
    }

    return BS_OK;
}

CHAR * VNETC_LIB_GetUserName()
{
    CHAR *pcUserName = "";
    
    CFF_GetPropAsString(g_hVnetcLibCffHandle, "Config", "UserName", &pcUserName);

    return pcUserName;
}

CHAR * VNETC_LIB_GetPassword()
{
    CHAR *pcPassword = "";
    static CHAR szPassword[VNET_CONF_MAX_USER_PASSWD_LEN + 1] = "";
    
    if (BS_OK != CFF_GetPropAsString(g_hVnetcLibCffHandle, "Config", "PassWord", &pcPassword))
    {
		return "";
    }

    if ((pcPassword == NULL) || (pcPassword[0] == '\0'))
    {
        return "";
    }

    PW_Base64Decrypt(pcPassword, szPassword, sizeof(szPassword));

    return szPassword;
}

BS_STATUS VNETC_LIB_SetDescription(IN CHAR *pcDescription)
{
    BS_STATUS eRet;
    
    CFF_SetPropAsString(g_hVnetcLibCffHandle, "Config", "Description", pcDescription);

    eRet = g_pstVnetcLibApiTbl->pfSetDescription(pcDescription);

    return eRet;
}

CHAR * VNETC_LIB_GetDescription()
{
    CHAR *pcValue = "";
    
    CFF_GetPropAsString(g_hVnetcLibCffHandle, "Config", "Description", &pcValue);

    return pcValue;
}

BS_STATUS VNETC_LIB_Login()
{
    return g_pstVnetcLibApiTbl->pfStart();
}

VOID VNETC_LIB_Logout()
{
    g_pstVnetcLibApiTbl->pfStop();
}

VNET_USER_STATUS_E VNETC_LIB_GetUserStatus()
{
    return g_pstVnetcLibApiTbl->pfGetUserStatus();
}

CHAR * VNETC_LIB_GetUserStatusString()
{
    return g_pstVnetcLibApiTbl->pfGetUserStatusString();
}

VNET_USER_STATUS_E VNETC_LIB_GetUserReason()
{
    return g_pstVnetcLibApiTbl->pfGetUserReason();
}

CHAR * VNETC_LIB_GetUserReasonString()
{
    return g_pstVnetcLibApiTbl->pfGetUserReasonString();
}


BS_STATUS VNETC_LIB_SetIfSavePassword(IN BOOL_T bSave)
{
    return CFF_SetPropAsUint(g_hVnetcLibCffHandle, "Config", "SavePassWord", bSave);
}

BOOL_T VNETC_LIB_GetIfSavePassword()
{
    UINT uiSave = 0;
    CFF_GetPropAsUint(g_hVnetcLibCffHandle, "Config", "SavePassWord", &uiSave);

    return (BOOL_T)uiSave;
}

BS_STATUS VNETC_LIB_SetIfAutoLogin(IN BOOL_T bAuto)
{
    return CFF_SetPropAsUint(g_hVnetcLibCffHandle, "Config", "AutoLogin", bAuto);
}

BOOL_T VNETC_LIB_GetIfAutoLogin()
{
    UINT bAuto = 0;
    CFF_GetPropAsUint(g_hVnetcLibCffHandle, "Config", "AutoLogin", &bAuto);

    return (BOOL_T)bAuto;
}

BS_STATUS VNETC_LIB_SetIfSelfStart(IN BOOL_T bAuto)
{
    return CFF_SetPropAsUint(g_hVnetcLibCffHandle, "Config", "SelfStart", bAuto);
}

BOOL_T VNETC_LIB_GetIfSelfStart()
{
    UINT bAuto = 0;
    CFF_GetPropAsUint(g_hVnetcLibCffHandle, "Config", "SelfStart", &bAuto);

    return (BOOL_T)bAuto;
}

VOID VNETC_LIB_SaveConfig()
{
    CFF_Save(g_hVnetcLibCffHandle);
}

BS_STATUS VNETC_LIB_RegAddrMonitorNotify
(
    IN PF_VNETC_AddrMonitor_Notify_Func pfFunc,
    IN USER_HANDLE_S *pstUserHandle
)
{
    return g_pstVnetcLibApiTbl->pfRegAddrMonitorNotify(pfFunc, pstUserHandle);
}

UINT VNETC_LIB_GetAddrMonitorIP()
{
    return g_pstVnetcLibApiTbl->pfAddrMonitorGetIP();
}

UINT VNETC_LIB_GetAddrMonitorMask()
{
    return g_pstVnetcLibApiTbl->pfAddrMonitorGetMask();
}

