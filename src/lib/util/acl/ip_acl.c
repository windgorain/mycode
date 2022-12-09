/******************************************************************************
* Copyright (C), 2000-2006,  LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2016-10-14
* Description: IP ACL
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/atomic_utl.h"
#include "utl/list_rule.h"
#include "utl/rule_list.h"
#include "utl/ip_acl.h"
#include "utl/mem_cap.h"

typedef struct {
    RULE_NODE_S stRuleListNode;
    IPACL_RULE_S stRule;
}_IPACL_RULE_DESC_S;

typedef struct {
    IPACL_MATCH_INFO_S *pstMatchInfo;
    BOOL_T bIsMatch;
    BS_ACTION_E enAction;
}_IPACL_MATCH_DESC_S;

typedef struct {
    PF_IPACL_RULE_SCAN pfScan;
    void *pUseData;
}_IPACL_SCAN_DESC_S;

static void _ipacl_inc_pool_ref(INOUT IPACL_POOL_KEY_S *pstPoolKeys)
{
    DBGASSERT(NULL != pstPoolKeys);

    if (pstPoolKeys->pstSipPool){
        pstPoolKeys->pstSipPool->uiRefCount ++;
    }
    if (pstPoolKeys->pstDipPool){
        pstPoolKeys->pstDipPool->uiRefCount ++;
    }
    if (pstPoolKeys->pstSportPool){
        pstPoolKeys->pstSportPool->uiRefCount ++;
    }
    if (pstPoolKeys->pstDportPool){
        pstPoolKeys->pstDportPool->uiRefCount ++;
    }
}

static void _ipacl_dec_pool_ref(INOUT IPACL_POOL_KEY_S *pstPoolKeys)
{
    DBGASSERT(NULL != pstPoolKeys);

    if (pstPoolKeys->pstSipPool){
        pstPoolKeys->pstSipPool->uiRefCount --;
    }
    if (pstPoolKeys->pstDipPool){
        pstPoolKeys->pstDipPool->uiRefCount --;
    }
    if (pstPoolKeys->pstSportPool){
        pstPoolKeys->pstSportPool->uiRefCount --;
    }
    if (pstPoolKeys->pstDportPool){
        pstPoolKeys->pstDportPool->uiRefCount --;
    }
}

static BOOL_T _ipacl_match_by_rule(IN IPACL_MATCH_INFO_S *pstMatchInfo, IN _IPACL_RULE_DESC_S *pstRule)
{
    IPACL_RULE_CFG_S *pstRuleCfg = &pstRule->stRule.stRuleCfg;
    IPACL_SINGLE_KEY_S *pstSingleKey = &pstRule->stRule.stRuleCfg.stKey.single;
    UINT uiMask;

    if (! pstRuleCfg->bEnable) {
        return FALSE;
    }

    if ((pstMatchInfo->uiKeyMask & IPACL_KEY_PROTO) && (pstRuleCfg->uiKeyMask & IPACL_KEY_PROTO)) {
        if (pstMatchInfo->ucProto != pstRuleCfg->ucProto) {
            return FALSE;
        }
    }
    
    if ((pstMatchInfo->uiKeyMask & IPACL_KEY_SIP) && (pstRuleCfg->uiKeyMask & IPACL_KEY_SIP)) {
        uiMask = pstSingleKey->stSIP.uiWildcard;
        if ((pstMatchInfo->uiSIP & uiMask) != (pstSingleKey->stSIP.uiIP & uiMask)) {
            return FALSE;
        }
    }

    if ((pstMatchInfo->uiKeyMask & IPACL_KEY_DIP) && (pstRuleCfg->uiKeyMask & IPACL_KEY_DIP)) {
        uiMask = pstSingleKey->stDIP.uiWildcard;
        if ((pstMatchInfo->uiDIP & uiMask) != (pstSingleKey->stDIP.uiIP & uiMask)) {
            return FALSE;
        }
    }

    if ((pstMatchInfo->uiKeyMask & IPACL_KEY_SPORT) && (pstRuleCfg->uiKeyMask & IPACL_KEY_SPORT)) {
        if ((pstMatchInfo->usSPort < pstSingleKey->stSPort.usBegin)
            || (pstMatchInfo->usSPort > pstSingleKey->stSPort.usEnd)) {
            return FALSE;
        }
    }

    if ((pstMatchInfo->uiKeyMask & IPACL_KEY_DPORT) && (pstRuleCfg->uiKeyMask & IPACL_KEY_DPORT)) {
        if ((pstMatchInfo->usDPort < pstSingleKey->stDPort.usBegin)
            || (pstMatchInfo->usDPort > pstSingleKey->stDPort.usEnd)) {
            return FALSE;
        }
    }

    if ((pstMatchInfo->uiKeyMask & IPACL_KEY_POOL_SIP) && (pstRuleCfg->uiKeyMask & IPACL_KEY_POOL_SIP)) {
        if (!AddressPool_MatchIpPool(pstRuleCfg->stKey.pools.pstSipPool,pstMatchInfo->uiSIP)) {
            return FALSE;
        }
    }

    if ((pstMatchInfo->uiKeyMask & IPACL_KEY_POOL_DIP) && (pstRuleCfg->uiKeyMask & IPACL_KEY_POOL_DIP)) {
        if (!AddressPool_MatchIpPool(pstRuleCfg->stKey.pools.pstDipPool,pstMatchInfo->uiDIP)){
            return FALSE;
        }
    }

    if ((pstMatchInfo->uiKeyMask & IPACL_KEY_POOL_SPORT) && (pstRuleCfg->uiKeyMask & IPACL_KEY_POOL_SPORT)) {
        if (!PortPool_MatchPortPool(pstRuleCfg->stKey.pools.pstSportPool, pstMatchInfo->usSPort)){
            return FALSE;
        }
    }

    if ((pstMatchInfo->uiKeyMask & IPACL_KEY_POOL_DPORT) && (pstRuleCfg->uiKeyMask & IPACL_KEY_POOL_DPORT)) {
        if (!PortPool_MatchPortPool(pstRuleCfg->stKey.pools.pstDportPool, pstMatchInfo->usDPort)){
            return FALSE;
        }
    }

    ATOM_INC_FETCH(&pstRule->stRule.stStatistics.ulMatchCount);

    return TRUE;
}

static BOOL_T _ipacl_scan_rule(IN void *pstRule, IN VOID *ud)
{
    UINT uiRuleID;
    _IPACL_SCAN_DESC_S *pScanDesc = ud;
    _IPACL_RULE_DESC_S *pstRuleDesc = container_of(pstRule, _IPACL_RULE_DESC_S, stRuleListNode);

    if (NULL != pScanDesc && NULL != pScanDesc->pfScan) {
        uiRuleID = Rule_GetIDByNode(&pstRuleDesc->stRuleListNode);
        return pScanDesc->pfScan(uiRuleID, &pstRuleDesc->stRule, pScanDesc->pUseData);
    }

    return BOOL_FALSE;
}

static BOOL_T _ipacl_match_rule(IN void *pstRule, IN VOID *ud)
{
    BOOL_T bIsMatch = FALSE;
    UINT uiRuleID;
    _IPACL_MATCH_DESC_S *pstMatch = ud;

    _IPACL_RULE_DESC_S *pstRuleDesc = container_of(pstRule, _IPACL_RULE_DESC_S, stRuleListNode);
    if (pstRuleDesc) {
        uiRuleID = Rule_GetIDByNode(&pstRuleDesc->stRuleListNode);
        bIsMatch = _ipacl_match_by_rule(pstMatch->pstMatchInfo, pstRuleDesc);
        if (BOOL_TRUE == bIsMatch) {
            pstMatch->bIsMatch = BOOL_TRUE;
            pstMatch->enAction = pstRuleDesc->stRule.stRuleCfg.enAction;
            pstMatch->pstMatchInfo->acl_id = uiRuleID;
            return BOOL_FALSE;
        }  
    }

    return BOOL_TRUE;
}

static VOID _ipacl_delete_rule(void *pstRule, void *ud)
{
    IPACL_HANDLE acl = ud;

    _IPACL_RULE_DESC_S *pstData = container_of(pstRule, _IPACL_RULE_DESC_S, stRuleListNode);
    if (NULL != pstData) {
        _ipacl_dec_pool_ref(&pstData->stRule.stRuleCfg.stKey.pools);
        MemCap_Free(acl->memcap, pstData);
    }
}

IPACL_HANDLE IPACL_Create(void *memcap)
{
    return ListRule_Create(memcap);
}

void IPACL_Destroy(IPACL_HANDLE hIpAcl)
{
    ListRule_Destroy(hIpAcl, _ipacl_delete_rule, hIpAcl);
}

void IPACL_Reset(IPACL_HANDLE hIpAcl)
{
    ListRule_Reset(hIpAcl, _ipacl_delete_rule, hIpAcl);
}

IPACL_LIST_HANDLE IPACL_CreateList(IPACL_HANDLE hIpAcl, char *pcListName)
{
    return ListRule_CreateList(hIpAcl, pcListName, 0);
}

VOID IPACL_DestroyList(IPACL_HANDLE hIpAcl, IPACL_LIST_HANDLE hIpAclList)
{
    ListRule_DestroyList(hIpAcl, hIpAclList, _ipacl_delete_rule, hIpAcl);
}

UINT IPACL_AddList(IN IPACL_HANDLE hIpAcl, IN CHAR *pcListName)
{
    return ListRule_AddList(hIpAcl, pcListName, 0);
}

VOID IPACL_DelList(IN IPACL_HANDLE hIpAcl, IN UINT uiListID)
{
    ListRule_DelListID(hIpAcl, uiListID, _ipacl_delete_rule, hIpAcl);
}

BS_STATUS IPACL_ReplaceList(IN IPACL_HANDLE hIpAcl, IN UINT uiListID, IN IPACL_LIST_HANDLE hIpAclListNew)
{
    return ListRule_ReplaceList(hIpAcl, uiListID, _ipacl_delete_rule, hIpAcl, hIpAclListNew);
}

BS_STATUS IPACL_AddListRef(IN IPACL_HANDLE hIpAcl, IN UINT uiListID)
{
    return ListRule_IncListRef(hIpAcl, uiListID);
}

BS_STATUS IPACL_DelListRef(IN IPACL_HANDLE hIpAcl, IN UINT uiListID)
{
    return ListRule_DecListRef(hIpAcl, uiListID);
}

UINT IPACL_ListGetRef(IN IPACL_HANDLE hIpAcl, IN UINT uiListID)
{
    return ListRule_GetListRef(hIpAcl, uiListID);
}

UINT IPACL_GetListByName(IN IPACL_HANDLE hIpAcl, IN CHAR *pcListName)
{
    return ListRule_GetListIDByName(hIpAcl, pcListName);
}

UINT IPACL_GetNextListID(IN IPACL_HANDLE hIpAcl, IN UINT uiCurrentListID/* 0表示获取第一个 */)
{
    return ListRule_GetNextListID(hIpAcl, uiCurrentListID);
}

CHAR * IPACL_GetListNameByID(IN IPACL_HANDLE hIpAcl, IN UINT ulListID)
{
    return ListRule_GetListNameByID(hIpAcl, ulListID);
}

BS_ACTION_E IPACL_GetDefaultActionByID(IN IPACL_HANDLE hIpAcl, IN UINT uiListID)
{
    return ListRule_GetDefaultActionByID(hIpAcl, uiListID);
}

BS_STATUS IPACL_SetDefaultActionByID(IN IPACL_HANDLE hIpAcl, IN UINT uiListID, BS_ACTION_E enAction)
{
    return ListRule_SetDefaultActionByID(hIpAcl, uiListID, enAction);
}

BS_STATUS IPACL_AddRule2List(IN IPACL_HANDLE hIpAcl, IN IPACL_LIST_HANDLE hIpAclList, 
                             IN UINT uiRuleID, IN IPACL_RULE_CFG_S *pstRuleCfg)
{
    LIST_RULE_HANDLE list_rule = hIpAcl;
    _IPACL_RULE_DESC_S *pstRule;
    BS_STATUS eRet;

    pstRule = MemCap_ZMalloc(list_rule->memcap, sizeof(_IPACL_RULE_DESC_S));
    if (pstRule) {
        return BS_NO_MEMORY;
    }
    pstRule->stRule.stRuleCfg = *pstRuleCfg;

    eRet = ListRule_AddRule2List(hIpAclList, uiRuleID, &pstRule->stRuleListNode);
    if (BS_OK != eRet) {
        MemCap_Free(list_rule->memcap, pstRule);
        return eRet;
    }

    _ipacl_inc_pool_ref(&pstRuleCfg->stKey.pools);

    return BS_OK;
}

BS_STATUS IPACL_AddRule(IPACL_HANDLE acl, IN UINT uiListID, IN UINT uiRuleID, IN IPACL_RULE_CFG_S *pstRuleCfg)
{
    _IPACL_RULE_DESC_S *pstRule;
    BS_STATUS eRet;

    pstRule = MemCap_ZMalloc(acl->memcap, sizeof(_IPACL_RULE_DESC_S));
    if (NULL == pstRule) {
        return BS_NO_MEMORY;
    }
    pstRule->stRule.stRuleCfg = *pstRuleCfg;

    eRet = ListRule_AddRule(acl, uiListID, uiRuleID, &pstRule->stRuleListNode);
    if (BS_OK != eRet) {
        MemCap_Free(acl->memcap, pstRule);
        return eRet;
    }

    _ipacl_inc_pool_ref(&pstRuleCfg->stKey.pools);

    return BS_OK;
}

VOID IPACL_DelRule(IN IPACL_HANDLE acl, IN UINT uiListID, IN UINT uiRuleID)
{
    _IPACL_RULE_DESC_S *pstRule;
    
    pstRule = (VOID*) ListRule_DelRule(acl, uiListID, uiRuleID);
    if (! pstRule) {
        return;
    }

    _ipacl_dec_pool_ref(&pstRule->stRule.stRuleCfg.stKey.pools);

    MemCap_Free(acl->memcap, pstRule);
}

IPACL_RULE_S * IPACL_GetRule(IN IPACL_HANDLE hIpAcl, IN UINT uiListID, IN UINT uiRuleID)
{
    _IPACL_RULE_DESC_S *pstRule;
    
    pstRule = (VOID*) ListRule_GetRule(hIpAcl, uiListID, uiRuleID);
    if (! pstRule) {
        return NULL;
    }

    return &pstRule->stRule;
}

UINT IPACL_GetLastRuleID(IN IPACL_HANDLE hIpAcl, IN UINT uiListID)
{
    _IPACL_RULE_DESC_S *pstRule;
    
    pstRule = (VOID*) ListRule_GetLastRule(hIpAcl, uiListID);
    if (! pstRule) {
        return 0;
    }

    return pstRule->stRuleListNode.uiRuleID;
}

VOID IPACL_UpdateRule(IN IPACL_RULE_S *pstRule, IN IPACL_RULE_CFG_S *pstRuleCfg)
{
    _ipacl_dec_pool_ref(&pstRule->stRuleCfg.stKey.pools);
    pstRule->stRuleCfg = *pstRuleCfg;
    _ipacl_inc_pool_ref(&pstRule->stRuleCfg.stKey.pools);
}

BS_STATUS IPACL_MoveRule(IN IPACL_HANDLE hIpAcl, IN UINT uiListID, IN UINT uiOldRuleID, IN UINT uiNewRuleID)
{
    return ListRule_MoveRule(hIpAcl, uiListID, uiOldRuleID, uiNewRuleID);
}

/*每个规则的ID都递增step，另一种方案是每两条规则的ID相差step，暂时未实现*/
BS_STATUS IPACL_RebaseID(IN IPACL_HANDLE hIpAcl, IN UINT uiListID, IN UINT uiStep)
{
    return ListRule_ResetID(hIpAcl, uiListID, uiStep);
}

BS_STATUS  IPACL_IncreaseID(IN IPACL_HANDLE hIpAcl, IN UINT uiListID, IN UINT uiStart, IN UINT uiEnd, IN UINT uiStep)
{
    return ListRule_IncreaseID(hIpAcl, uiListID, uiStart, uiEnd, uiStep);
}

UINT IPACL_GetNextRuleID(IN IPACL_HANDLE hIpAcl, IN UINT uiListID, IN UINT uiCurrentRuleID)
{
    RULE_NODE_S *pstNode;

    pstNode = ListRule_GetNextRule(hIpAcl, uiListID, uiCurrentRuleID);
    if (! pstNode) {
        return 0;
    }

    return pstNode->uiRuleID;
}

IPACL_RULE_S * IPACL_GetNextRule(IN IPACL_HANDLE hIpAcl, IN UINT uiListID, IN UINT uiCurrentRuleID)
{
    _IPACL_RULE_DESC_S *pstRuleDesc;

    pstRuleDesc = (VOID*)ListRule_GetNextRule(hIpAcl, uiListID, uiCurrentRuleID);
    if (! pstRuleDesc) {
        return NULL;
    }
    
    return &pstRuleDesc->stRule;
}

VOID IPACL_ScanRule(IPACL_HANDLE hIpAcl, UINT uiListID, PF_IPACL_RULE_SCAN pfFunc, IN VOID *ud)
{
    LIST_RULE_LIST_S* pstList;
    _IPACL_SCAN_DESC_S stScanDesc = {0};

    stScanDesc.pfScan = pfFunc;
    stScanDesc.pUseData = ud;

    pstList = ListRule_GetListByID(hIpAcl, uiListID);
    if (pstList) {
        RuleList_ScanRule(&pstList->stRuleList, _ipacl_scan_rule, &stScanDesc);
    }

    return;
}

BS_ACTION_E IPACL_Match(IPACL_HANDLE hIpAcl, UINT uiListID, IPACL_MATCH_INFO_S *pstMatchInfo)
{
    LIST_RULE_LIST_S* pstList;
    _IPACL_MATCH_DESC_S stMatchDesc;

    Mem_Zero(&stMatchDesc, sizeof(_IPACL_MATCH_DESC_S));
    stMatchDesc.pstMatchInfo = pstMatchInfo;
    stMatchDesc.bIsMatch = BOOL_FALSE;
    stMatchDesc.enAction = BS_ACTION_UNDEF;

    pstList = ListRule_GetListByID(hIpAcl, uiListID);
    if (pstList) {
        /* 按rule匹配 */
        RuleList_ScanRule(&pstList->stRuleList, _ipacl_match_rule, &stMatchDesc);
        if (!stMatchDesc.bIsMatch) {
            stMatchDesc.enAction = pstList->default_action;
        }
    }

    return stMatchDesc.enAction;
}

