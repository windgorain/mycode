/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-2-16
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/cjson.h"
#include "utl/kf_utl.h"
#include "utl/ws_utl.h"
#include "utl/txt_utl.h"
#include "utl/lstr_utl.h"
#include "utl/ulm_utl.h"
#include "utl/vldcode_utl.h"

#include "../../inc/vnet_conf.h"
#include "../inc/vnets_dc.h"
#include "../inc/vnets_domain.h"
#include "../inc/vnets_phy.h"
#include "../inc/vnets_ses.h"
#include "../inc/vnets_node.h"

#include "vnets_web_inner.h"
#include "vnets_web_vldcode.h"
#include "vnets_web_ulm.h"


#define VNETS_WEB_USER_TYPE_ANONYMOUS  0x1
#define VNETS_WEB_USER_TYPE_USER       0x2
#define VNETS_WEB_USER_TYPE_ALL        0xff


typedef BS_STATUS (*PF_vnets_webkf_map_func)(IN MIME_HANDLE hMime, IN VNETS_WEB_S *pstDweb);

typedef struct
{
    UINT uiFlag;
    CHAR *pcKey;
    PF_vnets_webkf_map_func pfFunc;
}_VNETS_KF_MAP_S;

static KF_HANDLE g_hVnetsWebKf = NULL;

static BS_STATUS vnets_webkf_GetUserInfo(IN VNETS_WEB_USER_S *pstWebUser, OUT ULM_USER_INFO_S *pstUserInfo)
{
    VNETS_NODE_INFO_S stNodeInfo;
    
    if (pstWebUser->ucType == VNETS_WEB_USER_TYPE_WEB)
    {
        return VNETS_WebUlm_GetUserInfo(pstWebUser->uiOnlineUserID, pstUserInfo);
    }

    if (pstWebUser->ucType == VNETS_WEB_USER_TYPE_CLIENT)
    {
        if (BS_OK != VNETS_NODE_GetNodeInfo(pstWebUser->uiOnlineUserID, &stNodeInfo))
        {
            return BS_ERR;
        }

        TXT_Strlcpy(pstUserInfo->szUserName, stNodeInfo.szUserName, sizeof(pstUserInfo->szUserName));
        pstUserInfo->ulLoginTime = stNodeInfo.uiLoginTime;

        return BS_OK;
    }

    return BS_ERR;
}

static BS_STATUS vnets_webkf_GetSelfDomain(IN VNETS_WEB_USER_S *pstOnlineUser, OUT CHAR *pcDomainName, IN UINT uiMaxSize)
{
    ULM_USER_INFO_S stUserInfo;

    if (BS_OK != vnets_webkf_GetUserInfo(pstOnlineUser, &stUserInfo))
    {
        return BS_ERR;
    }

    snprintf(pcDomainName, uiMaxSize, "@%s", stUserInfo.szUserName);

    return BS_OK;
}

static inline VOID vnets_webkf_SetSuccess(IN VNETS_WEB_S *pstDweb)
{
    cJSON_AddStringToObject(pstDweb->pstJson, "Result", "Success");
}

static inline VOID vnets_webkf_SetFailed(IN VNETS_WEB_S *pstDweb, IN CHAR *pcReason)
{
    cJSON_AddStringToObject(pstDweb->pstJson, "Result", "Failed");
    cJSON_AddStringToObject(pstDweb->pstJson, "Reason", pcReason);
}

static UINT vnets_webkf_GetVldCodeClientID(IN WS_TRANS_HANDLE hWsTrans)
{
    UINT uiClientId;
    CHAR *pcClientId;
    MIME_HANDLE hCookieMime;

    hCookieMime = WS_Trans_GetCookieMime(hWsTrans);
    if (NULL == hCookieMime)
    {
        return 0;
    }

    pcClientId = MIME_GetKeyValue(hCookieMime, "vnet_vldid");
    if (NULL == pcClientId)
    {
        return 0;
    }

    TXT_Atoui(pcClientId, &uiClientId);

    return uiClientId;
}

static BS_STATUS vnets_webkf_VldImg(IN MIME_HANDLE hMime, IN VNETS_WEB_S *pstDweb)
{
    UINT uiClientID;
    VLDBMP_S *pstBmp;
    HTTP_HEAD_PARSER hEncap;
    UINT uiBmpSize;
    CHAR szCookie[256];

    uiClientID = vnets_webkf_GetVldCodeClientID(pstDweb->hWsTrans);

    pstBmp = VNETS_WebVldCode_GenImg(&uiClientID);
    if (NULL == pstBmp)
    {
        vnets_webkf_SetFailed(pstDweb, "Memory error.");
        return BS_ERR;
    }

    pstDweb->uiFlag |= VNETS_WEB_FLAG_EMPTY_JSON;

    uiBmpSize = VLDBMP_SIZE(pstBmp);

    hEncap = WS_Trans_GetHttpEncap(pstDweb->hWsTrans);
    snprintf(szCookie, sizeof(szCookie), "vnet_vldid=%u; Max-Age=120; path=/", uiClientID);
    HTTP_SetHeadField(hEncap, HTTP_FIELD_SET_COOKIE, szCookie);
    HTTP_SetStatusCode(hEncap, HTTP_STATUS_OK);
    HTTP_SetContentLen(hEncap, uiBmpSize);
    HTTP_SetNoCache(hEncap);
    WS_Trans_SetHeadFieldFinish(pstDweb->hWsTrans);
    WS_Trans_AddReplyBodyByBuf(pstDweb->hWsTrans, (UCHAR*)(VOID*)pstBmp, uiBmpSize);
    WS_Trans_ReplyBodyFinish(pstDweb->hWsTrans);

    VLDBMP_Destory(pstBmp);

    return BS_OK;
}

static BS_STATUS vnets_webkf_UserReg(IN MIME_HANDLE hMime, IN VNETS_WEB_S *pstDweb)
{
    CHAR *pcUserName;
    CHAR *pcPassword;
    CHAR *pcEmail;
    CHAR *pcVldCode;
    UINT uiClientId;
    VLDCODE_RET_E eVldCodeRet;

    pcUserName = MIME_GetKeyValue(hMime, "UserName");
    pcPassword = MIME_GetKeyValue(hMime, "Password");
    pcEmail = MIME_GetKeyValue(hMime, "Email");
    pcVldCode = MIME_GetKeyValue(hMime, "VldCode");

    if (NULL == pcVldCode)
    {
        vnets_webkf_SetFailed(pstDweb, "Validate code error");
        return BS_ERR;
    }

    uiClientId = vnets_webkf_GetVldCodeClientID(pstDweb->hWsTrans);
    if (uiClientId == 0)
    {
        vnets_webkf_SetFailed(pstDweb, "Validate code error");
        return BS_ERR;
    }

    eVldCodeRet = VNETS_WebVldCode_Check(uiClientId, pcVldCode);
    switch (eVldCodeRet)
    {
        case VLDCODE_VALID:
        {
            break;
        }
        case VLDCODE_TIMEOUT:
        {
            vnets_webkf_SetFailed(pstDweb, "Validate code time out");
            return BS_BAD_PARA;
        }
        case VLDCODE_INVALID:
        case VLDCODE_NOT_FOUND:
        default:
        {
            vnets_webkf_SetFailed(pstDweb, "Validate code error");
            return BS_BAD_PARA;
        }
    }

    if ((NULL == pcUserName) || (NULL == pcPassword))
    {
        vnets_webkf_SetFailed(pstDweb, "Invalid user name or password");
        return BS_BAD_PARA;
    }

    if (strlen(pcUserName) > VNET_CONF_MAX_USER_NAME_LEN)
    {
        vnets_webkf_SetFailed(pstDweb, "User name is too long");
        return BS_BAD_PARA;
    }

    if (TRUE == VNETS_DC_IsUserExist(pcUserName))
    {
        vnets_webkf_SetFailed(pstDweb, "User exist");
        return BS_ERR;
    }

    if (BS_OK != VNETS_DC_AddUser(pcUserName, pcPassword))
    {
        vnets_webkf_SetFailed(pstDweb, "Failed to reg user");
        return BS_ERR;
    }

    if (NULL != pcEmail)
    {
        VNETS_DC_SetEmail(pcUserName, pcEmail);
    }

    vnets_webkf_SetSuccess(pstDweb);

    return BS_OK;
}

static BS_STATUS vnets_webkf_UserChangePassword(IN MIME_HANDLE hMime, IN VNETS_WEB_S *pstDweb)
{
    CHAR *pcOldPassword;
    CHAR *pcNewPassword;
    ULM_USER_INFO_S stUserInfo;

    pcOldPassword = MIME_GetKeyValue(hMime, "OldPassword");
    pcNewPassword = MIME_GetKeyValue(hMime, "NewPassword");

    if ((NULL == pcOldPassword) || (NULL == pcNewPassword))
    {
        vnets_webkf_SetFailed(pstDweb, "Password error");
        return BS_BAD_PARA;
    }

    if (0 == pstDweb->stOnlineUser.uiOnlineUserID)
    {
        vnets_webkf_SetFailed(pstDweb, "Please login");
        return BS_ERR;
    }

    if (BS_OK != vnets_webkf_GetUserInfo(&pstDweb->stOnlineUser, &stUserInfo))
    {
        vnets_webkf_SetFailed(pstDweb, "Please login");
        return BS_ERR;
    }

    if (TRUE != VNETS_DC_CheckUserPassword(stUserInfo.szUserName, pcOldPassword))
    {
        vnets_webkf_SetFailed(pstDweb, "Old password error");
        return BS_ERR;
    }

    VNETS_DC_SetPassword(stUserInfo.szUserName, pcNewPassword);

    vnets_webkf_SetSuccess(pstDweb);

    return BS_OK;
}

static BS_STATUS vnets_webkf_UserIsExist(IN MIME_HANDLE hMime, IN VNETS_WEB_S *pstDweb)
{
    CHAR *pcUserName;

    pcUserName = MIME_GetKeyValue(hMime, "UserName");

    if (NULL == pcUserName)
    {
        vnets_webkf_SetFailed(pstDweb, "Invalid user name");
        return BS_BAD_PARA;
    }

    if (TRUE != VNETS_DC_IsUserExist(pcUserName))
    {
        vnets_webkf_SetFailed(pstDweb, "User not exist");
        return BS_ERR;
    }

    vnets_webkf_SetSuccess(pstDweb);

    return BS_OK;
}

static BS_STATUS vnets_webkf_UserLogin(IN MIME_HANDLE hMime, IN VNETS_WEB_S *pstDweb)
{
    CHAR *pcUserName;
    CHAR *pcPassword;
    UINT uiUserID;
    HTTP_HEAD_PARSER hEncap;
    CHAR szCookie[256];
    CHAR szUserCookie[ULM_USER_COOKIE_LEN + 1];
    CHAR *pcVldCode;
    UINT uiClientId;
    VLDCODE_RET_E eVldCodeRet;

    pcVldCode = MIME_GetKeyValue(hMime, "VldCode");

    if (NULL == pcVldCode)
    {
        vnets_webkf_SetFailed(pstDweb, "Validate code error");
        return BS_ERR;
    }

    uiClientId = vnets_webkf_GetVldCodeClientID(pstDweb->hWsTrans);
    if (uiClientId == 0)
    {
        vnets_webkf_SetFailed(pstDweb, "Validate code error");
        return BS_ERR;
    }

    eVldCodeRet = VNETS_WebVldCode_Check(uiClientId, pcVldCode);
    switch (eVldCodeRet)
    {
        case VLDCODE_VALID:
        {
            break;
        }
        case VLDCODE_TIMEOUT:
        {
            vnets_webkf_SetFailed(pstDweb, "Validate code time out");
            return BS_BAD_PARA;
        }
        case VLDCODE_INVALID:
        case VLDCODE_NOT_FOUND:
        default:
        {
            vnets_webkf_SetFailed(pstDweb, "Validate code error");
            return BS_BAD_PARA;
        }
    }

    pcUserName = MIME_GetKeyValue(hMime, "UserName");
    pcPassword = MIME_GetKeyValue(hMime, "Password");

    if ((NULL == pcUserName) || (NULL == pcPassword))
    {
        vnets_webkf_SetFailed(pstDweb, "Invalid user name or password");
        return BS_BAD_PARA;
    }

    if (TRUE != VNETS_DC_CheckUserPassword(pcUserName, pcPassword))
    {
        vnets_webkf_SetFailed(pstDweb, "User name or password error");
        return BS_ERR;
    }

    uiUserID = VNETS_WebUlm_Add(pcUserName);
    if (0 == uiUserID)
    {
        vnets_webkf_SetFailed(pstDweb, "Full");
        return BS_ERR;
    }

    VNETS_WebUlm_GetCookie(uiUserID, szUserCookie);

    hEncap = WS_Trans_GetHttpEncap(pstDweb->hWsTrans);
    snprintf(szCookie, sizeof(szCookie), "vnet_userid=%s; path=/", szUserCookie);
    HTTP_SetHeadField(hEncap, HTTP_FIELD_SET_COOKIE, szCookie);

    vnets_webkf_SetSuccess(pstDweb);

    return BS_OK;
}

static BS_STATUS vnets_webkf_UserInfo(IN MIME_HANDLE hMime, IN VNETS_WEB_S *pstDweb)
{
    ULM_USER_INFO_S stUserInfo;
    CHAR szEmail[VNET_DC_MAX_EMAIL_LEN + 1];

    if (0 == pstDweb->stOnlineUser.uiOnlineUserID)
    {
        vnets_webkf_SetFailed(pstDweb, "Please login");
        return BS_ERR;
    }

    if (BS_OK != vnets_webkf_GetUserInfo(&pstDweb->stOnlineUser, &stUserInfo))
    {
        vnets_webkf_SetFailed(pstDweb, "Please login");
        return BS_ERR;
    }

    cJSON_AddStringToObject(pstDweb->pstJson, "UserName", stUserInfo.szUserName);

    if (BS_OK == VNETS_DC_GetEmail(stUserInfo.szUserName, szEmail))
    {
        cJSON_AddStringToObject(pstDweb->pstJson, "Email", szEmail);
    }

    vnets_webkf_SetSuccess(pstDweb);

    return BS_OK;
}

static BS_STATUS vnets_webkf_UserLogout(IN MIME_HANDLE hMime, IN VNETS_WEB_S *pstDweb)
{
    if ((pstDweb->stOnlineUser.ucType == VNETS_WEB_USER_TYPE_WEB) && (pstDweb->stOnlineUser.uiOnlineUserID != 0))
    {
        VNETS_WebUlm_Del(pstDweb->stOnlineUser.uiOnlineUserID);
    }

    vnets_webkf_SetSuccess(pstDweb);

    return BS_OK;
}

static BS_STATUS vnets_webkf_OlUserList(IN MIME_HANDLE hMime, IN VNETS_WEB_S *pstDweb)
{
    cJSON *pstArray;
    cJSON *pstUserJson;
    UINT uiNodeID = 0;
    VNETS_NODE_INFO_S stNodeInfo;
    VNETS_PHY_CONTEXT_S stPhyInfo;
    UINT uiPrefix;
    CHAR szTmp[32];

    pstArray = cJSON_CreateArray();
    if (NULL == pstArray)
    {
        vnets_webkf_SetFailed(pstDweb, "No enough memory");
        return BS_OK;
    }

    while ((uiNodeID = VNETS_NODE_GetNext(uiNodeID)) != 0)
    {
        pstUserJson = cJSON_CreateObject();
        if (NULL == pstUserJson)
        {
            break;
        }

        if (BS_OK != VNETS_NODE_GetNodeInfo(uiNodeID, &stNodeInfo))
        {
            continue;
        }

        if (BS_OK != VNETS_SES_GetPhyInfo(stNodeInfo.uiSesID, &stPhyInfo))
        {
            continue;
        }

        snprintf(szTmp, sizeof(szTmp), "%d", stNodeInfo.uiNodeID);
        cJSON_AddStringToObject(pstUserJson, "NodeID", szTmp);
        cJSON_AddStringToObject(pstUserJson, "UserName", stNodeInfo.szUserName);
        cJSON_AddStringToObject(pstUserJson, "Description", stNodeInfo.szDescription);
        BS_Snprintf(szTmp, sizeof(szTmp), "%pI4:%d",
            &stPhyInfo.unPhyContext.stUdpPhyContext.uiPeerIp,
            ntohs(stPhyInfo.unPhyContext.stUdpPhyContext.usPeerPort));
        cJSON_AddStringToObject(pstUserJson, "UserAddress", szTmp);
        uiPrefix = MASK_2_PREFIX(ntohl(stNodeInfo.uiMask));
        BS_Snprintf(szTmp, sizeof(szTmp), "%pI4/%d", &stNodeInfo.uiIP, uiPrefix);
        cJSON_AddStringToObject(pstUserJson, "VirtualAddress", szTmp);
        BS_Snprintf(szTmp, sizeof(szTmp), "%pM", &stNodeInfo.stMACAddr);
        cJSON_AddStringToObject(pstUserJson, "MAC", szTmp);

        cJSON_AddItemToArray(pstArray, pstUserJson);
    }

    cJSON_AddItemToObject(pstDweb->pstJson, "data", pstArray);

    return BS_OK;
}

static BS_STATUS vnets_webkf_OlUserDelete(IN MIME_HANDLE hMime, IN VNETS_WEB_S *pstDweb)
{
    CHAR *pcIdList;
    LSTR_S stNodeID;
    UINT uiNodeID;

    pcIdList = MIME_GetKeyValue(hMime, "NodeIDList");
    if (NULL == pcIdList)
    {
        vnets_webkf_SetFailed(pstDweb, "Param error");
        return BS_ERR;
    }

    LSTR_SCAN_ELEMENT_BEGIN(pcIdList, strlen(pcIdList), ',', &stNodeID)
    {
        if (stNodeID.uiLen != 0)
        {
            LSTR_Atoui(&stNodeID, &uiNodeID);
            VNETS_NODE_DelNode(uiNodeID);
        }
    }LSTR_SCAN_ELEMENT_END();

    vnets_webkf_SetSuccess(pstDweb);

    return BS_OK;
}

static BS_STATUS vnets_webkf_DhcpConfigIP(IN MIME_HANDLE hMime, IN VNETS_WEB_S *pstDweb)
{
    CHAR *pcEnable;
    CHAR *pcStartIp;
    CHAR *pcEndIp;
    CHAR *pcMask;
    CHAR *pcGateway;
    CHAR szDomainName[256];

    if (BS_OK != vnets_webkf_GetSelfDomain(&pstDweb->stOnlineUser, szDomainName, sizeof(szDomainName)))
    {
        vnets_webkf_SetFailed(pstDweb, "Please login");
        return BS_ERR;
    }

    pcEnable = MIME_GetKeyValue(hMime, "Enable");
    pcStartIp = MIME_GetKeyValue(hMime, "StartIP");
    pcEndIp = MIME_GetKeyValue(hMime, "EndIP");
    pcMask = MIME_GetKeyValue(hMime, "Mask");
    pcGateway = MIME_GetKeyValue(hMime, "Gateway");

    if ((pcEnable == NULL) || (pcStartIp == NULL) || (pcEndIp == NULL) || (pcMask == NULL))
    {
        vnets_webkf_SetFailed(pstDweb, "Config error");
        return BS_ERR;
    }

    VNETS_DC_SetDhcpConfig(szDomainName, pcEnable, pcStartIp, pcEndIp, pcMask, pcGateway);

    vnets_webkf_SetSuccess(pstDweb);

    return BS_OK;
}

static BS_STATUS vnets_webkf_GetDhcpConfigIP(IN MIME_HANDLE hMime, IN VNETS_WEB_S *pstDweb)
{
    CHAR szDomainName[256];
    CHAR szEnable[32];
    CHAR szStartIP[32];
    CHAR szEndIP[32];
    CHAR szMask[32];
    CHAR szGataeway[32];

    if (BS_OK != vnets_webkf_GetSelfDomain(&pstDweb->stOnlineUser, szDomainName, sizeof(szDomainName)))
    {
        vnets_webkf_SetFailed(pstDweb, "Please login");
        return BS_ERR;
    }

    VNETS_DC_GetDhcpConfig(szDomainName, szEnable, szStartIP, szEndIP, szMask, szGataeway);

    cJSON_AddStringToObject(pstDweb->pstJson, "Enable", szEnable);
    cJSON_AddStringToObject(pstDweb->pstJson, "StartIP", szStartIP);
    cJSON_AddStringToObject(pstDweb->pstJson, "EndIP", szEndIP);
    cJSON_AddStringToObject(pstDweb->pstJson, "Mask", szMask);
    cJSON_AddStringToObject(pstDweb->pstJson, "Gateway", szGataeway);

    vnets_webkf_SetSuccess(pstDweb);

    return BS_OK;
}

static BS_STATUS vnets_webkf_SysRebootDomain(IN MIME_HANDLE hMime, IN VNETS_WEB_S *pstDweb)
{
    CHAR szDomainName[256];

    if (BS_OK != vnets_webkf_GetSelfDomain(&pstDweb->stOnlineUser, szDomainName, sizeof(szDomainName)))
    {
        vnets_webkf_SetFailed(pstDweb, "Please login");
        return BS_ERR;
    }

    VNETS_Domain_RebootByName(szDomainName);

    vnets_webkf_SetSuccess(pstDweb);

    return BS_OK;
}

static _VNETS_KF_MAP_S g_astVnetsWebKfMap[] =
{
    {VNETS_WEB_USER_TYPE_ALL,
        "vldimg",   vnets_webkf_VldImg}, 
    {VNETS_WEB_USER_TYPE_ALL,
        "User.Reg", vnets_webkf_UserReg}, 
    {VNETS_WEB_USER_TYPE_ALL,
        "User.ChangePassword", vnets_webkf_UserChangePassword}, 
    {VNETS_WEB_USER_TYPE_ALL,
        "User.IsExist", vnets_webkf_UserIsExist}, 
    {VNETS_WEB_USER_TYPE_ALL,
        "User.Login", vnets_webkf_UserLogin}, 
    {VNETS_WEB_USER_TYPE_USER,
        "User.Info", vnets_webkf_UserInfo},  
    {VNETS_WEB_USER_TYPE_USER,
        "User.Logout", vnets_webkf_UserLogout},  
    {VNETS_WEB_USER_TYPE_USER,
        "OnlineUser.List", vnets_webkf_OlUserList},  
    {VNETS_WEB_USER_TYPE_USER,
        "OnlineUser.Delete", vnets_webkf_OlUserDelete},  
    {VNETS_WEB_USER_TYPE_USER,
        "Dhcp.IpConfig", vnets_webkf_DhcpConfigIP},  
    {VNETS_WEB_USER_TYPE_USER,
        "Dhcp.GetIpConfig", vnets_webkf_GetDhcpConfigIP},  
    {VNETS_WEB_USER_TYPE_USER,
        "Sys.RebootDomain", vnets_webkf_SysRebootDomain},  
};


static BS_STATUS vnets_webkf_CallBack(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN HANDLE hRunHandle)
{
    _VNETS_KF_MAP_S *pstMap = hUserHandle;
    VNETS_WEB_S *pstDweb = hRunHandle;
    UINT uiUserType = VNETS_WEB_USER_TYPE_ANONYMOUS;

    if (pstDweb->stOnlineUser.uiOnlineUserID != 0)
    {
        uiUserType = VNETS_WEB_USER_TYPE_USER;
    }

    if ((pstMap->uiFlag & uiUserType) == 0)
    {
        vnets_webkf_SetFailed(pstDweb, "Insuffcinet privileges");
        return BS_OK;
    }

    pstMap->pfFunc(hMime, pstDweb);

    return BS_OK;
}

BS_STATUS VNETS_WebKf_Init()
{
    UINT i;

    g_hVnetsWebKf = KF_Create("_do");
    if (NULL == g_hVnetsWebKf)
    {
        return BS_NO_MEMORY;
    }

    for (i=0; i<sizeof(g_astVnetsWebKfMap)/sizeof(_VNETS_KF_MAP_S); i++)
    {
        KF_AddFunc(g_hVnetsWebKf, g_astVnetsWebKfMap[i].pcKey, vnets_webkf_CallBack, &g_astVnetsWebKfMap[i]);
    }

    return BS_OK;
}

static MIME_HANDLE vnets_webkf_GetMime(IN VNETS_WEB_S *pstDweb)
{
    MIME_HANDLE hMime1;
    MIME_HANDLE hMime2;

    hMime1 = WS_Trans_GetBodyMime(pstDweb->hWsTrans);
    hMime2 = WS_Trans_GetQueryMime(pstDweb->hWsTrans);

    if (hMime1 == NULL)
    {
        return hMime2;
    }

    if (hMime2 == NULL)
    {
        return hMime1;
    }

    MIME_Cat(hMime1, hMime2);

    return hMime1;
}

BS_STATUS VNETS_WebKf_Run(IN VNETS_WEB_S *pstDweb)
{
    MIME_HANDLE hMime;

    hMime = vnets_webkf_GetMime(pstDweb);
    if (NULL == hMime)
    {
        return BS_OK;
    }

    return KF_RunMime(g_hVnetsWebKf, hMime, pstDweb);
}


