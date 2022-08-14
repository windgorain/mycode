/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2015-12-3
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/txt_utl.h"
#include "utl/exec_utl.h"
#include "utl/lstr_utl.h"
#include "utl/ws_utl.h"
#include "utl/time_utl.h"
#include "utl/rand_utl.h"
#include "utl/json_utl.h"
#include "utl/object_utl.h"
#include "utl/bit_opt.h"
#include "comp/comp_kfapp.h"

#include "wsapp_def.h"
#include "wsapp_gw.h"
#include "wsapp_worker.h"
#include "wsapp_master.h"
#include "wsapp_service.h"
#include "wsapp_cfglock.h"
#include "wsapp_service_cfg.h"

#define WSAPP_SERVICE_MAX_NUM 1024

#define _WSAPP_SERVICE_FLAG_BINDED   0x80000000 /* 已经被Bind */

static NO_HANDLE g_hWsAppServiceNo = NULL;

static CHAR *g_apcWsappServiceProperty[]
    = {"Description", "Enable", NULL};

static WSAPP_SERVICE_S * wsapp_service_Find(IN CHAR *pcServiceName)
{
    return NO_GetObjectByName(g_hWsAppServiceNo, pcServiceName);
}

static BS_STATUS wsapp_service_NoBindGatewayByString
(
    IN CHAR *pcService,
    IN LSTR_S *pstString
)
{
    LSTR_S astStr[3];
    CHAR szGateway[WSAPP_GW_NAME_LEN + 1];
    CHAR szVHost[WS_VHOST_MAX_LEN + 1];
    CHAR szDomain[WS_DOMAIN_MAX_LEN + 1];

    Mem_Zero(astStr, sizeof(astStr));
    LSTR_XSplit(pstString, '/', astStr, 3);

    LSTR_Lstr2Str(&astStr[0], szGateway, sizeof(szGateway));
    LSTR_Lstr2Str(&astStr[1], szVHost, sizeof(szVHost));
    LSTR_Lstr2Str(&astStr[2], szDomain, sizeof(szDomain));

    return WSAPP_Service_NoBindGateway(pcService, szGateway, szVHost, szDomain);
}

static BS_STATUS _wsapp_service_kf_IsExist(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN KFAPP_PARAM_S *pstParam)
{
    WSAPP_CfgLock_RLock();
    JSON_NO_IsExist(g_hWsAppServiceNo, hMime, pstParam->pstJson);
    WSAPP_CfgLock_RUnLock();

    return BS_OK;
}

static VOID _wsapp_service_UnbindAllGateway(IN CHAR *pcServiceName)
{
    WSAPP_SERVICE_S *pstService;
    UINT i;

    pstService = wsapp_service_Find(pcServiceName);
    if (NULL == pstService)
    {
        return;
    }

    for (i=0; i<WSAPP_SERVICE_MAX_BIND_NUM; i++)
    {
        if (pstService->astBindGateWay[i].hWsContext != NULL)
        {
            WSAPP_GW_DelService(pstService->astBindGateWay[i].szBindGwName, pstService->astBindGateWay[i].hWsContext);
        }
        pstService->astBindGateWay[i].hWsContext = NULL;
        pstService->astBindGateWay[i].szBindGwName[0] = '\0';
    }
}

static BOOL_T _wsapp_service_kf_DelNotify(IN HANDLE hUserHandle, IN CHAR *pcDelName)
{
    WSAPP_SERVICE_S *pstService;
    CHAR szInfo[128];
    KFAPP_PARAM_S *pstParam = hUserHandle;

    pstService = wsapp_service_Find(pcDelName);
    if (NULL == pstService)
    {
        return FALSE;
    }

    if (pstService->uiFlag & WSAPP_SERVICE_FLAG_WEB_READONLY)
    {
        snprintf(szInfo, sizeof(szInfo), "%s is readonly", pcDelName);
        JSON_AppendInfo(pstParam->pstJson, szInfo);
        return FALSE;
    }

    if (pstService->uiFlag & _WSAPP_SERVICE_FLAG_BINDED)
    {
        snprintf(szInfo, sizeof(szInfo), "%s is used", pcDelName);
        JSON_AppendInfo(pstParam->pstJson, szInfo);
        return FALSE;
    }

    WSAPP_Service_Stop(pcDelName);

    _wsapp_service_UnbindAllGateway(pcDelName);

    return TRUE;
}

static BS_STATUS _wsapp_service_kf_Del(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN KFAPP_PARAM_S *pstParam)
{
    WSAPP_CfgLock_WLock();
    JSON_NO_DeleteWithNotify(g_hWsAppServiceNo, hMime, pstParam->pstJson, _wsapp_service_kf_DelNotify, pstParam);
    WSAPP_CfgLock_WUnLock();

	return BS_OK;
}

static BOOL_T _wsapp_service_kf_ListIsPermit(IN HANDLE hUserHandle, IN UINT64 ulNodeID)
{
    WSAPP_SERVICE_S *pstService;

    pstService = NO_GetObjectByID(g_hWsAppServiceNo, ulNodeID);
    if (NULL == pstService)
    {
        return FALSE;
    }

    if (pstService->uiFlag & WSAPP_SERVICE_FLAG_WEB_HIDE)
    {
        return FALSE;
    }

    return TRUE;
}

static BS_STATUS _wsapp_service_kf_List(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN KFAPP_PARAM_S *pstParam)
{
    BS_STATUS eRet;
    
    WSAPP_CfgLock_RLock();
    eRet = JSON_NO_ListWithCallBack(g_hWsAppServiceNo, pstParam->pstJson,
        g_apcWsappServiceProperty, _wsapp_service_kf_ListIsPermit, NULL);
    WSAPP_CfgLock_RUnLock();

    return eRet;
}

static BS_STATUS _wsapp_service_kf_AddProcess(IN MIME_HANDLE hMime, IN KFAPP_PARAM_S *pstParam)
{
    WSAPP_SERVICE_S *pstService;
    CHAR *pcTmp;
    CHAR *pcName;
    UINT i;

    pcName = MIME_GetKeyValue(hMime, "Name");
    if (NULL == pcName)
    {
        JSON_SetFailed(pstParam->pstJson, "Bad param");
        return BS_ERR;
    }

    if (NULL != WSAPP_Service_GetByName(pcName))
    {
        JSON_SetFailed(pstParam->pstJson, "Already exist");
        return BS_ERR;
    }

    pstService = WSAPP_Service_Add(pcName);
    if (pstService == NULL)
    {
        JSON_SetFailed(pstParam->pstJson, "Add failed");
        return BS_ERR;
    }

    for (i=0; g_apcWsappServiceProperty[i]!=NULL; i++)
    {
        pcTmp = MIME_GetKeyValue(hMime, g_apcWsappServiceProperty[i]);
        if (pcTmp != NULL)
        {
            NO_SetKeyValue(g_hWsAppServiceNo, pstService, g_apcWsappServiceProperty[i], pcTmp);
        }
    }

    pcTmp = MIME_GetKeyValue(hMime, "Enable");
    if ((! TXT_IS_EMPTY(pcTmp)) && (pcTmp[0] == '1'))
    {
        pstService->bStart = TRUE;
    }

    JSON_SetSuccess(pstParam->pstJson);

    return BS_OK;
}

static BS_STATUS _wsapp_service_kf_Add(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN KFAPP_PARAM_S *pstParam)
{
    WSAPP_CfgLock_WLock();
    _wsapp_service_kf_AddProcess(hMime, pstParam);
    WSAPP_CfgLock_WUnLock();

    return BS_OK;
}

static BS_STATUS _wsapp_service_kf_ModifyProcess(IN MIME_HANDLE hMime, IN KFAPP_PARAM_S *pstParam)
{
    WSAPP_SERVICE_S *pstService;
    CHAR *pcTmp;
    CHAR *pcName;
    UINT i;

    pcName = MIME_GetKeyValue(hMime, "Name");
    if (NULL == pcName)
    {
        JSON_SetFailed(pstParam->pstJson, "Bad param");
        return BS_ERR;
    }

    pstService = WSAPP_Service_GetByName(pcName);
    if (NULL == pstService)
    {
        JSON_SetFailed(pstParam->pstJson, "Not found");
        return BS_ERR;
    }

    if (pstService->uiFlag & WSAPP_SERVICE_FLAG_WEB_READONLY)
    {
        JSON_SetFailed(pstParam->pstJson, "Not permit");
        return BS_NO_PERMIT;
    }

    for (i=0; g_apcWsappServiceProperty[i]!=NULL; i++)
    {
        pcTmp = MIME_GetKeyValue(hMime, g_apcWsappServiceProperty[i]);
        if (pcTmp != NULL)
        {
            NO_SetKeyValue(g_hWsAppServiceNo, pstService, g_apcWsappServiceProperty[i], pcTmp);
        }
    }

    pcTmp = MIME_GetKeyValue(hMime, "Enable");
    if ((! TXT_IS_EMPTY(pcTmp)) && (pcTmp[0] == '1'))
    {
        pstService->bStart = TRUE;
    }
    else
    {
        pstService->bStart = FALSE;
    }

    JSON_SetSuccess(pstParam->pstJson);

    return BS_OK;
}

static BS_STATUS _wsapp_service_kf_Modify(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN KFAPP_PARAM_S *pstParam)
{
    WSAPP_CfgLock_WLock();
    _wsapp_service_kf_ModifyProcess(hMime, pstParam);
    WSAPP_CfgLock_WUnLock();

	return BS_OK;
}

static BS_STATUS _wsapp_service_kf_Get(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN KFAPP_PARAM_S *pstParam)
{
    WSAPP_CfgLock_RLock();
    JSON_NO_Get(g_hWsAppServiceNo, hMime, pstParam->pstJson, g_apcWsappServiceProperty);
    WSAPP_CfgLock_RUnLock();

	return BS_OK;
}

static VOID _wsapp_servicebindgw_kf_ListProcess(IN MIME_HANDLE hMime, IN KFAPP_PARAM_S *pstParam)
{
    cJSON *pstArray;
    cJSON *pstResJson;
    UINT i;
    WSAPP_SERVICE_S *pstService;
    CHAR *pcService;
    CHAR szInfo[512];

    pcService = MIME_GetKeyValue(hMime, "Service");
    if (NULL == pcService)
    {
        JSON_SetFailed(pstParam->pstJson, "Bad param");
        return;
    }

    pstService = WSAPP_Service_GetByName(pcService);
    if (NULL == pstService)
    {
        JSON_SetFailed(pstParam->pstJson, "Not found");
        return;
    }

    pstArray = cJSON_CreateArray();
    if (NULL == pstArray)
    {
        JSON_SetFailed(pstParam->pstJson, "Not enough memory");
        return;
    }

    for (i=0; i<WSAPP_SERVICE_MAX_BIND_NUM; i++)
    {
        if (pstService->astBindGateWay[i].szBindGwName[0] == '\0')
        {
            continue;
        }
        
        pstResJson = cJSON_CreateObject();
        if (NULL == pstResJson)
        {
            continue;
        }

        sprintf(szInfo, "%s/%s/%s", pstService->astBindGateWay[i].szBindGwName,
            pstService->astBindGateWay[i].szVHostName, pstService->astBindGateWay[i].szDomain);

        cJSON_AddStringToObject(pstResJson, "Name", szInfo);
        cJSON_AddStringToObject(pstResJson, "Gateway", pstService->astBindGateWay[i].szBindGwName);
        cJSON_AddStringToObject(pstResJson, "VHost", pstService->astBindGateWay[i].szVHostName);
        cJSON_AddStringToObject(pstResJson, "Domain", pstService->astBindGateWay[i].szDomain);

        cJSON_AddItemToArray(pstArray, pstResJson);
    }

    cJSON_AddItemToObject(pstParam->pstJson, "data", pstArray);

    JSON_SetSuccess(pstParam->pstJson);

    return;
}

static BS_STATUS _wsapp_servicebindgw_kf_List(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN KFAPP_PARAM_S *pstParam)
{
    WSAPP_CfgLock_RLock();
    _wsapp_servicebindgw_kf_ListProcess(hMime, pstParam);
    WSAPP_CfgLock_RUnLock();

    return BS_OK;
}

static VOID _wsapp_servicebindgw_kf_AddProcess(IN MIME_HANDLE hMime, IN KFAPP_PARAM_S *pstParam)
{
    CHAR *pcService;
    CHAR *pcGateway;
    CHAR *pcVHost;
    CHAR *pcDomain;
    BS_STATUS eRet;

    pcService = MIME_GetKeyValue(hMime, "Service");
    pcGateway = MIME_GetKeyValue(hMime, "Gateway");
    if ((TXT_IS_EMPTY(pcService)) || (TXT_IS_EMPTY(pcGateway)))
    {
        JSON_SetFailed(pstParam->pstJson, "Bad param");
        return;
    }

    pcVHost = MIME_GetKeyValue(hMime, "VHost");
    pcDomain = MIME_GetKeyValue(hMime, "Domain");

    if (TXT_IS_EMPTY(pcVHost))
    {
        pcVHost = NULL;
    }

    if (TXT_IS_EMPTY(pcDomain))
    {
        pcDomain = NULL;
    }

    eRet = WSAPP_Service_BindGateway(pcService, pcGateway, pcVHost, pcDomain);
    if (eRet == BS_OUT_OF_RANGE)
    {
        JSON_SetFailed(pstParam->pstJson, "Exceed");
        return;
    }

    if (eRet == BS_NOT_FOUND)
    {
        JSON_SetFailed(pstParam->pstJson, "Not found");
        return;
    }

    if (eRet != BS_OK)
    {
        JSON_SetFailed(pstParam->pstJson, "Conflict");
        return;
    }

    JSON_SetSuccess(pstParam->pstJson);

    return;
}

static BS_STATUS _wsapp_servicebindgw_kf_Add(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN KFAPP_PARAM_S *pstParam)
{
    WSAPP_CfgLock_WLock();
    _wsapp_servicebindgw_kf_AddProcess(hMime, pstParam);
    WSAPP_CfgLock_WUnLock();

    return BS_OK;
}

static VOID _wsapp_servicebindgw_kf_DelProcess(IN MIME_HANDLE hMime, IN KFAPP_PARAM_S *pstParam)
{
    CHAR *pcNames;
    CHAR *pcService;
    LSTR_S stName;
    CHAR szName[512];

    pcService = MIME_GetKeyValue(hMime, "Service");
    if (TXT_IS_EMPTY(pcService))
    {
        JSON_SetFailed(pstParam->pstJson, "Bad param");
        return;
    }

    pcNames = MIME_GetKeyValue(hMime, "Delete");
    if (NULL == pcNames)
    {
        JSON_SetSuccess(pstParam->pstJson);
        return;
    }

    LSTR_SCAN_ELEMENT_BEGIN(pcNames, strlen(pcNames), ',', &stName)
    {
        if ((stName.uiLen != 0) && (stName.uiLen < sizeof(szName)))
        {
            wsapp_service_NoBindGatewayByString(pcService, &stName);
        }
    }LSTR_SCAN_ELEMENT_END();

    JSON_SetSuccess(pstParam->pstJson);

    return;
}

static BS_STATUS _wsapp_servicebindgw_kf_Del(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN KFAPP_PARAM_S *pstParam)
{
    WSAPP_CfgLock_WLock();
    _wsapp_servicebindgw_kf_DelProcess(hMime, pstParam);
    WSAPP_CfgLock_WUnLock();

	return BS_OK;
}

static VOID _wsapp_service_kf_Init()
{
    KFAPP_RegFunc("ws.service.IsExist", _wsapp_service_kf_IsExist, NULL);
    KFAPP_RegFunc("ws.service.Add", _wsapp_service_kf_Add, NULL);
    KFAPP_RegFunc("ws.service.Modify", _wsapp_service_kf_Modify, NULL);
    KFAPP_RegFunc("ws.service.Get", _wsapp_service_kf_Get, NULL);
    KFAPP_RegFunc("ws.service.Delete", _wsapp_service_kf_Del, NULL);
    KFAPP_RegFunc("ws.service.List", _wsapp_service_kf_List, NULL);

    KFAPP_RegFunc("ws.service.bindgw.Add", _wsapp_servicebindgw_kf_Add, NULL);
    KFAPP_RegFunc("ws.service.bindgw.Delete", _wsapp_servicebindgw_kf_Del, NULL);
    KFAPP_RegFunc("ws.service.bindgw.List", _wsapp_servicebindgw_kf_List, NULL);
}

BS_STATUS WSAPP_Service_Init()
{
    OBJECT_PARAM_S no_p = {0};

    no_p.uiMaxNum = WSAPP_SERVICE_MAX_NUM;
    no_p.uiObjSize = sizeof(WSAPP_SERVICE_S);

    g_hWsAppServiceNo = NO_CreateAggregate(&no_p);
    if (NULL == g_hWsAppServiceNo)
    {
        return BS_ERR;
    }

    _wsapp_service_kf_Init();

    return BS_OK;
}

CHAR * WSAPP_Service_GetName(IN HANDLE hService)
{
    return NO_GetName(hService);
}

/* 创建一个自动命名的Service, 并返回其名字 */
CHAR * WSAPP_Service_AddAutoNameService(IN UINT uiFlag)
{
    CHAR szAutoName[64];
    WSAPP_SERVICE_S * pstService;

    do {
        snprintf(szAutoName, sizeof(szAutoName), "@auto-%lx-%x", TM_NowInSec(), RAND_Get());
    }while (NULL != wsapp_service_Find(szAutoName));

    pstService = WSAPP_Service_Add(szAutoName);
    if (NULL == pstService)
    {
        return NULL;
    }

    pstService->uiFlag = uiFlag;

    return NO_GetName(pstService);
}

WSAPP_SERVICE_S * WSAPP_Service_Add(IN CHAR *pcServiceName)
{
    WSAPP_SERVICE_S *pstService;

    if (NULL != wsapp_service_Find(pcServiceName))
    {
        return NULL;
    }

    pstService = NO_NewObject(g_hWsAppServiceNo, pcServiceName);
    if (NULL == pstService)
    {
        return NULL;
    }

    return pstService;
}

BS_STATUS WSAPP_Service_Del(IN CHAR *pcServiceName)
{
    WSAPP_SERVICE_S *pstService;

    pstService = wsapp_service_Find(pcServiceName);
    if (NULL == pstService)
    {
        return BS_OK;
    }

    if (pstService->uiFlag & _WSAPP_SERVICE_FLAG_BINDED)
    {
        return BS_REF_NOT_ZERO;
    }

    WSAPP_Service_Stop(pcServiceName);

    _wsapp_service_UnbindAllGateway(pcServiceName);

    NO_FreeObject(g_hWsAppServiceNo, pstService);

    return BS_OK;
}

/* 设置webcenter的操作属性: Hide:对webcenter隐藏; readonly:对webcenter只读 */
BS_STATUS WSAPP_Service_SetWebCenterOpt(IN CHAR *pcServiceName, IN CHAR *pcOpt)
{
    WSAPP_SERVICE_S *pstService;

    pstService = wsapp_service_Find(pcServiceName);
    if (NULL == pstService)
    {
        return BS_ERR;
    }

    if (strcmp(pcOpt, "hide") == 0)
    {
        BIT_SET(pstService->uiFlag, WSAPP_SERVICE_FLAG_WEB_HIDE);
    }
    else if (strcmp(pcOpt, "readonly") == 0)
    {
        BIT_SET(pstService->uiFlag, WSAPP_SERVICE_FLAG_WEB_READONLY);
    }

    return BS_OK;
}

BS_STATUS WSAPP_Service_ClrWebCenterOpt(IN CHAR *pcServiceName, IN CHAR *pcOpt)
{
    WSAPP_SERVICE_S *pstService;

    pstService = wsapp_service_Find(pcServiceName);
    if (NULL == pstService)
    {
        return BS_ERR;
    }

    if (strcmp(pcOpt, "hide") == 0)
    {
        BIT_CLR(pstService->uiFlag, WSAPP_SERVICE_FLAG_WEB_HIDE);
    }
    else if (strcmp(pcOpt, "readonly") == 0)
    {
        BIT_CLR(pstService->uiFlag, WSAPP_SERVICE_FLAG_WEB_READONLY);
    }

    return BS_OK;
}

BS_STATUS WSAPP_Service_SetDescription(IN CHAR *pcServiceName, IN CHAR *pcDesc)
{
    WSAPP_SERVICE_S *pstService;

    pstService = wsapp_service_Find(pcServiceName);
    if (NULL == pstService)
    {
        return BS_ERR;
    }

    NO_SetKeyValue(g_hWsAppServiceNo, pstService, "Description", pcDesc);

    return BS_OK;
}

static WSAPP_SERVICE_BIND_INFO_S * wsapp_FindBindNode
(
    IN WSAPP_SERVICE_S *pstService,
    IN CHAR *pcGateWay,
    IN CHAR *pcVHost,
    IN CHAR *pcDomain
)
{
    UINT i;

    for (i=0; i<WSAPP_SERVICE_MAX_BIND_NUM; i++)
    {
        if ((strcmp(pcGateWay, pstService->astBindGateWay[i].szBindGwName) == 0)
            && (strcmp(pcVHost, pstService->astBindGateWay[i].szVHostName) == 0)
            && (strcmp(pcDomain, pstService->astBindGateWay[i].szDomain) == 0))
        {
            return &pstService->astBindGateWay[i];
        }
    }

    return NULL;
}

/* 获得一个空闲的位置 */
static WSAPP_SERVICE_BIND_INFO_S * wsapp_GetBindNode(IN WSAPP_SERVICE_S *pstService)
{
    UINT i;

    for (i=0; i<WSAPP_SERVICE_MAX_BIND_NUM; i++)
    {
        if (pstService->astBindGateWay[i].szBindGwName[0] == 0)
        {
            return &pstService->astBindGateWay[i];
        }
    }

    return NULL;
}

BS_STATUS WSAPP_Service_BindGateway
(
    IN CHAR *pcService,
    IN CHAR *pcGateWay,
    IN CHAR *pcVHost,
    IN CHAR *pcDomain
)
{
    WSAPP_SERVICE_S *pstService;
    WS_CONTEXT_HANDLE hWsContext;
    WSAPP_SERVICE_BIND_INFO_S * pstBindNode;

    if (pcVHost == NULL)
    {
        pcVHost = "";
    }

    if (pcDomain == NULL)
    {
        pcDomain = "";
    }

    pstService = wsapp_service_Find(pcService);
    if (NULL == pstService)
    {
        EXEC_OutInfo("Service %s is not exist.\r\n", pcService);
        return BS_NOT_FOUND;
    }

    pstBindNode = wsapp_GetBindNode(pstService);
    if (NULL == pstBindNode)
    {
        EXEC_OutInfo("The binded gateway has up to limit.\r\n");
        return BS_OUT_OF_RANGE;
    }

    hWsContext = WSAPP_GW_AddService(pcGateWay, pcVHost, pcDomain);
    if (NULL == hWsContext)
    {
        EXEC_OutInfo("Bind gateway failed. Vhost+domain maybe conflict.\r\n");
        return BS_ERR;
    }

    pstBindNode->hWsContext = hWsContext;
    WS_Context_SetUserData(hWsContext, UINT_HANDLE(NO_GetObjectID(g_hWsAppServiceNo, pstService)));

    if (NULL != pstService->hDeliverTbl)
    {
        WS_Context_BindDeliverTbl(hWsContext, pstService->hDeliverTbl);
    }

    if ('\0' != pstService->szRootPath[0])
    {
        WS_Context_SetRootPath(hWsContext, pstService->szRootPath);
    }
    WS_Context_SetSecRootPath(hWsContext, "web/");

    if ('\0' != pstService->szIndex[0])
    {
        WS_Context_SetIndex(hWsContext, pstService->szIndex);
    }

    TXT_Strlcpy(pstBindNode->szBindGwName, pcGateWay, sizeof(pstBindNode->szBindGwName));
    TXT_Strlcpy(pstBindNode->szVHostName, pcVHost, sizeof(pstBindNode->szVHostName));
    TXT_Strlcpy(pstBindNode->szDomain, pcDomain, sizeof(pstBindNode->szDomain));

    return BS_OK;
}

BS_STATUS WSAPP_Service_NoBindGateway
(
    IN CHAR *pcService,
    IN CHAR *pcGateWay,
    IN CHAR *pcVHost,
    IN CHAR *pcDomain
)
{
    WSAPP_SERVICE_S *pstService;
    WSAPP_SERVICE_BIND_INFO_S * pstBindNode;

    pstService = wsapp_service_Find(pcService);
    if (NULL == pstService)
    {
        return BS_OK;
    }

    pstBindNode = wsapp_FindBindNode(pstService, pcGateWay, pcVHost, pcDomain);
    if (NULL == pstBindNode)
    {
        return BS_OK;
    }

    if (pstBindNode->hWsContext != NULL)
    {
        WSAPP_GW_DelService(pcGateWay, pstBindNode->hWsContext);
    }

    Mem_Zero(pstBindNode, sizeof(WSAPP_SERVICE_BIND_INFO_S));
    
    return BS_OK;
}

BS_STATUS WSAPP_SetDocRoot(IN CHAR *pcServiceName, IN CHAR *pcDocRoot)
{
    WSAPP_SERVICE_S *pstService;
    UINT i;

    if (NULL == pcDocRoot)
    {
        pcDocRoot = "";
    }
 
    pstService = wsapp_service_Find(pcServiceName);
    if (NULL == pstService)
    {
        return BS_NOT_FOUND;
    }

    TXT_Strlcpy(pstService->szRootPath, pcDocRoot, sizeof(pstService->szRootPath));

    for (i=0; i<WSAPP_SERVICE_MAX_BIND_NUM; i++)
    {
        if (pstService->astBindGateWay[i].hWsContext != NULL)
        {
            WS_Context_SetRootPath(pstService->astBindGateWay[i].hWsContext, pcDocRoot);
        }
    }

    return BS_OK;
}

BS_STATUS WSAPP_SetIndex(IN CHAR *pcServiceName, IN CHAR *pcIndex)
{
    WSAPP_SERVICE_S *pstService;
    UINT i;

    if (NULL == pcIndex)
    {
        pcIndex = "";
    }
 
    pstService = wsapp_service_Find(pcServiceName);
    if (NULL == pstService)
    {
        return BS_NOT_FOUND;
    }

    TXT_Strlcpy(pstService->szIndex, pcIndex, sizeof(pstService->szIndex));

    for (i=0; i<WSAPP_SERVICE_MAX_BIND_NUM; i++)
    {
        if (pstService->astBindGateWay[i].hWsContext != NULL)
        {
            WS_Context_SetIndex(pstService->astBindGateWay[i].hWsContext, pcIndex);
        }
    }

    return BS_OK;
}

BS_STATUS WSAPP_Service_Start(IN CHAR *pcServiceName)
{
    WSAPP_SERVICE_S *pstService;

    pstService = wsapp_service_Find(pcServiceName);
    if (NULL == pstService)
    {
        return BS_ERR;
    }

    NO_SetKeyValue(g_hWsAppServiceNo, pstService, "Enable", "1");

    pstService->bStart = TRUE;

    return BS_OK;
}

BS_STATUS WSAPP_Service_Stop(IN CHAR *pcServiceName)
{
    WSAPP_SERVICE_S *pstService;

    pstService = wsapp_service_Find(pcServiceName);
    if (NULL == pstService)
    {
        return BS_NO_SUCH;
    }

    NO_SetKeyValue(g_hWsAppServiceNo, pstService, "Enable", NULL);

    pstService->bStart = FALSE;

    return BS_OK;
}

WSAPP_SERVICE_S * WSAPP_Service_GetByID(IN UINT uiServiceID)
{
    return NO_GetObjectByID(g_hWsAppServiceNo, uiServiceID);
}

BOOL_T WSAPP_Service_IsWebCenterOptHide(IN WSAPP_SERVICE_S *pstService)
{
    if (pstService->uiFlag & WSAPP_SERVICE_FLAG_WEB_HIDE)
    {
        return TRUE;
    }

    return FALSE;
}

BOOL_T WSAPP_Service_IsWebCenterOptReadonly(IN WSAPP_SERVICE_S *pstService)
{
    if (pstService->uiFlag & WSAPP_SERVICE_FLAG_WEB_READONLY)
    {
        return TRUE;
    }

    return FALSE;
}

CHAR * WSAPP_Service_GetDescription(IN WSAPP_SERVICE_S *pstService)
{
    return NO_GetKeyValue(pstService, "Description");
}

UINT WSAPP_Service_GetNextID(IN UINT uiCurId)
{
    return (UINT) NO_GetNextID(g_hWsAppServiceNo, uiCurId);
}

CHAR * WSAPP_Service_GetNameByID(IN UINT uiServiceID)
{
    return NO_GetNameByID(g_hWsAppServiceNo, uiServiceID);
}

BS_STATUS WSAPP_Service_Bind(IN CHAR *pcService)
{
    WSAPP_SERVICE_S *pstService;

    pstService = wsapp_service_Find(pcService);
    if (NULL == pstService)
    {
        return BS_NO_SUCH;
    }

    if (pstService->uiFlag & _WSAPP_SERVICE_FLAG_BINDED)
    {
        return BS_CONFLICT;
    }

    pstService->uiFlag |= _WSAPP_SERVICE_FLAG_BINDED;

    return BS_OK;
}

VOID WSAPP_Service_UnBind(IN CHAR *pcService)
{
    WSAPP_SERVICE_S *pstService;

    pstService = wsapp_service_Find(pcService);
    if (NULL == pstService)
    {
        return;
    }

    WSAPP_Service_SetDeliverTbl(pcService, NULL);

    BIT_CLR(pstService->uiFlag, _WSAPP_SERVICE_FLAG_BINDED);

    return;
}

WSAPP_SERVICE_HANDLE WSAPP_Service_GetByName(IN CHAR *pcService)
{
    return wsapp_service_Find(pcService);
}

BS_STATUS WSAPP_Service_SetDeliverTbl(IN CHAR *pcService, IN WS_DELIVER_TBL_HANDLE hDeliverTbl)
{
    WSAPP_SERVICE_S *pstService;
    UINT i;

    pstService = wsapp_service_Find(pcService);
    if (NULL == pstService)
    {
        return BS_NO_SUCH;
    }

    pstService->hDeliverTbl = hDeliverTbl;

    for (i=0; i<WSAPP_SERVICE_MAX_BIND_NUM; i++)
    {
        if (pstService->astBindGateWay[i].hWsContext != NULL)
        {
            WS_Context_BindDeliverTbl(pstService->astBindGateWay[i].hWsContext, hDeliverTbl);
        }
    }

    return BS_OK;
}

BS_STATUS WSAPP_SetUserData(IN CHAR *pcService, IN UINT64 ulUserData)
{
    WSAPP_SERVICE_S *pstService;

    pstService = wsapp_service_Find(pcService);
    if (NULL == pstService)
    {
        return BS_NO_SUCH;
    }

    pstService->ulUserData = ulUserData;

    return BS_OK;
}

UINT64 WSAPP_GetUserDataByWsContext(IN WS_CONTEXT_HANDLE hWsContext)
{
    UINT uiServiceID;
    WSAPP_SERVICE_S *pstService;

    uiServiceID = (UINT)(ULONG)WS_Context_GetUserData(hWsContext);
    if (0 == uiServiceID)
    {
        return 0;
    }

    pstService = NO_GetObjectByID(g_hWsAppServiceNo, uiServiceID);
    if (NULL == pstService)
    {
        return 0;
    }

    return pstService->ulUserData;
}


