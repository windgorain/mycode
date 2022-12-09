/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-6-16
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/txt_utl.h"
#include "utl/lstr_utl.h"
#include "utl/mutex_utl.h"
#include "utl/uri_acl.h"
#include "utl/exec_utl.h"

#include "../h/svpn_def.h"
#include "../h/svpn_context.h"
#include "../h/svpn_ctxdata.h"
#include "../h/svpn_dweb.h"
#include "../h/svpn_mf.h"

typedef struct
{
    MUTEX_S stMutex;
    LIST_RULE_HANDLE hUriAcl;
}SVPN_ACL_CTX_S;

SVPN_ACL_CTX_S * svpn_acl_GetCtrl(IN SVPN_CONTEXT_HANDLE hSvpnContext)
{
    return SVPN_Context_GetUserData(hSvpnContext, SVPN_CONTEXT_DATA_INDEX_ACL);
}

static BS_STATUS svpn_acl_SetAclContent(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN CHAR *pcListName, IN CHAR *pcRules)
{
    SVPN_ACL_CTX_S *pstAclCtx;
    UINT64 uiListID;
    UINT64 uiOldListID;
    UINT uiRuleID = 0;
    LSTR_S stString;
    CHAR cSaved;

    pstAclCtx = svpn_acl_GetCtrl(hSvpnContext);
    if (NULL == pstAclCtx)
    {
        return BS_ERR;
    }
    uiOldListID = URI_ACL_FindListByName(pstAclCtx->hUriAcl, pcListName);

    uiListID = URI_ACL_AddList(pstAclCtx->hUriAcl, pcListName);
    if (0 == uiListID)
    {
        return BS_NO_MEMORY;
    }

    LSTR_SCAN_ELEMENT_BEGIN(pcRules, strlen(pcRules), '>', &stString)
    {
        LSTR_Strim(&stString, " \t\r\n", &stString);
        if (stString.uiLen > 0)
        {
            cSaved = stString.pcData[stString.uiLen];
            stString.pcData[stString.uiLen] = '\0';
            uiRuleID ++;
            URI_ACL_AddRuleToListID(pstAclCtx->hUriAcl, uiListID, uiRuleID, stString.pcData, NULL);
            stString.pcData[stString.uiLen] = cSaved;
        }
    }LSTR_SCAN_ELEMENT_END();

    if (uiOldListID != 0)
    {
        URI_ACL_DelList(pstAclCtx->hUriAcl, uiOldListID);
    }

    return BS_OK;
}

static BS_STATUS svpn_acl_RestoreAclList(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN CHAR *pcListName)
{
    SVPN_ACL_CTX_S *pstAclCtx;
    CHAR *pcRules;
    HSTRING hString;

    pstAclCtx = svpn_acl_GetCtrl(hSvpnContext);
    if (NULL == pstAclCtx)
    {
        return BS_ERR;
    }

    pcRules = "";

    hString = SVPN_CtxData_GetPropAsHString(hSvpnContext, SVPN_CTXDATA_ACL, pcListName, "Rules");
    if (NULL != hString)
    {
        pcRules = STRING_GetBuf(hString);
    }

    svpn_acl_SetAclContent(hSvpnContext, pcListName, pcRules);

    STRING_Delete(hString);

    return BS_OK;
}

static BS_STATUS svpn_acl_SetAcl(IN MIME_HANDLE hMime,  IN SVPN_DWEB_S *pstDweb)
{
    CHAR *pcName;
    CHAR *pcRules;

    pcName = MIME_GetKeyValue(hMime, "Name");
    if ((NULL == pcName) || (pcName[0] == '\0'))
    {
        return BS_ERR;
    }

    pcRules = MIME_GetKeyValue(hMime, "Rules");
    if (NULL == pcRules)
    {
        pcRules = "";
    }

    return svpn_acl_SetAclContent(pstDweb->hSvpnContext, pcName, pcRules);
}

static VOID svpn_acl_DelAcl(IN MIME_HANDLE hMime,  IN SVPN_DWEB_S *pstDweb)
{
    SVPN_ACL_CTX_S *pstAclCtx;
    CHAR *pcName;
    UINT64 uiListID;

    pstAclCtx = svpn_acl_GetCtrl(pstDweb->hSvpnContext);
    if (NULL == pstAclCtx)
    {
        return;
    }

    pcName = MIME_GetKeyValue(hMime, "Name");
    if ((NULL == pcName) || (pcName[0] == '\0'))
    {
        return;
    }

    uiListID = URI_ACL_FindListByName(pstAclCtx->hUriAcl, pcName);
    if (0 != uiListID)
    {
        URI_ACL_DelList(pstAclCtx->hUriAcl, uiListID);
    }
}

static BS_STATUS svpn_acl_ContextCreate(IN SVPN_CONTEXT_HANDLE hSvpnContext)
{
    SVPN_ACL_CTX_S *pstAclCtx;
    CHAR szAclList[SVPN_MAX_RES_NAME_LEN + 1] = "";

    pstAclCtx = MEM_ZMalloc(sizeof(SVPN_ACL_CTX_S));
    if (NULL == pstAclCtx)
    {
        return BS_NO_MEMORY;
    }

    pstAclCtx->hUriAcl = URI_ACL_Create(NULL);

    if (NULL == pstAclCtx->hUriAcl)
    {
        MEM_Free(pstAclCtx);
        return BS_ERR;
    }

    MUTEX_Init(&pstAclCtx->stMutex);

    SVPN_Context_SetUserData(hSvpnContext, SVPN_CONTEXT_DATA_INDEX_ACL, pstAclCtx);

    /* 恢复配置 */
    while (BS_OK == SVPN_CtxData_GetNextObject(hSvpnContext,
        SVPN_CTXDATA_ACL, szAclList, szAclList, sizeof(szAclList)))
    {
        svpn_acl_RestoreAclList(hSvpnContext, szAclList);
    }

    return BS_OK;
}

static BS_STATUS svpn_acl_ContextDestroy(IN SVPN_CONTEXT_HANDLE hSvpnContext)
{
    SVPN_ACL_CTX_S *pstAclCtx;

    pstAclCtx = svpn_acl_GetCtrl(hSvpnContext);
    if (NULL != pstAclCtx)
    {
        SVPN_Context_SetUserData(hSvpnContext, SVPN_CONTEXT_DATA_INDEX_ACL, NULL);
        MUTEX_Final(&pstAclCtx->stMutex);
        URI_ACL_Destroy(pstAclCtx->hUriAcl);
        MEM_Free(pstAclCtx);
    }

    return BS_OK;
}

static BS_STATUS svpn_acl_ContextEvent(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN UINT uiEvent)
{
    BS_STATUS eRet = BS_OK;

    switch (uiEvent)
    {
        case SVPN_CONTEXT_EVENT_CREATE:
        {
            svpn_acl_ContextCreate(hSvpnContext);
            break;
        }

        case SVPN_CONTEXT_EVENT_DESTROY:
        {
            svpn_acl_ContextDestroy(hSvpnContext);
            break;
        }

        default:
        {
            break;
        }
    }

    return eRet;
}

BS_STATUS SVPN_ACL_Match
(
    IN SVPN_CONTEXT_HANDLE hSvpnContext,
    IN CHAR *pcAclListName,
    IN URI_ACL_MATCH_INFO_S *pstMatchInfo,
    OUT BOOL_T *pbPermit
)
{
    SVPN_ACL_CTX_S *pstAclCtx;
    UINT64 uiListID;
    BS_ACTION_E enAction;

    *pbPermit = FALSE;

    pstAclCtx = svpn_acl_GetCtrl(hSvpnContext);
    if (NULL == pstAclCtx)
    {
        return BS_ERR;
    }

    uiListID = URI_ACL_FindListByName(pstAclCtx->hUriAcl, pcAclListName);
    if (0 == uiListID)
    {
        return BS_NOT_FOUND;
    }

    if (BS_OK != URI_ACL_Match(pstAclCtx->hUriAcl, uiListID, pstMatchInfo, &enAction))
    {
        return BS_NOT_FOUND;
    }

    if(enAction == BS_ACTION_PERMIT)
    {
        *pbPermit = TRUE;
    }

    return BS_OK;
}

/********MF接口*********/
static CHAR *g_apcSvpnAclProperty[] = {"Description", "Rules"};
static UINT g_uiSvpnAclPropertyCount = sizeof(g_apcSvpnAclProperty)/sizeof(CHAR*);

static VOID svpn_aclmf_IsExist(IN MIME_HANDLE hMime, IN SVPN_DWEB_S *pstDweb)
{
    SVPN_MF_CommonIsExist(hMime, pstDweb, SVPN_CTXDATA_ACL);
}

static VOID svpn_aclmf_Add(IN MIME_HANDLE hMime,  IN SVPN_DWEB_S *pstDweb)
{
    if (BS_OK != svpn_acl_SetAcl(hMime, pstDweb))
    {
        svpn_mf_SetFailed(pstDweb, "Internal error");
        return;
    }

    if (BS_OK != SVPN_MF_CommonAdd(hMime, pstDweb, SVPN_CTXDATA_ACL,
        g_apcSvpnAclProperty, g_uiSvpnAclPropertyCount))
    {
        svpn_acl_DelAcl(hMime, pstDweb);
        return;
    }
}

static VOID svpn_aclmf_Modify(IN MIME_HANDLE hMime, IN SVPN_DWEB_S *pstDweb)
{
    if (BS_OK != svpn_acl_SetAcl(hMime, pstDweb))
    {
        svpn_mf_SetFailed(pstDweb, "Internal error");
        return;
    }

    SVPN_MF_CommonModify(hMime, pstDweb, SVPN_CTXDATA_ACL, g_apcSvpnAclProperty, g_uiSvpnAclPropertyCount);
}

static VOID svpn_aclmf_Get(IN MIME_HANDLE hMime, IN SVPN_DWEB_S *pstDweb)
{
    SVPN_MF_CommonGet(hMime, pstDweb, SVPN_CTXDATA_ACL, g_apcSvpnAclProperty, g_uiSvpnAclPropertyCount);
}

static VOID svpn_acl_DeleteNotify(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN CHAR *pcName)
{
    SVPN_CtxData_AllDelPropElement(hSvpnContext, SVPN_CTXDATA_ROLE, "ACL", pcName, SVPN_PROPERTY_SPLIT);
}

static VOID svpn_aclmf_Delete(IN MIME_HANDLE hMime, IN SVPN_DWEB_S *pstDweb)
{
    SVPN_MF_CommonDelete(hMime, pstDweb, SVPN_CTXDATA_ACL, svpn_acl_DeleteNotify);
}

static VOID svpn_aclmf_List(IN MIME_HANDLE hMime, IN SVPN_DWEB_S *pstDweb)
{
    SVPN_MF_CommonList(hMime, pstDweb, SVPN_CTXDATA_ACL, g_apcSvpnAclProperty, g_uiSvpnAclPropertyCount);
}

static SVPN_MF_MAP_S g_astSvpnUriAclMfMap[] =
{
    {SVPN_USER_TYPE_ADMIN, "ACL.IsExist",    svpn_aclmf_IsExist},
    {SVPN_USER_TYPE_ADMIN, "ACL.Add",        svpn_aclmf_Add},
    {SVPN_USER_TYPE_ADMIN, "ACL.Modify",     svpn_aclmf_Modify},
    {SVPN_USER_TYPE_ADMIN, "ACL.Get",        svpn_aclmf_Get},
    {SVPN_USER_TYPE_ADMIN, "ACL.Delete",     svpn_aclmf_Delete},
    {SVPN_USER_TYPE_ADMIN, "ACL.List",       svpn_aclmf_List},
};

BS_STATUS SVPN_ACL_Init()
{
    SVPN_Context_RegEvent(svpn_acl_ContextEvent);

    return SVPN_MF_Reg(g_astSvpnUriAclMfMap, sizeof(g_astSvpnUriAclMfMap)/sizeof(SVPN_MF_MAP_S));
}


/* 命令行 */

/* acl xxx */
PLUG_API BS_STATUS SVPN_AclCmd_EnterView(UINT ulArgc, CHAR **argv, VOID *pEnv)
{
    return SVPN_CD_EnterView(pEnv, SVPN_CTXDATA_ACL, argv[1]);
}

/* description xxx */
PLUG_API BS_STATUS SVPN_AclCmd_SetDescription(UINT ulArgc,
        char **argv, VOID *pEnv)
{
    return SVPN_CD_SetProp(pEnv, SVPN_CTXDATA_ACL, "Description", argv[1]);
}

/* rule {permit|deny} xxx */
PLUG_API BS_STATUS SVPN_AclCmd_SetRule(IN UINT ulArgc, IN UCHAR **argv, IN VOID *pEnv)
{
    CHAR szTmp[512];

    snprintf(szTmp, sizeof(szTmp), "%s %s", argv[1], argv[2]);

    return SVPN_CD_AddPropElement(pEnv, SVPN_CTXDATA_ACL, "Rules", szTmp, SVPN_PROPERTY_LINE_SPLIT);
}

/* submit */
PLUG_API BS_STATUS SVPN_AclCmd_Submit(IN UINT ulArgc, IN UCHAR **argv, IN VOID *pEnv)
{
    SVPN_CONTEXT_HANDLE hSvpnContext;
    CHAR *pcName;
    CHAR *pcRules;
    HSTRING hString;

    hSvpnContext = SVPN_Context_GetByEnv(pEnv, 1);
    if (hSvpnContext == NULL)
    {
        EXEC_OutString("Can't get context");
        return BS_ERR;
    }

    pcName = CMD_EXP_GetCurrentModeValue(pEnv);
    if (NULL == pcName)
    {
        EXEC_OutString("Can't get name");
        return BS_ERR;
    }

    hString = SVPN_CtxData_GetPropAsHString(hSvpnContext, SVPN_CTXDATA_ACL, pcName, "Rules");
    if (NULL == hString)
    {
        return BS_OK;
    }

    pcRules = STRING_GetBuf(hString);
    if ((NULL != pcRules) && (pcRules[0] != 0))
    {
        svpn_acl_SetAclContent(hSvpnContext, pcName, pcRules);
    }

    STRING_Delete(hString);

    return BS_OK;
}

BS_STATUS SVPN_AclCmd_Save(IN SVPN_CONTEXT_HANDLE hSvpnContext, IN HANDLE hFile)
{
    CHAR szName[SVPN_MAX_RES_NAME_LEN + 1] = "";
        
    while (BS_OK == SVPN_CtxData_GetNextObject(hSvpnContext, SVPN_CTXDATA_ACL, szName, szName, SVPN_MAX_RES_NAME_LEN + 1))
    {
        if (0 != CMD_EXP_OutputMode(hFile, "acl %s", szName)) {
            continue;
        }

        SVPN_CD_SaveProp(hSvpnContext, SVPN_CTXDATA_ACL, szName, "Description", "description", hFile);
        SVPN_CD_SaveElements(hSvpnContext, SVPN_CTXDATA_ACL, szName, "Rules", "rule", SVPN_PROPERTY_LINE_SPLIT, hFile);

        CMD_EXP_OutputCmd(hFile, "submit");
        
        CMD_EXP_OutputModeQuit(hFile);
    }

    return BS_OK;
}

