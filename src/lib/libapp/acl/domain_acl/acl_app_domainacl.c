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
#include "utl/domain_acl.h"
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
#include "../h/acl_ip_group.h"
#include "../h/acl_port_group.h"

#define DOMAINACL_CMD_RULE_ELEMENT_MAX 14
#define DOMAINACL_CMD_RULE_ELEMENT_MIN 4

static inline int _acpdomain_set_instance_locked(ACL_MUC_S *acl_muc, DOMAINACL_HANDLE domainacl_handle)
{
    if (acl_muc->domainacl_handle != NULL) {
        RETURN(BS_ALREADY_EXIST);
    }

    acl_muc->domainacl_handle = domainacl_handle;

    return 0;
}

static inline void _acldomain_create_instance(ACL_MUC_S *acl_muc)
{
    int ret;

    DOMAINACL_HANDLE domainacl_handle = DOMAINACL_Create(RcuEngine_GetMemcap());
    if (! domainacl_handle) {
        return;
    }

    SPLX_P();
    ret = _acpdomain_set_instance_locked(acl_muc, domainacl_handle);
    SPLX_V();

    if (ret != 0) {
        DOMAINACL_Destroy(domainacl_handle);
    }

    return;
}

static DOMAINACL_HANDLE _acldomain_get_create_instance(int muc_id)
{
    ACL_MUC_S *acl_muc;

    acl_muc = AclMuc_Get(muc_id);
    if (! acl_muc) {
        return NULL;
    }

    if (acl_muc->domainacl_handle) {
        return acl_muc->domainacl_handle;
    }

    _acldomain_create_instance(acl_muc);

    return acl_muc->domainacl_handle;
}

static inline DOMAINACL_HANDLE _acldomain_get_instance(int muc_id)
{
    ACL_MUC_S *acl_muc;

    acl_muc = AclMuc_Get(muc_id);
    if (! acl_muc) {
        return NULL;
    }

    return acl_muc->domainacl_handle;
}

static DOMAINACL_HANDLE _acldomain_get_create_instance_by_env(void *env)
{
    int muc_id = CmdExp_GetEnvMucID(env);
    return _acldomain_get_create_instance(muc_id);
}

static DOMAINACL_HANDLE _acldomain_get_instance_by_env(void *env)
{
    int muc_id = CmdExp_GetEnvMucID(env);
    return _acldomain_get_instance(muc_id);
}

static BS_STATUS _acldomain_GetListIDByName(IN DOMAINACL_HANDLE domainacl, IN ACL_NAME_ID_S *pstNameID)
{
    UINT ulListID;

    ulListID = DOMAINACL_GetListByName(domainacl, pstNameID->pcAclListName);
    pstNameID->ulAclListID = ulListID;
    if (ulListID == ACL_INVALID_LIST_ID)
    {
        return BS_NOT_FOUND;
    }

    return BS_OK;
}


static BS_STATUS _acldomain_AddListRef(IN DOMAINACL_HANDLE domainacl, IN CHAR *pcListName)
{
    return DOMAINACL_AddListRef(domainacl, DOMAINACL_GetListByName(domainacl, pcListName));
}

static BS_STATUS _acldomain_DelListRef(IN DOMAINACL_HANDLE domainacl, IN CHAR *pcListName)
{
    return DOMAINACL_DelListRef(domainacl, DOMAINACL_GetListByName(domainacl, pcListName));
}

static BS_STATUS _acldomain_DelListRefByID(IN DOMAINACL_HANDLE domainacl, IN UINT *pulListID)
{
    return DOMAINACL_DelListRef(domainacl, *pulListID);
}

static BS_STATUS _acldomain_EnterListView(IN DOMAINACL_HANDLE domainacl, IN CHAR *pcListName)
{
    UINT uiListID;

    uiListID = DOMAINACL_GetListByName(domainacl, pcListName);
    if (uiListID == 0)
    {
        uiListID = DOMAINACL_AddList(domainacl, pcListName);
    }

    if (uiListID == 0)
    {
        return BS_NO_MEMORY;
    }

    return BS_OK;
}

static BS_STATUS _acldomain_CfgRule(IN DOMAINACL_HANDLE domainacl, IN UINT uiListID, IN UINT uiRuleID, IN DOMAINACL_RULE_CFG_S *pstRuleCfg)
{
    BS_STATUS enRet = BS_OK;
    DOMAINACL_RULE_S *pstRule = NULL;

    pstRule = DOMAINACL_GetRule(domainacl, uiListID, uiRuleID);
    if (NULL != pstRule)
    {
        DOMAINACL_UpdateRule(pstRule, pstRuleCfg);
    }
    else
    {
        enRet = DOMAINACL_AddRule(domainacl, uiListID, uiRuleID, pstRuleCfg);
    }

    return enRet;
}

static BS_STATUS _acldomain_NoAclList(IN DOMAINACL_HANDLE domainacl,IN CHAR *pcListName)
{
    UINT uiListID;

    uiListID = DOMAINACL_GetListByName(domainacl, pcListName);
    if (0 == uiListID)
    {
        return BS_OK;
    }

    if (DOMAINACL_ListGetRef(domainacl, uiListID) > 0)
    {
        return BS_REF_NOT_ZERO;
    }

    DOMAINACL_DelList(domainacl, uiListID);

    return BS_OK;
}

static BS_STATUS _acldomain_MoveRule(IN DOMAINACL_HANDLE domainacl, IN CHAR *pcListName, IN UINT uiOldRuleID, IN UINT uiNewRuleID)
{
    BS_STATUS enRet;
    UINT uiListID;
    DOMAINACL_RULE_S *pstRule;

    uiListID = DOMAINACL_GetListByName(domainacl, pcListName);
    if (0 == uiListID)
    {
        return BS_NO_SUCH;
    }

    pstRule = DOMAINACL_GetRule(domainacl, uiListID, uiNewRuleID);
    if (NULL != pstRule)
    {
        EXEC_OutInfo("New rule %u is already exist\n", uiNewRuleID);
        return BS_CONFLICT;
    }

    enRet = DOMAINACL_MoveRule(domainacl, uiListID, uiOldRuleID, uiNewRuleID);
    if(enRet == BS_NOT_FOUND) {
        EXEC_OutInfo("Not found rule %u\n", uiOldRuleID);
    }

    return enRet;
}

static VOID _acldomain_NoRule(IN DOMAINACL_HANDLE domainacl, IN CHAR *pcListName, IN UINT uiRuleID)
{
    DOMAINACL_DelRule(domainacl, DOMAINACL_GetListByName(domainacl, pcListName), uiRuleID);
}

static BOOL_T _acldomain_ShowStats(IN UINT uiRuleID, IN DOMAINACL_RULE_S *pstRule, IN VOID *pUserHandle)
{
    EXEC_OutInfo("%-10u %-10llu \r\n", uiRuleID, pstRule->stStatistics.ulMatchCount);
    return BOOL_TRUE;
}

static BOOL_T _acldomain_ClearStats(IN UINT uiRuleID, IN DOMAINACL_RULE_S *pstRule, IN VOID *pUserHandle)
{
    Mem_Zero(&pstRule->stStatistics, sizeof(DOMAINACL_RULE_STATISTICS_S));
    return BOOL_TRUE;
}

static INT _acldomain_IpKey2Str(IN DOMAINACL_IPKEY_S *pstIpKey, IN CHAR *pCmd, IN INT iStrLen, CHAR *pStr)
{
    INT len;
    IP_MASK_S stIpMask;

    stIpMask.uiIP = pstIpKey->uiIP;
    stIpMask.uiMask = pstIpKey->uiWildcard;

    len = SNPRINTF(pStr, iStrLen, "%s", pCmd);
    if (len < 0) {
        return len;
    }
    len += IPString_IpMask2String_OutIpPrefix(&stIpMask, iStrLen - len, pStr + len);

    return len;
}

static INT _acldomain_PortKey2Str(IN DOMAINACL_PORTKEY_S *pstPortKey, IN CHAR *pCmd, IN INT iStrLen, CHAR *pStr)
{
    INT len = 0;

    len = scnprintf(pStr, iStrLen, "%s", pCmd);
    len += scnprintf(pStr + len, iStrLen - len, "%hu/%hu", pstPortKey->usBegin, pstPortKey->usEnd);
    return len;
}

static BOOL_T _acldomain_SaveCmdInRule(IN UINT uiRuleID, IN DOMAINACL_RULE_S *pstRule, IN VOID *pUserHandle)
{
    HANDLE hFile = pUserHandle;
    DOMAINACL_RULE_CFG_S *pstRuleCfg = &pstRule->stRuleCfg;
    DOMAINACL_SINGLE_KEY_S *pstSingleKey = &pstRule->stRuleCfg.stKey.single;
    DOMAINACL_POOL_KEY_S *pstPoolKey = &pstRule->stRuleCfg.stKey.pools;
    CHAR acAction[32] = {0};
    CHAR szRuleStr[1024] = {0};
    INT length = 0;

    if (pstRuleCfg->uiKeyMask & DOMAINACL_KEY_SIP) {
       length = _acldomain_IpKey2Str(&pstSingleKey->stSIP, " --src_ip ", sizeof(szRuleStr), szRuleStr);
    }

    if (pstRuleCfg->uiKeyMask & DOMAINACL_KEY_DPORT) {
        length += _acldomain_PortKey2Str(&pstSingleKey->stDPort, " --dst_port ", sizeof(szRuleStr)-length, szRuleStr+length);
    }

    if (pstRuleCfg->uiKeyMask & DOMAINACL_KEY_POOL_SIP) {
        length += scnprintf(szRuleStr+length, sizeof(szRuleStr)-length, " --src_ip_group %s", pstPoolKey->pstSipList->list_name);
    }

    if (pstRuleCfg->uiKeyMask & DOMAINACL_KEY_POOL_DPORT) {
        length += scnprintf(szRuleStr+length, sizeof(szRuleStr)-length, " --dst_port_group %s", pstPoolKey->pstDportList->list_name);
    }

    if (pstRuleCfg->uiKeyMask & DOMAINACL_KEY_DOMAIN) {
        length += scnprintf(szRuleStr+length, sizeof(szRuleStr)-length," --domain %s", pstSingleKey->szDomain);
    }

    if (pstRuleCfg->uiKeyMask & DOMAINACL_KEY_POOL_DOAMIN) {
        length += scnprintf(szRuleStr+length, sizeof(szRuleStr)-length, " --domain_group %s", pstPoolKey->pstDomainGroup->list_name);
    }

    if (pstRuleCfg->uiKeyMask & DOMAINACL_KEY_PROTO) {
        length += scnprintf(szRuleStr+length, sizeof(szRuleStr)-length," --protocol %u", pstRuleCfg->ucProto);
    }

    if (pstRuleCfg->enAction == BS_ACTION_DENY) {
        scnprintf(acAction, sizeof(acAction), " action deny");
    } else {
        scnprintf(acAction, sizeof(acAction), " action permit");
    }

    CMD_EXP_OutputCmd(hFile, "rule %u%s%s", uiRuleID, acAction, szRuleStr);

    return BOOL_TRUE;
}

static VOID _acldomain_SaveCmdInList(IN DOMAINACL_HANDLE domainacl, IN HANDLE hFile, IN UINT uiListID)
{
    BS_ACTION_E enAction;

    DOMAINACL_ScanRule(domainacl, uiListID, _acldomain_SaveCmdInRule, hFile);

    enAction = DOMAINACL_GetDefaultActionByID(domainacl, uiListID);
    if (enAction != BS_ACTION_UNDEF)
    {
        CMD_EXP_OutputCmd(hFile, "default action %s", (enAction == BS_ACTION_DENY) ? "deny" : "permit");
    }
}

static BS_STATUS _acldomain_ParseIp(IN CHAR *pcIpStr, OUT DOMAINACL_IPKEY_S *pstIpKey)
{
    BS_STATUS enRet;
    IP_MASK_S stMask;

    enRet = IPString_IpPrefixString2IpMask(pcIpStr, &stMask);
    if (BS_OK == enRet)
    {
        pstIpKey->uiIP = stMask.uiIP;
        pstIpKey->uiWildcard = stMask.uiMask;
    }

    return enRet;
}

static BS_STATUS _acldomain_ParsePort(IN CHAR *pcPortStr, OUT DOMAINACL_PORTKEY_S *pstPorkKey)
{
    LSTR_S stPortMin;
    LSTR_S stPortMax;
    UINT uiPortMin;
    UINT uiPortMax;
    TXT_MStrSplit(pcPortStr, "/-", &stPortMin, &stPortMax);

    LSTR_Strim(&stPortMin, TXT_BLANK_CHARS, &stPortMin);
    LSTR_Strim(&stPortMax, TXT_BLANK_CHARS, &stPortMax);

    uiPortMin = atoi(stPortMin.pcData);

    if (stPortMax.uiLen == 0)
    {
        uiPortMax = uiPortMin;
    }
    else
    {
        uiPortMax = atoi(stPortMax.pcData);
    }

    if (uiPortMin < 1 || uiPortMin > 65535)
    {
        return BS_BAD_PARA;
    }
    if (uiPortMax < 1 || uiPortMax > 65535)
    {
        return BS_BAD_PARA;
    }
    if (uiPortMin > uiPortMax)
    {
        return BS_BAD_PARA;
    }

    pstPorkKey->usBegin = uiPortMin;
    pstPorkKey->usEnd = uiPortMax;

    return BS_OK;
}

static BS_STATUS _acldomain_ParseDomain(IN CHAR* pcArgc, OUT CHAR* pcDomain)
{
    int length = strlen(pcArgc);
    int i = 0;

    if(length > DOMAIN_NAME_MAX_LEN) {
        EXEC_OutInfo("Acl domain too long(1-255).\r\n");
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

static BS_STATUS _acldomain_ParseRule(IN INT muc_id, IN UINT uiArgc, IN CHAR **ppcArgv, OUT UINT *puiRuleID, OUT DOMAINACL_RULE_CFG_S *pstRuleCfg)
{
    BS_STATUS enRet;
    UINT uiRuleID;
    CHAR *pcRuleID = ppcArgv[1];
    BS_ACTION_E enAction;
    CHAR *pcAction = ppcArgv[3];
    DOMAINACL_RULE_CFG_S stRuleCfg = {0};

    CHAR *pcSip = NULL;
    CHAR *pcDport = NULL;
    CHAR *pcSipPool = NULL;
    CHAR *pcDportPool = NULL;
    CHAR *pcDomain = NULL;
    CHAR *pcDomainGroup = NULL;
    CHAR *pcApp = NULL, *pcGroup = NULL;
    UINT uiProto = 0;
    CHAR acHelp[1024];
    GETOPT2_NODE_S opts[] = {
        {'o', 's', "src_ip", GETOPT2_V_STRING, &pcSip, "source ip/prefix, eg 10.10.10.10 or 10.0.0.0/8", 0},
        {'o', 'o', "dst_port", GETOPT2_V_STRING, &pcDport, "destination port range,  eg 80 or 100/200", 0},
        {'o', 'S', "src_ip_group", GETOPT2_V_STRING, &pcSipPool, "source ip addresses group name, eg sip-pool", 0},
        {'o', 'O', "dst_port_group", GETOPT2_V_STRING, &pcDportPool, "destination port group name, eg dport-pool", 0},
        {'o', 'd', "domain", GETOPT2_V_STRING, &pcDomain, "Domain string", 0},
        {'o', 'D', "domain_group", GETOPT2_V_STRING, &pcDomainGroup, "Domain group name", 0},
        {'o', 't', "protocol", GETOPT2_V_U32, &uiProto, "protocal num, eg 17", 0},
        {'o', 'a', "application_group", GETOPT2_V_STRING, &pcApp, "Application_group name, eg:google/Search", 0},
        {'o', 'A', "application_set", GETOPT2_V_STRING, &pcGroup, "Application or group set", 0},
        {0}};


    TXT_Atoui(pcRuleID, &uiRuleID);
    if (uiRuleID == 0)
    {
        EXEC_OutString("ACL rule id invalid \r\n");
        RETURN(BS_NO_SUCH);
    }

    if (uiRuleID > DOMAINACL_RULE_ID_MAX)
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
        DOMAINACL_SINGLE_KEY_S *pstSingleKey = &stRuleCfg.stKey.single;
        DOMAINACL_POOL_KEY_S *pstPoolKey = &stRuleCfg.stKey.pools;
        enRet = GETOPT2_ParseFromArgv0(uiArgc - 4, ppcArgv + 4, opts);
        if (BS_OK != enRet)
        {
            EXEC_OutInfo("%s\r\n", GETOPT2_BuildHelpinfo(opts, acHelp, sizeof(acHelp)));
            RETURN(BS_BAD_PARA);
        }

        if (((NULL != pcSip) && (NULL != pcSipPool)))
        {
            EXEC_OutString("Parameter conflict: src_ip and src_ip_group can only be configed one at same time.\r\n");
            RETURN(BS_CONFLICT);
        }
        if (((NULL != pcDport) && (NULL != pcDportPool)))
        {
            EXEC_OutString("Parameter conflict: dst_port and dst_port_group can only be configed one at same time.\r\n");
            RETURN(BS_CONFLICT);
        }

        if (pcSip)
        {
            enRet = _acldomain_ParseIp(pcSip, &pstSingleKey->stSIP);
            if (BS_OK != enRet)
            {
                EXEC_OutInfo("ACL rule sip invalid \r\n");
                RETURN(enRet);
            }
            stRuleCfg.uiKeyMask |= DOMAINACL_KEY_SIP;
        }

        if (pcDport)
        {
            enRet = _acldomain_ParsePort(pcDport, &pstSingleKey->stDPort);
            if (BS_OK != enRet)
            {
                EXEC_OutInfo("ACL rule dprot invalid \r\n");
                RETURN(enRet);
            }
            stRuleCfg.uiKeyMask |= DOMAINACL_KEY_DPORT;
        }

        if (pcDomain)
        {
            enRet = _acldomain_ParseDomain(pcDomain, pstSingleKey->szDomain);
            if (BS_OK != enRet)
            {
                EXEC_OutInfo("ACL rule domain invalid \r\n");
                RETURN(enRet);
            }
            stRuleCfg.uiKeyMask |= DOMAINACL_KEY_DOMAIN;
        }

        if (pcDomainGroup) {
            LIST_RULE_LIST_S * pstList = AclDomainGroup_FindByName(muc_id, pcDomainGroup);
            if(NULL == pstList) {
                EXEC_OutInfo("Domain group not found.\r\n");
                return BS_NOT_FOUND;
            }
            stRuleCfg.uiKeyMask |= DOMAINACL_KEY_POOL_DOAMIN;
            pstPoolKey->pstDomainGroup = pstList;
        }

        if (NULL != pcSipPool){
            LIST_RULE_LIST_S *pstList = AclIPGroup_FindByName(muc_id, pcSipPool);
            if (! pstList) {
                EXEC_OutString("Acl src_ip_group not found.\r\n");
                return BS_NOT_FOUND;
            }
            stRuleCfg.uiKeyMask |= DOMAINACL_KEY_POOL_SIP;
            pstPoolKey->pstSipList = pstList;
        }


        if (NULL != pcDportPool){
            LIST_RULE_LIST_S *pstList = AclPortGroup_FindByName(muc_id, pcDportPool);
            if (! pstList) {
                EXEC_OutString("Acl dst_port_group not found.\r\n");
                return BS_NOT_FOUND;
            }
            stRuleCfg.uiKeyMask |= DOMAINACL_KEY_POOL_DPORT;
            pstPoolKey->pstDportList = pstList;
        }

        if (0 != uiProto) {
            if (uiProto < 1 || uiProto > 255) 
            {
                EXEC_OutInfo("ACL rule protocol invalid \r\n");
                return BS_BAD_PARA;
            }
            stRuleCfg.ucProto = uiProto;
            stRuleCfg.uiKeyMask |= DOMAINACL_KEY_PROTO;
        }
    }

    stRuleCfg.enAction = enAction;

    *puiRuleID = uiRuleID;
    *pstRuleCfg = stRuleCfg;
    return BS_OK;
}

BS_STATUS AclDomain_Init()
{
    return BS_OK;
}

void AclDomain_DestroyMuc(ACL_MUC_S *acl_muc)
{
    if (acl_muc->domainacl_handle) {
        IPACL_Destroy(acl_muc->domainacl_handle);
        acl_muc->domainacl_handle = NULL;
    }
    return;
}


PLUG_API BS_STATUS AclDomain_EnterListView(IN UINT uiArgc, IN CHAR **ppcArgv, IN VOID *pEnv)
{
    BS_STATUS eRet;
    DOMAINACL_HANDLE domainacl = _acldomain_get_create_instance_by_env(pEnv);
    if (! domainacl) {
        EXEC_OutInfo("Can't get domain acl instance \r\n");
        return BS_ERR;
    }

    eRet = _acldomain_EnterListView(domainacl, ppcArgv[1]);
    if (BS_OK != eRet)
    {
        EXEC_OutInfo("%s \r\n", ErrInfo_Get(eRet));
        return BS_ERR;
    }

    return BS_OK;
}


PLUG_API BS_STATUS AclDomain_CmdNoList(IN UINT uiArgc, IN CHAR **ppcArgv, IN VOID *pEnv)
{
    BS_STATUS eRet;
    DOMAINACL_HANDLE domainacl = _acldomain_get_instance_by_env(pEnv);
    if (! domainacl) {
        return BS_OK;
    }

    eRet = _acldomain_NoAclList(domainacl, ppcArgv[2]);
    if (eRet == BS_REF_NOT_ZERO)
    {
        EXEC_OutInfo("%s \r\n", ErrInfo_Get(eRet));
    }

    return eRet;
}

PLUG_API BS_STATUS AclDomain_Clear(IN UINT uiArgc, IN CHAR **ppcArgv, IN VOID *pEnv)
{
    DOMAINACL_HANDLE domainacl = _acldomain_get_instance_by_env(pEnv);
    if (! domainacl) {
        return BS_OK;
    }
    DOMAINACL_Reset(domainacl);

    RETURN(BS_OK);
}



PLUG_API BS_STATUS AclDomain_CmdMoveRule(IN UINT uiArgc, IN CHAR **ppcArgv, IN VOID *pEnv)
{
    CHAR *pcListName;
    UINT uiOldRuleID;
    UINT uiNewRuleID;
    BS_STATUS eRet;
    DOMAINACL_HANDLE domainacl = _acldomain_get_instance_by_env(pEnv);
    if (! domainacl) {
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

    eRet = _acldomain_MoveRule(domainacl, pcListName, uiOldRuleID, uiNewRuleID);
    if (eRet != BS_OK)
    {
        EXEC_OutInfo("%s \r\n", ErrInfo_Get(eRet));
    }

    return eRet;
}


PLUG_API BS_STATUS AclDomain_CmdNoRule(IN UINT uiArgc, IN CHAR **ppcArgv, IN VOID *pEnv)
{
    CHAR *pcListName;
    UINT uiRuleID = 0;
    CHAR *pcRuleID = ppcArgv[2];

    DOMAINACL_HANDLE domainacl = _acldomain_get_instance_by_env(pEnv);
    if (! domainacl) {
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
    _acldomain_NoRule(domainacl, pcListName, uiRuleID);

    return BS_OK;
}

PLUG_API BS_STATUS AclDomain_CmdRebaseRuleID(IN UINT uiArgc, IN CHAR **ppcArgv, IN VOID *pEnv)
{
    UINT uiListID;
    UINT uiRuleID;
    UINT uiStep = 0;
    CHAR *pcListName;
    DOMAINACL_HANDLE domainacl = _acldomain_get_instance_by_env(pEnv);
    if (! domainacl) {
        EXEC_OutInfo("Can't get domainacl instance \r\n");
        return BS_ERR;
    }

    TXT_Atoui(ppcArgv[2], &uiStep);
    if (uiStep == 0)
    {
        EXEC_OutString("Rebase step is invalid\r\n");
        return BS_BAD_PARA;
    }

    pcListName = CMD_EXP_GetCurrentModeValue(pEnv);
    uiListID = DOMAINACL_GetListByName(domainacl, pcListName);
    if (0 == uiListID)
    {
        EXEC_OutInfo("ACL list %s is not found \r\n", pcListName);
        return BS_NO_SUCH;
    }

    uiRuleID = IPACL_GetLastRuleID(domainacl, uiListID);
    if (0 != uiRuleID && uiRuleID + uiStep > DOMAINACL_RULE_ID_MAX)
    {
        EXEC_OutString("Rule id reatch max\r\n");
        return BS_REACH_MAX;
    }

    return DOMAINACL_RebaseID(domainacl, uiListID, uiStep);
}

PLUG_API BS_STATUS AclDomain_Cmd_IncreaseID(IN UINT uiArgc, IN CHAR **ppcArgv, IN VOID *pEnv)
{
    BS_STATUS enRet;
    UINT uiStart;
    UINT uiEnd;
    UINT uiStep = 1;
    UINT uiListID;
    UINT uiRuleID;
    DOMAINACL_RULE_S *pstRule;
    CHAR *pcListName;
    DOMAINACL_HANDLE domainacl = _acldomain_get_instance_by_env(pEnv);
    if (! domainacl) {
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

    if (uiEnd + uiStep > DOMAINACL_RULE_ID_MAX)
    {
        EXEC_OutString("Rule id reatch max\r\n");
        return BS_REACH_MAX;
    }

    pcListName = CMD_EXP_GetCurrentModeValue(pEnv);
    uiListID = DOMAINACL_GetListByName(domainacl, pcListName);
    if (0 == uiListID)
    {
        EXEC_OutInfo("ACL list %s is not found \r\n", pcListName);
        return BS_NO_SUCH;
    }

    pstRule = DOMAINACL_GetRule(domainacl, uiListID, uiStart);
    if (NULL == pstRule)
    {
        EXEC_OutString("ACL rule id invalid \r\n");
        return BS_BAD_PARA;
    }

    pstRule = DOMAINACL_GetRule(domainacl, uiListID, uiEnd);
    if (NULL == pstRule)
    {
        EXEC_OutString("ACL rule id invalid \r\n");
        return BS_BAD_PARA;
    }

    uiRuleID = DOMAINACL_GetNextRuleID(domainacl, uiListID, uiEnd);
    if ((0 != uiRuleID) && (uiRuleID <= uiEnd + uiStep))
    {
        EXEC_OutString("Increase id conflict with other rule \r\n");
        return BS_BAD_PARA;
    }

    DOMAINACL_IncreaseID(domainacl, uiListID, uiStart, uiEnd, uiStep);

    return BS_OK;
}


PLUG_API BS_STATUS AclDomain_CmdSet_DefaultAction(IN UINT uiArgc, IN CHAR **ppcArgv, IN VOID *pEnv)
{
    CHAR *pcListName = CMD_EXP_GetCurrentModeValue(pEnv);
    BS_ACTION_E enAction;
    UINT uiListID;
    DOMAINACL_HANDLE domainacl = _acldomain_get_instance_by_env(pEnv);
    if (! domainacl) {
        EXEC_OutInfo("Can't get domainacl instance \r\n");
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

    uiListID = DOMAINACL_GetListByName(domainacl, pcListName);
    if (0 == uiListID)
    {
        return BS_NO_SUCH;
    }

    return DOMAINACL_SetDefaultActionByID(domainacl, uiListID, enAction);
}


PLUG_API BS_STATUS AclDomain_CmdRule(IN UINT uiArgc, IN CHAR **ppcArgv, IN VOID *pEnv)
{
    BS_STATUS enRet = BS_OK;
    CHAR *pcListName;
    UINT uiListID;
    UINT uiRuleID = 0;
    DOMAINACL_RULE_CFG_S stRuleCfg = {0};
    int muc_id = CmdExp_GetEnvMucID(pEnv);
    DOMAINACL_HANDLE domainacl = _acldomain_get_instance(muc_id);
    if (! domainacl) {
        EXEC_OutInfo("Can't get ipacl instance \r\n");
        return BS_ERR;
    }

    pcListName = CMD_EXP_GetCurrentModeValue(pEnv);
    uiListID = DOMAINACL_GetListByName(domainacl, pcListName);
    if (0 == uiListID)
    {
        EXEC_OutInfo("ACL list %s is not found \r\n", pcListName);
        RETURN(BS_NO_SUCH);
    }

    enRet = _acldomain_ParseRule(muc_id, uiArgc, ppcArgv, &uiRuleID, &stRuleCfg);
    if (BS_OK != enRet)
    {
        RETURN(enRet);
    }
    
    enRet = _acldomain_CfgRule(domainacl, uiListID, uiRuleID, &stRuleCfg);
    if (BS_OK != enRet)
    {
        RETURN(enRet);
    }

    return BS_OK;
}

PLUG_API BS_STATUS AclDomain_CmdShowRefer(IN UINT uiArgc, IN CHAR **ppcArgv, IN VOID *pEnv)
{
    UINT uiListID;
    UINT uiRef;
    CHAR *pcListName = CMD_EXP_GetCurrentModeValue(pEnv);
    DOMAINACL_HANDLE domainacl = _acldomain_get_instance_by_env(pEnv);
    if (! domainacl) {
        RETURN(BS_NO_SUCH);
    }

    uiListID = DOMAINACL_GetListByName(domainacl, pcListName);
    if (0 == uiListID)
    {
        EXEC_OutInfo("ACL list %s is not found \r\n", pcListName);
        RETURN(BS_NO_SUCH);
    }

    uiRef = DOMAINACL_ListGetRef(domainacl, uiListID);
    EXEC_OutInfo("refered-count: %u\r\n", uiRef);
    return BS_OK;
}


PLUG_API BS_STATUS AclDomain_CmdShowStats(IN UINT uiArgc, IN CHAR **ppcArgv, IN VOID *pEnv)
{
    UINT uiListID;
    CHAR *pcListName = CMD_EXP_GetCurrentModeValue(pEnv);
    DOMAINACL_HANDLE domainacl = _acldomain_get_instance_by_env(pEnv);
    if (! domainacl) {
        RETURN(BS_NO_SUCH);
    }
    uiListID = DOMAINACL_GetListByName(domainacl, pcListName);
    if (0 == uiListID)
    {
        EXEC_OutInfo("ACL list %s is not found \r\n", pcListName);
        RETURN(BS_NO_SUCH);
    }

    EXEC_OutString("rule       match \r\n----------------------\r\n");

    DOMAINACL_ScanRule(domainacl, uiListID, _acldomain_ShowStats, NULL);
    return BS_OK;
}

PLUG_API BS_STATUS AclDomain_CmdClearStats(IN UINT uiArgc, IN CHAR **ppcArgv, IN VOID *pEnv)
{
    UINT uiListID;
    CHAR *pcListName = CMD_EXP_GetCurrentModeValue(pEnv);
    DOMAINACL_HANDLE domainacl = _acldomain_get_instance_by_env(pEnv);
    if (! domainacl) {
        return 0;
    }
    uiListID = DOMAINACL_GetListByName(domainacl, pcListName);
    if (0 == uiListID)
    {
        EXEC_OutInfo("ACL list %s is not found \r\n", pcListName);
        RETURN(BS_NO_SUCH);
    }

    DOMAINACL_ScanRule(domainacl, uiListID, _acldomain_ClearStats, NULL);
    return BS_OK;
}

BS_STATUS AclDomain_Save(IN HANDLE hFile)
{
    UINT ulListID = 0;
    CHAR *pcListName;
    int muc_id = CmdExp_GetMucIDBySaveHandle(hFile);

    DOMAINACL_HANDLE domainacl = _acldomain_get_instance(muc_id);
    if (! domainacl) {
        return 0;
    }
    while ((ulListID = DOMAINACL_GetNextListID(domainacl, ulListID)) != 0)
    {
        pcListName = DOMAINACL_GetListNameByID(domainacl, ulListID);
        if (0 == CMD_EXP_OutputMode(hFile, "domain-acl %s", pcListName)) {
            _acldomain_SaveCmdInList(domainacl, hFile, ulListID);
            CMD_EXP_OutputModeQuit(hFile);
        }
    }

    return BS_OK;
}

BS_STATUS AclDomain_Ioctl(IN INT muc_id, IN COMP_ACL_IOCTL_E enCmd, IN VOID *pData)
{
    BS_STATUS eRet = BS_OK;
    DOMAINACL_HANDLE hDomainAcl = _acldomain_get_instance(muc_id);
    if (! hDomainAcl) {
        RETURN(BS_NO_SUCH);
    }   

    switch (enCmd)
    {
    case COMP_ACL_IOCTL_ADD_LIST_REF:
        eRet = _acldomain_AddListRef(hDomainAcl, pData);
        break;
    case COMP_ACL_IOCTL_DEL_LIST_REF:
        eRet = _acldomain_DelListRef(hDomainAcl, pData);
        break;
    case COMP_ACL_IOCTL_DEL_LIST_REF_BY_ID:
        eRet = _acldomain_DelListRefByID(hDomainAcl, pData);
        break;
    case COMP_ACL_IOCTL_GET_LIST_ID:
        eRet = _acldomain_GetListIDByName(hDomainAcl,pData);
        break;
    default:
        eRet = BS_NOT_SUPPORT;
        break;
    }

    return eRet;
}

BS_ACTION_E AclDomain_Match(int muc_id, UINT ulListID, IN DOMAINACL_MATCH_INFO_S *pstMatchInfo)
{
    DOMAINACL_HANDLE domainacl = _acldomain_get_instance(muc_id);
    if (! domainacl) {
        return BS_ACTION_UNDEF;
    }

    return DOMAINACL_Match(domainacl, ulListID, pstMatchInfo);
}
