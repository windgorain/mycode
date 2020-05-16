/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-2-17
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/txt_utl.h"
#include "utl/mutex_utl.h"

#include "vnets_web_inner.h"
#include "vnets_web_ulm.h"

#define VNETS_WEB_ULM_MAX_USER (1024 * 1024)

static ULM_HANDLE g_hVnetsWebUlm = NULL;
static MUTEX_S g_stVnetsWebUlmLock;


UINT VNETS_WebUlm_Add(IN CHAR *pcUserName)
{
    return ULM_AddUser(g_hVnetsWebUlm, pcUserName);
}

BS_STATUS VNETS_WebUlm_Init()
{
    g_hVnetsWebUlm = ULM_CreateInstance(VNETS_WEB_ULM_MAX_USER);
    if (NULL == g_hVnetsWebUlm)
    {
        return BS_NO_MEMORY;
    }

    MUTEX_Init(&g_stVnetsWebUlmLock);

    return BS_OK;
}

BS_STATUS VNETS_WebUlm_Del(IN UINT uiUserID)
{
    BS_STATUS eRet;

    MUTEX_P(&g_stVnetsWebUlmLock);
    eRet = ULM_DelUser(g_hVnetsWebUlm, uiUserID);
    MUTEX_V(&g_stVnetsWebUlmLock);

    return eRet;
}

BS_STATUS VNETS_WebUlm_GetCookie(IN UINT uiUserID, OUT CHAR szUserCookie[ULM_USER_COOKIE_LEN + 1])
{
    CHAR *pcUserCookie;
    BS_STATUS eRet = BS_OK;

    MUTEX_P(&g_stVnetsWebUlmLock);
    pcUserCookie = ULM_GetUserCookie(g_hVnetsWebUlm, uiUserID);
    if (NULL == pcUserCookie)
    {
        eRet = BS_NOT_FOUND;
    }
    else
    {
        TXT_Strlcpy(szUserCookie, pcUserCookie, ULM_USER_COOKIE_LEN + 1);
    }
    MUTEX_V(&g_stVnetsWebUlmLock);

    return eRet;
}

UINT VNETS_WebUlm_GetUserIDByCookie(IN CHAR *pcCookie)
{
    UINT uiUserID;

    MUTEX_P(&g_stVnetsWebUlmLock);
    uiUserID = ULM_GetUserIDByCookie(g_hVnetsWebUlm, pcCookie);
    MUTEX_V(&g_stVnetsWebUlmLock);

    return uiUserID;
}

BS_STATUS VNETS_WebUlm_GetUserInfo(IN UINT uiUserID, OUT ULM_USER_INFO_S *pstUserInfo)
{
    BS_STATUS eRet;
    

    MUTEX_P(&g_stVnetsWebUlmLock);
    eRet = ULM_GetUserInfo(g_hVnetsWebUlm, uiUserID, pstUserInfo);
    MUTEX_V(&g_stVnetsWebUlmLock);

    return BS_OK;
}


