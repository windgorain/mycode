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
#include "utl/domain_acl.h"
#include "utl/mem_cap.h"

typedef struct
{
    HANDLE hDOMAINAclHandle;
    void *memcap;
}DOMAINACL_CTRL_S;

typedef struct
{
    RULE_NODE_S stRuleListNode;
    DOMAINACL_RULE_S stRule;
}_DOMAINACL_RULE_DESC_S;

typedef struct
{
    DOMAINACL_MATCH_INFO_S *pstMatchInfo;
    BOOL_T bIsMatch;
    BS_ACTION_E enAction;
}_DOMAINACL_MATCH_DESC_S;

typedef struct
{
    PF_DOMAINACL_RULE_SCAN pfScan;
    void *pUseData;
}_DOMAINACL_SCAN_DESC_S;

STATIC BOOL_T _domainacl_MatchByRule(IN DOMAINACL_MATCH_INFO_S *pstMatchInfo, IN _DOMAINACL_RULE_DESC_S *pstRule)
{
    DOMAINACL_RULE_CFG_S *pstRuleCfg = &pstRule->stRule.stRuleCfg;

    if(pstRuleCfg->uiKeyMask & DOMAINACL_KEY_SIP) {
        UINT uiMask = pstRuleCfg->stKey.single.stSIP.uiWildcard;
        if ((pstMatchInfo->uiSIP & uiMask) != (pstRuleCfg->stKey.single.stSIP.uiIP & uiMask)) {
            return BOOL_FALSE;
        }
    }
    
    if(pstRuleCfg->uiKeyMask & DOMAINACL_KEY_POOL_SIP) {
        if (! AddressPool_MatchIpPool(pstRuleCfg->stKey.pools.pstSipList, pstMatchInfo->uiSIP)) {
            return BOOL_FALSE;
        }
    }

    if (pstRuleCfg->uiKeyMask & DOMAINACL_KEY_DPORT) {
        DOMAINACL_SINGLE_KEY_S *pstSingleKey = &pstRuleCfg->stKey.single;
        if ((pstMatchInfo->usDPort < pstSingleKey->stDPort.usBegin)
            || (pstMatchInfo->usDPort > pstSingleKey->stDPort.usEnd)) {
            return BOOL_FALSE;
        }
    }

    if (pstRuleCfg->uiKeyMask & DOMAINACL_KEY_POOL_DPORT) {
        if (! PortPool_MatchPortPool(pstRuleCfg->stKey.pools.pstDportList, pstMatchInfo->usDPort)) {
            return BOOL_FALSE;
        }
    }

    if (pstRuleCfg->uiKeyMask & DOMAINACL_KEY_DOMAIN) {
        CHAR* pcPattern = pstRuleCfg->stKey.single.szDomain; 
        CHAR* pcStart;
        int pattern_len;
        int domain_len;

        if(*pcPattern == '*') {
            pcPattern++;
        }

        pattern_len = strlen(pcPattern);
        domain_len = strlen(pstMatchInfo->szDomain);
        if(domain_len < pattern_len) {
            return BOOL_FALSE;
        }

        pcStart = pstMatchInfo->szDomain+(domain_len-pattern_len);
        if(memcmp(pcStart, pcPattern, pattern_len) != 0) {
            return BOOL_FALSE;
        }
    }

    if(pstRuleCfg->uiKeyMask & DOMAINACL_KEY_POOL_DOAMIN) {
        if(! DomainGroup_Match(pstRuleCfg->stKey.pools.pstDomainGroup, pstMatchInfo->szDomain)) {
            return BOOL_FALSE;
        }
    }

    ATOM_INC_FETCH(&pstRule->stRule.stStatistics.ulMatchCount);

    return BOOL_TRUE;
}

static BOOL_T _domainacl_RuleSacnOuter(IN void *pstRule, IN VOID *pUserHandle)
{
    UINT uiRuleID;
    _DOMAINACL_SCAN_DESC_S *pScanDesc = pUserHandle;
    _DOMAINACL_RULE_DESC_S *pstRuleDesc = container_of(pstRule, _DOMAINACL_RULE_DESC_S, stRuleListNode);

    if (NULL != pScanDesc && NULL != pScanDesc->pfScan)
    {
        uiRuleID = Rule_GetIDByNode(&pstRuleDesc->stRuleListNode);
        return pScanDesc->pfScan(uiRuleID, &pstRuleDesc->stRule, pScanDesc->pUseData);
    }

    return BOOL_FALSE;
}

static BOOL_T _domainacl_MatchRuleScan(IN void *pstRule, IN VOID *pUserHandle)
{
    BOOL_T bIsMatch = FALSE;
    UINT uiRuleID;
    _DOMAINACL_MATCH_DESC_S *pstMatch = pUserHandle;

    _DOMAINACL_RULE_DESC_S *pstRuleDesc = container_of(pstRule, _DOMAINACL_RULE_DESC_S, stRuleListNode);
    if (NULL != pstRuleDesc){
        uiRuleID = Rule_GetIDByNode(&pstRuleDesc->stRuleListNode);
        bIsMatch = _domainacl_MatchByRule(pstMatch->pstMatchInfo, pstRuleDesc);
        if (BOOL_TRUE == bIsMatch)
        {
            pstMatch->bIsMatch = BOOL_TRUE;
            pstMatch->enAction = pstRuleDesc->stRule.stRuleCfg.enAction;
            pstMatch->pstMatchInfo->acl_id = uiRuleID;
            return BOOL_FALSE;
        }  
    }

    return BOOL_TRUE;
}

static VOID _domainacl_DeleteRule(void *pstRule, void *pUserHandle)
{
    DOMAINACL_CTRL_S *pstDOMAINAcl = pUserHandle;

    _DOMAINACL_RULE_DESC_S *pstData = container_of(pstRule, _DOMAINACL_RULE_DESC_S, stRuleListNode);
    if (NULL != pstData){
        DOMAINACL_DecPoolReferedNumber(&pstData->stRule.stRuleCfg.stKey.pools);
        MemCap_Free(pstDOMAINAcl->memcap, pstData);
    }
}

DOMAINACL_HANDLE DOMAINACL_Create(void* memcap)
{
    DOMAINACL_CTRL_S *pstDOMAINAcl = NULL;

    pstDOMAINAcl = MemCap_ZMalloc(memcap, sizeof(DOMAINACL_CTRL_S));
    if (NULL == pstDOMAINAcl) {
        return NULL;
    }

    pstDOMAINAcl->hDOMAINAclHandle = ListRule_Create(memcap);
    if (NULL == pstDOMAINAcl->hDOMAINAclHandle) {
        MemCap_Free(memcap, pstDOMAINAcl);
        return NULL;
    }
    pstDOMAINAcl->memcap = memcap;

    return pstDOMAINAcl;
}

VOID DOMAINACL_Destroy(IN DOMAINACL_HANDLE hDOMAINAcl)
{
    DOMAINACL_CTRL_S *pstDOMAINAcl = hDOMAINAcl;
    void *memcap = pstDOMAINAcl->memcap;
    ListRule_Destroy(pstDOMAINAcl->hDOMAINAclHandle, _domainacl_DeleteRule, pstDOMAINAcl);
    MemCap_Free(memcap, pstDOMAINAcl);
    return;
}

DOMAINACL_LIST_HANDLE DOMAINACL_CreateList(IN DOMAINACL_HANDLE hDOMAINAcl, IN CHAR *pcListName)
{
    DOMAINACL_CTRL_S *pstDOMAINAcl = hDOMAINAcl;
    return ListRule_CreateList(pstDOMAINAcl->hDOMAINAclHandle, pcListName, 0);
}

VOID DOMAINACL_DestroyList(IN DOMAINACL_HANDLE hDOMAINAcl, IN DOMAINACL_LIST_HANDLE hDOMAINAclList)
{
    DOMAINACL_CTRL_S *pstDOMAINAcl = hDOMAINAcl;
    ListRule_DestroyList(pstDOMAINAcl->hDOMAINAclHandle, hDOMAINAclList, _domainacl_DeleteRule, pstDOMAINAcl);
    return;
}

UINT DOMAINACL_AddList(IN DOMAINACL_HANDLE hDOMAINAcl, IN CHAR *pcListName)
{
    DOMAINACL_CTRL_S *pstDOMAINAcl = hDOMAINAcl;
    return ListRule_AddList(pstDOMAINAcl->hDOMAINAclHandle, pcListName, 0);
}

VOID DOMAINACL_DelList(IN DOMAINACL_HANDLE hDOMAINAcl, IN UINT uiListID)
{
    DOMAINACL_CTRL_S *pstDOMAINAcl = hDOMAINAcl;
    ListRule_DelListID(pstDOMAINAcl->hDOMAINAclHandle, uiListID, _domainacl_DeleteRule, pstDOMAINAcl);
    return;
}

BS_STATUS DOMAINACL_ReplaceList(IN DOMAINACL_HANDLE hDOMAINAcl, IN UINT uiListID, IN DOMAINACL_LIST_HANDLE hDOMAINAclListNew)
{
    DOMAINACL_CTRL_S *pstDOMAINAcl = hDOMAINAcl;
    return ListRule_ReplaceList(pstDOMAINAcl->hDOMAINAclHandle, uiListID, _domainacl_DeleteRule, pstDOMAINAcl, hDOMAINAclListNew);
}

BS_STATUS DOMAINACL_AddListRef(IN DOMAINACL_HANDLE hDOMAINAcl, IN UINT uiListID)
{
    DOMAINACL_CTRL_S *pstDOMAINAcl = hDOMAINAcl;
    return ListRule_IncListRef(pstDOMAINAcl->hDOMAINAclHandle, uiListID);
}

BS_STATUS DOMAINACL_DelListRef(IN DOMAINACL_HANDLE hDOMAINAcl, IN UINT uiListID)
{
    DOMAINACL_CTRL_S *pstDOMAINAcl = hDOMAINAcl;
    return ListRule_DecListRef(pstDOMAINAcl->hDOMAINAclHandle, uiListID);
}

UINT DOMAINACL_ListGetRef(IN DOMAINACL_HANDLE hDOMAINAcl, IN UINT uiListID)
{
    DOMAINACL_CTRL_S *pstDOMAINAcl = hDOMAINAcl;
    return ListRule_GetListRef(pstDOMAINAcl->hDOMAINAclHandle, uiListID);
}

UINT DOMAINACL_GetListByName(IN DOMAINACL_HANDLE hDOMAINAcl, IN CHAR *pcListName)
{
    DOMAINACL_CTRL_S *pstDOMAINAcl = hDOMAINAcl;
    return ListRule_GetListIDByName(pstDOMAINAcl->hDOMAINAclHandle, pcListName);
}

UINT DOMAINACL_GetNextListID(IN DOMAINACL_HANDLE hDOMAINAcl, IN UINT uiCurrentListID)
{
    DOMAINACL_CTRL_S *pstDOMAINAcl = hDOMAINAcl;
    return ListRule_GetNextListID(pstDOMAINAcl->hDOMAINAclHandle, uiCurrentListID);
}

CHAR * DOMAINACL_GetListNameByID(IN DOMAINACL_HANDLE hDOMAINAcl, IN UINT ulListID)
{
    DOMAINACL_CTRL_S *pstDOMAINAcl = hDOMAINAcl;
    return ListRule_GetListNameByID(pstDOMAINAcl->hDOMAINAclHandle, ulListID);
}

BS_ACTION_E DOMAINACL_GetDefaultActionByID(IN DOMAINACL_HANDLE hDOMAINAcl, IN UINT uiListID)
{
    DOMAINACL_CTRL_S *pstDOMAINAcl = hDOMAINAcl;
    return ListRule_GetDefaultActionByID(pstDOMAINAcl->hDOMAINAclHandle, uiListID);
}

BS_STATUS DOMAINACL_SetDefaultActionByID(IN DOMAINACL_HANDLE hDOMAINAcl, IN UINT uiListID, BS_ACTION_E enAction)
{
    DOMAINACL_CTRL_S *pstDOMAINAcl = hDOMAINAcl;
    return ListRule_SetDefaultActionByID(pstDOMAINAcl->hDOMAINAclHandle, uiListID, enAction);
}

BS_STATUS DOMAINACL_AddRule2List(IN DOMAINACL_HANDLE hDOMAINAcl, IN DOMAINACL_LIST_HANDLE hDOMAINAclList, 
                             IN UINT uiRuleID, IN DOMAINACL_RULE_CFG_S *pstRuleCfg)
{
    DOMAINACL_CTRL_S *pstDOMAINAcl = hDOMAINAcl;
    _DOMAINACL_RULE_DESC_S *pstRule;
    BS_STATUS eRet;

    pstRule = MemCap_ZMalloc(pstDOMAINAcl->memcap, sizeof(_DOMAINACL_RULE_DESC_S));
    if (NULL == pstRule)
    {
        return BS_NO_MEMORY;
    }
    pstRule->stRule.stRuleCfg = *pstRuleCfg;

    eRet = ListRule_AddRule2List(hDOMAINAclList, uiRuleID, &pstRule->stRuleListNode);
    if (BS_OK != eRet)
    {
        MemCap_Free(pstDOMAINAcl->memcap, pstRule);
        return eRet;
    }

    DOMAINACL_IncPoolReferedNumber(&pstRuleCfg->stKey.pools);
    return BS_OK;
}

BS_STATUS DOMAINACL_AddRule(IN DOMAINACL_HANDLE hDOMAINAcl, IN UINT uiListID, IN UINT uiRuleID, IN DOMAINACL_RULE_CFG_S *pstRuleCfg)
{
    DOMAINACL_CTRL_S *pstDOMAINAcl = hDOMAINAcl;
    _DOMAINACL_RULE_DESC_S *pstRule;
    BS_STATUS eRet;

    pstRule = MemCap_ZMalloc(pstDOMAINAcl->memcap, sizeof(_DOMAINACL_RULE_DESC_S));
    if (NULL == pstRule)
    {
        return BS_NO_MEMORY;
    }
    pstRule->stRule.stRuleCfg = *pstRuleCfg;

    eRet = ListRule_AddRule(pstDOMAINAcl->hDOMAINAclHandle, uiListID, uiRuleID, &pstRule->stRuleListNode);
    if (BS_OK != eRet)
    {
        MemCap_Free(pstDOMAINAcl->memcap, pstRule);
        return eRet;
    }

    DOMAINACL_IncPoolReferedNumber(&pstRuleCfg->stKey.pools);
    return BS_OK;
}

VOID DOMAINACL_DelRule(IN DOMAINACL_HANDLE hDOMAINAcl, IN UINT uiListID, IN UINT uiRuleID)
{
    DOMAINACL_CTRL_S *pstDOMAINAcl = hDOMAINAcl;
    _DOMAINACL_RULE_DESC_S *pstRule;
    
    pstRule = (VOID*) ListRule_DelRule(pstDOMAINAcl->hDOMAINAclHandle, uiListID, uiRuleID);
    if (NULL == pstRule)
    {
        return;
    }
    DOMAINACL_DecPoolReferedNumber(&pstRule->stRule.stRuleCfg.stKey.pools);

    MemCap_Free(pstDOMAINAcl->memcap, pstRule);
}

DOMAINACL_RULE_S * DOMAINACL_GetRule(IN DOMAINACL_HANDLE hDOMAINAcl, IN UINT uiListID, UINT uiRuleID)
{
    DOMAINACL_CTRL_S *pstDOMAINAcl = hDOMAINAcl;
    _DOMAINACL_RULE_DESC_S *pstRule;
    

    pstRule = (VOID*) ListRule_GetRule(pstDOMAINAcl->hDOMAINAclHandle, uiListID, uiRuleID);
    if (NULL == pstRule)
    {
        return NULL;
    }

    return &pstRule->stRule;
}

UINT DOMAINACL_GetLastRuleID(IN DOMAINACL_HANDLE hDOMAINAcl, IN UINT uiListID)
{
    DOMAINACL_CTRL_S *pstDOMAINAcl = hDOMAINAcl;
    _DOMAINACL_RULE_DESC_S *pstRule;
    
    pstRule = (VOID*) ListRule_GetLastRule(pstDOMAINAcl->hDOMAINAclHandle, uiListID);
    if (NULL == pstRule)
    {
        return 0;
    }

    return pstRule->stRuleListNode.uiRuleID;
}

VOID DOMAINACL_UpdateRule(IN DOMAINACL_RULE_S *pstRule, IN DOMAINACL_RULE_CFG_S *pstRuleCfg)
{
    DOMAINACL_DecPoolReferedNumber(&pstRule->stRuleCfg.stKey.pools);
    pstRule->stRuleCfg = *pstRuleCfg;
    DOMAINACL_IncPoolReferedNumber(&pstRule->stRuleCfg.stKey.pools);
    return;
}

BS_STATUS DOMAINACL_MoveRule(IN DOMAINACL_HANDLE hDOMAINAcl, IN UINT uiListID, IN UINT uiOldRuleID, IN UINT uiNewRuleID)
{
    DOMAINACL_CTRL_S *pstDOMAINAcl = hDOMAINAcl;
    return ListRule_MoveRule(pstDOMAINAcl->hDOMAINAclHandle, uiListID, uiOldRuleID, uiNewRuleID);
}



BS_STATUS DOMAINACL_RebaseID(IN DOMAINACL_HANDLE hDOMAINAcl, IN UINT uiListID, IN UINT uiStep)
{
    DOMAINACL_CTRL_S *pstDOMAINAcl = hDOMAINAcl;
    return ListRule_ResetID(pstDOMAINAcl->hDOMAINAclHandle, uiListID, uiStep);
}


BS_STATUS  DOMAINACL_IncreaseID(IN DOMAINACL_HANDLE hDOMAINAcl, IN UINT uiListID, IN UINT uiStart, IN UINT uiEnd, IN UINT uiStep)
{
    DOMAINACL_CTRL_S *pstDOMAINAcl = hDOMAINAcl;
    return ListRule_IncreaseID(pstDOMAINAcl->hDOMAINAclHandle, uiListID, uiStart, uiEnd, uiStep);
}

UINT DOMAINACL_GetNextRuleID(IN DOMAINACL_HANDLE hDOMAINAcl, IN UINT uiListID, IN UINT uiCurrentRuleID)
{
    DOMAINACL_CTRL_S *pstDOMAINAcl = hDOMAINAcl;
    RULE_NODE_S *pstNode;

    pstNode = ListRule_GetNextRule(pstDOMAINAcl->hDOMAINAclHandle, uiListID, uiCurrentRuleID);
    if (NULL == pstNode)
    {
        return 0;
    }

    return pstNode->uiRuleID;
}

DOMAINACL_RULE_S * DOMAINACL_GetNextRule(IN DOMAINACL_HANDLE hDOMAINAcl, IN UINT uiListID, IN UINT uiCurrentRuleID)
{
    DOMAINACL_CTRL_S *pstDOMAINAcl = hDOMAINAcl;
    _DOMAINACL_RULE_DESC_S *pstRuleDesc;

    pstRuleDesc = (VOID*)ListRule_GetNextRule(pstDOMAINAcl->hDOMAINAclHandle, uiListID, uiCurrentRuleID);

    if (NULL == pstRuleDesc)
    {
        return NULL;
    }
    
    return &pstRuleDesc->stRule;
}

VOID DOMAINACL_ScanRule(IN DOMAINACL_HANDLE hDOMAINAcl, IN UINT uiListID, PF_DOMAINACL_RULE_SCAN pfFunc, IN VOID *pUserHandle)
{
    DOMAINACL_CTRL_S *pstDOMAINAcl = hDOMAINAcl;
    LIST_RULE_LIST_S* pstList;
    _DOMAINACL_SCAN_DESC_S stScanDesc = {0};

    stScanDesc.pfScan = pfFunc;
    stScanDesc.pUseData = pUserHandle;

    pstList = ListRule_GetListByID(pstDOMAINAcl->hDOMAINAclHandle, uiListID);
    if (NULL != pstList) {
        RuleList_ScanRule(&pstList->stRuleList, _domainacl_RuleSacnOuter, &stScanDesc);
    }

    return;
}

BS_ACTION_E DOMAINACL_Match(IN DOMAINACL_HANDLE hDOMAINAcl, IN UINT uiListID, IN DOMAINACL_MATCH_INFO_S *pstMatchInfo)
{
    DOMAINACL_CTRL_S *pstDOMAINAcl = hDOMAINAcl;
    LIST_RULE_LIST_S* pstList;
    _DOMAINACL_MATCH_DESC_S stMatchDesc;

    Mem_Zero(&stMatchDesc, sizeof(_DOMAINACL_MATCH_DESC_S));
    stMatchDesc.pstMatchInfo = pstMatchInfo;
    stMatchDesc.bIsMatch = BOOL_FALSE;
    stMatchDesc.enAction = BS_ACTION_UNDEF;

    pstList = ListRule_GetListByID(pstDOMAINAcl->hDOMAINAclHandle, uiListID);
    if (NULL != pstList) {
        
        RuleList_ScanRule(&pstList->stRuleList, _domainacl_MatchRuleScan, &stMatchDesc);

        if (!stMatchDesc.bIsMatch) {
            stMatchDesc.enAction = pstList->default_action;
        }
    }

    return stMatchDesc.enAction;
}

void DOMAINACL_IncPoolReferedNumber(INOUT DOMAINACL_POOL_KEY_S *pstPoolKeys)
{
    DBGASSERT(NULL != pstPoolKeys);

    if (pstPoolKeys->pstSipList) {
        ATOM_INC_FETCH(&pstPoolKeys->pstSipList->uiRefCount);
    }

    if (pstPoolKeys->pstDportList) {
        ATOM_INC_FETCH(&pstPoolKeys->pstDportList->uiRefCount);
    }

    if (pstPoolKeys->pstDomainGroup) {
        ATOM_INC_FETCH(&pstPoolKeys->pstDomainGroup->uiRefCount);
    }
}

void DOMAINACL_DecPoolReferedNumber(INOUT DOMAINACL_POOL_KEY_S *pstPoolKeys)
{
    DBGASSERT(NULL != pstPoolKeys);

    if (pstPoolKeys->pstSipList) {
        ATOM_DEC_FETCH(&pstPoolKeys->pstSipList->uiRefCount);
    }

    if (pstPoolKeys->pstDportList) {
        ATOM_DEC_FETCH(&pstPoolKeys->pstDportList->uiRefCount);
    }

    if (pstPoolKeys->pstDomainGroup) {
        ATOM_DEC_FETCH(&pstPoolKeys->pstDomainGroup->uiRefCount);
    }
}

void DOMAINACL_Reset(DOMAINACL_HANDLE hDOMAINAcl)
{
    DOMAINACL_CTRL_S *pstDOMAINAcl = hDOMAINAcl;
    ListRule_Reset(pstDOMAINAcl->hDOMAINAclHandle, _domainacl_DeleteRule, pstDOMAINAcl);
    return;
}
