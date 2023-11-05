/******************************************************************************
* Copyright (C), 2000-2006,  LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2016-11-2
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/json_utl.h"
#include "utl/txt_utl.h"
#include "utl/file_utl.h"
#include "utl/local_info.h"

#include "webcenter_inner.h"

static CHAR g_szWebCenterBindWsService[WSAPP_SERVICE_NAME_LEN + 1] = "";
static CHAR g_szWebCenterInnerBindWsService[WSAPP_SERVICE_NAME_LEN + 1] = "";  

static BS_STATUS _webcenter_kf_SaveConfig(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN KFAPP_PARAM_S *pstParam)
{
    CMD_EXP_DoCmd("save");

    JSON_SetSuccess(pstParam->pstJson);

    return BS_OK;
}

static BS_STATUS _webcenter_kf_SetConfig(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN KFAPP_PARAM_S *pstParam)
{
    CHAR *pcWsService;

    pcWsService = MIME_GetKeyValue(hMime, "WsService");

    if (BS_OK != WebCenter_BindWsService(pcWsService, FALSE))
    {
        JSON_SetFailed(pstParam->pstJson, "Can't bind ws service");
        return BS_OK;
    }

    JSON_SetSuccess(pstParam->pstJson);

    return BS_OK;
}

static BS_STATUS _webcenter_kf_GetConfig(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN KFAPP_PARAM_S *pstParam)
{
    JSON_AddString(pstParam->pstJson, "WsService", g_szWebCenterBindWsService);
    JSON_SetSuccess(pstParam->pstJson);

    return BS_OK;
}

BS_STATUS WebCenter_BindWsService(IN CHAR *pcWsService, IN BOOL_T bInner)
{
    CHAR szFullPath[FILE_MAX_PATH_LEN + 1];
    CHAR *pcString;

    pcString = g_szWebCenterBindWsService;
    if (bInner)
    {
        pcString = g_szWebCenterInnerBindWsService;
    }
    
    if (NULL == pcWsService)
    {
        pcWsService = "";
    }

    if (strcmp(pcWsService, pcString) == 0)
    {
        return BS_OK;
    }

    if (! TXT_IS_EMPTY(pcWsService))
    {
        if (BS_OK != WSAPP_BindService(pcWsService))
        {
            return BS_ERR;
        }
    }

    if (pcString[0] != '\0')
    {
        WSAPP_UnBindService(pcString);
    }

    strlcpy(pcString, pcWsService, WSAPP_SERVICE_NAME_LEN + 1);

    if (! TXT_IS_EMPTY(pcWsService))
    {
        WebCenter_Deliver_BindService(pcWsService);
        LOCAL_INFO_ExpandToConfPath("web/", szFullPath);
        WSAPP_SetDocRoot(pcWsService, szFullPath);
        WSAPP_SetIndex(pcWsService, "/index.htm");
    }

    return BS_OK;
}

CHAR * WebCenter_GetBindedInnerWsService()
{
    return g_szWebCenterInnerBindWsService;
}

CHAR * WebCenter_GetBindedWsService()
{
    return g_szWebCenterBindWsService;
}

BS_STATUS WebCenter_KF_Init()
{
    KFAPP_RegFunc("webcenter.SaveConfig", _webcenter_kf_SaveConfig, NULL);
    KFAPP_RegFunc("webcenter.SetConfig", _webcenter_kf_SetConfig, NULL);
    KFAPP_RegFunc("webcenter.GetConfig", _webcenter_kf_GetConfig, NULL);

	return BS_OK;
}

