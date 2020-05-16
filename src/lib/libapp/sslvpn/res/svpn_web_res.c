/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2015-8-25
* Description: bookmark
* History:     
******************************************************************************/
#include "bs.h"

#include "../h/svpn_def.h"
#include "../h/svpn_context.h"
#include "../h/svpn_ctxdata.h"
#include "../h/svpn_dweb.h"
#include "../h/svpn_mf.h"

/********MF接口*********/
static CHAR *g_apcSvpnWebResProperty[] = {"Description", "URL", "Permit"};
static UINT g_uiSvpnWebResPropertyCount = sizeof(g_apcSvpnWebResProperty)/sizeof(CHAR*);
static VOID svpn_webres_IsExist(IN MIME_HANDLE hMime, IN SVPN_DWEB_S *pstDweb)
{
    SVPN_MF_CommonIsExist(hMime, pstDweb, SVPN_CTXDATA_WEB_RES);
}

static VOID svpn_webres_Add(IN MIME_HANDLE hMime,  IN SVPN_DWEB_S *pstDweb)
{
    SVPN_MF_CommonAdd(hMime, pstDweb, SVPN_CTXDATA_WEB_RES, g_apcSvpnWebResProperty, g_uiSvpnWebResPropertyCount);
}

static VOID svpn_webres_DeleteNotify(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN CHAR *pcName)
{
    SVPN_CtxData_AllDelPropElement(hSvpnContext, SVPN_CTXDATA_ROLE, "WebRes", pcName, SVPN_PROPERTY_SPLIT);
}

static VOID svpn_webres_Del(IN MIME_HANDLE hMime,  IN SVPN_DWEB_S *pstDweb)
{
    SVPN_MF_CommonDelete(hMime, pstDweb, SVPN_CTXDATA_WEB_RES, svpn_webres_DeleteNotify);
}

static VOID svpn_webres_Modify(IN MIME_HANDLE hMime, IN SVPN_DWEB_S *pstDweb)
{
    SVPN_MF_CommonModify(hMime, pstDweb, SVPN_CTXDATA_WEB_RES, g_apcSvpnWebResProperty, g_uiSvpnWebResPropertyCount);
}

static VOID svpn_webres_Get(IN MIME_HANDLE hMime, IN SVPN_DWEB_S *pstDweb)
{
    SVPN_MF_CommonGet(hMime, pstDweb, SVPN_CTXDATA_WEB_RES, g_apcSvpnWebResProperty, g_uiSvpnWebResPropertyCount);
}

static VOID svpn_webres_List(IN MIME_HANDLE hMime,  IN SVPN_DWEB_S *pstDweb)
{
    SVPN_MF_CommonList(hMime, pstDweb, SVPN_CTXDATA_WEB_RES, g_apcSvpnWebResProperty, g_uiSvpnWebResPropertyCount);
}


static SVPN_MF_MAP_S g_astSvpnWebResMfMap[] =
{
    {SVPN_USER_TYPE_ADMIN, "WebRes.IsExist",  svpn_webres_IsExist},
    {SVPN_USER_TYPE_ADMIN, "WebRes.Add",      svpn_webres_Add},
    {SVPN_USER_TYPE_ADMIN, "WebRes.Delete",   svpn_webres_Del},
    {SVPN_USER_TYPE_ADMIN, "WebRes.Modify",   svpn_webres_Modify},
    {SVPN_USER_TYPE_ADMIN, "WebRes.Get",      svpn_webres_Get},
    {SVPN_USER_TYPE_ADMIN|SVPN_USER_TYPE_USER, "WebRes.List",     svpn_webres_List},
};

BS_STATUS SVPN_WebRes_Init()
{
    return SVPN_MF_Reg(g_astSvpnWebResMfMap, sizeof(g_astSvpnWebResMfMap)/sizeof(SVPN_MF_MAP_S));
}


/* 命令行 */

/* web-resource xxx */
PLUG_API BS_STATUS SVPN_WebResCmd_EnterView(int argc, char **argv, void *pEnv)
{
    return SVPN_CD_EnterView(pEnv, SVPN_CTXDATA_WEB_RES, argv[1]);
}

/* description xxx */
PLUG_API BS_STATUS SVPN_WebResCmd_SetDescription(int argc, char **argv, void *pEnv)
{
    return SVPN_CD_SetProp(pEnv, SVPN_CTXDATA_WEB_RES, "Description", argv[1]);
}

/* url xxx */
PLUG_API BS_STATUS SVPN_WebResCmd_SetUrl(int argc, char **argv, void *pEnv)
{
    return SVPN_CD_SetProp(pEnv, SVPN_CTXDATA_WEB_RES, "URL", argv[1]);
}

/* [no] permit */
PLUG_API BS_STATUS SVPN_WebResCmd_Permit(int argc, char **argv, void *pEnv)
{
    if (argv[0][0] == 'n')
    {
        return SVPN_CD_SetProp(pEnv, SVPN_CTXDATA_WEB_RES, "Permit", "false");
    }
    else
    {
        return SVPN_CD_SetProp(pEnv, SVPN_CTXDATA_WEB_RES, "Permit", "true");
    }
}

BS_STATUS SVPN_WebResCmd_Save(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN HANDLE hFile)
{
    CHAR szName[SVPN_MAX_RES_NAME_LEN + 1] = "";
        
    while (BS_OK == SVPN_CtxData_GetNextObject(hSvpnContext, SVPN_CTXDATA_WEB_RES, szName, szName, SVPN_MAX_RES_NAME_LEN + 1))
    {
        CMD_EXP_OutputMode(hFile, "web-resource %s", szName);

        SVPN_CD_SaveProp(hSvpnContext, SVPN_CTXDATA_WEB_RES, szName, "Description", "description", hFile);
        SVPN_CD_SaveProp(hSvpnContext, SVPN_CTXDATA_WEB_RES, szName, "URL", "url", hFile);
        SVPN_CD_SaveBoolProp(hSvpnContext, SVPN_CTXDATA_WEB_RES, szName, "Permit", "permit", hFile);

        CMD_EXP_OutputModeQuit(hFile);
    }

    return BS_OK;
}
