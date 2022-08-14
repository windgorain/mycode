/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-5-26
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/txt_utl.h"
#include "utl/lstr_utl.h"

#include "../h/svpn_def.h"
#include "../h/svpn_context.h"
#include "../h/svpn_ctxdata.h"
#include "../h/svpn_dweb.h"
#include "../h/svpn_mf.h"

BS_STATUS SVPN_Role_Add(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN CHAR *pcRoleName)
{
    return SVPN_CtxData_AddObject(hSvpnContext, SVPN_CTXDATA_ROLE, pcRoleName);
}

VOID SVPN_Role_Del(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN CHAR *pcRoleName)
{
    SVPN_CtxData_AllDelPropElement(hSvpnContext, SVPN_CTXDATA_LOCAL_USER, "role", pcRoleName, SVPN_PROPERTY_SPLIT);
    SVPN_CtxData_DelObject(hSvpnContext, SVPN_CTXDATA_ROLE, pcRoleName);
}

BOOL_T SVPN_Role_IsExist(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN CHAR *pcRoleName)
{
    return SVPN_CtxData_IsObjectExist(hSvpnContext, SVPN_CTXDATA_ROLE, pcRoleName);
}

BS_STATUS SVPN_Role_GetNext
(
    IN SVPN_CONTEXT_HANDLE hSvpnContext,
    IN CHAR *pcRoleName,
    OUT CHAR szNextRoleName[SVPN_MAX_RES_NAME_LEN + 1]
)
{
    return SVPN_CtxData_GetNextObject(hSvpnContext, SVPN_CTXDATA_ROLE,
        pcRoleName, szNextRoleName, SVPN_MAX_RES_NAME_LEN + 1);
}

HSTRING SVPN_Role_GetACL(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN CHAR *pcRoleName)
{
    return SVPN_CtxData_GetPropAsHString(hSvpnContext, SVPN_CTXDATA_ROLE, pcRoleName, "ACL");
}

/********MF接口*********/
static CHAR *g_apcSvpnRoleProperty[] = {"Description", "ACL","WebRes","TcpRes","IpRes"};
static UINT g_uiSvpnRolePropertyCount = sizeof(g_apcSvpnRoleProperty)/sizeof(CHAR*);
static VOID svpn_rolemf_IsExist(IN MIME_HANDLE hMime, IN SVPN_DWEB_S *pstDweb)
{
    SVPN_MF_CommonIsExist(hMime, pstDweb, SVPN_CTXDATA_ROLE);
}

static VOID svpn_rolemf_Add(IN MIME_HANDLE hMime,  IN SVPN_DWEB_S *pstDweb)
{
    SVPN_MF_CommonAdd(hMime, pstDweb, SVPN_CTXDATA_ROLE, g_apcSvpnRoleProperty, g_uiSvpnRolePropertyCount);
}

static VOID svpn_rolemf_DeleteNotify(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN CHAR *pcName)
{
    SVPN_CtxData_AllDelPropElement(hSvpnContext, SVPN_CTXDATA_LOCAL_USER, "role", pcName, SVPN_PROPERTY_SPLIT);
}

static VOID svpn_rolemf_Del(IN MIME_HANDLE hMime,  IN SVPN_DWEB_S *pstDweb)
{
    SVPN_MF_CommonDelete(hMime, pstDweb, SVPN_CTXDATA_ROLE, svpn_rolemf_DeleteNotify);
}

static VOID svpn_rolemf_Modify(IN MIME_HANDLE hMime, IN SVPN_DWEB_S *pstDweb)
{
    SVPN_MF_CommonModify(hMime, pstDweb, SVPN_CTXDATA_ROLE, g_apcSvpnRoleProperty, g_uiSvpnRolePropertyCount);
}

static VOID svpn_rolemf_Get(IN MIME_HANDLE hMime, IN SVPN_DWEB_S *pstDweb)
{
    SVPN_MF_CommonGet(hMime, pstDweb, SVPN_CTXDATA_ROLE, g_apcSvpnRoleProperty, g_uiSvpnRolePropertyCount);
}

static VOID svpn_rolemf_List(IN MIME_HANDLE hMime,  IN SVPN_DWEB_S *pstDweb)
{
    SVPN_MF_CommonList(hMime, pstDweb, SVPN_CTXDATA_ROLE, g_apcSvpnRoleProperty, g_uiSvpnRolePropertyCount);
}

static SVPN_MF_MAP_S g_astSvpnRoleMfMap[] =
{
    {SVPN_USER_TYPE_ADMIN, "Role.IsExist",  svpn_rolemf_IsExist},
    {SVPN_USER_TYPE_ADMIN, "Role.Add",      svpn_rolemf_Add},
    {SVPN_USER_TYPE_ADMIN, "Role.Delete",   svpn_rolemf_Del},
    {SVPN_USER_TYPE_ADMIN, "Role.Modify",   svpn_rolemf_Modify},
    {SVPN_USER_TYPE_ADMIN, "Role.Get",      svpn_rolemf_Get},
    {SVPN_USER_TYPE_ADMIN, "Role.List",     svpn_rolemf_List},
};

BS_STATUS SVPN_Role_Init()
{
    return SVPN_MF_Reg(g_astSvpnRoleMfMap, sizeof(g_astSvpnRoleMfMap)/sizeof(SVPN_MF_MAP_S));
}

/* 命令行 */

/* role xxx */
PLUG_API BS_STATUS SVPN_RoleCmd_EnterView(UINT ulArgc, char **argv, VOID *pEnv)
{
    return SVPN_CD_EnterView(pEnv, SVPN_CTXDATA_ROLE, argv[1]);
}

/* description xxx */
PLUG_API BS_STATUS SVPN_RoleCmd_SetDescription(UINT argc,
        char **argv, VOID *pEnv)
{
    return SVPN_CD_SetProp(pEnv, SVPN_CTXDATA_ROLE, "Description", argv[1]);
}

/* acl xxx */
PLUG_API BS_STATUS SVPN_RoleCmd_SetAcl(IN UINT ulArgc, IN CHAR **argv, IN VOID *pEnv)
{
    return SVPN_CD_AddPropElement(pEnv, SVPN_CTXDATA_ROLE, "ACL", argv[1], SVPN_PROPERTY_SPLIT);
}

/* web-resource xxx */
PLUG_API BS_STATUS SVPN_RoleCmd_AddWebRes(IN UINT ulArgc, IN CHAR **argv, IN VOID *pEnv)
{
    return SVPN_CD_AddPropElement(pEnv, SVPN_CTXDATA_ROLE, "WebRes", argv[1], SVPN_PROPERTY_SPLIT);
}

/* tcp-resource xxx */
PLUG_API BS_STATUS SVPN_RoleCmd_AddTcpRes(IN UINT ulArgc, IN CHAR **argv, IN VOID *pEnv)
{
    return SVPN_CD_AddPropElement(pEnv, SVPN_CTXDATA_ROLE, "tcpRes", argv[1], SVPN_PROPERTY_SPLIT);
}

/* ip-resource xxx */
PLUG_API BS_STATUS SVPN_RoleCmd_AddIpRes(IN UINT ulArgc, IN CHAR **argv, IN VOID *pEnv)
{
    return SVPN_CD_AddPropElement(pEnv, SVPN_CTXDATA_ROLE, "IpRes", argv[1], SVPN_PROPERTY_SPLIT);
}

BS_STATUS SVPN_RoleCmd_Save(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN HANDLE hFile)
{
    CHAR szName[SVPN_MAX_RES_NAME_LEN + 1] = "";
        
    while (BS_OK == SVPN_CtxData_GetNextObject(hSvpnContext, SVPN_CTXDATA_ROLE, szName, szName, SVPN_MAX_RES_NAME_LEN + 1))
    {
        if (0 != CMD_EXP_OutputMode(hFile, "role %s", szName)) {
            continue;
        }

        SVPN_CD_SaveProp(hSvpnContext, SVPN_CTXDATA_ROLE, szName, "Description", "description", hFile);
        SVPN_CD_SaveElements(hSvpnContext, SVPN_CTXDATA_ROLE, szName, "ACL", "acl", SVPN_PROPERTY_SPLIT, hFile);
        SVPN_CD_SaveElements(hSvpnContext, SVPN_CTXDATA_ROLE, szName, "WebRes", "web-resource", SVPN_PROPERTY_SPLIT, hFile);
        SVPN_CD_SaveElements(hSvpnContext, SVPN_CTXDATA_ROLE, szName, "TcpRes", "tcp-resource", SVPN_PROPERTY_SPLIT, hFile);
        SVPN_CD_SaveElements(hSvpnContext, SVPN_CTXDATA_ROLE, szName, "IpRes", "ip-resource", SVPN_PROPERTY_SPLIT, hFile);

        CMD_EXP_OutputModeQuit(hFile);
    }

    return BS_OK;
}

