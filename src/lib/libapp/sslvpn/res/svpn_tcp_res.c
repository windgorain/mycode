/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-6-30
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "../h/svpn_def.h"
#include "../h/svpn_context.h"
#include "../h/svpn_ctxdata.h"
#include "../h/svpn_dweb.h"
#include "../h/svpn_mf.h"


static CHAR *g_apcSvpnTcpResProperty[] = {"Description", "ServerAddress"};
static UINT g_uiSvpnTcpResPropertyCount = sizeof(g_apcSvpnTcpResProperty)/sizeof(CHAR*);

static VOID svpn_tcpres_IsExist(IN MIME_HANDLE hMime, IN SVPN_DWEB_S *pstDweb)
{
    SVPN_MF_CommonIsExist(hMime, pstDweb, SVPN_CTXDATA_TCP_RES);
}

static VOID svpn_tcpres_Add(IN MIME_HANDLE hMime,  IN SVPN_DWEB_S *pstDweb)
{
    SVPN_MF_CommonAdd(hMime, pstDweb, SVPN_CTXDATA_TCP_RES, g_apcSvpnTcpResProperty, g_uiSvpnTcpResPropertyCount);
}

static VOID svpn_tcpres_DeleteNotify(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN CHAR *pcName)
{
    SVPN_CtxData_AllDelPropElement(hSvpnContext, SVPN_CTXDATA_ROLE, "TcpRes", pcName, SVPN_PROPERTY_SPLIT);
}

static VOID svpn_tcpres_Del(IN MIME_HANDLE hMime,  IN SVPN_DWEB_S *pstDweb)
{
    SVPN_MF_CommonDelete(hMime, pstDweb, SVPN_CTXDATA_TCP_RES, svpn_tcpres_DeleteNotify);
}

static VOID svpn_tcpres_Modify(IN MIME_HANDLE hMime, IN SVPN_DWEB_S *pstDweb)
{
    SVPN_MF_CommonModify(hMime, pstDweb, SVPN_CTXDATA_TCP_RES, g_apcSvpnTcpResProperty, g_uiSvpnTcpResPropertyCount);
}

static VOID svpn_tcpres_Get(IN MIME_HANDLE hMime, IN SVPN_DWEB_S *pstDweb)
{
    SVPN_MF_CommonGet(hMime, pstDweb, SVPN_CTXDATA_TCP_RES, g_apcSvpnTcpResProperty, g_uiSvpnTcpResPropertyCount);
}

static VOID svpn_tcpres_List(IN MIME_HANDLE hMime,  IN SVPN_DWEB_S *pstDweb)
{
    SVPN_MF_CommonList(hMime, pstDweb, SVPN_CTXDATA_TCP_RES, g_apcSvpnTcpResProperty, g_uiSvpnTcpResPropertyCount);
}

static SVPN_MF_MAP_S g_astSvpnTcpRelayBkmMfMap[] =
{
    {SVPN_USER_TYPE_ADMIN, "TcpRes.IsExist",  svpn_tcpres_IsExist},
    {SVPN_USER_TYPE_ADMIN, "TcpRes.Add",      svpn_tcpres_Add},
    {SVPN_USER_TYPE_ADMIN, "TcpRes.Delete",   svpn_tcpres_Del},
    {SVPN_USER_TYPE_ADMIN, "TcpRes.Modify",   svpn_tcpres_Modify},
    {SVPN_USER_TYPE_ADMIN, "TcpRes.Get",      svpn_tcpres_Get},
    {SVPN_USER_TYPE_ADMIN|SVPN_USER_TYPE_USER, "TcpRes.List", svpn_tcpres_List},
};

BS_STATUS SVPN_TcpRes_Init()
{
    return SVPN_MF_Reg(g_astSvpnTcpRelayBkmMfMap, sizeof(g_astSvpnTcpRelayBkmMfMap)/sizeof(SVPN_MF_MAP_S));
}




PLUG_API BS_STATUS SVPN_TcpResCmd_EnterView(int argc, char **argv, char *pEnv)
{
    return SVPN_CD_EnterView(pEnv, SVPN_CTXDATA_TCP_RES, argv[1]);
}


PLUG_API BS_STATUS SVPN_TcpResCmd_SetDescription(int argc, char **argv,
        void *pEnv)
{
    return SVPN_CD_SetProp(pEnv, SVPN_CTXDATA_TCP_RES, "Description", argv[1]);
}


PLUG_API BS_STATUS SVPN_TcpResCmd_SetServerAddress(int argc, char **argv,
        void *pEnv)
{
    return SVPN_CD_AddPropElement(pEnv, SVPN_CTXDATA_TCP_RES, "ServerAddress", argv[1], ';');
}

BS_STATUS SVPN_TcpResCmd_Save(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN HANDLE hFile)
{
    CHAR szName[SVPN_MAX_RES_NAME_LEN + 1] = "";
        
    while (BS_OK == SVPN_CtxData_GetNextObject(hSvpnContext, SVPN_CTXDATA_TCP_RES, szName, szName, SVPN_MAX_RES_NAME_LEN + 1))
    {
        if (0 != CMD_EXP_OutputMode(hFile, "tcp-resource %s", szName)) {
            continue;
        }

        SVPN_CD_SaveProp(hSvpnContext, SVPN_CTXDATA_TCP_RES, szName, "Description", "description", hFile);
        SVPN_CD_SaveElements(hSvpnContext, SVPN_CTXDATA_TCP_RES, szName, "ServerAddress", "address", ';', hFile);

        CMD_EXP_OutputModeQuit(hFile);
    }

    return BS_OK;
}


