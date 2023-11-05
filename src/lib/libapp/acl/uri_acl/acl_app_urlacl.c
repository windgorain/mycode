/******************************************************************************
* Copyright (C), 2000-2006,  LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2016-10-18
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/mutex_utl.h"
#include "utl/socket_utl.h"
#include "utl/time_utl.h"
#include "utl/uri_acl.h"
#include "utl/txt_utl.h"
#include "utl/exec_utl.h"
#include "utl/bit_opt.h"
#include "utl/getopt2_utl.h"
#include "utl/ip_utl.h"
#include "utl/ip_string.h"
#include "utl/lstr_utl.h"
#include "utl/file_utl.h"
#include "utl/args_utl.h"
#include "comp/comp_acl.h"
#include "../h/acl_muc.h"
#include "../h/acl_app_func.h"
#include "utl/uri_acl.h"

#define URL_ACL_CMD_RULE_ELEMENT_MAX 14
#define URL_ACL_CMD_RULE_ELEMENT_MIN 4

static inline int _aclurl_set_instance_locked(ACL_MUC_S *acl_muc, URL_ACL_HANDLE urlacl_handle)
{
    if (acl_muc->urlacl_handle != NULL) {
        RETURN(BS_ALREADY_EXIST);
    }

    acl_muc->urlacl_handle = urlacl_handle;

    return 0;
}

static inline void _aclurl_create_instance(ACL_MUC_S *acl_muc)
{
    int ret;

    URL_ACL_HANDLE urlacl_handle = URL_ACL_Create(RcuEngine_GetMemcap());
    if (! urlacl_handle) {
        return;
    }

    SPLX_P();
    ret = _aclurl_set_instance_locked(acl_muc, urlacl_handle);
    SPLX_V();

    if (ret != 0) {
        URL_ACL_Destroy(urlacl_handle);
    }

    return;
}

static URL_ACL_HANDLE _aclurl_get_create_instance(int muc_id)
{
    ACL_MUC_S *acl_muc;

    acl_muc = AclMuc_Get(muc_id);
    if (! acl_muc) {
        return NULL;
    }

    if (acl_muc->urlacl_handle) {
        return acl_muc->urlacl_handle;
    }

    _aclurl_create_instance(acl_muc);

    return acl_muc->urlacl_handle;
}

static inline URL_ACL_HANDLE _aclurl_get_instance(int muc_id)
{
    ACL_MUC_S *acl_muc;

    acl_muc = AclMuc_Get(muc_id);
    if (! acl_muc) {
        return NULL;
    }

    return acl_muc->urlacl_handle;
}

static URL_ACL_HANDLE _aclurl_get_create_instance_by_env(void *env)
{
    int muc_id = CmdExp_GetEnvMucID(env);
    return _aclurl_get_create_instance(muc_id);
}

static URL_ACL_HANDLE _aclurl_get_instance_by_env(void *env)
{
    int muc_id = CmdExp_GetEnvMucID(env);
    return _aclurl_get_instance(muc_id);
}

static BS_STATUS _aclurl_GetListIDByName(IN URL_ACL_HANDLE urlacl, IN ACL_NAME_ID_S *pstNameID)
{
    UINT ulListID;

    ulListID = URL_ACL_GetListByName(urlacl, pstNameID->pcAclListName);
    pstNameID->ulAclListID = ulListID;
    if (ulListID == ACL_INVALID_LIST_ID)
    {
        return BS_NOT_FOUND;
    }

    return BS_OK;
}


static BS_STATUS _aclurl_AddListRef(IN URL_ACL_HANDLE urlacl, IN CHAR *pcListName)
{
    return URL_ACL_AddListRef(urlacl, URL_ACL_GetListByName(urlacl, pcListName));
}

static BS_STATUS _aclurl_DelListRef(IN URL_ACL_HANDLE urlacl, IN CHAR *pcListName)
{
    return URL_ACL_DelListRef(urlacl, URL_ACL_GetListByName(urlacl, pcListName));
}

static BS_STATUS _aclurl_DelListRefByID(IN URL_ACL_HANDLE urlacl, IN UINT *pulListID)
{
    return URL_ACL_DelListRef(urlacl, *pulListID);
}

static BS_STATUS _aclurl_EnterListView(IN URL_ACL_HANDLE urlacl, IN CHAR *pcListName)
{
    UINT uiListID;

    uiListID = URL_ACL_GetListByName(urlacl, pcListName);
    if (uiListID == 0)
    {
        uiListID = URL_ACL_AddList(urlacl, pcListName);
    }

    if (uiListID == 0)
    {
        return BS_NO_MEMORY;
    }

    return BS_OK;
}

static BS_STATUS _aclurl_CfgRule(IN URL_ACL_HANDLE urlacl, IN UINT uiListID, IN UINT uiRuleID, IN URL_ACL_RULE_CFG_S *pstRuleCfg)
{
    BS_STATUS enRet = BS_OK;
    URL_ACL_RULE_S *pstRule = NULL;

    pstRule = URL_ACL_GetRule(urlacl, uiListID, uiRuleID);
    if (NULL != pstRule)
    {
        URL_ACL_UpdateRule(pstRule, pstRuleCfg);
    }
    else
    {
        enRet = URL_ACL_AddRule(urlacl, uiListID, uiRuleID, pstRuleCfg);
    }

    return enRet;
}

static BS_STATUS _aclurl_NoAclList(IN URL_ACL_HANDLE urlacl,IN CHAR *pcListName)
{
    UINT uiListID;

    uiListID = URL_ACL_GetListByName(urlacl, pcListName);
    if (0 == uiListID)
    {
        return BS_OK;
    }

    if (URL_ACL_ListGetRef(urlacl, uiListID) > 0)
    {
        return BS_REF_NOT_ZERO;
    }

    URL_ACL_DelList(urlacl, uiListID);

    return BS_OK;
}

static BS_STATUS _aclurl_MoveRule(IN URL_ACL_HANDLE urlacl, IN CHAR *pcListName, IN UINT uiOldRuleID, IN UINT uiNewRuleID)
{
    BS_STATUS enRet;
    UINT uiListID;
    URL_ACL_RULE_S *pstRule;

    uiListID = URL_ACL_GetListByName(urlacl, pcListName);
    if (0 == uiListID)
    {
        return BS_NO_SUCH;
    }

    pstRule = URL_ACL_GetRule(urlacl, uiListID, uiNewRuleID);
    if (NULL != pstRule)
    {
        EXEC_OutInfo("New rule %u is already exist\n", uiNewRuleID);
        return BS_CONFLICT;
    }

    enRet = URL_ACL_MoveRule(urlacl, uiListID, uiOldRuleID, uiNewRuleID);
    if(enRet == BS_NOT_FOUND) {
        EXEC_OutInfo("Not found rule %u\n", uiOldRuleID);
    }

    return enRet;
}

static VOID _aclurl_NoRule(IN URL_ACL_HANDLE urlacl, IN CHAR *pcListName, IN UINT uiRuleID)
{
    URL_ACL_DelRule(urlacl, URL_ACL_GetListByName(urlacl, pcListName), uiRuleID);
}

static BOOL_T _aclurl_ShowStats(IN UINT uiRuleID, IN URL_ACL_RULE_S *pstRule, IN VOID *pUserHandle)
{
    
    return BOOL_TRUE;
}

static BOOL_T _aclurl_ClearStats(IN UINT uiRuleID, IN URL_ACL_RULE_S *pstRule, IN VOID *pUserHandle)
{
    
    return BOOL_TRUE;
}

static BOOL_T _aclurl_SaveCmdInRule(IN UINT uiRuleID, IN URL_ACL_RULE_S *pstRule, IN VOID *pUserHandle)
{
#if 0
    HANDLE hFile = pUserHandle;
    URL_ACL_RULE_CFG_S *pstRuleCfg = &pstRule->stRuleCfg;
    URL_ACL_SINGLE_KEY_S *pstSingleKey = &pstRule->stRuleCfg.stKey.single;
    URL_ACL_POOL_KEY_S *pstPoolKey = &pstRule->stRuleCfg.stKey.pools;
    CHAR acAction[32] = {0};
    CHAR szRuleStr[1024] = {0};
    INT length = 0;

    if (pstRuleCfg->uiKeyMask & URL_ACL_KEY_SIP)
    {
       length = _aclurl_IpKey2Str(&pstSingleKey->stSIP, " --src_ip ", sizeof(szRuleStr), szRuleStr);
    }

    if (pstRuleCfg->uiKeyMask & URL_ACL_KEY_DPORT)
    {
        length += _aclurl_PortKey2Str(&pstSingleKey->stDPort, " --dst_port ", sizeof(szRuleStr)-length, szRuleStr+length);
    }

    if (pstRuleCfg->uiKeyMask & URL_ACL_KEY_POOL_SIP){
        length += scnprintf(szRuleStr+length, sizeof(szRuleStr)-length, " --src_ip_group %s", pstPoolKey->pstSipPool->pcListName);
    }

    if (pstRuleCfg->uiKeyMask & URL_ACL_KEY_POOL_DPORT){
        length += scnprintf(szRuleStr+length, sizeof(szRuleStr)-length, " --dst_port_group %s", pstPoolKey->pstDportPool->pcListName);
    }

    if (pstRuleCfg->uiKeyMask & URL_ACL_KEY_URL){
        length += scnprintf(szRuleStr+length, sizeof(szRuleStr)-length," --url %s", pstSingleKey->szDomain);
    }

    if (pstRuleCfg->uiKeyMask & URL_ACL_KEY_POOL_DOAMIN){
        length += scnprintf(szRuleStr+length, sizeof(szRuleStr)-length, " --url_group %s", pstPoolKey->pstDomainGroup->pcListName);
    }

    if (pstRuleCfg->uiKeyMask & URL_ACL_KEY_PROTO)
    {
        length += scnprintf(szRuleStr+length, sizeof(szRuleStr)-length," --protocol %u", pstRuleCfg->ucProto);
    }

    if (pstRuleCfg->enAction == BS_ACTION_DENY)
    {
        scnprintf(acAction, sizeof(acAction), " action deny");
    }
    else
    {
        scnprintf(acAction, sizeof(acAction), " action permit");
    }

    CMD_EXP_OutputCmd(hFile, "rule %u%s%s",
            uiRuleID, acAction, szRuleStr);
#endif
    return BOOL_TRUE;
}

static VOID aclurl_SaveCmdInList(IN URL_ACL_HANDLE urlacl, IN HANDLE hFile, IN UINT uiListID)
{
    BS_ACTION_E enAction;

    URL_ACL_ScanRule(urlacl, uiListID, _aclurl_SaveCmdInRule, hFile);

    enAction = URL_ACL_GetDefaultActionByID(urlacl, uiListID);
    if (enAction != BS_ACTION_UNDEF)
    {
        CMD_EXP_OutputCmd(hFile, "default action %s", (enAction == BS_ACTION_DENY) ? "deny" : "permit");
    }
}

BS_STATUS AclAppURL_Init()
{
    return BS_OK;
}

void AclURL_DestroyMuc(ACL_MUC_S *acl_muc)
{
    if (acl_muc->urlacl_handle) {
        IPACL_Destroy(acl_muc->urlacl_handle);
        acl_muc->urlacl_handle = NULL;
    }
    return;
}



PLUG_API BS_STATUS AclURL_EnterListView(IN UINT uiArgc, IN CHAR **ppcArgv, IN VOID *pEnv)
{
    BS_STATUS eRet;
    URL_ACL_HANDLE urlacl = _aclurl_get_create_instance_by_env(pEnv);
    if (! urlacl) {
        EXEC_OutInfo("Can't get url acl instance \r\n");
        return BS_ERR;
    }

    eRet = _aclurl_EnterListView(urlacl, ppcArgv[1]);
    if (BS_OK != eRet)
    {
        EXEC_OutInfo("%s \r\n", ErrInfo_Get(eRet));
        return BS_ERR;
    }

    return BS_OK;
}


PLUG_API BS_STATUS AclURL_CmdNoList(IN UINT uiArgc, IN CHAR **ppcArgv, IN VOID *pEnv)
{
    BS_STATUS eRet;
    URL_ACL_HANDLE urlacl = _aclurl_get_instance_by_env(pEnv);
    if (! urlacl) {
        return BS_OK;
    }

    eRet = _aclurl_NoAclList(urlacl, ppcArgv[2]);
    if (eRet == BS_REF_NOT_ZERO)
    {
        EXEC_OutInfo("%s \r\n", ErrInfo_Get(eRet));
    }

    return eRet;
}

PLUG_API BS_STATUS AclURL_Clear(IN UINT uiArgc, IN CHAR **ppcArgv, IN VOID *pEnv)
{
    URL_ACL_HANDLE urlacl = _aclurl_get_instance_by_env(pEnv);
    if (! urlacl) {
        return BS_OK;
    }
    URL_ACL_Reset(urlacl);

    RETURN(BS_OK);
}



PLUG_API BS_STATUS AclURL_CmdMoveRule(IN UINT uiArgc, IN CHAR **ppcArgv, IN VOID *pEnv)
{
    CHAR *pcListName;
    UINT uiOldRuleID;
    UINT uiNewRuleID;
    BS_STATUS eRet;
    URL_ACL_HANDLE urlacl = _aclurl_get_instance_by_env(pEnv);
    if (! urlacl) {
        EXEC_OutInfo("Can't get ipacl instance \r\n");
        return BS_ERR;
    }

    uiOldRuleID = TXT_Str2Ui(ppcArgv[2]);
    uiNewRuleID = TXT_Str2Ui(ppcArgv[4]);

    if ((uiOldRuleID == 0) || (uiNewRuleID == 0))
    {
        return BS_BAD_PARA;
    }

    if (uiOldRuleID == uiNewRuleID)
    {
        return BS_OK;
    }

    pcListName = CMD_EXP_GetCurrentModeValue(pEnv);

    eRet = _aclurl_MoveRule(urlacl, pcListName, uiOldRuleID, uiNewRuleID);
    if (eRet != BS_OK)
    {
        EXEC_OutInfo("%s \r\n", ErrInfo_Get(eRet));
    }

    return eRet;
}


PLUG_API BS_STATUS AclURL_CmdNoRule(IN UINT uiArgc, IN CHAR **ppcArgv, IN VOID *pEnv)
{
    CHAR *pcListName;
    UINT uiRuleID = 0;
    CHAR *pcRuleID = ppcArgv[2];

    URL_ACL_HANDLE urlacl = _aclurl_get_instance_by_env(pEnv);
    if (! urlacl) {
        EXEC_OutInfo("Can't get ipacl instance \r\n");
        return BS_ERR;
    }

    TXT_Atoui(pcRuleID, &uiRuleID);

    if (uiRuleID == 0)
    {
        EXEC_OutString("ACL rule id invalid \r\n");
        RETURN(BS_BAD_PARA);
    }

    pcListName = CMD_EXP_GetCurrentModeValue(pEnv);
    _aclurl_NoRule(urlacl, pcListName, uiRuleID);

    return BS_OK;
}

PLUG_API BS_STATUS AclURL_CmdRebaseRuleID(IN UINT uiArgc, IN CHAR **ppcArgv, IN VOID *pEnv)
{
    UINT uiListID;
    UINT uiRuleID;
    UINT uiStep = 0;
    CHAR *pcListName;
    URL_ACL_HANDLE urlacl = _aclurl_get_instance_by_env(pEnv);
    if (! urlacl) {
        EXEC_OutInfo("Can't get urlacl instance \r\n");
        return BS_ERR;
    }

    TXT_Atoui(ppcArgv[2], &uiStep);
    if (uiStep == 0)
    {
        EXEC_OutString("Rebase step is invalid\r\n");
        return BS_BAD_PARA;
    }

    pcListName = CMD_EXP_GetCurrentModeValue(pEnv);
    uiListID = URL_ACL_GetListByName(urlacl, pcListName);
    if (0 == uiListID)
    {
        EXEC_OutInfo("ACL list %s is not found \r\n", pcListName);
        return BS_NO_SUCH;
    }

    uiRuleID = IPACL_GetLastRuleID(urlacl, uiListID);
    if (0 != uiRuleID && uiRuleID + uiStep > URL_ACL_RULE_ID_MAX)
    {
        EXEC_OutString("Rule id reatch max\r\n");
        return BS_REACH_MAX;
    }

    return URL_ACL_RebaseID(urlacl, uiListID, uiStep);
}

PLUG_API BS_STATUS AclURL_CmdIncreaseID(IN UINT uiArgc, IN CHAR **ppcArgv, IN VOID *pEnv)
{
    BS_STATUS enRet;
    UINT uiStart;
    UINT uiEnd;
    UINT uiStep = 1;
    UINT uiListID;
    UINT uiRuleID;
    URL_ACL_RULE_S *pstRule;
    CHAR *pcListName;
    URL_ACL_HANDLE urlacl = _aclurl_get_instance_by_env(pEnv);
    if (! urlacl) {
        EXEC_OutInfo("Can't get ipacl instance \r\n");
        return BS_ERR;
    }

    enRet = TXT_Atoui(ppcArgv[2], &uiStart);
    enRet |= TXT_Atoui(ppcArgv[4], &uiEnd);
    if (uiArgc > 5)
    {
        enRet |= TXT_Atoui(ppcArgv[6], &uiStep);
    }

    if (BS_OK != enRet)
    {
        EXEC_OutString("Parameter error \r\n");
        return BS_BAD_PARA;
    }

    if (uiEnd < uiStart)
    {
        EXEC_OutString("End id should not be less than start id\r\n");
        return BS_BAD_PARA;
    }

    if (uiEnd + uiStep > URL_ACL_RULE_ID_MAX)
    {
        EXEC_OutString("Rule id reatch max\r\n");
        return BS_REACH_MAX;
    }

    pcListName = CMD_EXP_GetCurrentModeValue(pEnv);
    uiListID = URL_ACL_GetListByName(urlacl, pcListName);
    if (0 == uiListID)
    {
        EXEC_OutInfo("ACL list %s is not found \r\n", pcListName);
        return BS_NO_SUCH;
    }

    pstRule = URL_ACL_GetRule(urlacl, uiListID, uiStart);
    if (NULL == pstRule)
    {
        EXEC_OutString("ACL rule id invalid \r\n");
        return BS_BAD_PARA;
    }

    pstRule = URL_ACL_GetRule(urlacl, uiListID, uiEnd);
    if (NULL == pstRule)
    {
        EXEC_OutString("ACL rule id invalid \r\n");
        return BS_BAD_PARA;
    }

    uiRuleID = URL_ACL_GetNextRuleID(urlacl, uiListID, uiEnd);
    if ((0 != uiRuleID) && (uiRuleID <= uiEnd + uiStep))
    {
        EXEC_OutString("Increase id conflict with other rule \r\n");
        return BS_BAD_PARA;
    }

    URL_ACL_IncreaseID(urlacl, uiListID, uiStart, uiEnd, uiStep);

    return BS_OK;
}

#if 0

static BS_STATUS _aclurl_ParseDomain(IN CHAR* pcArgc, OUT CHAR* pcDomain)
{
    int length = strlen(pcArgc);
    int i = 0;

    if(length > COMMON_URL_MAX_LEN) {
        EXEC_OutInfo("Acl url too long(1-255).\r\n");
        return BS_ERR;
    }

    for(i=1;i<length; i++) {
        if(pcArgc[i] == '*') {
            return BS_BAD_PARA;
        }
    }

    strlcpy(pcDomain, pcArgc, length+1);
    return BS_OK;
    
}
#endif



PLUG_API BS_STATUS AclURL_CmdSet_DefaultAction(IN UINT uiArgc, IN CHAR **ppcArgv, IN VOID *pEnv)
{
    CHAR *pcListName = CMD_EXP_GetCurrentModeValue(pEnv);
    BS_ACTION_E enAction;
    UINT uiListID;
    URL_ACL_HANDLE urlacl = _aclurl_get_instance_by_env(pEnv);
    if (! urlacl) {
        EXEC_OutInfo("Can't get urlacl instance \r\n");
        return BS_ERR;
    }
    if (ppcArgv[0][0] == 'n')
    {
        enAction = BS_ACTION_UNDEF;
    }
    else if (ppcArgv[2][0] == 'p')
    {
        enAction = BS_ACTION_PERMIT;
    }
    else
    {
        enAction = BS_ACTION_DENY;
    }

    uiListID = URL_ACL_GetListByName(urlacl, pcListName);
    if (0 == uiListID)
    {
        return BS_NO_SUCH;
    }

    return URL_ACL_SetDefaultActionByID(urlacl, uiListID, enAction);
}

static BS_STATUS _aclurl_ParseRule(IN INT muc_id, IN UINT uiArgc, IN CHAR **ppcArgv, OUT UINT *puiRuleID, OUT URL_ACL_RULE_CFG_S *pstRuleCfg)
{
#if 0
    BS_STATUS enRet;
    UINT uiRuleID;
    CHAR *pcRuleID = ppcArgv[1];
    BS_ACTION_E enAction;
    CHAR *pcAction = ppcArgv[3];
    URL_ACL_RULE_CFG_S stRuleCfg = {0};
    CHAR *pcDomain = NULL;
    CHAR *pcDomainGroup = NULL;
    CHAR *pcApp = NULL, *pcGroup = NULL;
    UINT uiProto = 0;
    CHAR acHelp[1024];
    GETOPT2_NODE_S opts[] = {
        {'o', 'd', "url", GETOPT2_V_STRING, &pcDomain, "Domain string", 0},
        {'o', 'D', "url_group", GETOPT2_V_STRING, &pcDomainGroup, "Domain group name", 0},
        {0}};


    TXT_Atoui(pcRuleID, &uiRuleID);
    if (uiRuleID == 0)
    {
        EXEC_OutString("ACL rule id invalid \r\n");
        RETURN(BS_NO_SUCH);
    }

    if (uiRuleID > URL_ACL_RULE_ID_MAX)
    {
        EXEC_OutString("ACL rule id invalid \r\n");
        RETURN(BS_NO_SUCH);
    }

    if ('p' == pcAction[0])
    {
        enAction = BS_ACTION_PERMIT;
    }
    else if ('d' == pcAction[0])
    {
        enAction = BS_ACTION_DENY;
    }
    else
    {
        EXEC_OutString("ACL rule action invalid \r\n");
        RETURN(BS_NO_SUCH);
    }

    if (uiArgc > 4)
    {
        URL_ACL_SINGLE_KEY_S *pstSingleKey = &stRuleCfg.stKey.single;
        URL_ACL_POOL_KEY_S *pstPoolKey = &stRuleCfg.stKey.pools;
        enRet = GETOPT2_ParseFromArgv0(uiArgc - 4, ppcArgv + 4, opts);
        if (BS_OK != enRet)
        {
            EXEC_OutInfo("%s\r\n", GETOPT2_BuildHelpinfo(opts, acHelp, sizeof(acHelp)));
            RETURN(BS_BAD_PARA);
        }

        if (pcDomain)
        {
            enRet = _aclurl_ParseDomain(pcDomain, pstSingleKey->szDomain);
            if (BS_OK != enRet)
            {
                EXEC_OutInfo("ACL rule url invalid \r\n");
                RETURN(enRet);
            }
            stRuleCfg.uiKeyMask |= URL_ACL_KEY_URL;
        }

        if (pcDomainGroup) {
            URL_GROUP_LIST_S * pstList = AclDomainGroup_FindByName(muc_id, pcDomainGroup);
            if(NULL == pstList) {
                EXEC_OutInfo("Domain group not found.\r\n");
                return BS_NOT_FOUND;
            }
            stRuleCfg.uiKeyMask |= URL_ACL_KEY_POOL_DOAMIN;
            pstPoolKey->pstDomainGroup = pstList;
        }

    }

    stRuleCfg.enAction = enAction;

    *puiRuleID = uiRuleID;
    *pstRuleCfg = stRuleCfg;
#endif

	*puiRuleID = 1;

    return BS_OK;
}


PLUG_API BS_STATUS AclURL_CmdRule(IN UINT uiArgc, IN CHAR **ppcArgv, IN VOID *pEnv)
{
    BS_STATUS enRet = BS_OK;
    CHAR *pcListName;
    UINT uiListID;
    UINT uiRuleID = 0;
    URL_ACL_RULE_CFG_S stRuleCfg = {0};
    int muc_id = CmdExp_GetEnvMucID(pEnv);
    URL_ACL_HANDLE urlacl = _aclurl_get_instance(muc_id);
    if (! urlacl) {
        EXEC_OutInfo("Can't get ipacl instance \r\n");
        return BS_ERR;
    }

    pcListName = CMD_EXP_GetCurrentModeValue(pEnv);
    uiListID = URL_ACL_GetListByName(urlacl, pcListName);
    if (0 == uiListID)
    {
        EXEC_OutInfo("ACL list %s is not found \r\n", pcListName);
        RETURN(BS_NO_SUCH);
    }

    enRet = _aclurl_ParseRule(muc_id, uiArgc, ppcArgv, &uiRuleID, &stRuleCfg);
    if (BS_OK != enRet)
    {
        RETURN(enRet);
    }
    
    enRet = _aclurl_CfgRule(urlacl, uiListID, uiRuleID, &stRuleCfg);
    if (BS_OK != enRet)
    {
        RETURN(enRet);
    }

    return BS_OK;
}

static BS_STATUS _aclurl_LoadRule(IN INT muc_id, IN URL_ACL_HANDLE urlacl, IN UINT uiListID, IN CHAR *pcListName, IN CHAR *pcFileName)
{
    FILE *fp;
    BS_STATUS enRet = BS_OK;
    INT iLen;
    CHAR acLine[1024];
    UINT uiArgc;
    CHAR *ppcArgv[URL_ACL_CMD_RULE_ELEMENT_MAX];
    UINT uiRuleID = 0;
    URL_ACL_RULE_CFG_S stRuleCfg = {0};
    URL_ACL_HANDLE hURLAclList;

    hURLAclList = URL_ACL_CreateList(urlacl, pcListName);
    if (NULL == hURLAclList)
    {
        return BS_NO_MEMORY;
    }

    fp = fopen(pcFileName, "rb");
    if (NULL == fp) {
        return BS_CAN_NOT_OPEN;
    }

    while (BOOL_TRUE)
    {
        iLen = FILE_ReadLine(fp, acLine, sizeof(acLine), '\n');
        if (iLen <= 0)
        {
            break;
        }

        Mem_Zero(ppcArgv, sizeof(CHAR*)*URL_ACL_CMD_RULE_ELEMENT_MAX);
        uiArgc = ARGS_Split(acLine, ppcArgv, URL_ACL_CMD_RULE_ELEMENT_MAX);
        if (uiArgc == 0)
        {
            
            continue;
        } 
        
        if (uiArgc < URL_ACL_CMD_RULE_ELEMENT_MIN)
        {
            EXEC_OutInfo("ACL rule argc %u error \r\n", uiArgc);
            enRet = BS_NO_SUCH;
            break;
        }

        enRet = _aclurl_ParseRule(muc_id, uiArgc, ppcArgv, &uiRuleID, &stRuleCfg);
        if (BS_OK != enRet)
        {
            break;
        }

        enRet = URL_ACL_AddRuleToList(urlacl, hURLAclList, uiRuleID, &stRuleCfg);
        if (BS_OK != enRet)
        {
            break;
        }
    }

    if (BS_OK == enRet)
    {
        enRet = URL_ACL_ReplaceList(urlacl, uiListID, hURLAclList);
    }

    if (BS_OK != enRet)
    {
        URL_ACL_DestroyList(urlacl, hURLAclList);
    }
    
    fclose(fp);
    return enRet;
}

PLUG_API BS_STATUS AclURL_Cmd_LoadFile(IN UINT uiArgc, IN CHAR **ppcArgv, IN VOID *pEnv)
{
    CHAR *pcFileName = ppcArgv[1];
    CHAR *pcListName;
    UINT uiListID;
    BS_STATUS enRet;
    UINT64 uiSize;
    URL_ACL_HANDLE urlacl = _aclurl_get_instance_by_env(pEnv);
    if (! urlacl) {
        RETURN(BS_NO_SUCH);
    }
    pcListName = CMD_EXP_GetCurrentModeValue(pEnv);
    uiListID = URL_ACL_GetListByName(urlacl, pcListName);
    if (0 == uiListID)
    {
        EXEC_OutInfo("ACL list %s is not found \r\n", pcListName);
        RETURN(BS_NO_SUCH);
    }

    if(!FILE_IsFileExist(pcFileName))
    {
        EXEC_OutString("File is not exist \r\n");
        RETURN(BS_NO_SUCH);
    }

    enRet = FILE_GetSize(pcFileName, &uiSize);
    if (BS_OK != enRet)
    {
        EXEC_OutInfo("%s.\r\n", ErrInfo_Get(enRet));
        RETURN(enRet);
    }

    enRet = _aclurl_LoadRule(CmdExp_GetEnvMucID(pEnv), urlacl, uiListID, pcListName, pcFileName);
    if (BS_OK != enRet)
    {
        EXEC_OutInfo("%s \r\n", ErrInfo_Get(enRet));
        RETURN(enRet);
    }

    return BS_OK;
}


PLUG_API BS_STATUS AclURL_CmdShowRefer(IN UINT uiArgc, IN CHAR **ppcArgv, IN VOID *pEnv)
{
    UINT uiListID;
    UINT uiRef;
    CHAR *pcListName = CMD_EXP_GetCurrentModeValue(pEnv);
    URL_ACL_HANDLE urlacl = _aclurl_get_instance_by_env(pEnv);
    if (! urlacl) {
        RETURN(BS_NO_SUCH);
    }

    uiListID = URL_ACL_GetListByName(urlacl, pcListName);
    if (0 == uiListID)
    {
        EXEC_OutInfo("ACL list %s is not found \r\n", pcListName);
        RETURN(BS_NO_SUCH);
    }

    uiRef = URL_ACL_ListGetRef(urlacl, uiListID);
    EXEC_OutInfo("refered-count: %u\r\n", uiRef);
    return BS_OK;
}


PLUG_API BS_STATUS AclURL_CmdShowStats(IN UINT uiArgc, IN CHAR **ppcArgv, IN VOID *pEnv)
{
    UINT uiListID;
    CHAR *pcListName = CMD_EXP_GetCurrentModeValue(pEnv);
    URL_ACL_HANDLE urlacl = _aclurl_get_instance_by_env(pEnv);
    if (! urlacl) {
        RETURN(BS_NO_SUCH);
    }
    uiListID = URL_ACL_GetListByName(urlacl, pcListName);
    if (0 == uiListID)
    {
        EXEC_OutInfo("ACL list %s is not found \r\n", pcListName);
        RETURN(BS_NO_SUCH);
    }

    EXEC_OutString("rule       match \r\n----------------------\r\n");

    URL_ACL_ScanRule(urlacl, uiListID, _aclurl_ShowStats, NULL);
    return BS_OK;
}

PLUG_API BS_STATUS AclURL_CmdClearStats(IN UINT uiArgc, IN CHAR **ppcArgv, IN VOID *pEnv)
{
    UINT uiListID;
    CHAR *pcListName = CMD_EXP_GetCurrentModeValue(pEnv);
    URL_ACL_HANDLE urlacl = _aclurl_get_instance_by_env(pEnv);
    if (! urlacl) {
        return 0;
    }
    uiListID = URL_ACL_GetListByName(urlacl, pcListName);
    if (0 == uiListID)
    {
        EXEC_OutInfo("ACL list %s is not found \r\n", pcListName);
        RETURN(BS_NO_SUCH);
    }

    URL_ACL_ScanRule(urlacl, uiListID, _aclurl_ClearStats, NULL);
    return BS_OK;
}

BS_STATUS AclAppURL_Save(IN HANDLE hFile)
{
    UINT ulListID = 0;
    CHAR *pcListName;
    int muc_id = CmdExp_GetMucIDBySaveHandle(hFile);

    URL_ACL_HANDLE urlacl = _aclurl_get_instance(muc_id);
    if (! urlacl) {
        return 0;
    }
    while ((ulListID = URL_ACL_GetNextListID(urlacl, ulListID)) != 0) {
        pcListName = URL_ACL_GetListNameByID(urlacl, ulListID);
        if (0 == CMD_EXP_OutputMode(hFile, "url-acl %s", pcListName)) {
            aclurl_SaveCmdInList(urlacl, hFile, ulListID);
            CMD_EXP_OutputModeQuit(hFile);
        }
    }

    return BS_OK;
}

BS_STATUS ACL_URL_Ioctl(IN INT muc_id, IN COMP_ACL_IOCTL_E enCmd, IN VOID *pData)
{
    BS_STATUS eRet = BS_OK;
    URL_ACL_HANDLE hURLAcl = _aclurl_get_instance(muc_id);
    if (! hURLAcl) {
        RETURN(BS_NO_SUCH);
    }   

    switch (enCmd)
    {
    case COMP_ACL_IOCTL_ADD_LIST_REF:
        eRet = _aclurl_AddListRef(hURLAcl, pData);
        break;
    case COMP_ACL_IOCTL_DEL_LIST_REF:
        eRet = _aclurl_DelListRef(hURLAcl, pData);
        break;
    case COMP_ACL_IOCTL_DEL_LIST_REF_BY_ID:
        eRet = _aclurl_DelListRefByID(hURLAcl, pData);
        break;
    case COMP_ACL_IOCTL_GET_LIST_ID:
        eRet = _aclurl_GetListIDByName(hURLAcl,pData);
        break;
    default:
        eRet = BS_NOT_SUPPORT;
        break;
    }

    return eRet;
}

BS_ACTION_E AclURL_Match(int muc_id, UINT ulListID, IN URL_ACL_MATCH_INFO_S *pstMatchInfo)
{
	URL_ACL_HANDLE urlacl = _aclurl_get_instance(muc_id);
	if (!urlacl) {
		return BS_ACTION_UNDEF;
	}

	return URL_ACL_Match(urlacl, ulListID, pstMatchInfo);
}
