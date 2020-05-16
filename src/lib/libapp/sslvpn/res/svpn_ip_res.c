/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-7-5
* Description: IP隧道书签
* History:     
******************************************************************************/
#include "bs.h"

#include "../h/svpn_def.h"
#include "../h/svpn_context.h"
#include "../h/svpn_ctxdata.h"
#include "../h/svpn_dweb.h"
#include "../h/svpn_mf.h"

/********MF接口*********/
static CHAR *g_apcSvpnIpResProperty[] = {"Description", "Address"};
static UINT g_uiSvpnIpResPropertyCount = sizeof(g_apcSvpnIpResProperty)/sizeof(CHAR*);

static VOID svpn_ipres_IsExist(IN MIME_HANDLE hMime, IN SVPN_DWEB_S *pstDweb)
{
    SVPN_MF_CommonIsExist(hMime, pstDweb, SVPN_CTXDATA_IP_RES);
}

static VOID svpn_ipres_Add(IN MIME_HANDLE hMime,  IN SVPN_DWEB_S *pstDweb)
{
    SVPN_MF_CommonAdd(hMime, pstDweb, SVPN_CTXDATA_IP_RES, g_apcSvpnIpResProperty, g_uiSvpnIpResPropertyCount);
}

static VOID svpn_ipres_DeleteNotify(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN CHAR *pcName)
{
    SVPN_CtxData_AllDelPropElement(hSvpnContext, SVPN_CTXDATA_ROLE, "IpRes", pcName, SVPN_PROPERTY_SPLIT);
}

static VOID svpn_ipres_Del(IN MIME_HANDLE hMime,  IN SVPN_DWEB_S *pstDweb)
{
    SVPN_MF_CommonDelete(hMime, pstDweb, SVPN_CTXDATA_IP_RES, svpn_ipres_DeleteNotify);
}

static VOID svpn_ipres_Modify(IN MIME_HANDLE hMime, IN SVPN_DWEB_S *pstDweb)
{
    SVPN_MF_CommonModify(hMime, pstDweb, SVPN_CTXDATA_IP_RES, g_apcSvpnIpResProperty, g_uiSvpnIpResPropertyCount);
}

static VOID svpn_ipres_Get(IN MIME_HANDLE hMime, IN SVPN_DWEB_S *pstDweb)
{
    SVPN_MF_CommonGet(hMime, pstDweb, SVPN_CTXDATA_IP_RES, g_apcSvpnIpResProperty, g_uiSvpnIpResPropertyCount);
}

static VOID svpn_ipres_List(IN MIME_HANDLE hMime,  IN SVPN_DWEB_S *pstDweb)
{
    SVPN_MF_CommonList(hMime, pstDweb, SVPN_CTXDATA_IP_RES, g_apcSvpnIpResProperty, g_uiSvpnIpResPropertyCount);
}

static SVPN_MF_MAP_S g_astSvpnTcpRelayBkmMfMap[] =
{
    {SVPN_USER_TYPE_ADMIN, "IpRes.IsExist",  svpn_ipres_IsExist},
    {SVPN_USER_TYPE_ADMIN, "IpRes.Add",      svpn_ipres_Add},
    {SVPN_USER_TYPE_ADMIN, "IpRes.Delete",   svpn_ipres_Del},
    {SVPN_USER_TYPE_ADMIN, "IpRes.Modify",   svpn_ipres_Modify},
    {SVPN_USER_TYPE_ADMIN, "IpRes.Get",      svpn_ipres_Get},
    {SVPN_USER_TYPE_ADMIN|SVPN_USER_TYPE_USER, "IpRes.List", svpn_ipres_List},
};

BS_STATUS SVPN_IPRes_Init()
{
    return SVPN_MF_Reg(g_astSvpnTcpRelayBkmMfMap, sizeof(g_astSvpnTcpRelayBkmMfMap)/sizeof(SVPN_MF_MAP_S));
}

/* 命令行 */

/* ip-resource xxx */
PLUG_API BS_STATUS SVPN_IpResCmd_EnterView(int argc, char **argv, void *pEnv)
{
    return SVPN_CD_EnterView(pEnv, SVPN_CTXDATA_IP_RES, argv[1]);
}

/* description xxx */
PLUG_API BS_STATUS SVPN_IpResCmd_SetDescription(int argc, char **argv,
        void *pEnv)
{
    return SVPN_CD_SetProp(pEnv, SVPN_CTXDATA_IP_RES, "Description", argv[1]);
}

/* address xxx */
PLUG_API BS_STATUS SVPN_IpResCmd_AddAddress(int argc, char **argv, void *pEnv)
{
    return SVPN_CD_AddPropElement(pEnv, SVPN_CTXDATA_IP_RES, "Address", argv[1], ';');
}

BS_STATUS SVPN_IpResCmd_Save(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN HANDLE hFile)
{
    CHAR szName[SVPN_MAX_RES_NAME_LEN + 1] = "";
        
    while (BS_OK == SVPN_CtxData_GetNextObject(hSvpnContext, SVPN_CTXDATA_IP_RES, szName, szName, SVPN_MAX_RES_NAME_LEN + 1))
    {
        CMD_EXP_OutputMode(hFile, "ip-resource %s", szName);

        SVPN_CD_SaveProp(hSvpnContext, SVPN_CTXDATA_IP_RES, szName, "Description", "description", hFile);
        SVPN_CD_SaveElements(hSvpnContext, SVPN_CTXDATA_IP_RES, szName, "Address", "address", ';', hFile);

        CMD_EXP_OutputModeQuit(hFile);
    }

    return BS_OK;
}


