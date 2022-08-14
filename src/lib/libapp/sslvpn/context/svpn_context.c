/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2014-8-6
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/txt_utl.h"
#include "utl/file_utl.h"
#include "utl/ws_utl.h"
#include "utl/nap_utl.h"
#include "utl/json_utl.h"
#include "utl/object_utl.h"
#include "utl/local_info.h"
#include "comp/comp_kfapp.h"
#include "comp/comp_wsapp.h"

#include "../h/svpn_def.h"

#include "../h/svpn_context.h"
#include "../h/svpn_deliver.h"
#include "../h/svpn_cfglock.h"
#include "../h/svpn_local_user.h"

#include "svpn_context_inner.h"

#define SVPN_CONTEXT_MAX_NUM 64

typedef struct
{
    VOID * apUserData[SVPN_CONTEXT_DATA_INDEX_MAX];
}SVPN_CONTEXT_S;

typedef struct
{
    DLL_NODE_S stLinkNode;
    PF_SVPN_CONTEXT_IssuEvent pfFunc;
}SVPN_CONTEXT_LISTENER_S;

static NO_HANDLE g_hSvpnContextNo = NULL;
static DLL_HEAD_S g_stSvpnContextListenerList = DLL_HEAD_INIT_VALUE(&g_stSvpnContextListenerList);
static CHAR *g_apcSvpnContextProperty[] = {"Description", "WsService", NULL};

static BS_STATUS _svpn_context_BindServiceExt
(
    IN CHAR *pcContextName,
    IN CHAR *pcWsService,
    IN CHAR *pcLocalKey
)
{
    SVPN_CONTEXT_S *pstContext;
    CHAR *pcOldWsService;
    CHAR szFullPath[FILE_MAX_PATH_LEN + 1];

    pstContext = NO_GetObjectByName(g_hSvpnContextNo, pcContextName);
    if (NULL == pstContext)
    {
        return BS_NOT_FOUND;
    }

    pcOldWsService = NO_GetKeyValue(pstContext, pcLocalKey);

    if (NULL == pcWsService)
    {
        pcWsService = "";
    }

    if (NULL == pcOldWsService)
    {
        pcOldWsService = "";
    }

    if (strcmp(pcOldWsService, pcWsService) == 0)
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

    pcOldWsService = NO_GetKeyValue(pstContext, pcLocalKey);
    if (! TXT_IS_EMPTY(pcOldWsService))
    {
        WSAPP_UnBindService(pcOldWsService);
    }

    if (! TXT_IS_EMPTY(pcWsService))
    {
        SVPN_Deliver_BindContext(pcWsService);
        WSAPP_SetUserData(pcWsService, NO_GetObjectID(g_hSvpnContextNo, pstContext));

        LOCAL_INFO_ExpandToConfPath("web/context/", szFullPath);
        WSAPP_SetDocRoot(pcWsService, szFullPath);
        WSAPP_SetIndex(pcWsService, "/index.htm");
    }

    NO_SetKeyValue(g_hSvpnContextNo, pstContext, pcLocalKey, pcWsService);

    return BS_OK;
}

static BS_STATUS _svpn_context_IssuEvent(IN SVPN_CONTEXT_S *pstSvpnContext, IN UINT uiEvent)
{
    BS_STATUS eRet = BS_OK;
    SVPN_CONTEXT_LISTENER_S *pstListener;

    DLL_SCAN(&g_stSvpnContextListenerList, pstListener)
    {
        eRet |= pstListener->pfFunc(pstSvpnContext, uiEvent);
    }

    return eRet;
}

static BS_STATUS _svpn_context_kf_IsExist(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN KFAPP_PARAM_S *pstParam)
{
    SVPN_CfgLock_RLock();
    JSON_NO_IsExist(g_hSvpnContextNo, hMime, pstParam->pstJson);
    SVPN_CfgLock_RUnLock();

    return BS_OK;
}

static VOID _svpn_context_kf_ProcessAdd(IN MIME_HANDLE hMime, IN KFAPP_PARAM_S *pstParam)
{
    CHAR *pcName;
    CHAR *pcWsService;
    CHAR *pcAdmin;
    CHAR *pcAdminPassword;
    SVPN_CONTEXT_HANDLE hContext;

    pcName = MIME_GetKeyValue(hMime, "Name");

    if (TXT_IS_EMPTY(pcName))
    {
        JSON_SetFailed(pstParam->pstJson, "Empty name");
        return;
    }

    if (NULL != NO_GetObjectByName(g_hSvpnContextNo, pcName))
    {
        JSON_SetFailed(pstParam->pstJson, "Already exist");
        return;
    }

    if (BS_OK != SVPN_Context_AddContext(pcName))
    {
        JSON_SetFailed(pstParam->pstJson, "Add failed");
        return;
    }

    SVPN_Context_SetDescription(pcName, MIME_GetKeyValue(hMime, "Description"));

    pcWsService = MIME_GetKeyValue(hMime, "WsService");
    if (! TXT_IS_EMPTY(pcWsService))
    {
        if (BS_OK != SVPN_Context_BindService(pcName, pcWsService))
        {
            JSON_SetFailed(pstParam->pstJson, "Bind WS Service failed");
            SVPN_Context_DelContext(pcName);
            return;
        }
    }

    pcAdmin = MIME_GetKeyValue(hMime, "Admin");
    pcAdminPassword = MIME_GetKeyValue(hMime, "AdminPassword");
    if (! TXT_IS_EMPTY(pcAdmin))
    {
        hContext = SVPN_Context_GetByName(pcName);
        SVPN_LocalAdmin_AddUser(hContext, pcAdmin, pcAdminPassword);
    }

    JSON_SetSuccess(pstParam->pstJson);
    return;
}

static BS_STATUS _svpn_context_kf_Add(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN KFAPP_PARAM_S *pstParam)
{
    SVPN_CfgLock_WLock();
    _svpn_context_kf_ProcessAdd(hMime, pstParam);
    SVPN_CfgLock_WUnLock();

    return BS_OK;
}

static VOID _svpn_context_kf_ProcessModify(IN MIME_HANDLE hMime, IN KFAPP_PARAM_S *pstParam)
{
    CHAR *pcName;
    CHAR *pcWsService;
    CHAR *pcAdmin;
    CHAR *pcAdminPassword;
    CHAR szOldAdmin[SVPN_MAX_USER_NAME_LEN + 1];
    SVPN_CONTEXT_HANDLE hContext;

    pcName = MIME_GetKeyValue(hMime, "Name");
    if (TXT_IS_EMPTY(pcName))
    {
        JSON_SetFailed(pstParam->pstJson, "Empty name");
        return;
    }

    if (NULL == NO_GetObjectByName(g_hSvpnContextNo, pcName))
    {
        JSON_SetFailed(pstParam->pstJson, "Not exist");
        return;
    }

    SVPN_Context_SetDescription(pcName, MIME_GetKeyValue(hMime, "Description"));

    pcWsService = MIME_GetKeyValue(hMime, "WsService");
    if (BS_OK != SVPN_Context_BindService(pcName, pcWsService))
    {
        JSON_SetFailed(pstParam->pstJson, "Bind WS Service failed");
        return;
    }

    hContext = SVPN_Context_GetByName(pcName);

    pcAdmin = MIME_GetKeyValue(hMime, "Admin");
    if (TXT_IS_EMPTY(pcAdmin))
    {
        JSON_SetSuccess(pstParam->pstJson);
        return;
    }

    pcAdminPassword = MIME_GetKeyValue(hMime, "AdminPassword");

    if (BS_OK != SVPN_LocalAdmin_GetNext(hContext, NULL, szOldAdmin))
    {
        szOldAdmin[0] = '\0';
    }

    if (strcmp(pcAdmin, szOldAdmin) != 0)
    {
        if (szOldAdmin[0] != '\0')
        {
            SVPN_LocalAdmin_DelUser(hContext, szOldAdmin);
        }
    }

    SVPN_LocalAdmin_SetUser(hContext, pcAdmin, pcAdminPassword);

    JSON_SetSuccess(pstParam->pstJson);
    return;
}

static BS_STATUS _svpn_context_kf_Modify(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN KFAPP_PARAM_S *pstParam)
{
    SVPN_CfgLock_WLock();
    _svpn_context_kf_ProcessModify(hMime, pstParam);
    SVPN_CfgLock_WUnLock();

	return BS_OK;
}

static BS_STATUS _svpn_context_kf_Get(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN KFAPP_PARAM_S *pstParam)
{
    SVPN_CfgLock_RLock();
    JSON_NO_Get(g_hSvpnContextNo, hMime, pstParam->pstJson, g_apcSvpnContextProperty);
    SVPN_CfgLock_RUnLock();

	return BS_OK;
}

static BOOL_T _svpn_context_DelNotify(IN HANDLE hUserHandle, IN CHAR *pcNodeName)
{
    SVPN_Context_DelContext(pcNodeName);

    /* 已经删除了,无需再删除一次,所以返回FALSE */
    return FALSE;
}

static BS_STATUS _svpn_context_kf_Del(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN KFAPP_PARAM_S *pstParam)
{
    SVPN_CfgLock_WLock();
    JSON_NO_DeleteWithNotify(g_hSvpnContextNo, hMime, pstParam->pstJson, _svpn_context_DelNotify, NULL);
    SVPN_CfgLock_WUnLock();

	return BS_OK;
}

static BS_STATUS _svpn_context_kf_List(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN KFAPP_PARAM_S *pstParam)
{
    BS_STATUS eRet;
    
    SVPN_CfgLock_RLock();
    eRet = JSON_NO_List(g_hSvpnContextNo, pstParam->pstJson, g_apcSvpnContextProperty);
    SVPN_CfgLock_RUnLock();

    return eRet;
}

VOID SVPN_ContextKf_Init()
{
    KFAPP_RegFunc("svpn.context.IsExist", _svpn_context_kf_IsExist, NULL);
    KFAPP_RegFunc("svpn.context.Add", _svpn_context_kf_Add, NULL);
    KFAPP_RegFunc("svpn.context.Modify", _svpn_context_kf_Modify, NULL);
    KFAPP_RegFunc("svpn.context.Get", _svpn_context_kf_Get, NULL);
    KFAPP_RegFunc("svpn.context.Delete", _svpn_context_kf_Del, NULL);
    KFAPP_RegFunc("svpn.context.List", _svpn_context_kf_List, NULL);
}

BS_STATUS SVPN_Context_Init()
{
    OBJECT_PARAM_S obj_param = {0};

    obj_param.uiMaxNum = SVPN_CONTEXT_MAX_NUM;
    obj_param.uiObjSize = sizeof(SVPN_CONTEXT_S);

    g_hSvpnContextNo = NO_CreateAggregate(&obj_param);
    if (NULL == g_hSvpnContextNo)
    {
        return BS_ERR;
    }

    return BS_OK;
}

CHAR * SVPN_Context_GetName(IN SVPN_CONTEXT_HANDLE hSvpnContext)
{
    SVPN_CONTEXT_S *pstSvpnContext = hSvpnContext;

    return NO_GetName(pstSvpnContext);
}

VOID SVPN_Context_RegEvent(IN PF_SVPN_CONTEXT_IssuEvent pfFunc)
{
    SVPN_CONTEXT_LISTENER_S *pstListener;

    pstListener = MEM_ZMalloc(sizeof(SVPN_CONTEXT_LISTENER_S));
    if (NULL == pstListener)
    {
        return;
    }

    pstListener->pfFunc = pfFunc;

    DLL_ADD(&g_stSvpnContextListenerList, pstListener);
}

VOID SVPN_Context_SetUserData(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN UINT uiIndex, IN VOID *pData)
{
    SVPN_CONTEXT_S *pstSvpnContext = hSvpnContext;

    if (uiIndex >= SVPN_CONTEXT_DATA_INDEX_MAX)
    {
        BS_DBGASSERT(0);
        return;
    }

    pstSvpnContext->apUserData[uiIndex] = pData;
}

VOID * SVPN_Context_GetUserData(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN UINT uiIndex)
{
    SVPN_CONTEXT_S *pstSvpnContext = hSvpnContext;

    if (uiIndex >= SVPN_CONTEXT_DATA_INDEX_MAX)
    {
        BS_DBGASSERT(0);
        return NULL;
    }

    return pstSvpnContext->apUserData[uiIndex];
}

BS_STATUS SVPN_Context_AddContext(IN CHAR *pcContextName)
{
    SVPN_CONTEXT_S *pstContext;
    CHAR *pcAutoWsService;
    CHAR szDomainName[512];

    if (NULL != NO_GetObjectByName(g_hSvpnContextNo, pcContextName))
    {
        return BS_OK;
    }

    pstContext = NO_NewObject(g_hSvpnContextNo, pcContextName);
    if (NULL == pstContext)
    {
        return BS_ERR;
    }

    pcAutoWsService = WSAPP_AddAutoNameService(WSAPP_SERVICE_FLAG_SAVE_HIDE
            | WSAPP_SERVICE_FLAG_WEB_HIDE
            | WSAPP_SERVICE_FLAG_WEB_READONLY);

    if (NULL != pcAutoWsService)
    {
        snprintf(szDomainName, sizeof(szDomainName), "_svpn_%s", pcContextName);
        WSAPP_BindGw(pcAutoWsService, WSAPP_INNER_GW_NAME, NULL, szDomainName);
        _svpn_context_BindServiceExt(pcContextName, pcAutoWsService, "AutoWsService");
    }

    _svpn_context_IssuEvent(pstContext, SVPN_CONTEXT_EVENT_CREATE);

    return BS_OK;
}

BS_STATUS SVPN_Context_DelContext(IN CHAR *pcContextName)
{
    SVPN_CONTEXT_S *pstContext;
    CHAR *pcWsService;

    pstContext = NO_GetObjectByName(g_hSvpnContextNo, pcContextName);
    if (NULL == pstContext)
    {
        return BS_OK;
    }

    pcWsService = NO_GetKeyValue(pstContext, "WsService");
    if (! TXT_IS_EMPTY(pcWsService))
    {
        WSAPP_UnBindService(pcWsService);
    }

    pcWsService = NO_GetKeyValue(pstContext, "AutoWsService");
    if (! TXT_IS_EMPTY(pcWsService))
    {
        WSAPP_UnBindService(pcWsService);
        WSAPP_DelService(pcWsService);
    }

    _svpn_context_IssuEvent(pstContext, SVPN_CONTEXT_EVENT_DESTROY);

    NO_FreeObject(g_hSvpnContextNo, pstContext);

    return BS_OK;
}

BS_STATUS SVPN_Context_BindService(IN CHAR *pcContextName, IN CHAR *pcWsService)
{
    return _svpn_context_BindServiceExt(pcContextName, pcWsService, "WsService");
}

BS_STATUS SVPN_Context_SetDescription(IN CHAR *pcContextName, IN CHAR *pcDesc)
{
    SVPN_CONTEXT_S *pstContext;

    if (pcDesc == NULL)
    {
        pcDesc = "";
    }

    pstContext = NO_GetObjectByName(g_hSvpnContextNo, pcContextName);
    if (NULL == pstContext)
    {
        return BS_NOT_FOUND;
    }

    NO_SetKeyValue(g_hSvpnContextNo, pstContext, "Description", pcDesc);

    return BS_OK;
}

SVPN_CONTEXT_HANDLE SVPN_Context_GetByID(IN UINT64 uiContextID)
{
    return NO_GetObjectByID(g_hSvpnContextNo, uiContextID);
}

SVPN_CONTEXT_HANDLE SVPN_Context_GetByName(IN CHAR *pcName)
{
    return NO_GetObjectByName(g_hSvpnContextNo, pcName);
}

CHAR * SVPN_Context_GetNameByID(IN UINT64 uiContextID)
{
    return NO_GetNameByID(g_hSvpnContextNo, uiContextID);
}

CHAR * SVPN_Context_GetWsService(IN SVPN_CONTEXT_HANDLE hSvpnContext)
{
    return NO_GetKeyValue(hSvpnContext, "WsService");
}

CHAR * SVPN_Context_GetDescription(IN SVPN_CONTEXT_HANDLE hSvpnContext)
{
    return NO_GetKeyValue(hSvpnContext, "Description");
}

UINT64 SVPN_Context_GetNextID(IN UINT64 uiCurCtxId)
{
    return NO_GetNextID(g_hSvpnContextNo, uiCurCtxId);
}

SVPN_CONTEXT_HANDLE SVPN_Context_GetContextByWsTrans(IN WS_TRANS_HANDLE hWsTrans)
{
    WS_CONTEXT_HANDLE hWsContext;
    UINT64 uiSvpnContextID;

    hWsContext = WS_Trans_GetContext(hWsTrans);
    uiSvpnContextID = WSAPP_GetUserDataByWsContext(hWsContext);

    return SVPN_Context_GetByID(uiSvpnContextID);
}

