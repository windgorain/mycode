/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2015-6-15
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/ulm_utl.h"
#include "utl/mutex_utl.h"
#include "utl/txt_utl.h"

#include "../h/svpn_def.h"
#include "../h/svpn_context.h"
#include "../h/svpn_ulm.h"
#include "../h/svpn_local_user.h"

#define SVPN_ULM_MAX_USER_NUM 5000

typedef struct
{
    MUTEX_S stMutex;
    ULM_HANDLE hUlm;
}SVPN_ULM_S;

SVPN_ULM_S * svpn_ulm_GetCtrl(IN SVPN_CONTEXT_HANDLE hSvpnContext)
{
    return SVPN_Context_GetUserData(hSvpnContext, SVPN_CONTEXT_DATA_INDEX_ULM);
}

static BS_STATUS svpn_ulm_ContextCreate(IN SVPN_CONTEXT_HANDLE hSvpnContext)
{
    SVPN_ULM_S *pstUlm;

    pstUlm = MEM_ZMalloc(sizeof(SVPN_ULM_S));
    if (NULL == pstUlm)
    {
        return BS_NO_MEMORY;
    }

    pstUlm->hUlm = ULM_CreateInstance(SVPN_ULM_MAX_USER_NUM);
    if (NULL == pstUlm->hUlm)
    {
        MEM_Free(pstUlm);
        return BS_ERR;
    }

    MUTEX_Init(&pstUlm->stMutex);

    SVPN_Context_SetUserData(hSvpnContext, SVPN_CONTEXT_DATA_INDEX_ULM, pstUlm);

    return BS_OK;
}

static BS_STATUS svpn_ulm_ContextDestroy(IN SVPN_CONTEXT_HANDLE hSvpnContext)
{
    SVPN_ULM_S *pstUlm;

    pstUlm = svpn_ulm_GetCtrl(hSvpnContext);
    if (NULL != pstUlm)
    {
        MUTEX_Final(&pstUlm->stMutex);
        ULM_DesTroyInstance(pstUlm->hUlm);
        MEM_Free(pstUlm);
        SVPN_Context_SetUserData(hSvpnContext, SVPN_CONTEXT_DATA_INDEX_ULM, NULL);
    }

    return BS_OK;
}

static BS_STATUS svpn_ulm_ContextEvent(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN UINT uiEvent)
{
    BS_STATUS eRet = BS_OK;

    switch (uiEvent)
    {
        case SVPN_CONTEXT_EVENT_CREATE:
        {
            svpn_ulm_ContextCreate(hSvpnContext);
            break;
        }

        case SVPN_CONTEXT_EVENT_DESTROY:
        {
            svpn_ulm_ContextDestroy(hSvpnContext);
            break;
        }

        default:
        {
            break;
        }
    }

    return eRet;
}


BS_STATUS SVPN_ULM_Init()
{
    SVPN_Context_RegEvent(svpn_ulm_ContextEvent);

    return BS_OK;
}

UINT SVPN_ULM_AddUser(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN CHAR *pcUserName, IN UINT uiUserType)
{
    SVPN_ULM_S *pstUlm;
    UINT uiUserID;
    CHAR *pcRoles;
    HSTRING hString;

    pstUlm = svpn_ulm_GetCtrl(hSvpnContext);
    if (NULL == pstUlm)
    {
        return BS_NOT_READY;
    }

    hString = SVPN_LocalUser_GetRoleAsHString(hSvpnContext, pcUserName);
    if (NULL == hString)
    {
        return BS_NO_MEMORY;
    }

    pcRoles = STRING_GetBuf(hString);

    MUTEX_P(&pstUlm->stMutex);
    uiUserID = ULM_AddUser(pstUlm->hUlm, pcUserName);
    if (0 != uiUserID)
    {
        ULM_SetUserFlag(pstUlm->hUlm, uiUserID, uiUserType);
        ULM_SetUserKeyValue(pstUlm->hUlm, uiUserID, "Roles", pcRoles);
    }
    MUTEX_V(&pstUlm->stMutex);

    STRING_Delete(hString);

    return uiUserID;
}

UINT SVPN_ULM_GetUserType(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN UINT uiOlUserID)
{
    SVPN_ULM_S *pstUlm;
    UINT uiUserFlag;
    UINT uiUserType = SVPN_USER_TYPE_ANONYMOUS;
    BS_STATUS eRet;

    pstUlm = svpn_ulm_GetCtrl(hSvpnContext);
    if (NULL == pstUlm)
    {
        return SVPN_USER_TYPE_ANONYMOUS;
    }

    MUTEX_P(&pstUlm->stMutex);
    eRet = ULM_GetUserFlag(pstUlm->hUlm, uiOlUserID, &uiUserFlag);
    if (eRet == BS_OK)
    {
        uiUserType = uiUserFlag;
    }
    MUTEX_V(&pstUlm->stMutex);

    return uiUserType;
}

BS_STATUS SVPN_ULM_GetUserCookie
(
    IN SVPN_CONTEXT_HANDLE hSvpnContext,
    IN UINT uiUserID,
    OUT CHAR szUserCookie[ULM_USER_COOKIE_LEN + 1]
)
{
    SVPN_ULM_S *pstUlm;
    CHAR *pcCookie;
    BS_STATUS eRet = BS_ERR;

    pstUlm = svpn_ulm_GetCtrl(hSvpnContext);
    if (NULL == pstUlm)
    {
        return BS_NOT_READY;
    }

    MUTEX_P(&pstUlm->stMutex);
    pcCookie = ULM_GetUserCookie(pstUlm->hUlm, uiUserID);
    if (NULL != pcCookie)
    {
        TXT_Strlcpy(szUserCookie, pcCookie, ULM_USER_COOKIE_LEN + 1);
        eRet = BS_OK;
    }
    MUTEX_V(&pstUlm->stMutex);

    return eRet;
}

UINT SVPN_ULM_GetUserIDByCookie(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN CHAR *pcCookie)
{
    SVPN_ULM_S *pstUlm;
    UINT uiUserID = 0;

    pstUlm = svpn_ulm_GetCtrl(hSvpnContext);
    if (NULL == pstUlm)
    {
        return 0;
    }

    MUTEX_P(&pstUlm->stMutex);
    uiUserID = ULM_GetUserIDByCookie(pstUlm->hUlm, pcCookie);
    MUTEX_V(&pstUlm->stMutex);

    return uiUserID;
}

BS_STATUS SVPN_ULM_GetUserName
(
    IN SVPN_CONTEXT_HANDLE hSvpnContext,
    IN UINT uiUserID,
    OUT CHAR *pcUserName
)
{
    SVPN_ULM_S *pstUlm;
    CHAR *pcUserNameTmp;
    BS_STATUS eRet = BS_NOT_FOUND;

    pstUlm = svpn_ulm_GetCtrl(hSvpnContext);
    if (NULL == pstUlm)
    {
        return BS_NOT_INIT;
    }

    MUTEX_P(&pstUlm->stMutex);
    pcUserNameTmp = ULM_GetUserNameById(pstUlm->hUlm, uiUserID);
    if (NULL != pcUserNameTmp)
    {
        TXT_Strlcpy(pcUserName, pcUserNameTmp, SVPN_MAX_USER_NAME_LEN + 1);
        eRet = BS_OK;
    }
    MUTEX_V(&pstUlm->stMutex);

    return eRet;
}

HSTRING SVPN_ULM_GetUserRoles
(
    IN SVPN_CONTEXT_HANDLE hSvpnContext,
    IN UINT uiUserID
)
{
    SVPN_ULM_S *pstUlm;
    CHAR *pcRoles;
    HSTRING hString = NULL;

    pstUlm = svpn_ulm_GetCtrl(hSvpnContext);
    if (NULL == pstUlm)
    {
        return NULL;
    }

    MUTEX_P(&pstUlm->stMutex);
    pcRoles = ULM_GetUserKeyValue(pstUlm->hUlm, uiUserID, "Roles");
    if (NULL != pcRoles)
    {
        hString = STRING_Create();
        if (NULL != hString)
        {
            STRING_CpyFromBuf(hString, pcRoles);
        }
    }
    MUTEX_V(&pstUlm->stMutex);

    return hString;
}


