/******************************************************************************
* Copyright (C), 2000-2006,  LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2016-10-18
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/mutex_utl.h"
#include "utl/socket_utl.h"
#include "utl/ip_acl.h"
#include "utl/uri_acl.h"
#include "utl/txt_utl.h"
#include "utl/exec_utl.h"
#include "utl/list_rule.h"
#include "utl/bit_opt.h"
#include "comp/comp_acl.h"

static IPACL_HANDLE g_hAclAppURIAcl;
static MUTEX_S g_stAclAppURIMutex;

static BS_STATUS _aclappuri_GetListIDByName(IN ACL_NAME_ID_S *pstNameID)
{
    UINT ulListID;
    
    MUTEX_P(&g_stAclAppURIMutex);
    ulListID = URI_ACL_FindListByName(g_hAclAppURIAcl, pstNameID->pcAclListName);
    MUTEX_V(&g_stAclAppURIMutex);

    pstNameID->ulAclListID = ulListID;

    if (ulListID == ACL_INVALID_LIST_ID)
    {
        return BS_NOT_FOUND;
    }

    return BS_OK;
}

/* 增加List引用计数 */
static BS_STATUS _aclappuri_AddListRef(IN CHAR *pcListName)
{
    BS_STATUS eRet;
    
    MUTEX_P(&g_stAclAppURIMutex);
    eRet = URI_ACL_AddListRef(g_hAclAppURIAcl, URI_ACL_FindListByName(g_hAclAppURIAcl, pcListName));
    MUTEX_V(&g_stAclAppURIMutex);

    return eRet;
}

static BS_STATUS _aclappuri_DelListRef(IN CHAR *pcListName)
{
    BS_STATUS eRet;
    
    MUTEX_P(&g_stAclAppURIMutex);
    eRet = URI_ACL_DelListRef(g_hAclAppURIAcl, URI_ACL_FindListByName(g_hAclAppURIAcl, pcListName));
    MUTEX_V(&g_stAclAppURIMutex);

    return eRet;
}

static BS_STATUS _aclappuri_DelListRefByID(IN UINT *pulListID)
{
    BS_STATUS eRet;
    
    MUTEX_P(&g_stAclAppURIMutex);
    eRet = URI_ACL_DelListRef(g_hAclAppURIAcl, *pulListID);
    MUTEX_V(&g_stAclAppURIMutex);

    return eRet;
}

static BS_STATUS _aclappuri_EnterListView(IN CHAR *pcListName)
{
    UINT uiListID;

    uiListID = URI_ACL_FindListByName(g_hAclAppURIAcl, pcListName);
    if (uiListID == 0)
    {
        uiListID = URI_ACL_AddList(g_hAclAppURIAcl, pcListName);
    }

    if (uiListID == 0)
    {
        return BS_ERR;
    }

    return BS_OK;
}
#if 0
static BS_STATUS _aclappuri_EnterRuleView(IN CHAR *pcListName, IN UINT uiRuleID)
{
    UINT ulListID;
    IPACL_RULE_CFG_S stRuleCfg = {0};

    ulListID = URI_ACL_FindListByName(g_hAclAppURIAcl, pcListName);
    if (0 == ulListID)
    {
        return BS_ERR;
    }

    if (NULL != IPACL_GetRule(g_hAclAppURIAcl, ulListID, uiRuleID))
    {
        return BS_OK;
    }

    stRuleCfg.enAction = BS_ACTION_DENY;

    return IPACL_AddRule(g_hAclAppURIAcl, ulListID, uiRuleID, &stRuleCfg);
}
#endif


static BS_STATUS _aclappuri_NoAclList(IN CHAR *pcListName)
{
    UINT ulListID;

    ulListID = IPACL_GetListByName(g_hAclAppURIAcl, pcListName);
    if (0 == ulListID)
    {
        return BS_OK;
    }

    if (IPACL_ListGetRef(g_hAclAppURIAcl, ulListID) > 0)
    {
        return BS_REF_NOT_ZERO;
    }

    IPACL_DelList(g_hAclAppURIAcl, ulListID);

    return BS_OK;
}

static BS_STATUS _aclappuri_MoveRule(IN CHAR *pcListName, IN UINT uiOldRuleID, IN UINT uiNewRuleID)
{
    return IPACL_MoveRule(g_hAclAppURIAcl, IPACL_GetListByName(g_hAclAppURIAcl, pcListName), uiOldRuleID, uiNewRuleID);
}

static VOID _aclappuri_SaveCmdInRule(IN HANDLE hFile, IN UINT ulListID, IN UINT uiRuleID)
{
    URI_ACL_RULE_S *pstRule;
    CHAR *action;

    pstRule = URI_ACL_GetRule(g_hAclAppURIAcl, ulListID, uiRuleID);
    if (NULL == pstRule)
    {
        return;
    }

    if (pstRule->action == BS_ACTION_PERMIT)
    {
        action = "permit";
    }
    else
    {
        action = "deny";
    }

    CMD_EXP_OutputCmd(hFile, "rule %d uri %s action %s", uiRuleID, pstRule->stPattern.szPattern, action);
}

static VOID aclappuri_SaveCmdInList(IN HANDLE hFile, IN UINT ulListID)
{
    UINT uiRuleID = RULE_ID_INVALID;

    while ((uiRuleID = IPACL_GetNextRuleID(g_hAclAppURIAcl, ulListID, uiRuleID)) != 0) {
        _aclappuri_SaveCmdInRule(hFile, ulListID, uiRuleID);
    }
}

BS_STATUS AclAppURI_Init()
{
    g_hAclAppURIAcl = URI_ACL_Create(RcuEngine_GetMemcap());
    if (NULL == g_hAclAppURIAcl)
    {
        return BS_NO_MEMORY;
    }

    MUTEX_Init(&g_stAclAppURIMutex);

    return BS_OK;
}


/* CMD */

/* uri-acl %STRING */
PLUG_API BS_STATUS AclAppURI_EnterListView(IN UINT uiArgc, IN CHAR **ppcArgv, IN VOID *pEnv)
{
    BS_STATUS eRet;

    MUTEX_P(&g_stAclAppURIMutex);
    eRet = _aclappuri_EnterListView(ppcArgv[1]);
    MUTEX_V(&g_stAclAppURIMutex);
    
    if (BS_OK != eRet)
    {
        EXEC_OutInfo("%s.\r\n", ErrInfo_Get(eRet));
        return BS_ERR;
    }

    return BS_OK;
}

/* no uri-acl %STRING */
PLUG_API BS_STATUS AclAppURI_CmdNoList(IN UINT uiArgc, IN CHAR **ppcArgv, IN VOID *pEnv)
{
    BS_STATUS eRet;

    MUTEX_P(&g_stAclAppURIMutex);
    eRet = _aclappuri_NoAclList(ppcArgv[2]);
    MUTEX_V(&g_stAclAppURIMutex);

    if (eRet == BS_REF_NOT_ZERO)
    {
        EXEC_OutInfo("%s.\r\n", ErrInfo_Get(eRet));
    }

    return eRet;
}

/* move rule %INT to %INT */
PLUG_API BS_STATUS AclAppURI_CmdMoveRule(IN UINT uiArgc, IN CHAR **ppcArgv, IN VOID *pEnv)
{
    CHAR *pcListName;
    UINT uiOldRuleID;
    UINT uiNewRuleID;
    BS_STATUS eRet;

    uiOldRuleID = TXT_Str2Ui(ppcArgv[2]);
    uiNewRuleID = TXT_Str2Ui(ppcArgv[4]);

    if ((uiOldRuleID == 0) || (uiNewRuleID == 0))
    {
        return BS_ERR;
    }

    pcListName = CMD_EXP_GetCurrentModeValue(pEnv);

    MUTEX_P(&g_stAclAppURIMutex);
    eRet = _aclappuri_MoveRule(pcListName, uiOldRuleID, uiNewRuleID);
    MUTEX_V(&g_stAclAppURIMutex);

    if (eRet != BS_OK)
    {
        EXEC_OutInfo("%s.\r\n", ErrInfo_Get(eRet));
    }

    return eRet;
}

/* rule %INT<1-1000> uri %STRING<1-255> action {permit | deny} */
/* [no] rule %IN%<1-1000> */
PLUG_API BS_STATUS AclAppURI_CmdCfgRule(IN UINT uiArgc, IN CHAR **ppcArgv, IN VOID *pEnv)
{
    CHAR *pcListName;
    UINT rule_id;
    LIST_RULE_LIST_S* acl_list;

    pcListName = CMD_EXP_GetCurrentModeValue(pEnv);
    acl_list = URI_ACL_GetListByName(g_hAclAppURIAcl, pcListName);
    
    if(ppcArgv[0][0] == 'n') {
        TXT_Atoui(ppcArgv[2], &rule_id);
        MUTEX_P(&g_stAclAppURIMutex);
        URI_ACL_DelRule(acl_list, rule_id);
        MUTEX_V(&g_stAclAppURIMutex);
    }else {
        TXT_Atoui(ppcArgv[1], &rule_id);
        URI_ACL_AddRule(g_hAclAppURIAcl, acl_list, rule_id, ppcArgv[3], ppcArgv[5]);
    }

    return BS_OK;
}

PLUG_API BS_ACTION_E ACLURI_Match(IN UINT ulListID, IN URI_ACL_MATCH_INFO_S *pstMatchInfo)
{
    BS_ACTION_E enAction;

    MUTEX_P(&g_stAclAppURIMutex);
    URI_ACL_Match(g_hAclAppURIAcl, ulListID, pstMatchInfo, &enAction);
    MUTEX_V(&g_stAclAppURIMutex);

    return enAction;
}

/*match %STRING<1-255>*/
PLUG_API BS_STATUS ACLURI_Match_Test(IN UINT uiArgc, IN CHAR **ppcArgv, IN VOID *pEnv)
{
    CHAR *pcListName;
    UINT list_id;
    URI_ACL_MATCH_INFO_S matchinfo;
    BS_ACTION_E eRet;

    pcListName = CMD_EXP_GetCurrentModeValue(pEnv);
    list_id = URI_ACL_FindListByName(g_hAclAppURIAcl, pcListName);
    
    matchinfo.uiFlag = URI_ACL_KEY_URI;
    TXT_Strlcpy((CHAR*)matchinfo.szDomain, ppcArgv[1], sizeof(matchinfo.szDomain));
    matchinfo.enProtocol = URI_ACL_PROTOCOL_HTTP;

    eRet = ACLURI_Match(list_id, &matchinfo); 

    printf("Match Result %d\r\n", eRet);

    return BS_OK;
}
PLUG_API BS_STATUS ACLIP_Ioctl(IN COMP_ACL_IOCTL_E enCmd, IN VOID *pData)
{
    BS_STATUS eRet = BS_OK;

    switch (enCmd) {
        case COMP_ACL_IOCTL_ADD_LIST_REF:
            eRet = _aclappuri_AddListRef(pData);
            break;
        case COMP_ACL_IOCTL_DEL_LIST_REF:
            eRet = _aclappuri_DelListRef(pData);
            break;
        case COMP_ACL_IOCTL_DEL_LIST_REF_BY_ID:
            _aclappuri_DelListRefByID(pData);
            break;
        case COMP_ACL_IOCTL_GET_LIST_ID:
            eRet = _aclappuri_GetListIDByName(pData);
            break;
        default:
            eRet = BS_NOT_SUPPORT;
            break;
    }

    return eRet;
}

BS_STATUS AclAppURI_Save(IN HANDLE hFile)
{
    UINT ulListID = 0;
    CHAR *pcListName;

    MUTEX_P(&g_stAclAppURIMutex);

    while ((ulListID = ListRule_GetNextListID(g_hAclAppURIAcl, ulListID)) != 0) {
        pcListName = ListRule_GetListNameByID(g_hAclAppURIAcl, ulListID);
        if (0 == CMD_EXP_OutputMode(hFile, "uri-acl %s", pcListName)) {
            aclappuri_SaveCmdInList(hFile, ulListID);
            CMD_EXP_OutputModeQuit(hFile);
        }
    }

    MUTEX_V(&g_stAclAppURIMutex);

    return BS_OK;
}

