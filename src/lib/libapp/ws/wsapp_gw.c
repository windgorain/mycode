/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2015-11-30
* Description: gateway
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/txt_utl.h"
#include "utl/socket_utl.h"
#include "utl/exec_utl.h"
#include "utl/ws_utl.h"
#include "utl/nap_utl.h"
#include "utl/json_utl.h"
#include "utl/ssl_utl.h"
#include "utl/file_utl.h"
#include "utl/object_utl.h"
#include "utl/local_info.h"
#include "comp/comp_acl.h"
#include "comp/comp_kfapp.h"

#include "wsapp_def.h"
#include "wsapp_gw.h"
#include "wsapp_worker.h"
#include "wsapp_master.h"
#include "wsapp_cfglock.h"

#define WSAPP_GW_MAX_NUM  1024

#define WSAPP_GW_WCNTOPT_HIDE_STR "WebCenterOptHide"
#define WSAPP_GW_WCNTOPT_READONLY_STR "WebCenterOptReadOnly"
#define WSAPP_GW_DESC_STR "Description"
#define WSAPP_GW_TYPE_STR "Type"
#define WSAPP_GW_IP_STR "IP"
#define WSAPP_GW_PORT_STR "Port"
#define WSAPP_GW_CA_CERT_STR "CaCert"
#define WSAPP_GW_LOCAL_CERT_STR "LocalCert"
#define WSAPP_GW_KEY_FILE_STR "KeyFile"
#define WSAPP_GW_IPACL_STR "IPACL"
#define WSAPP_GW_ENABLE_STR "Enable"

static NO_HANDLE g_hWsAppGwNo = NULL;
static CHAR *g_apcWsappGwProperty[]
    = {WSAPP_GW_DESC_STR, WSAPP_GW_TYPE_STR, WSAPP_GW_IP_STR, WSAPP_GW_PORT_STR, WSAPP_GW_ENABLE_STR, NULL};

static WSAPP_GW_S * wsapp_gw_Find(IN CHAR *pcGwName)
{
    return NO_GetObjectByName(g_hWsAppGwNo, pcGwName);
}

static int wsapp_gw_WsEventNotify(IN INT iSocketId, IN UINT uiEvent, IN USER_HANDLE_S *pstUserHandle)
{
    WS_CONN_HANDLE hWsConn = pstUserHandle->ahUserHandle[0];

    WS_Conn_EventHandler(hWsConn, uiEvent);

    return 0;
}

static BS_STATUS wsapp_gw_WsSetEvent(IN WS_CONN_HANDLE hWsConn, IN UINT uiEvent)
{
    USER_HANDLE_S stUserHandle;
    CONN_HANDLE hConn = WS_Conn_GetRawConn(hWsConn);

    if (hConn == NULL)
    {
        return BS_ERR;
    }

    stUserHandle.ahUserHandle[0] = hWsConn;

    return CONN_SetEvent(hConn, uiEvent, wsapp_gw_WsEventNotify, &stUserHandle);
}

static VOID wsapp_gw_WsClose(IN WS_CONN_HANDLE hWsConn)
{
    CONN_HANDLE hConn = WS_Conn_GetRawConn(hWsConn);

    if (NULL != hConn)
    {
        CONN_Free(hConn);
    }
}

static INT wsapp_gw_WsRead(IN WS_CONN_HANDLE hWsConn, OUT UCHAR *pucData, IN UINT uiDataLen)
{
    CONN_HANDLE hConn = WS_Conn_GetRawConn(hWsConn);

    if (NULL == hConn)
    {
        return -1;
    }

    return CONN_Read(hConn, pucData, uiDataLen);
}

static INT wsapp_gw_WsWrite(IN WS_CONN_HANDLE hWsConn, IN UCHAR *pucData, IN UINT uiDataLen)
{
    CONN_HANDLE hConn = WS_Conn_GetRawConn(hWsConn);

    if (NULL == hConn)
    {
        return -1;
    }

    return CONN_Write(hConn, pucData, uiDataLen);
}

static BS_STATUS wsapp_gw_WsInit(IN WSAPP_GW_S *pstGW)
{
    WS_FUNC_TBL_S stFuncTbl;

    stFuncTbl.pfSetEvent = wsapp_gw_WsSetEvent;
    stFuncTbl.pfClose = wsapp_gw_WsClose;
    stFuncTbl.pfRead = wsapp_gw_WsRead;
    stFuncTbl.pfWrite = wsapp_gw_WsWrite;

    pstGW->hWsHandle = WS_Create(&stFuncTbl);
    if (NULL == pstGW->hWsHandle)
    {
        return BS_ERR;
    }

    return BS_OK;
}

static VOID wsapp_gw_FiniSSL(IN WSAPP_GW_S *pstGW)
{
    UINT i;

    for (i=0; i<WSAPP_WROKER_NUM; i++)
    {
        if (pstGW->apSslCtx[i] == NULL)
        {
            continue;
        }

        SSL_UTL_Ctx_Free(pstGW->apSslCtx[i]);
        pstGW->apSslCtx[i] = NULL;
    }
}

static UINT wsapp_gw_GetIP(IN WSAPP_GW_S *pstGW)
{
    CHAR *pcIP;

    pcIP = NO_GetKeyValue(pstGW, WSAPP_GW_IP_STR);
    if (TXT_IS_EMPTY(pcIP))
    {
        return 0;
    }

    return Socket_Ipsz2IpHost(pcIP);
}

static USHORT wsapp_gw_GetPort(IN WSAPP_GW_S *pstGW)
{
    CHAR *pcPort;
    USHORT usPort;

    pcPort = NO_GetKeyValue(pstGW, WSAPP_GW_PORT_STR);
    if (TXT_IS_EMPTY(pcPort))
    {
        return 0;
    }

    usPort = TXT_Str2Ui(pcPort);
    if (usPort == 0)
    {
        return 0;
    }

    return usPort;
}

static BS_STATUS wsapp_gw_InitSSL(IN WSAPP_GW_S *pstGW)
{
    VOID *pSslCtx;
    CHAR *pcTmp;
    UINT i;
    CHAR szCaCert[ FILE_MAX_PATH_LEN + 1 ];
    CHAR szLocalCert[ FILE_MAX_PATH_LEN + 1 ];
    CHAR szKeyFile[ FILE_MAX_PATH_LEN + 1 ];
    BOOL_T bUseSelfSignCert = TRUE;
    PKI_DOMAIN_CONFIGURE_S stConfig;

    pcTmp = NO_GetKeyValue(pstGW, WSAPP_GW_TYPE_STR);
    if ((NULL == pcTmp) || (pcTmp[0] != 'S'))
    {
        return BS_OK;
    }

    pcTmp = NO_GetKeyValue(pstGW, WSAPP_GW_CA_CERT_STR);
    if (!TXT_IS_EMPTY(pcTmp))
    {
        bUseSelfSignCert = FALSE;
        LOCAL_INFO_ExpandToConfPath(pcTmp, szCaCert);
        LOCAL_INFO_ExpandToConfPath(NO_GetKeyValue(pstGW, WSAPP_GW_LOCAL_CERT_STR), szLocalCert);
        LOCAL_INFO_ExpandToConfPath(NO_GetKeyValue(pstGW, WSAPP_GW_KEY_FILE_STR), szKeyFile);
    }
    else
    {
        PKI_InitDftConfig(&stConfig);
        LOCAL_INFO_ExpandToConfPath(stConfig.szCertFileName, szLocalCert);
        strlcpy(stConfig.szCertFileName, szLocalCert, sizeof(stConfig.szCertFileName));
    }

    for (i=0; i<WSAPP_WROKER_NUM; i++)
    {
        pSslCtx = SSL_UTL_Ctx_Create(0, 0);
        if (NULL == pSslCtx)
        {
            wsapp_gw_FiniSSL(pstGW);
            return BS_ERR;
        }

        if (bUseSelfSignCert == TRUE)
        {
            SSL_UTL_Ctx_LoadSelfSignCert(pSslCtx, &stConfig);
        }
        else
        {
            SSL_UTL_Ctx_LoadCert(pSslCtx, szCaCert, szLocalCert, szKeyFile);
        }

        if (NULL != pstGW->apSslCtx[i])
        {
            SSL_UTL_Ctx_Free(pstGW->apSslCtx[i]);
        }

        pstGW->apSslCtx[i] = pSslCtx;
    }

    return BS_OK;
}

static BS_STATUS _wsapp_gw_kf_IsExist(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN KFAPP_PARAM_S *pstParam)
{
    WSAPP_CfgLock_RLock();
    JSON_NO_IsExist(g_hWsAppGwNo, hMime, pstParam->pstJson);
    WSAPP_CfgLock_RUnLock();

    return BS_OK;
}

static BS_STATUS _wsapp_gw_kf_AddProcess(IN MIME_HANDLE hMime, IN KFAPP_PARAM_S *pstParam)
{
    WSAPP_GW_S *pstGW;
    CHAR *pcTmp;
    CHAR *pcName;
    BS_STATUS eRet;
    UINT i;

    pcName = MIME_GetKeyValue(hMime, "Name");
    if (NULL == pcName)
    {
        JSON_SetFailed(pstParam->pstJson, "Bad param");
        return BS_ERR;
    }

    pstGW = WSAPP_GW_Add(pcName);
    if (pstGW == NULL)
    {
        JSON_SetFailed(pstParam->pstJson, "Add failed");
        return BS_ERR;
    }
    
    pcTmp = MIME_GetKeyValue(hMime, "Type");
    if ((pcTmp != NULL) && (pcTmp[0] == 'S'))
    {
        pstGW->bIsSSL = TRUE;
    }

    for (i=0; g_apcWsappGwProperty[i]!=NULL; i++)
    {
        pcTmp = MIME_GetKeyValue(hMime, g_apcWsappGwProperty[i]);
        if (pcTmp != NULL)
        {
            NO_SetKeyValue(g_hWsAppGwNo, pstGW, g_apcWsappGwProperty[i], pcTmp);
        }
    }

    pcTmp = MIME_GetKeyValue(hMime, "Enable");
    if ((TXT_IS_EMPTY(pcTmp)) || (pcTmp[0] != '1'))
    {
        JSON_SetSuccess(pstParam->pstJson);
        return BS_OK;
    }

    eRet = WSAPP_GW_Start(pcName);
    if (eRet != BS_OK)
    {
        if (eRet == BS_CONFLICT)
        {
            JSON_SetFailed(pstParam->pstJson, "Port conflict");
        }
        else
        {
            JSON_SetFailed(pstParam->pstJson, "Operation failed");
        }

        WSAPP_GW_Del(pcName);
        return BS_ERR;
    }
    else
    {
        JSON_SetSuccess(pstParam->pstJson);
    }

    return eRet;
}

static BS_STATUS _wsapp_gw_kf_Add(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN KFAPP_PARAM_S *pstParam)
{
    WSAPP_CfgLock_WLock();
    _wsapp_gw_kf_AddProcess(hMime, pstParam);
    WSAPP_CfgLock_WUnLock();

    return BS_OK;
}

static BS_STATUS _wsapp_gw_kf_ModifyProcess(IN MIME_HANDLE hMime, IN KFAPP_PARAM_S *pstParam)
{
    WSAPP_GW_S *pstGW;
    CHAR *pcTmp;
    CHAR *pcName;
    BS_STATUS eRet;
    UINT i;

    pcName = MIME_GetKeyValue(hMime, "Name");
    if (NULL == pcName)
    {
        JSON_SetFailed(pstParam->pstJson, "Bad param");
        return BS_ERR;
    }

    pstGW = wsapp_gw_Find(pcName);
    if (NULL == pstGW)
    {
        JSON_SetFailed(pstParam->pstJson, "No such name");
        return BS_NO_SUCH;
    }

    pcTmp = NO_GetKeyValue(pstGW, WSAPP_GW_WCNTOPT_READONLY_STR);
    if (NULL != pcTmp)
    {
        JSON_SetFailed(pstParam->pstJson, "Not permit");
        return BS_NO_PERMIT;
    }

    WSAPP_GW_Stop(pcName);

    pcTmp = MIME_GetKeyValue(hMime, "Type");
    if ((pcTmp != NULL) && (pcTmp[0] == 'S'))
    {
        pstGW->bIsSSL = TRUE;
    }

    for (i=0; g_apcWsappGwProperty[i]!=NULL; i++)
    {
        pcTmp = MIME_GetKeyValue(hMime, g_apcWsappGwProperty[i]);
        if (pcTmp != NULL)
        {
            NO_SetKeyValue(g_hWsAppGwNo, pstGW, g_apcWsappGwProperty[i], pcTmp);
        }
    }

    pcTmp = MIME_GetKeyValue(hMime, "Enable");
    if ((TXT_IS_EMPTY(pcTmp)) || (pcTmp[0] != '1'))
    {
        JSON_SetSuccess(pstParam->pstJson);
        return BS_OK;
    }

    eRet = WSAPP_GW_Start(pcName);
    if (eRet != BS_OK)
    {
        if (eRet == BS_CONFLICT)
        {
            JSON_SetFailed(pstParam->pstJson, "Port conflict");
        }
        else
        {
            JSON_SetFailed(pstParam->pstJson, "Operation failed");
        }

        NO_SetKeyValue(g_hWsAppGwNo, pstGW, WSAPP_GW_ENABLE_STR, NULL);

        return BS_ERR;
    }
    else
    {
        JSON_SetSuccess(pstParam->pstJson);
    }

    return eRet;
}

static BS_STATUS _wsapp_gw_kf_Modify(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN KFAPP_PARAM_S *pstParam)
{
    WSAPP_CfgLock_WLock();
    _wsapp_gw_kf_ModifyProcess(hMime, pstParam);
    WSAPP_CfgLock_WUnLock();

	return BS_OK;
}

static BS_STATUS _wsapp_gw_kf_Get(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN KFAPP_PARAM_S *pstParam)
{
    WSAPP_CfgLock_RLock();
    JSON_NO_Get(g_hWsAppGwNo, hMime, pstParam->pstJson, g_apcWsappGwProperty);
    WSAPP_CfgLock_RUnLock();

	return BS_OK;
}

static BOOL_T _wsapp_gw_kf_DelNotify(IN HANDLE hUserHandle, IN CHAR *pcDelName)
{
    CHAR *pcReadonly;
    WSAPP_GW_S *pstGW;
    CHAR szInfo[128];
    KFAPP_PARAM_S *pstParam = hUserHandle;

    pstGW = wsapp_gw_Find(pcDelName);
    if (NULL == pstGW)
    {
        return FALSE;
    }

    pcReadonly = NO_GetKeyValue(pstGW, WSAPP_GW_WCNTOPT_READONLY_STR);
    if (NULL != pcReadonly)
    {
        snprintf(szInfo, sizeof(szInfo), "%s is readonly", pcDelName);
        JSON_AppendInfo(pstParam->pstJson, szInfo);
        return FALSE;
    }

    if (pstGW->uiRefCount != 0)
    {
        snprintf(szInfo, sizeof(szInfo), "%s is used", pcDelName);
        JSON_AppendInfo(pstParam->pstJson, szInfo);
        return FALSE;
    }

    WSAPP_GW_Stop(pcDelName);

    return TRUE;
}

static BS_STATUS _wsapp_gw_kf_Del(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN KFAPP_PARAM_S *pstParam)
{
    WSAPP_CfgLock_WLock();
    JSON_NO_DeleteWithNotify(g_hWsAppGwNo, hMime, pstParam->pstJson, _wsapp_gw_kf_DelNotify, pstParam);
    WSAPP_CfgLock_WUnLock();

	return BS_OK;
}

static BOOL_T _wsapp_gw_kf_ListIsPermit(IN HANDLE hUserHandle, IN UINT64 ulNodeID)
{
    CHAR *pcOpt;
    
    pcOpt = NO_GetKeyValueByID(g_hWsAppGwNo, ulNodeID, WSAPP_GW_WCNTOPT_HIDE_STR);

    if (NULL != pcOpt)
    {
        return FALSE;
    }

    return TRUE;
}

static BS_STATUS _wsapp_gw_kf_List(IN MIME_HANDLE hMime, IN HANDLE hUserHandle, IN KFAPP_PARAM_S *pstParam)
{
    BS_STATUS eRet;

    WSAPP_CfgLock_RLock();
    eRet = JSON_NO_ListWithCallBack(g_hWsAppGwNo, 
        pstParam->pstJson, g_apcWsappGwProperty,
        _wsapp_gw_kf_ListIsPermit, NULL);
    WSAPP_CfgLock_RUnLock();

    return eRet;
}

static VOID _wsapp_gw_kf_Init()
{
    KFAPP_RegFunc("ws.gateway.IsExist", _wsapp_gw_kf_IsExist, NULL);
    KFAPP_RegFunc("ws.gateway.Add", _wsapp_gw_kf_Add, NULL);
    KFAPP_RegFunc("ws.gateway.Modify", _wsapp_gw_kf_Modify, NULL);
    KFAPP_RegFunc("ws.gateway.Get", _wsapp_gw_kf_Get, NULL);
    KFAPP_RegFunc("ws.gateway.Delete", _wsapp_gw_kf_Del, NULL);
    KFAPP_RegFunc("ws.gateway.List", _wsapp_gw_kf_List, NULL);
}

BS_STATUS WSAPP_GW_Init()
{
    OBJECT_PARAM_S obj_param = {0};

    obj_param.uiMaxNum = WSAPP_GW_MAX_NUM;
    obj_param.uiObjSize = sizeof(WSAPP_GW_S);

    g_hWsAppGwNo = NO_CreateAggregate(&obj_param);
    if (NULL == g_hWsAppGwNo)
    {
        return BS_ERR;
    }

    NO_EnableSeq(g_hWsAppGwNo, 0xffff0000, 1);

    _wsapp_gw_kf_Init();

    return BS_OK;
}

WSAPP_GW_S * WSAPP_GW_Add(IN CHAR *pcGwName)
{
    WSAPP_GW_S *pstGW;

    pstGW = NO_NewObject(g_hWsAppGwNo, pcGwName);
    if (NULL == pstGW)
    {
        return NULL;
    }

    pstGW->iListenSocket = -1;

    if (BS_OK != wsapp_gw_WsInit(pstGW))
    {
        NO_FreeObject(g_hWsAppGwNo, pstGW);
        return NULL;
    }

    NO_SetKeyValue(g_hWsAppGwNo, pstGW, WSAPP_GW_TYPE_STR, "TCP");
    NO_SetKeyValue(g_hWsAppGwNo, pstGW, WSAPP_GW_PORT_STR, "80");

    return pstGW;
}

BS_STATUS WSAPP_GW_Del(IN CHAR *pcGwName)
{
    WSAPP_GW_S *pstGW;

    pstGW = wsapp_gw_Find(pcGwName);
    if (NULL == pstGW)
    {
        return BS_OK;
    }

    if (pstGW->uiRefCount != 0)
    {
        return BS_REF_NOT_ZERO;
    }
    
    WSAPP_GW_Stop(pcGwName);
    NO_FreeObjectByName(g_hWsAppGwNo, pcGwName);

    return BS_OK;
}

BOOL_T WSAPP_GW_IsExist(IN CHAR *pcGwName)
{
    if (NULL != wsapp_gw_Find(pcGwName))
    {
        return TRUE;
    }

    return FALSE;
}


BS_STATUS WSAPP_GW_SetWebCenterOpt(IN CHAR *pcGwName, IN CHAR *pcOpt)
{
    WSAPP_GW_S *pstGW;

    pstGW = wsapp_gw_Find(pcGwName);
    if (NULL == pstGW)
    {
        return BS_ERR;
    }

    if (strcmp(pcOpt, "hide") == 0)
    {
        NO_SetKeyValue(g_hWsAppGwNo, pstGW, WSAPP_GW_WCNTOPT_HIDE_STR, "1");
    }
    else if (strcmp(pcOpt, "readonly") == 0)
    {
        NO_SetKeyValue(g_hWsAppGwNo, pstGW, WSAPP_GW_WCNTOPT_READONLY_STR, "1");
    }

    return BS_OK;
}

BS_STATUS WSAPP_GW_ClrWebCenterOpt(IN CHAR *pcGwName, IN CHAR *pcOpt)
{
    WSAPP_GW_S *pstGW;

    pstGW = wsapp_gw_Find(pcGwName);
    if (NULL == pstGW)
    {
        return BS_ERR;
    }

    if (strcmp(pcOpt, "hide") == 0)
    {
        NO_SetKeyValue(g_hWsAppGwNo, pstGW, WSAPP_GW_WCNTOPT_HIDE_STR, NULL);
    }
    else if (strcmp(pcOpt, "readonly") == 0)
    {
        NO_SetKeyValue(g_hWsAppGwNo, pstGW, WSAPP_GW_WCNTOPT_READONLY_STR, NULL);
    }

    return BS_OK;
}

BS_STATUS WSAPP_GW_SetDescription(IN CHAR *pcGwName, IN CHAR *pcDesc)
{
    WSAPP_GW_S *pstGW;

    pstGW = wsapp_gw_Find(pcGwName);
    if (NULL == pstGW)
    {
        return BS_ERR;
    }

    NO_SetKeyValue(g_hWsAppGwNo, pstGW, WSAPP_GW_DESC_STR, pcDesc);

    return BS_OK;
}

BS_STATUS WSAPP_GW_SetType(IN CHAR *pcGwName, IN CHAR *pcType)
{
    WSAPP_GW_S *pstGW;

    pstGW = wsapp_gw_Find(pcGwName);
    if (NULL == pstGW)
    {
        return BS_ERR;
    }

    NO_SetKeyValue(g_hWsAppGwNo, pstGW, WSAPP_GW_TYPE_STR, pcType);

    if (pcType[0] == 'S')
    {
        pstGW->bIsSSL = TRUE;
    }
    else
    {
        NO_SetKeyValue(g_hWsAppGwNo, pstGW, WSAPP_GW_CA_CERT_STR, NULL);
        NO_SetKeyValue(g_hWsAppGwNo, pstGW, WSAPP_GW_LOCAL_CERT_STR, NULL);
        NO_SetKeyValue(g_hWsAppGwNo, pstGW, WSAPP_GW_KEY_FILE_STR, NULL);
        pstGW->bIsSSL = FALSE;
    }

    return BS_OK;
}

BS_STATUS WSAPP_GW_SetSslParam(IN CHAR *pcGwName, IN CHAR *pcCACert, IN CHAR *pcLocalCert, IN CHAR *pcKeyFile)
{
    WSAPP_GW_S *pstGW;

    pstGW = wsapp_gw_Find(pcGwName);
    if (NULL == pstGW)
    {
        return BS_ERR;
    }

    NO_SetKeyValue(g_hWsAppGwNo, pstGW, WSAPP_GW_CA_CERT_STR, pcCACert);
    NO_SetKeyValue(g_hWsAppGwNo, pstGW, WSAPP_GW_LOCAL_CERT_STR, pcLocalCert);
    NO_SetKeyValue(g_hWsAppGwNo, pstGW, WSAPP_GW_KEY_FILE_STR, pcKeyFile);

    return BS_OK;
}

BS_STATUS WSAPP_GW_SetIP(IN CHAR *pcGwName, IN CHAR *pcIP)
{
    WSAPP_GW_S *pstGW;
    UINT uiIP;

    uiIP = Socket_Ipsz2IpHost(pcIP);
    if (uiIP == 0)
    {
        EXEC_OutString("The ip address is invalid.\r\n");
        return BS_ERR;
    }

    pstGW = wsapp_gw_Find(pcGwName);
    if (NULL == pstGW)
    {
        EXEC_OutString("There is not such gateway.\r\n");
        return BS_ERR;
    }

    if (BS_OK != NO_SetKeyValue(g_hWsAppGwNo, pstGW, WSAPP_GW_IP_STR, pcIP))
    {
        EXEC_OutString("Memory limitted.\r\n");
        return BS_NO_MEMORY;
    }

    return BS_OK;
}

BS_STATUS WSAPP_GW_SetPort(IN CHAR *pcGwName, IN CHAR *pcPort)
{
    WSAPP_GW_S *pstGW;
    USHORT usPort;

    usPort = TXT_Str2Ui(pcPort);
    if (usPort == 0)
    {
        EXEC_OutString("The port is invalid.\r\n");
        return BS_BAD_PARA;
    }

    pstGW = wsapp_gw_Find(pcGwName);
    if (NULL == pstGW)
    {
        EXEC_OutString("There is not such gateway.\r\n");
        return BS_ERR;
    }

    if (BS_OK != NO_SetKeyValue(g_hWsAppGwNo, pstGW, WSAPP_GW_PORT_STR, pcPort))
    {
        EXEC_OutString("Memory limitted.\r\n");
        return BS_NO_MEMORY;
    }

    return BS_OK;
}

BS_STATUS WSAPP_GW_NoRefIpAcl(IN CHAR *pcGwName)
{
    WSAPP_GW_S *pstGW;
    CHAR *pcAclListName;

    pstGW = wsapp_gw_Find(pcGwName);
    if (NULL == pstGW)
    {
        return BS_ERR;
    }

    pcAclListName = NO_GetKeyValue(pstGW, WSAPP_GW_IPACL_STR);

    if ((pcAclListName != NULL) && (pcAclListName[0] != '\0'))
    {
        ACL_Ioctl(0, ACL_TYPE_IP, COMP_ACL_IOCTL_DEL_LIST_REF, pcAclListName);
        NO_SetKeyValue(g_hWsAppGwNo, pstGW, WSAPP_GW_IPACL_STR, NULL);
        pstGW->uiIpAclListID = 0;
    }

    return BS_OK;
}

BS_STATUS WSAPP_GW_RefIpAcl(IN CHAR *pcGwName, IN CHAR *pcIpAclListName)
{
    WSAPP_GW_S *pstGW;
    CHAR *pcAclListName;
    ACL_NAME_ID_S stAclNameID;

    pstGW = wsapp_gw_Find(pcGwName);
    if (NULL == pstGW)
    {
        return BS_ERR;
    }

    pcAclListName = NO_GetKeyValue(pstGW, WSAPP_GW_IPACL_STR);
    if (!TXT_IS_EMPTY(pcAclListName))
    {
        if (strcmp(pcAclListName, pcIpAclListName) == 0)
        {
            return BS_OK;
        }
    }

    if (BS_OK != ACL_Ioctl(0, ACL_TYPE_IP, COMP_ACL_IOCTL_ADD_LIST_REF, pcIpAclListName))
    {
        EXEC_OutString("The acl is not exist.\r\n");
        return BS_ERR;
    }

    WSAPP_GW_NoRefIpAcl(pcGwName);

    if (BS_OK != NO_SetKeyValue(g_hWsAppGwNo, pstGW, WSAPP_GW_IPACL_STR, pcIpAclListName))
    {
        ACL_Ioctl(0, ACL_TYPE_IP, COMP_ACL_IOCTL_DEL_LIST_REF, pcIpAclListName);
        EXEC_OutString("No memory.\r\n");
        return BS_ERR;
    }

    stAclNameID.pcAclListName = pcIpAclListName;

    ACL_Ioctl(0, ACL_TYPE_IP, COMP_ACL_IOCTL_GET_LIST_ID, &stAclNameID);

    pstGW->uiIpAclListID = stAclNameID.ulAclListID;

    return BS_OK;
}

BS_STATUS WSAPP_GW_Start(IN CHAR *pcGwName)
{
    WSAPP_GW_S *pstGW;
    BS_STATUS eRet;

    pstGW = wsapp_gw_Find(pcGwName);
    if (NULL == pstGW)
    {
        return BS_NOT_FOUND;
    }

    if (BS_OK != wsapp_gw_InitSSL(pstGW))
    {
        return BS_ERR;
    }

    eRet = WSAPP_Master_AddGW(pstGW, wsapp_gw_GetIP(pstGW), wsapp_gw_GetPort(pstGW));

    if (BS_OK != eRet)
    {
        wsapp_gw_FiniSSL(pstGW);
        return eRet;
    }

    NO_SetKeyValue(g_hWsAppGwNo, pstGW, WSAPP_GW_ENABLE_STR, "1");

    pstGW->bStart = TRUE;

    return BS_OK;
}

BS_STATUS WSAPP_GW_Stop(IN CHAR *pcGwName)
{
    WSAPP_GW_S *pstGW;

    pstGW = wsapp_gw_Find(pcGwName);
    if (NULL == pstGW)
    {
        return BS_ERR;
    }

    wsapp_gw_FiniSSL(pstGW);
    NO_SetKeyValue(g_hWsAppGwNo, pstGW, WSAPP_GW_ENABLE_STR, NULL);
    pstGW->bStart = FALSE;

    WSAPP_Master_DelGW(pstGW);

    return BS_OK;
}

VOID WSAPP_GW_SetWsDebugFlag(IN CHAR *pcGwName, IN CHAR *pcFlagName)
{
    WSAPP_GW_S *pstGW;

    pstGW = wsapp_gw_Find(pcGwName);
    if (NULL == pstGW)
    {
        return;
    }

    WS_SetDbgFlagByName(pstGW->hWsHandle, pcFlagName);
}

VOID WSAPP_GW_ClrWsDebugFlag(IN CHAR *pcGwName, IN CHAR *pcFlagName)
{
    WSAPP_GW_S *pstGW;

    pstGW = wsapp_gw_Find(pcGwName);
    if (NULL == pstGW)
    {
        return;
    }

    WS_ClrDbgFlagByName(pstGW->hWsHandle, pcFlagName);
}

VOID WSAPP_GW_Walk(IN PF_WSAPP_GW_WALK_FUNC pfFunc, IN HANDLE hUserContext)
{
    UINT64 uiID = 0;

    while ((uiID = NO_GetNextID(g_hWsAppGwNo, uiID)) != 0)
    {
        pfFunc(NO_GetObjectByID(g_hWsAppGwNo, uiID), hUserContext);
    }

    return;
}

VOID * WSAPP_GW_GetSslCtx(IN UINT uiGwID, IN UINT uiWorkerID)
{
    WSAPP_GW_S *pstGW;

    pstGW = NO_GetObjectByID(g_hWsAppGwNo, uiGwID);
    if (NULL == pstGW)
    {
        return NULL;
    }

    return pstGW->apSslCtx[uiWorkerID];
}

static int wsapp_gw_SslEventNotify(IN INT iSocketId, IN UINT uiEvent, IN USER_HANDLE_S *pstUserHandle)
{
    INT iRet;
    WSAPP_GW_S *pstGW;
    CONN_HANDLE hConn = pstUserHandle->ahUserHandle[0];
    UINT uiGwID = HANDLE_UINT(pstUserHandle->ahUserHandle[1]);
    VOID *pstSsl;

    pstSsl = CONN_GetSsl(hConn);

    pstGW = NO_GetObjectByID(g_hWsAppGwNo, uiGwID);
    if (NULL == pstGW)
    {
        CONN_Free(hConn);
        return 0;
    }

    iRet = SSL_UTL_Accept(pstSsl);
    if (iRet < 0)
    {
        if (iRet == SSL_UTL_E_WANT_READ)
        {
            CONN_ModifyEvent(hConn, MYPOLL_EVENT_IN);
            return 0;
        }

        if (iRet == SSL_UTL_E_WANT_WRITE)
        {
            CONN_ModifyEvent(hConn, MYPOLL_EVENT_OUT);
            return 0;
        }

        CONN_Free(hConn);
        return 0;
    }

    CONN_ClearEvent(hConn);

    if (BS_OK != WS_Conn_Add(pstGW->hWsHandle, hConn, NULL))
    {
        CONN_Free(hConn);
        return 0;
    }

    return 0;
}

static BS_STATUS wsapp_gw_ProcessSslNew(IN WSAPP_GW_S *pstGW, IN CONN_HANDLE hConn)
{
    USER_HANDLE_S stUserHandle;
    UINT uiWorkerID;
    VOID *pstSsl;

    uiWorkerID = (UINT)(ULONG)CONN_GetUserData(hConn, CONN_USER_DATA_INDEX_0);

    pstSsl = SSL_UTL_New(pstGW->apSslCtx[uiWorkerID]);
    if (NULL == pstSsl)
    {
        return BS_ERR;
    }

    CONN_SetSsl(hConn, pstSsl);
    SSL_UTL_SetFd(pstSsl, CONN_GetFD(hConn));

    stUserHandle.ahUserHandle[0] = hConn;
    stUserHandle.ahUserHandle[1] = UINT_HANDLE((UINT)NO_GetObjectID(g_hWsAppGwNo, pstGW));

    return CONN_SetEvent(hConn, MYPOLL_EVENT_IN, wsapp_gw_SslEventNotify, &stUserHandle);
}

static BS_STATUS wsapp_gw_ProcessNewConn(IN UINT uiGwID, IN CONN_HANDLE hConn)
{
    WSAPP_GW_S *pstGW;

    pstGW = NO_GetObjectByID(g_hWsAppGwNo, uiGwID);
    if (NULL == pstGW)
    {
        return BS_ERR;
    }

    if (pstGW->bIsSSL)
    {
        return wsapp_gw_ProcessSslNew(pstGW, hConn);
    }

    if (BS_OK != WS_Conn_Add(pstGW->hWsHandle, hConn, NULL))
    {
        return BS_ERR;
    }

    return BS_OK;
}

BS_STATUS WSAPP_GW_NewConn(IN UINT uiGwID, IN CONN_HANDLE hConn)
{
    BS_STATUS eRet = BS_OK;

    eRet = wsapp_gw_ProcessNewConn(uiGwID, hConn);

    if (BS_OK != eRet)
    {
        CONN_Free(hConn);
    }

    return eRet;
}

WSAPP_GW_S * WSAPP_GW_GetByID(IN UINT uiGwID)
{
    return NO_GetObjectByID(g_hWsAppGwNo, uiGwID);
}

UINT WSAPP_GW_GetID(IN WSAPP_GW_S *pstGW)
{
    return (UINT)NO_GetObjectID(g_hWsAppGwNo, pstGW);
}

static WS_CONTEXT_HANDLE wsapp_gw_AddService
(
    IN CHAR *pcGwName,
    IN CHAR *pcVHost,   
    IN CHAR *pcDomain   
)
{
    WSAPP_GW_S *pstGW;
    WS_VHOST_HANDLE hVHost;
    WS_CONTEXT_HANDLE hContext;
    CHAR szFullPath[FILE_MAX_PATH_LEN + 1];

    pstGW = wsapp_gw_Find(pcGwName);
    if (NULL == pstGW)
    {
        return NULL;
    }

    hVHost = WS_VHost_Find(pstGW->hWsHandle, pcVHost);
    if (NULL == hVHost)
    {
        hVHost = WS_VHost_Add(pstGW->hWsHandle, pcVHost);
        if (NULL == hVHost)
        {
            return NULL;
        }
        hContext = WS_Context_GetDftContext(hVHost);
        LOCAL_INFO_ExpandToConfPath("web/", szFullPath);
        WS_Context_SetRootPath(hContext, szFullPath);
        WS_Context_SetSecRootPath(hContext, "web/");
        WS_Context_SetIndex(hContext, "/index.cgi");
    }

    if (NULL != WS_Context_Find(hVHost, pcDomain))
    {
        return NULL;
    }

    hContext = WS_Context_Add(hVHost, pcDomain);
    if (NULL == hContext)
    {
        return NULL;
    }

    pstGW->uiRefCount ++;

    return hContext;
}

WS_CONTEXT_HANDLE WSAPP_GW_AddService
(
    IN CHAR *pcGwName,
    IN CHAR *pcVHost,   
    IN CHAR *pcDomain   
)
{
    return wsapp_gw_AddService(pcGwName, pcVHost, pcDomain);
}


VOID WSAPP_GW_DelService(IN CHAR *pcGwName, IN WS_CONTEXT_HANDLE hContext)
{
    WS_VHOST_HANDLE hVHost;
    WSAPP_GW_S *pstGW;

    if ((NULL == pcGwName) || (NULL == hContext))
    {
        return;
    }

    hVHost = WS_Context_GetVHost(hContext);
    WS_Context_Del(hContext);

    if (NULL != hVHost)
    {
        if (WS_VHost_GetContextCount(hVHost) == 0)
        {
            WS_VHost_Del(hVHost);
        }
    }

    pstGW = wsapp_gw_Find(pcGwName);
    if (NULL != pstGW)
    {
        pstGW->uiRefCount --;
    }
}

CHAR * WSAPP_GW_GetName(IN WSAPP_GW_S *pstGW)
{
    return NO_GetName(pstGW);
}

BOOL_T WSAPP_GW_IsWebCenterOptHide(IN WSAPP_GW_S *pstGW)
{
    if (NULL != NO_GetKeyValue(pstGW, WSAPP_GW_WCNTOPT_HIDE_STR))
    {
        return TRUE;
    }

    return FALSE;
}

BOOL_T WSAPP_GW_IsWebCenterOptReadonly(IN WSAPP_GW_S *pstGW)
{
    if (NULL != NO_GetKeyValue(pstGW, WSAPP_GW_WCNTOPT_READONLY_STR))
    {
        return TRUE;
    }

    return FALSE;
}

CHAR * WSAPP_GW_GetDesc(IN WSAPP_GW_S *pstGW)
{
    return NO_GetKeyValue(pstGW, WSAPP_GW_DESC_STR);
}

CHAR * WSAPP_GW_GetParamCaCert(IN WSAPP_GW_S *pstGW)
{
    return NO_GetKeyValue(pstGW, WSAPP_GW_CA_CERT_STR);
}

CHAR * WSAPP_GW_GetParamLocalCert(IN WSAPP_GW_S *pstGW)
{
    return NO_GetKeyValue(pstGW, WSAPP_GW_LOCAL_CERT_STR);
}

CHAR * WSAPP_GW_GetParamKeyFile(IN WSAPP_GW_S *pstGW)
{
    return NO_GetKeyValue(pstGW, WSAPP_GW_KEY_FILE_STR);
}

CHAR * WSAPP_GW_GetIP(IN WSAPP_GW_S *pstGW)
{
    return NO_GetKeyValue(pstGW, WSAPP_GW_IP_STR);
}

CHAR * WSAPP_GW_GetPort(IN WSAPP_GW_S *pstGW)
{
    return NO_GetKeyValue(pstGW, WSAPP_GW_PORT_STR);
}

CHAR * WSAPP_GW_GetIpAclList(IN WSAPP_GW_S *pstGW)
{
    return NO_GetKeyValue(pstGW, WSAPP_GW_IPACL_STR);
}

BOOL_T WSAPP_GW_IsFilterPermit(IN UINT uiGwID, IN INT iSocketID)
{
    WSAPP_GW_S *pstGW = WSAPP_GW_GetByID(uiGwID);
    IPACL_MATCH_INFO_S stMatchInfo;
    BS_ACTION_E eAction;

    if (NULL == pstGW)
    {
        return FALSE;
    }

    if (pstGW->uiIpAclListID == 0)
    {
        return TRUE;
    }

    Socket_GetLocalIpPort(iSocketID, &stMatchInfo.uiDIP, &stMatchInfo.usDPort);
    Socket_GetPeerIpPort(iSocketID, &stMatchInfo.uiSIP, &stMatchInfo.usSPort);

    stMatchInfo.uiKeyMask = IPACL_KEY_SIP | IPACL_KEY_DIP | IPACL_KEY_SPORT | IPACL_KEY_DPORT;

    eAction = ACL_Match(0, ACL_TYPE_IP, pstGW->uiIpAclListID, &stMatchInfo);
    if (eAction == BS_ACTION_PERMIT)
    {
        return TRUE;
    }

    return FALSE;
}

