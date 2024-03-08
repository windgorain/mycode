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
#include "utl/ip_acl.h"
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

#define IPACL_CMD_RULE_ELEMENT_MAX 14
#define IPACL_CMD_RULE_ELEMENT_MIN 4

static inline int _acpappip_set_instance_locked(ACL_MUC_S *acl_muc, IPACL_HANDLE ipacl)
{
    if (acl_muc->ipacl != NULL) {
        RETURN(BS_ALREADY_EXIST);
    }

    acl_muc->ipacl = ipacl;

    return 0;
}

static inline void _aclappip_create_instance(ACL_MUC_S *acl_muc)
{
    int ret;

    IPACL_HANDLE ipacl = IPACL_Create(RcuEngine_GetMemcap());
    if (! ipacl) {
        return;
    }

    SPLX_P();
    ret = _acpappip_set_instance_locked(acl_muc, ipacl);
    SPLX_V();

    if (ret != 0) {
        IPACL_Destroy(ipacl);
    }

    return;
}

static IPACL_HANDLE _aclappip_get_create_instance(int muc_id)
{
    ACL_MUC_S *acl_muc;

    acl_muc = AclMuc_Get(muc_id);
    if (! acl_muc) {
        return NULL;
    }

    if (acl_muc->ipacl) {
        return acl_muc->ipacl;
    }

    _aclappip_create_instance(acl_muc);

    return acl_muc->ipacl;
}

static inline IPACL_HANDLE _aclappip_get_instance(int muc_id)
{
    ACL_MUC_S *acl_muc;

    acl_muc = AclMuc_Get(muc_id);
    if (! acl_muc) {
        return NULL;
    }

    return acl_muc->ipacl;
}

static IPACL_HANDLE _aclappip_get_create_instance_by_env(void *env)
{
    int muc_id = CmdExp_GetEnvMucID(env);
    return _aclappip_get_create_instance(muc_id);
}

static IPACL_HANDLE _aclappip_get_instance_by_env(void *env)
{
    int muc_id = CmdExp_GetEnvMucID(env);
    return _aclappip_get_instance(muc_id);
}

static BS_STATUS _aclappip_GetListIDByName(IPACL_HANDLE ipacl, IN ACL_NAME_ID_S *pstNameID)
{
    UINT ulListID;

    ulListID = IPACL_GetListByName(ipacl, pstNameID->pcAclListName);
    pstNameID->ulAclListID = ulListID;
    if (ulListID == ACL_INVALID_LIST_ID)
    {
        return BS_NOT_FOUND;
    }

    return BS_OK;
}


static BS_STATUS _aclappip_AddListRef(IPACL_HANDLE ipacl, IN CHAR *pcListName)
{
    return IPACL_AddListRef(ipacl, IPACL_GetListByName(ipacl, pcListName));
}

static BS_STATUS _aclappip_DelListRef(IPACL_HANDLE ipacl, IN CHAR *pcListName)
{
    return IPACL_DelListRef(ipacl, IPACL_GetListByName(ipacl, pcListName));
}

static BS_STATUS _aclappip_DelListRefByID(IPACL_HANDLE ipacl, IN UINT *pulListID)
{
    return IPACL_DelListRef(ipacl, *pulListID);
}

static BS_STATUS _aclappip_EnterListView(IPACL_HANDLE ipacl, char *pcListName)
{
    UINT uiListID;

    uiListID = IPACL_GetListByName(ipacl, pcListName);
    if (uiListID == 0)
    {
        uiListID = IPACL_AddList(ipacl, pcListName);
    }

    if (uiListID == 0)
    {
        return BS_NO_MEMORY;
    }

    return BS_OK;
}

static BS_STATUS _aclappip_CfgRule(IPACL_HANDLE ipacl, UINT uiListID, UINT uiRuleID, IN IPACL_RULE_CFG_S *pstRuleCfg)
{
    BS_STATUS enRet = BS_OK;
    IPACL_RULE_S *pstRule = NULL;

    pstRule = IPACL_GetRule(ipacl, uiListID, uiRuleID);
    if (NULL != pstRule)
    {
        IPACL_UpdateRule(pstRule, pstRuleCfg);
    }
    else
    {
        enRet = IPACL_AddRule(ipacl, uiListID, uiRuleID, pstRuleCfg);
    }

    return enRet;
}

static BS_STATUS _aclappip_NoAclList(IPACL_HANDLE ipacl, char *pcListName)
{
    UINT uiListID;

    uiListID = IPACL_GetListByName(ipacl, pcListName);
    if (0 == uiListID)
    {
        return BS_OK;
    }

    if (IPACL_ListGetRef(ipacl, uiListID) > 0)
    {
        return BS_REF_NOT_ZERO;
    }

    IPACL_DelList(ipacl, uiListID);

    return BS_OK;
}

static BS_STATUS _aclappip_MoveRule(IPACL_HANDLE ipacl, IN CHAR *pcListName, IN UINT uiOldRuleID, IN UINT uiNewRuleID)
{
    BS_STATUS enRet;
    UINT uiListID;
    IPACL_RULE_S *pstRule;

    uiListID = IPACL_GetListByName(ipacl, pcListName);
    if (0 == uiListID)
    {
        return BS_NO_SUCH;
    }

    pstRule = IPACL_GetRule(ipacl, uiListID, uiNewRuleID);
    if (NULL != pstRule)
    {
        EXEC_OutInfo("New rule %u is already exist\n", uiNewRuleID);
        return BS_CONFLICT;
    }

    enRet = IPACL_MoveRule(ipacl, uiListID, uiOldRuleID, uiNewRuleID);
    if(enRet == BS_NOT_FOUND) {
        EXEC_OutInfo("Not found rule %u\n", uiOldRuleID);
    }

    return enRet;
}

static VOID _aclappip_NoRule(IPACL_HANDLE ipacl, IN CHAR *pcListName, IN UINT uiRuleID)
{
    IPACL_DelRule(ipacl, IPACL_GetListByName(ipacl, pcListName), uiRuleID);
}

static BOOL_T _aclappip_ShowStats(IN UINT uiRuleID, IN IPACL_RULE_S *pstRule, IN VOID *pUserHandle)
{
    EXEC_OutInfo("%-10u %-10llu \r\n", uiRuleID, pstRule->stStatistics.ulMatchCount);
    return BOOL_TRUE;
}

static BOOL_T _aclappip_ClearStats(IN UINT uiRuleID, IN IPACL_RULE_S *pstRule, IN VOID *pUserHandle)
{
    Mem_Zero(&pstRule->stStatistics, sizeof(IPACL_RULE_STATISTICS_S));
    return BOOL_TRUE;
}

static INT _aclappip_IpKey2Str(IN IPACL_IPKEY_S *pstIpKey, IN CHAR *pCmd, IN INT iStrLen, CHAR *pStr)
{
    INT len;
    IP_MASK_S stIpMask;

    stIpMask.uiIP = pstIpKey->uiIP;
    stIpMask.uiMask = pstIpKey->uiWildcard;

    len = scnprintf(pStr, iStrLen, "%s", pCmd);
    len += IPString_IpMask2String_OutIpPrefix(&stIpMask, iStrLen - len, pStr + len);
    return len;
}

static INT _aclappip_PortKey2Str(IN IPACL_PORTKEY_S *pstPortKey, IN CHAR *pCmd, IN INT iStrLen, CHAR *pStr)
{
    INT len = 0;

    len = scnprintf(pStr, iStrLen, "%s", pCmd);
    len += scnprintf(pStr + len, iStrLen - len, "%hu/%hu", pstPortKey->usBegin, pstPortKey->usEnd);
    return len;
}

static BOOL_T _aclappip_SaveCmdInRule(IN UINT uiRuleID, IN IPACL_RULE_S *pstRule, IN VOID *pUserHandle)
{
    HANDLE hFile = pUserHandle;
    IPACL_RULE_CFG_S *pstRuleCfg = &pstRule->stRuleCfg;
    IPACL_SINGLE_KEY_S *pstSingleKey = &pstRule->stRuleCfg.stKey.single;
    IPACL_POOL_KEY_S *pstPoolKey = &pstRule->stRuleCfg.stKey.pools;
    CHAR acAction[32] = {0};
    CHAR acSip[32] = {0};
    CHAR acDip[32] = {0};
    CHAR acSport[32] = {0};
    CHAR acDport[32] = {0};
    CHAR acProto[32] = {0};
    CHAR acPoolSip[ACL_POOL_NAME_LEN_MAX] = {0};
    CHAR acPoolDip[ACL_POOL_NAME_LEN_MAX] = {0};
    CHAR acPoolSport[ACL_POOL_NAME_LEN_MAX] = {0};
    CHAR acPoolDport[ACL_POOL_NAME_LEN_MAX] = {0};

    if (pstRuleCfg->uiKeyMask & IPACL_KEY_SIP) {
        _aclappip_IpKey2Str(&pstSingleKey->stSIP, " --src_ip ", sizeof(acSip), acSip);
    }
    if (pstRuleCfg->uiKeyMask & IPACL_KEY_DIP) {
        _aclappip_IpKey2Str(&pstSingleKey->stDIP, " --dst_ip ", sizeof(acDip), acDip);
    }
    if (pstRuleCfg->uiKeyMask & IPACL_KEY_SPORT) {
        _aclappip_PortKey2Str(&pstSingleKey->stSPort, " --src_port ", sizeof(acSport), acSport);
    }
    if (pstRuleCfg->uiKeyMask & IPACL_KEY_DPORT) {
        _aclappip_PortKey2Str(&pstSingleKey->stDPort, " --dst_port ", sizeof(acDport), acDport);
    }
    if (pstRuleCfg->uiKeyMask & IPACL_KEY_POOL_SIP) {
        scnprintf(acPoolSip, sizeof(acPoolSip), " --src_ip_group %s", pstPoolKey->pstSipPool->list_name);
    }
    if (pstRuleCfg->uiKeyMask & IPACL_KEY_POOL_DIP){
        scnprintf(acPoolDip, sizeof(acPoolDip), " --dst_ip_group %s", pstPoolKey->pstDipPool->list_name);
    }
    if (pstRuleCfg->uiKeyMask & IPACL_KEY_POOL_SPORT){
        scnprintf(acPoolSport, sizeof(acPoolSport), " --src_port_group %s", pstPoolKey->pstSportPool->list_name);
    }
    if (pstRuleCfg->uiKeyMask & IPACL_KEY_POOL_DPORT){
        scnprintf(acPoolDport, sizeof(acPoolDport), " --dst_port_group %s", pstPoolKey->pstDportPool->list_name);
    }
    if (pstRuleCfg->uiKeyMask & IPACL_KEY_PROTO) {
        scnprintf(acProto, sizeof(acProto), " --protocol %u", pstRuleCfg->ucProto);
    }
    if (pstRuleCfg->enAction == BS_ACTION_DENY) {
        scnprintf(acAction, sizeof(acAction), " action deny");
    } else {
        scnprintf(acAction, sizeof(acAction), " action permit");
    }

    CMD_EXP_OutputCmd(hFile, "rule %u%s%s%s%s%s%s%s%s%s%s",
            uiRuleID, acAction, acSip, acSport, acDip, acDport, acProto, acPoolSip, acPoolDip, acPoolSport, acPoolDport);
    return BOOL_TRUE;
}

static VOID _aclappip_SaveCmdInList(IPACL_HANDLE ipacl, HANDLE hFile, IN UINT uiListID)
{
    BS_ACTION_E enAction;

    IPACL_ScanRule(ipacl, uiListID, _aclappip_SaveCmdInRule, hFile);

    enAction = IPACL_GetDefaultActionByID(ipacl, uiListID);
    if (enAction != BS_ACTION_UNDEF)
    {
        CMD_EXP_OutputCmd(hFile, "default action %s", (enAction == BS_ACTION_DENY) ? "deny" : "permit");
    }
}

static BS_STATUS _aclappip_ParsePort(IN CHAR *pcPortStr, OUT IPACL_PORTKEY_S *pstPorkKey)
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

static BS_STATUS _aclappip_ParseIp(IN CHAR *pcIpStr, OUT IPACL_IPKEY_S *pstIpKey)
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

static BS_STATUS _aclappip_ParseRule(IN INT muc_id, IN UINT uiArgc, IN CHAR **ppcArgv, OUT UINT *puiRuleID, OUT IPACL_RULE_CFG_S *pstRuleCfg)
{
    BS_STATUS enRet;
    UINT uiRuleID;
    CHAR *pcRuleID = ppcArgv[1];
    BS_ACTION_E enAction;
    CHAR *pcAction = ppcArgv[3];
    IPACL_RULE_CFG_S stRuleCfg = {0};
    CHAR *pcSip = NULL;
    CHAR *pcDip = NULL;
    CHAR *pcSport = NULL;
    CHAR *pcDport = NULL;
    CHAR *pcSipPool = NULL;
    CHAR *pcDipPool = NULL;
    CHAR *pcSportPool = NULL;
    CHAR *pcDportPool = NULL;
    UINT uiProto = 0;
    CHAR acHelp[1024];
    GETOPT2_NODE_S opts[] = {
        {'o', 's', "src_ip", GETOPT2_V_STRING, &pcSip, "source ip/prefix, eg 10.10.10.10 or 10.0.0.0/8", 0},
        {'o', 'd', "dst_ip", GETOPT2_V_STRING, &pcDip, "destination ip/prefix, eg 10.10.10.10 or 10.0.0.0/8", 0},
        {'o', 'p', "src_port", GETOPT2_V_STRING, &pcSport, "source port range, eg 80 or 100/200", 0},
        {'o', 'o', "dst_port", GETOPT2_V_STRING, &pcDport, "destination port range,  eg 80 or 100/200", 0},
        {'o', 'S', "src_ip_group", GETOPT2_V_STRING, &pcSipPool, "source ip addresses group name, eg sip-pool", 0},
        {'o', 'D', "dst_ip_group", GETOPT2_V_STRING, &pcDipPool, "destination ip addresses group name, eg dip-pool", 0},
        {'o', 'P', "src_port_group", GETOPT2_V_STRING, &pcSportPool, "source port group name, eg sport-pool", 0},
        {'o', 'O', "dst_port_group", GETOPT2_V_STRING, &pcDportPool, "destination port group name, eg dport-pool", 0},
        {'o', 't', "protocol", GETOPT2_V_U32, &uiProto, "protocal num, eg 17", 0},
        {0}};


    TXT_Atoui(pcRuleID, &uiRuleID);
    if (uiRuleID == 0)
    {
        EXEC_OutString("ACL rule id invalid \r\n");
        RETURN(BS_NO_SUCH);
    }

    if (uiRuleID > IPACL_RULE_ID_MAX)
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
        IPACL_SINGLE_KEY_S *pstSingleKey = &stRuleCfg.stKey.single;
        IPACL_POOL_KEY_S *pstPoolKey = &stRuleCfg.stKey.pools;
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
        if (((NULL != pcDip) && (NULL != pcDipPool)))
        {
            EXEC_OutString("Parameter conflict: dst_ip and dst_ip_group can only be configed one at same time.\r\n");
            RETURN(BS_CONFLICT);
        }
        if (((NULL != pcSport) && (NULL != pcSportPool)))
        {
            EXEC_OutString("Parameter conflict: src_port and src_port_group can only be configed one at same time.\r\n");
            RETURN(BS_CONFLICT);
        }
        if (((NULL != pcDport) && (NULL != pcDportPool)))
        {
            EXEC_OutString("Parameter conflict: dst_port and dst_port_group can only be configed one at same time.\r\n");
            RETURN(BS_CONFLICT);
        }

        if (pcSip)
        {
            enRet = _aclappip_ParseIp(pcSip, &pstSingleKey->stSIP);
            if (BS_OK != enRet)
            {
                EXEC_OutInfo("ACL rule sip invalid \r\n");
                RETURN(enRet);
            }
            stRuleCfg.uiKeyMask |= IPACL_KEY_SIP;
        }

        if (pcDip)
        {
            enRet = _aclappip_ParseIp(pcDip, &pstSingleKey->stDIP);
            if (BS_OK != enRet)
            {
                EXEC_OutInfo("ACL rule dip invalid \r\n");
                RETURN(enRet);
            }
            stRuleCfg.uiKeyMask |= IPACL_KEY_DIP;
        }

        if (pcSport)
        {
            enRet = _aclappip_ParsePort(pcSport, &pstSingleKey->stSPort);
            if (BS_OK != enRet)
            {
                EXEC_OutInfo("ACL rule sport invalid \r\n");
                RETURN(enRet);
            }
            stRuleCfg.uiKeyMask |= IPACL_KEY_SPORT;
        }

        if (pcDport)
        {
            enRet = _aclappip_ParsePort(pcDport, &pstSingleKey->stDPort);
            if (BS_OK != enRet)
            {
                EXEC_OutInfo("ACL rule dprot invalid \r\n");
                RETURN(enRet);
            }
            stRuleCfg.uiKeyMask |= IPACL_KEY_DPORT;
        }

        if (NULL != pcSipPool){
            LIST_RULE_LIST_S *pstList = AclIPGroup_FindByName(muc_id, pcSipPool);
            if (! pstList) {
                EXEC_OutString("Acl src_ip_group not found.\r\n");
                return BS_NOT_FOUND;
            }
            stRuleCfg.uiKeyMask |= IPACL_KEY_POOL_SIP;
            pstPoolKey->pstSipPool = pstList;
        }
        if (NULL != pcDipPool)
        {
            LIST_RULE_LIST_S *pstList = AclIPGroup_FindByName(muc_id, pcDipPool);
            if (! pstList) {
                EXEC_OutString("Acl dst_ip_group not found.\r\n");
                return BS_NOT_FOUND;
            }
            stRuleCfg.uiKeyMask |= IPACL_KEY_POOL_DIP;
            pstPoolKey->pstDipPool = pstList;
        }
        if (NULL != pcSportPool){
            LIST_RULE_LIST_S *pstList = AclPortGroup_FindByName(muc_id, pcSportPool);
            if (! pstList) {
                EXEC_OutString("Acl src_port_group not found.\r\n");
                return BS_NOT_FOUND;
            }
            stRuleCfg.uiKeyMask |= IPACL_KEY_POOL_SPORT;
            pstPoolKey->pstSportPool = pstList;
        }
        if (NULL != pcDportPool){
            LIST_RULE_LIST_S *pstList = AclPortGroup_FindByName(muc_id, pcDportPool);
            if (! pstList) {
                EXEC_OutString("Acl dst_port_group not found.\r\n");
                return BS_NOT_FOUND;
            }
            stRuleCfg.uiKeyMask |= IPACL_KEY_POOL_DPORT;
            pstPoolKey->pstDportPool = pstList;
        }

        if (0 != uiProto) {
            if (uiProto < 1 || uiProto > 255) 
            {
                EXEC_OutInfo("ACL rule protocol invalid \r\n");
                return BS_BAD_PARA;
            }
            stRuleCfg.ucProto = uiProto;
            stRuleCfg.uiKeyMask |= IPACL_KEY_PROTO;
        }
    }

    stRuleCfg.bEnable = TRUE;
    stRuleCfg.enAction = enAction;

    *puiRuleID = uiRuleID;
    *pstRuleCfg = stRuleCfg;
    return BS_OK;
}

static BS_STATUS _aclappip_LoadRule(int muc_id, IPACL_HANDLE ipacl, IN UINT uiListID, IN CHAR *pcListName, IN CHAR *pcFileName)
{
    FILE *fp;
    BS_STATUS enRet = BS_OK;
    INT iLen;
    CHAR acLine[1024];
    UINT uiArgc;
    CHAR *ppcArgv[IPACL_CMD_RULE_ELEMENT_MAX];
    UINT uiRuleID = 0;
    IPACL_RULE_CFG_S stRuleCfg = {0};
    IPACL_LIST_HANDLE hIpAclList;

    hIpAclList = IPACL_CreateList(ipacl, pcListName);
    if (NULL == hIpAclList)
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

        Mem_Zero(ppcArgv, sizeof(CHAR*)*IPACL_CMD_RULE_ELEMENT_MAX);
        uiArgc = ARGS_Split(acLine, ppcArgv, IPACL_CMD_RULE_ELEMENT_MAX);
        if (uiArgc == 0)
        {
            
            continue;
        } 
        
        if (uiArgc < IPACL_CMD_RULE_ELEMENT_MIN)
        {
            EXEC_OutInfo("ACL rule argc %u error \r\n", uiArgc);
            enRet = BS_NO_SUCH;
            break;
        }

        enRet = _aclappip_ParseRule(muc_id, uiArgc, ppcArgv, &uiRuleID, &stRuleCfg);
        if (BS_OK != enRet)
        {
            break;
        }

        enRet = IPACL_AddRule2List(ipacl, hIpAclList, uiRuleID, &stRuleCfg);
        if (BS_OK != enRet)
        {
            break;
        }
    }

    if (BS_OK == enRet)
    {
        enRet = IPACL_ReplaceList(ipacl, uiListID, hIpAclList);
    }

    if (BS_OK != enRet)
    {
        IPACL_DestroyList(ipacl, hIpAclList);
    }
    
    fclose(fp);
    return enRet;
}

BS_STATUS AclAppIP_Init()
{
    return BS_OK;
}

void AclAppIP_DestroyMuc(ACL_MUC_S *acl_muc)
{
    if (acl_muc->ipacl) {
        IPACL_Destroy(acl_muc->ipacl);
        acl_muc->ipacl = NULL;
    }
}




PLUG_API BS_STATUS AclAppIP_EnterListView(IN UINT uiArgc, IN CHAR **ppcArgv, IN VOID *pEnv)
{
    BS_STATUS eRet;
    IPACL_HANDLE ipacl = _aclappip_get_create_instance_by_env(pEnv);

    if (! ipacl) {
        EXEC_OutInfo("Can't get ipacl instance \r\n");
        return BS_ERR;
    }

    eRet = _aclappip_EnterListView(ipacl, ppcArgv[1]);
    if (BS_OK != eRet)
    {
        EXEC_OutInfo("%s \r\n", ErrInfo_Get(eRet));
        return BS_ERR;
    }

    return BS_OK;
}


PLUG_API BS_STATUS AclAppIP_CmdNoList(IN UINT uiArgc, IN CHAR **ppcArgv, IN VOID *pEnv)
{
    BS_STATUS eRet;
    IPACL_HANDLE ipacl = _aclappip_get_instance_by_env(pEnv);

    if (! ipacl) {
        EXEC_OutInfo("Can't get ipacl instance \r\n");
        return BS_ERR;
    }

    eRet = _aclappip_NoAclList(ipacl, ppcArgv[2]);
    if (eRet == BS_REF_NOT_ZERO)
    {
        EXEC_OutInfo("%s \r\n", ErrInfo_Get(eRet));
    }

    return eRet;
}

PLUG_API BS_STATUS AclAppIP_Clear(IN UINT uiArgc, IN CHAR **ppcArgv, IN VOID *pEnv)
{
    IPACL_HANDLE ipacl = _aclappip_get_instance_by_env(pEnv);

    if (! ipacl) {
        return 0;
    }

    IPACL_Reset(ipacl);

    return 0;
}


PLUG_API BS_STATUS AclAppIP_CmdMoveRule(IN UINT uiArgc, IN CHAR **ppcArgv, IN VOID *pEnv)
{
    CHAR *pcListName;
    UINT uiOldRuleID;
    UINT uiNewRuleID;
    BS_STATUS eRet;
    IPACL_HANDLE ipacl = _aclappip_get_instance_by_env(pEnv);

    if (! ipacl) {
        EXEC_OutInfo("Can't get ipacl instance \r\n");
        return BS_ERR;
    }

    uiOldRuleID = TXT_Str2Ui(ppcArgv[2]);
    uiNewRuleID = TXT_Str2Ui(ppcArgv[4]);

    if ((uiOldRuleID == 0) || (uiNewRuleID == 0)) {
        return BS_BAD_PARA;
    }

    if (uiOldRuleID == uiNewRuleID) {
        return BS_OK;
    }

    pcListName = CMD_EXP_GetCurrentModeValue(pEnv);

    eRet = _aclappip_MoveRule(ipacl, pcListName, uiOldRuleID, uiNewRuleID);
    if (eRet != BS_OK) {
        EXEC_OutInfo("%s \r\n", ErrInfo_Get(eRet));
    }

    return eRet;
}


PLUG_API BS_STATUS AclAppIP_CmdNoRule(IN UINT uiArgc, IN CHAR **ppcArgv, IN VOID *pEnv)
{
    CHAR *pcListName;
    UINT uiRuleID = 0;
    CHAR *pcRuleID = ppcArgv[2];

    IPACL_HANDLE ipacl = _aclappip_get_instance_by_env(pEnv);
    if (! ipacl) {
        EXEC_OutInfo("Can't get ipacl instance \r\n");
        return BS_ERR;
    }

    uiRuleID = TXT_Str2Ui(pcRuleID);

    if (uiRuleID == 0) {
        EXEC_OutString("ACL rule id invalid \r\n");
        RETURN(BS_BAD_PARA);
    }

    pcListName = CMD_EXP_GetCurrentModeValue(pEnv);
    _aclappip_NoRule(ipacl, pcListName, uiRuleID);

    return BS_OK;
}

PLUG_API BS_STATUS AclAppIP_CmdRebaseRuleID(IN UINT uiArgc, IN CHAR **ppcArgv, IN VOID *pEnv)
{
    UINT uiListID;
    UINT uiRuleID;
    UINT uiStep = 0;
    CHAR *pcListName;

    IPACL_HANDLE ipacl = _aclappip_get_instance_by_env(pEnv);
    if (! ipacl) {
        EXEC_OutInfo("Can't get ipacl instance \r\n");
        return BS_ERR;
    }

    TXT_Atoui(ppcArgv[2], &uiStep);
    if (uiStep == 0)
    {
        EXEC_OutString("Rebase step is invalid\r\n");
        return BS_BAD_PARA;
    }

    pcListName = CMD_EXP_GetCurrentModeValue(pEnv);
    uiListID = IPACL_GetListByName(ipacl, pcListName);
    if (0 == uiListID)
    {
        EXEC_OutInfo("ACL list %s is not found \r\n", pcListName);
        return BS_NO_SUCH;
    }

    uiRuleID = IPACL_GetLastRuleID(ipacl, uiListID);
    if (0 != uiRuleID && uiRuleID + uiStep > IPACL_RULE_ID_MAX)
    {
        EXEC_OutString("Rule id reatch max\r\n");
        return BS_REACH_MAX;
    }

    return IPACL_RebaseID(ipacl, uiListID, uiStep);
}

PLUG_API BS_STATUS AclAppIP_Cmd_IncreaseID(IN UINT uiArgc, IN CHAR **ppcArgv, IN VOID *pEnv)
{
    BS_STATUS enRet;
    UINT uiStart;
    UINT uiEnd;
    UINT uiStep = 1;
    UINT uiListID;
    UINT uiRuleID;
    IPACL_RULE_S *pstRule;
    CHAR *pcListName;

    IPACL_HANDLE ipacl = _aclappip_get_instance_by_env(pEnv);
    if (! ipacl) {
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

    if (uiEnd + uiStep > IPACL_RULE_ID_MAX)
    {
        EXEC_OutString("Rule id reatch max\r\n");
        return BS_REACH_MAX;
    }

    pcListName = CMD_EXP_GetCurrentModeValue(pEnv);
    uiListID = IPACL_GetListByName(ipacl, pcListName);
    if (0 == uiListID)
    {
        EXEC_OutInfo("ACL list %s is not found \r\n", pcListName);
        return BS_NO_SUCH;
    }

    pstRule = IPACL_GetRule(ipacl, uiListID, uiStart);
    if (NULL == pstRule)
    {
        EXEC_OutString("ACL rule id invalid \r\n");
        return BS_BAD_PARA;
    }

    pstRule = IPACL_GetRule(ipacl, uiListID, uiEnd);
    if (NULL == pstRule)
    {
        EXEC_OutString("ACL rule id invalid \r\n");
        return BS_BAD_PARA;
    }

    uiRuleID = IPACL_GetNextRuleID(ipacl, uiListID, uiEnd);
    if ((0 != uiRuleID) && (uiRuleID <= uiEnd + uiStep))
    {
        EXEC_OutString("Increase id conflict with other rule \r\n");
        return BS_BAD_PARA;
    }

    IPACL_IncreaseID(ipacl, uiListID, uiStart, uiEnd, uiStep);

    return BS_OK;
}


PLUG_API BS_STATUS AclAppIP_CmdSet_DefaultAction(IN UINT uiArgc, IN CHAR **ppcArgv, IN VOID *pEnv)
{
    CHAR *pcListName = CMD_EXP_GetCurrentModeValue(pEnv);
    BS_ACTION_E enAction;
    UINT uiListID;

    IPACL_HANDLE ipacl = _aclappip_get_instance_by_env(pEnv);
    if (! ipacl) {
        EXEC_OutInfo("Can't get ipacl instance \r\n");
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

    uiListID = IPACL_GetListByName(ipacl, pcListName);
    if (0 == uiListID)
    {
        return BS_NO_SUCH;
    }

    return IPACL_SetDefaultActionByID(ipacl, uiListID, enAction);
}


PLUG_API BS_STATUS AclAppIP_CmdRule(IN UINT uiArgc, IN CHAR **ppcArgv, IN VOID *pEnv)
{
    BS_STATUS enRet = BS_OK;
    CHAR *pcListName;
    UINT uiListID;
    UINT uiRuleID = 0;
    IPACL_RULE_CFG_S stRuleCfg = {0};
    int muc_id = CmdExp_GetEnvMucID(pEnv);
    IPACL_HANDLE ipacl = _aclappip_get_instance(muc_id);
    if (! ipacl) {
        EXEC_OutInfo("Can't get ipacl instance \r\n");
        return BS_ERR;
    }

    pcListName = CMD_EXP_GetCurrentModeValue(pEnv);
    uiListID = IPACL_GetListByName(ipacl, pcListName);
    if (0 == uiListID)
    {
        EXEC_OutInfo("ACL list %s is not found \r\n", pcListName);
        RETURN(BS_NO_SUCH);
    }

    enRet = _aclappip_ParseRule(muc_id, uiArgc, ppcArgv, &uiRuleID, &stRuleCfg);
    if (BS_OK != enRet)
    {
        RETURN(enRet);
    }
    
    enRet = _aclappip_CfgRule(ipacl, uiListID, uiRuleID, &stRuleCfg);
    if (BS_OK != enRet)
    {
        RETURN(enRet);
    }

    return BS_OK;
}

PLUG_API BS_STATUS AclAppIP_Cmd_LoadFile(IN UINT uiArgc, IN CHAR **ppcArgv, IN VOID *pEnv)
{
    CHAR *pcFileName = ppcArgv[1];
    CHAR *pcListName;
    UINT uiListID;
    S64 filesize;
    int ret;

    IPACL_HANDLE ipacl = _aclappip_get_instance_by_env(pEnv);
    if (! ipacl) {
        EXEC_OutInfo("Can't get ipacl instance \r\n");
        return BS_ERR;
    }

    pcListName = CMD_EXP_GetCurrentModeValue(pEnv);
    uiListID = IPACL_GetListByName(ipacl, pcListName);
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

    filesize = FILE_GetSize(pcFileName);
    if (filesize < 0) {
        EXEC_OutInfo("Can't get file %s size \r\n", pcFileName);
        RETURN(BS_ERR);
    }

    ret = _aclappip_LoadRule(CmdExp_GetEnvMucID(pEnv), ipacl, uiListID, pcListName, pcFileName);
    if (ret < 0) {
        EXEC_OutInfo("%s \r\n", ErrInfo_Get(ret));
        return ret;
    }

    return BS_OK;
}


PLUG_API BS_STATUS AclAppIP_CmdShowRefer(IN UINT uiArgc, IN CHAR **ppcArgv, IN VOID *pEnv)
{
    UINT uiListID;
    UINT uiRef;
    CHAR *pcListName = CMD_EXP_GetCurrentModeValue(pEnv);

    IPACL_HANDLE ipacl = _aclappip_get_instance_by_env(pEnv);
    if (! ipacl) {
        EXEC_OutInfo("Can't get ipacl instance \r\n");
        return BS_ERR;
    }

    uiListID = IPACL_GetListByName(ipacl, pcListName);
    if (0 == uiListID)
    {
        EXEC_OutInfo("ACL list %s is not found \r\n", pcListName);
        RETURN(BS_NO_SUCH);
    }

    uiRef = IPACL_ListGetRef(ipacl, uiListID);
    EXEC_OutInfo("refered-count: %u\r\n", uiRef);
    return BS_OK;
}


PLUG_API BS_STATUS AclAppIP_CmdShowStats(IN UINT uiArgc, IN CHAR **ppcArgv, IN VOID *pEnv)
{
    UINT uiListID;
    CHAR *pcListName = CMD_EXP_GetCurrentModeValue(pEnv);

    IPACL_HANDLE ipacl = _aclappip_get_instance_by_env(pEnv);
    if (! ipacl) {
        EXEC_OutInfo("Can't get ipacl instance \r\n");
        return BS_ERR;
    }

    uiListID = IPACL_GetListByName(ipacl, pcListName);
    if (0 == uiListID)
    {
        EXEC_OutInfo("ACL list %s is not found \r\n", pcListName);
        RETURN(BS_NO_SUCH);
    }

    EXEC_OutString("rule       match \r\n----------------------\r\n");

    IPACL_ScanRule(ipacl, uiListID, _aclappip_ShowStats, NULL);

    return BS_OK;
}

PLUG_API BS_STATUS AclAppIP_CmdClearStats(IN UINT uiArgc, IN CHAR **ppcArgv, IN VOID *pEnv)
{
    UINT uiListID;
    CHAR *pcListName = CMD_EXP_GetCurrentModeValue(pEnv);

    IPACL_HANDLE ipacl = _aclappip_get_instance_by_env(pEnv);
    if (! ipacl) {
        EXEC_OutInfo("Can't get ipacl instance \r\n");
        return BS_ERR;
    }

    uiListID = IPACL_GetListByName(ipacl, pcListName);
    if (0 == uiListID)
    {
        EXEC_OutInfo("ACL list %s is not found \r\n", pcListName);
        RETURN(BS_NO_SUCH);
    }

    IPACL_ScanRule(ipacl, uiListID, _aclappip_ClearStats, NULL);
    return BS_OK;
}

BS_STATUS AclAppIP_Save(IN HANDLE hFile)
{
    UINT ulListID = 0;
    CHAR *pcListName;
    
    int muc_id = CmdExp_GetMucIDBySaveHandle(hFile);

    IPACL_HANDLE ipacl = _aclappip_get_instance(muc_id);
    if (! ipacl) {
        return 0;
    }

    while ((ulListID = IPACL_GetNextListID(ipacl, ulListID)) != 0) {
        pcListName = IPACL_GetListNameByID(ipacl, ulListID);
        if (0 == CMD_EXP_OutputMode(hFile, "ip-acl %s", pcListName)) {
            _aclappip_SaveCmdInList(ipacl, hFile, ulListID);
            CMD_EXP_OutputModeQuit(hFile);
        }
    }

    return BS_OK;
}

BS_ACTION_E AclAppIp_Match(int muc_id, UINT ulListID, IN IPACL_MATCH_INFO_S *pstMatchInfo)
{
    IPACL_HANDLE ipacl = _aclappip_get_instance(muc_id);
    if (! ipacl) {
        return BS_ACTION_UNDEF;
    }

    return IPACL_Match(ipacl, ulListID, pstMatchInfo);
}

BS_STATUS AclAppIp_Ioctl(int muc_id, COMP_ACL_IOCTL_E enCmd, IN VOID *pData)
{
    BS_STATUS eRet = BS_OK;

    IPACL_HANDLE ipacl = _aclappip_get_instance(muc_id);
    if (! ipacl) {
        RETURN(BS_NO_SUCH);
    }

    switch (enCmd)
    {
    case COMP_ACL_IOCTL_ADD_LIST_REF:
        eRet = _aclappip_AddListRef(ipacl, pData);
        break;
    case COMP_ACL_IOCTL_DEL_LIST_REF:
        eRet = _aclappip_DelListRef(ipacl, pData);
        break;
    case COMP_ACL_IOCTL_DEL_LIST_REF_BY_ID:
        eRet = _aclappip_DelListRefByID(ipacl, pData);
        break;
    case COMP_ACL_IOCTL_GET_LIST_ID:
        eRet = _aclappip_GetListIDByName(ipacl, pData);
        break;
    default:
        eRet = BS_NOT_SUPPORT;
        break;
    }

    return eRet;
}

PLUG_API BS_STATUS AclAppIP_TestIpMatch(IN UINT uiArgc, IN CHAR **ppcArgv, IN VOID *pEnv)
{
    CHAR *pcListName;
    IPACL_MATCH_INFO_S stMatchInfo = {0};
    BS_ACTION_E ret = BS_ACTION_UNDEF;
    CHAR *pcSip;
    CHAR *pcDip;
    UINT uiSport;
    UINT uiDport;
    UINT uiProto;
    UINT ulListID;
    UINT uiCount = 1;
    UINT i;
    UINT uiRun;
    struct timeval time_run;
    struct timeval time_start;
    struct timeval time_end;
    
    int muc_id = CmdExp_GetEnvMucID(pEnv);

    IPACL_HANDLE ipacl = _aclappip_get_instance_by_env(pEnv);
    if (! ipacl) {
        EXEC_OutInfo("Can't get ipacl instance \r\n");
        return BS_ERR;
    }

    pcSip = ppcArgv[2];
    pcDip = ppcArgv[4];
    TXT_Atoui(ppcArgv[6], &uiSport);
    TXT_Atoui(ppcArgv[8], &uiDport);
    TXT_Atoui(ppcArgv[10], &uiProto);

    if (uiArgc >= 12)
    {
        TXT_Atoui(ppcArgv[12], &uiCount);
    }

    if (!Socket_IsIPv4(pcSip) || !Socket_IsIPv4(pcDip))
    {
        EXEC_OutString("not ipv4 string.\r\n");
        return BS_BAD_PARA;
    }

    pcListName = CMD_EXP_GetCurrentModeValue(pEnv);
    ulListID = IPACL_GetListByName(ipacl, pcListName);
    if (0 == ulListID)
    {
        EXEC_OutString("not found acl list.\r\n");
        return BS_NOT_FOUND;
    }

    stMatchInfo.uiKeyMask = IPACL_KEY_SIP | IPACL_KEY_DIP | IPACL_KEY_SPORT | IPACL_KEY_DPORT | IPACL_KEY_PROTO | IPACL_KEY_POOL_SPORT | IPACL_KEY_POOL_DIP | IPACL_KEY_POOL_SIP | IPACL_KEY_POOL_DPORT;
    stMatchInfo.uiSIP = Socket_Ipsz2IpHost(pcSip);
    stMatchInfo.uiDIP = Socket_Ipsz2IpHost(pcDip);
    stMatchInfo.usSPort = uiSport;
    stMatchInfo.usDPort = uiDport;
    stMatchInfo.ucProto = uiProto;
    
    gettimeofday(&time_start, NULL);

    for (i=0; i < uiCount; i++)
    {
        ret = AclAppIp_Match(muc_id, ulListID, &stMatchInfo);
        if (BS_ACTION_UNDEF == ret || BS_ACTION_MAX == ret)
        {
            EXEC_OutInfo("test not match: %s\r\n", (ret == BS_ACTION_UNDEF) ? "undef" : "max");
            return BS_NOT_FOUND;
        }
    }

    gettimeofday(&time_end, NULL);
    timersub(&time_end, &time_start, &time_run);
    uiRun = time_run.tv_sec * 1000000 + time_run.tv_usec;

    EXEC_OutInfo("test match: %s\r\n", (ret == BS_ACTION_DENY) ? "deny" : "permit");
    EXEC_OutInfo("rum time: %d\r\n", uiRun);
    return BS_OK;
}
