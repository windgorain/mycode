/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2015-6-1
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/txt_utl.h"
#include "utl/lstr_utl.h"

#include "../h/svpn_def.h"
#include "../h/svpn_context.h"
#include "../h/svpn_local_user.h"
#include "../h/svpn_role.h"
#include "../h/svpn_dweb.h"
#include "../h/svpn_ctxdata.h"
#include "../h/svpn_mf.h"
#include "../h/svpn_acl.h"
#include "../h/svpn_res.h"
#include "../h/svpn_ulm.h"


static KF_HANDLE g_hSvpnMf = NULL;

static VOID svpn_mf_map_CheckOnline(IN MIME_HANDLE hMime, IN SVPN_DWEB_S *pstDweb)
{
    if (pstDweb->uiOnlineUserID == 0)
    {
        cJSON_AddStringToObject(pstDweb->pstJson, "online", "false");
    }
    else
    {
        cJSON_AddStringToObject(pstDweb->pstJson, "online", "true");
    }

    svpn_mf_SetSuccess(pstDweb);

    return;
}

static VOID svpn_mf_map_UserLogin(IN MIME_HANDLE hMime,  IN SVPN_DWEB_S *pstDweb)
{
    CHAR *pcUserName;
    CHAR *pcPassWord;
    UINT uiUserID;
    CHAR szUserCookie[ULM_USER_COOKIE_LEN + 1];
    CHAR szCookie[256];

    pcUserName = MIME_GetKeyValue(hMime, "UserName");
    pcPassWord = MIME_GetKeyValue(hMime, "Password");
    if ((NULL == pcUserName) || (NULL == pcPassWord) || (pcUserName[0] == '\0') || (pcPassWord[0] == '\0'))
    {
        svpn_mf_SetFailed(pstDweb, "Username or password error");
        return;
    }

    if (TRUE != SVPN_LocalUser_Check(pstDweb->hSvpnContext, pcUserName, pcPassWord))
    {
        svpn_mf_SetFailed(pstDweb, "Username or password error");
        return;
    }

    uiUserID = SVPN_ULM_AddUser(pstDweb->hSvpnContext, pcUserName, SVPN_USER_TYPE_USER);
    if (0 == uiUserID)
    {
        svpn_mf_SetFailed(pstDweb, "Can't add user");
        return;
    }

    SVPN_ULM_GetUserCookie(pstDweb->hSvpnContext, uiUserID, szUserCookie);
    snprintf(szCookie, sizeof(szCookie), "%s=%s; path=/", SVPN_ONLINE_COOKIE_KEY, szUserCookie);
    HTTP_SetHeadField(WS_Trans_GetHttpEncap(pstDweb->hWsTrans), HTTP_FIELD_SET_COOKIE, szCookie);

    svpn_mf_SetSuccess(pstDweb);

    return;
}

static VOID svpn_mf_map_ContextName(IN MIME_HANDLE hMime, IN SVPN_DWEB_S *pstDweb)
{
    CHAR *pcContextName;

    pcContextName = SVPN_Context_GetName(pstDweb->hSvpnContext);
    if (NULL != pcContextName)
    {
        cJSON_AddStringToObject(pstDweb->pstJson, "name", pcContextName);
        svpn_mf_SetSuccess(pstDweb);
    }
    else
    {
        svpn_mf_SetFailed(pstDweb, "Can't get context name.");
    }

    return;
}

static VOID svpn_mf_map_AdminLogin(IN MIME_HANDLE hMime, IN SVPN_DWEB_S *pstDweb)
{
    CHAR *pcUserName;
    CHAR *pcPassWord;
    UINT uiUserID;
    CHAR szUserCookie[ULM_USER_COOKIE_LEN + 1];
    CHAR szCookie[256];

    pcUserName = MIME_GetKeyValue(hMime, "UserName");
    pcPassWord = MIME_GetKeyValue(hMime, "Password");
    if ((NULL == pcUserName) || (NULL == pcPassWord) || (pcUserName[0] == '\0') || (pcPassWord[0] == '\0'))
    {
        svpn_mf_SetFailed(pstDweb, "Username or password error");
        return;
    }

    if (TRUE != SVPN_LocalAdmin_Check(pstDweb->hSvpnContext, pcUserName, pcPassWord))
    {
        svpn_mf_SetFailed(pstDweb, "Username or password error");
        return;
    }

    uiUserID = SVPN_ULM_AddUser(pstDweb->hSvpnContext, pcUserName, SVPN_USER_TYPE_ADMIN);
    if (0 == uiUserID)
    {
        svpn_mf_SetFailed(pstDweb, "Can't add user");
        return;
    }

    SVPN_ULM_GetUserCookie(pstDweb->hSvpnContext, uiUserID, szUserCookie);
    snprintf(szCookie, sizeof(szCookie), "%s=%s; path=/", SVPN_ONLINE_COOKIE_KEY, szUserCookie);
    HTTP_SetHeadField(WS_Trans_GetHttpEncap(pstDweb->hWsTrans), HTTP_FIELD_SET_COOKIE, szCookie);

    svpn_mf_SetSuccess(pstDweb);

    return;
}

static VOID svpn_mf_map_AdminUserChangePwd(IN MIME_HANDLE hMime, IN SVPN_DWEB_S *pstDweb)
{
    CHAR *pcOldPwd;
    CHAR *pcNewPwd;

    pcOldPwd = MIME_GetKeyValue(hMime, "OldPassword");
    pcNewPwd = MIME_GetKeyValue(hMime, "Password");
    if ((NULL == pcOldPwd) || (NULL == pcNewPwd) || (pcOldPwd[0] == '\0') || (pcNewPwd[0] == '\0'))
    {
        svpn_mf_SetFailed(pstDweb, "Password is empty");
        return;
    }

    if (TRUE != SVPN_LocalAdmin_Check(pstDweb->hSvpnContext, "admin", pcOldPwd))
    {
        svpn_mf_SetFailed(pstDweb, "Password error");
        return;
    }

    if (BS_OK != SVPN_LocalAdmin_AddUser(pstDweb->hSvpnContext, "admin", pcNewPwd))
    {
        svpn_mf_SetFailed(pstDweb, "Set user password failed");
        return;
    }

    svpn_mf_SetSuccess(pstDweb);

    return;
}

static SVPN_MF_MAP_S g_astSvpnMfMap[] =
{
    {SVPN_USER_TYPE_ALL,   "CheckOnline",          svpn_mf_map_CheckOnline}, /* 检查是否在线 */
    {SVPN_USER_TYPE_ALL,   "Context.Name",         svpn_mf_map_ContextName},  /* 获取Context名字 */

    {SVPN_USER_TYPE_ALL,   "Admin.Login",          svpn_mf_map_AdminLogin},  /* 管理员登陆 */
    {SVPN_USER_TYPE_ADMIN, "Admin.Password.Change",svpn_mf_map_AdminUserChangePwd}, /* 更改管理员密码 */

    {SVPN_USER_TYPE_ALL,   "User.Login",           svpn_mf_map_UserLogin},  /* 用户登录 */
};

static BS_STATUS svpn_mf_CallBack(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN HANDLE hRunHandle)
{
    SVPN_MF_MAP_S *pstMap = hUserHandle;
    SVPN_DWEB_S *pstDweb = hRunHandle;
    UINT uiUserType;

    uiUserType = SVPN_ULM_GetUserType(pstDweb->hSvpnContext, pstDweb->uiOnlineUserID);

    if ((pstMap->uiRightFlag & uiUserType) == 0)
    {
        svpn_mf_SetFailed(pstDweb, "Insuffcinet privileges");
        return BS_NO_PERMIT;
    }

    pstMap->pfFunc(hMime, pstDweb);

    return BS_OK;
}

BS_STATUS SVPN_MF_Init()
{
    UINT i;

    g_hSvpnMf = KF_Create("_do");
    if (NULL == g_hSvpnMf)
    {
        return BS_NO_MEMORY;
    }

    for (i=0; i<sizeof(g_astSvpnMfMap)/sizeof(SVPN_MF_MAP_S); i++)
    {
        KF_AddFunc(g_hSvpnMf, g_astSvpnMfMap[i].pcKey, svpn_mf_CallBack, &g_astSvpnMfMap[i]);
    }

    SVPN_LocalUser_Init();
    SVPN_ACL_Init();
    SVPN_WebRes_Init();
    SVPN_TcpRes_Init();
    SVPN_IPRes_Init();
    SVPN_Role_Init();
    SVPN_IPPoolMf_Init();
    SVPN_InnerDNS_Init();

    return BS_OK;
}

BS_STATUS SVPN_MF_Reg(IN SVPN_MF_MAP_S *pstMap, IN UINT uiMapCount)
{
    UINT i;

    for (i=0; i<uiMapCount; i++)
    {
        KF_AddFunc(g_hSvpnMf, pstMap[i].pcKey, svpn_mf_CallBack, &pstMap[i]);
    }

    return BS_OK;
}

BS_STATUS SVPN_MF_Run(IN SVPN_DWEB_S *pstDweb)
{
    MIME_HANDLE hMime;

    hMime = WS_Trans_GetBodyMime(pstDweb->hWsTrans);
    if (NULL == hMime)
    {
        hMime = WS_Trans_GetQueryMime(pstDweb->hWsTrans);
    }

    if (NULL == hMime)
    {
        return BS_OK;
    }

    return KF_RunMime(g_hSvpnMf, hMime, pstDweb);
}

VOID SVPN_MF_CommonIsExist
(
    IN MIME_HANDLE hMime,
    IN SVPN_DWEB_S *pstDweb,
    IN SVPN_CTXDATA_E enDataIndex
)
{
    CHAR *pcName;

    pcName = MIME_GetKeyValue(hMime, "Name");
    if ((NULL == pcName) || (pcName[0] == '\0'))
    {
        svpn_mf_SetFailed(pstDweb, "Empty name");
        return;
    }

    if (TRUE != SVPN_CtxData_IsObjectExist(pstDweb->hSvpnContext, enDataIndex, pcName))
    {
        svpn_mf_SetSuccess(pstDweb);
        cJSON_AddStringToObject(pstDweb->pstJson, "exist", "False");
        return;
    }

    cJSON_AddStringToObject(pstDweb->pstJson, "exist", "True");
    svpn_mf_SetSuccess(pstDweb);
}

BS_STATUS SVPN_MF_CommonAdd
(
    IN MIME_HANDLE hMime,
    IN SVPN_DWEB_S *pstDweb,
    IN SVPN_CTXDATA_E enDataIndex,
    IN CHAR **apcPropertys,
    IN UINT uiPropertyCount
)
{
    CHAR *pcName;
    CHAR *pcPropertyValue;
    BS_STATUS eRet;
    UINT i;

    pcName = MIME_GetKeyValue(hMime, "Name");
    if ((NULL == pcName) || (pcName[0] == '\0'))
    {
        svpn_mf_SetFailed(pstDweb, "Empty name");
        return BS_ERR;
    }

    eRet = SVPN_CtxData_AddObject(pstDweb->hSvpnContext, enDataIndex, pcName);
    if (eRet == BS_ALREADY_EXIST)
    {
        svpn_mf_SetFailed(pstDweb, "Already exist");
        return BS_ERR;
    }
    
    if (eRet != BS_OK)
    {
        svpn_mf_SetFailed(pstDweb, "Can't add.");
        return BS_ERR;
    }

    for (i=0; i<uiPropertyCount; i++)
    {
        pcPropertyValue = MIME_GetKeyValue(hMime, apcPropertys[i]);
        if (pcPropertyValue == NULL)
        {
            pcPropertyValue = "";
        }

        SVPN_CtxData_SetProp(pstDweb->hSvpnContext, enDataIndex, pcName, apcPropertys[i], pcPropertyValue);
    }

    svpn_mf_SetSuccess(pstDweb);

    return BS_OK;
}

VOID SVPN_MF_CommonModify
(
    IN MIME_HANDLE hMime,
    IN SVPN_DWEB_S *pstDweb,
    IN SVPN_CTXDATA_E enDataIndex,
    IN CHAR **apcPropertys,
    IN UINT uiPropertyCount
)
{
    CHAR *pcName;
    CHAR *pcPropertyValue;
    UINT i;

    pcName = MIME_GetKeyValue(hMime, "Name");
    if ((NULL == pcName) || (pcName[0] == '\0'))
    {
        svpn_mf_SetFailed(pstDweb, "Empty name");
        return;
    }

    if (TRUE != SVPN_CtxData_IsObjectExist(pstDweb->hSvpnContext, enDataIndex, pcName))
    {
        svpn_mf_SetFailed(pstDweb, "Not exist");
        return;
    }

    for (i=0; i<uiPropertyCount; i++)
    {
        pcPropertyValue = MIME_GetKeyValue(hMime, apcPropertys[i]);
        if (pcPropertyValue == NULL)
        {
            pcPropertyValue = "";
        }

        SVPN_CtxData_SetProp(pstDweb->hSvpnContext, enDataIndex, pcName, apcPropertys[i], pcPropertyValue);
    }

    svpn_mf_SetSuccess(pstDweb);
}

VOID SVPN_MF_CommonGet
(
    IN MIME_HANDLE hMime,
    IN SVPN_DWEB_S *pstDweb,
    IN SVPN_CTXDATA_E enDataIndex,
    IN CHAR **apcPropertys,
    IN UINT uiPropertyCount
)
{
    CHAR *pcName;
    HSTRING hsString;
    CHAR *pcString;
    UINT i;

    pcName = MIME_GetKeyValue(hMime, "Name");
    if ((NULL == pcName) || (pcName[0] == '\0'))
    {
        svpn_mf_SetFailed(pstDweb, "Empty name");
        return;
    }

    if (TRUE != SVPN_CtxData_IsObjectExist(pstDweb->hSvpnContext, enDataIndex, pcName))
    {
        svpn_mf_SetFailed(pstDweb, "Not exist");
        return;
    }

    cJSON_AddStringToObject(pstDweb->pstJson, "Name", pcName);

    for (i=0; i<uiPropertyCount; i++)
    {
        pcString = "";

        hsString = SVPN_CtxData_GetPropAsHString(pstDweb->hSvpnContext, enDataIndex, pcName, apcPropertys[i]);
        if (hsString != NULL)
        {
            pcString = STRING_GetBuf(hsString);
        }

        cJSON_AddStringToObject(pstDweb->pstJson, apcPropertys[i], pcString);

        if (NULL != hsString)
        {
            STRING_Delete(hsString);
        }
    }

    svpn_mf_SetSuccess(pstDweb);

    return;
}

VOID SVPN_MF_CommonDelete
(
    IN MIME_HANDLE hMime,
    IN SVPN_DWEB_S *pstDweb,
    IN SVPN_CTXDATA_E enDataIndex,
    IN PF_svpn_mf_CommonDeleteNotify pfDeleteNotify
)
{
    CHAR *pcNames;
    LSTR_S stName;
    CHAR szName[256];

    pcNames = MIME_GetKeyValue(hMime, "Delete");
    if (NULL == pcNames)
    {
        return;
    }

    LSTR_SCAN_ELEMENT_BEGIN(pcNames, strlen(pcNames), ',', &stName)
    {
        if ((stName.uiLen != 0) && (stName.uiLen < sizeof(szName)))
        {
            TXT_Strlcpy(szName, stName.pcData, stName.uiLen + 1);
            if (NULL != pfDeleteNotify)
            {
                pfDeleteNotify(pstDweb->hSvpnContext, szName);
            }
            SVPN_CtxData_DelObject(pstDweb->hSvpnContext, enDataIndex, szName);
        }
    }LSTR_SCAN_ELEMENT_END();

    svpn_mf_SetSuccess(pstDweb);
}

VOID SVPN_MF_CommonList
(
    IN MIME_HANDLE hMime,
    IN SVPN_DWEB_S *pstDweb,
    IN SVPN_CTXDATA_E enDataIndex,
    IN CHAR **apcPropertys,
    IN UINT uiPropertyCount
)
{
    cJSON *pstArray;
    cJSON *pstResJson;
    CHAR szName[256] = "";
    HSTRING hsUrl;
    CHAR *pcProperty;
    UINT i;

    pstArray = cJSON_CreateArray();
    if (NULL == pstArray)
    {
        svpn_mf_SetFailed(pstDweb, "Not enough memory");
        return;
    }

    while (BS_OK == SVPN_CtxData_GetNextObject(pstDweb->hSvpnContext, enDataIndex, szName, szName, sizeof(szName)))
    {
        pstResJson = cJSON_CreateObject();
        if (NULL == pstResJson)
        {
            continue;
        }

        for (i=0; i<uiPropertyCount; i++)
        {
            pcProperty = "";
            hsUrl = SVPN_CtxData_GetPropAsHString(pstDweb->hSvpnContext, enDataIndex, szName, apcPropertys[i]);
            if (hsUrl != NULL)
            {
                pcProperty = STRING_GetBuf(hsUrl);
            }
            cJSON_AddStringToObject(pstResJson, apcPropertys[i], pcProperty);
            if (NULL != hsUrl)
            {
                STRING_Delete(hsUrl);
            }
        }

        cJSON_AddStringToObject(pstResJson, "Name", szName);
        cJSON_AddItemToArray(pstArray, pstResJson);
    }

    cJSON_AddItemToObject(pstDweb->pstJson, "data", pstArray);

    svpn_mf_SetSuccess(pstDweb);

    return;
}
