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
#include "utl/dpi_acl.h"
#include "utl/mem_cap.h"

typedef struct
{
    HANDLE hDPIAclHandle;
    void *memcap;
}DPIACL_CTRL_S;

typedef struct
{
    RULE_NODE_S stRuleListNode;
    DPIACL_RULE_S stRule;
}_DPIACL_RULE_DESC_S;

typedef struct
{
    DPIACL_MATCH_INFO_S *pstMatchInfo;
    BOOL_T bIsMatch;
    BS_ACTION_E enAction;
}_DPIACL_MATCH_DESC_S;

typedef struct
{
    PF_DPIACL_RULE_SCAN pfScan;
    void *pUseData;
}_DPIACL_SCAN_DESC_S;

STATIC BOOL_T _dpiacl_MatchByRule(IN DPIACL_MATCH_INFO_S *pstMatchInfo, IN _DPIACL_RULE_DESC_S *pstRule)
{
    DPIACL_RULE_CFG_S *pstRuleCfg = &pstRule->stRule.stRuleCfg;

    if(pstRuleCfg->uiKeyMask & DPIACL_KEY_SIP) {
        UINT uiMask = pstRuleCfg->stKey.single.stSIP.uiWildcard;
        if ((pstMatchInfo->uiSIP & uiMask) != (pstRuleCfg->stKey.single.stSIP.uiIP & uiMask))
        {
            return BOOL_FALSE;
        }
    }
    
    if(pstRuleCfg->uiKeyMask & DPIACL_KEY_POOL_SIP) {
        if (!AddressPool_MatchIpPool(pstRuleCfg->stKey.pools.pstSipPool,pstMatchInfo->uiSIP)){
            return BOOL_FALSE;
        }
    }

    if (pstRuleCfg->uiKeyMask & DPIACL_KEY_DPORT)
    {
        DPIACL_SINGLE_KEY_S *pstSingleKey = &pstRuleCfg->stKey.single;
        if ((pstMatchInfo->usDPort < pstSingleKey->stDPort.usBegin)
            || (pstMatchInfo->usDPort > pstSingleKey->stDPort.usEnd))
        {
            return BOOL_FALSE;
        }
    }

    if (pstRuleCfg->uiKeyMask & DPIACL_KEY_POOL_DPORT) {
        if (! PortPool_MatchPortPool(pstRuleCfg->stKey.pools.pstDportPool, pstMatchInfo->usDPort)) {
            return BOOL_FALSE;
        }
    }

    if (pstRuleCfg->uiKeyMask & DPIACL_KEY_DOMAIN) {
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

    if(pstRuleCfg->uiKeyMask & DPIACL_KEY_POOL_DOAMIN) {
        if(! DomainGroup_Match(pstRuleCfg->stKey.pools.pstDomainGroup, pstMatchInfo->szDomain)) {
            return BOOL_FALSE;
        }
    }

    ATOM_INC_FETCH(&pstRule->stRule.stStatistics.ulMatchCount);

    return BOOL_TRUE;
}

static BOOL_T _dpiacl_RuleSacnOuter(IN void *pstRule, IN VOID *pUserHandle)
{
    UINT uiRuleID;
    _DPIACL_SCAN_DESC_S *pScanDesc = pUserHandle;
    _DPIACL_RULE_DESC_S *pstRuleDesc = container_of(pstRule, _DPIACL_RULE_DESC_S, stRuleListNode);

    if (NULL != pScanDesc && NULL != pScanDesc->pfScan)
    {
        uiRuleID = Rule_GetIDByNode(&pstRuleDesc->stRuleListNode);
        return pScanDesc->pfScan(uiRuleID, &pstRuleDesc->stRule, pScanDesc->pUseData);
    }

    return BOOL_FALSE;
}

static BOOL_T _dpiacl_MatchRuleScan(IN void *pstRule, IN VOID *pUserHandle)
{
    BOOL_T bIsMatch = FALSE;
    UINT uiRuleID;
    _DPIACL_MATCH_DESC_S *pstMatch = pUserHandle;

    _DPIACL_RULE_DESC_S *pstRuleDesc = container_of(pstRule, _DPIACL_RULE_DESC_S, stRuleListNode);
    if (NULL != pstRuleDesc){
        uiRuleID = Rule_GetIDByNode(&pstRuleDesc->stRuleListNode);
        bIsMatch = _dpiacl_MatchByRule(pstMatch->pstMatchInfo, pstRuleDesc);
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

static VOID _dpiacl_DeleteRule(void *pstRule, void *pUserHandle)
{
    DPIACL_CTRL_S *pstDPIAcl = pUserHandle;

    _DPIACL_RULE_DESC_S *pstData = container_of(pstRule, _DPIACL_RULE_DESC_S, stRuleListNode);
    if (NULL != pstData){
        DPIACL_DecPoolReferedNumber(&pstData->stRule.stRuleCfg.stKey.pools);
        MemCap_Free(pstDPIAcl->memcap, pstData);
    }
}

DPIACL_HANDLE DPIACL_Create(void* memcap)
{
    DPIACL_CTRL_S *pstDPIAcl = NULL;

    pstDPIAcl = MemCap_ZMalloc(memcap, sizeof(DPIACL_CTRL_S));
    if (NULL == pstDPIAcl) {
        return NULL;
    }

    pstDPIAcl->hDPIAclHandle = ListRule_Create(memcap);
    if (NULL == pstDPIAcl->hDPIAclHandle) {
        MemCap_Free(memcap, pstDPIAcl);
        return NULL;
    }

    pstDPIAcl->memcap = memcap;

    return pstDPIAcl;
}

VOID DPIACL_Destroy(IN DPIACL_HANDLE hDPIAcl)
{
    DPIACL_CTRL_S *pstDPIAcl = hDPIAcl;
    void *memcap = pstDPIAcl->memcap;
    ListRule_Destroy(pstDPIAcl->hDPIAclHandle, _dpiacl_DeleteRule, pstDPIAcl);
    MemCap_Free(memcap, pstDPIAcl);
    return;
}

DPIACL_LIST_HANDLE DPIACL_CreateList(IN DPIACL_HANDLE hDPIAcl, IN CHAR *pcListName)
{
    DPIACL_CTRL_S *pstDPIAcl = hDPIAcl;
    return ListRule_CreateList(pstDPIAcl->hDPIAclHandle, pcListName, 0);
}

VOID DPIACL_DestroyList(IN DPIACL_HANDLE hDPIAcl, IN DPIACL_LIST_HANDLE hDPIAclList)
{
    DPIACL_CTRL_S *pstDPIAcl = hDPIAcl;
    ListRule_DestroyList(pstDPIAcl->hDPIAclHandle, hDPIAclList, _dpiacl_DeleteRule, pstDPIAcl);
    return;
}

UINT DPIACL_AddList(IN DPIACL_HANDLE hDPIAcl, IN CHAR *pcListName)
{
    DPIACL_CTRL_S *pstDPIAcl = hDPIAcl;
    return ListRule_AddList(pstDPIAcl->hDPIAclHandle, pcListName, 0);
}

VOID DPIACL_DelList(IN DPIACL_HANDLE hDPIAcl, IN UINT uiListID)
{
    DPIACL_CTRL_S *pstDPIAcl = hDPIAcl;
    ListRule_DelListID(pstDPIAcl->hDPIAclHandle, uiListID, _dpiacl_DeleteRule, pstDPIAcl);
    return;
}

BS_STATUS DPIACL_ReplaceList(IN DPIACL_HANDLE hDPIAcl, IN UINT uiListID, IN DPIACL_LIST_HANDLE hDPIAclListNew)
{
    DPIACL_CTRL_S *pstDPIAcl = hDPIAcl;
    return ListRule_ReplaceList(pstDPIAcl->hDPIAclHandle, uiListID, _dpiacl_DeleteRule, pstDPIAcl, hDPIAclListNew);
}

BS_STATUS DPIACL_AddListRef(IN DPIACL_HANDLE hDPIAcl, IN UINT uiListID)
{
    DPIACL_CTRL_S *pstDPIAcl = hDPIAcl;
    return ListRule_IncListRef(pstDPIAcl->hDPIAclHandle, uiListID);
}

BS_STATUS DPIACL_DelListRef(IN DPIACL_HANDLE hDPIAcl, IN UINT uiListID)
{
    DPIACL_CTRL_S *pstDPIAcl = hDPIAcl;
    return ListRule_DecListRef(pstDPIAcl->hDPIAclHandle, uiListID);
}

UINT DPIACL_ListGetRef(IN DPIACL_HANDLE hDPIAcl, IN UINT uiListID)
{
    DPIACL_CTRL_S *pstDPIAcl = hDPIAcl;
    return ListRule_GetListRef(pstDPIAcl->hDPIAclHandle, uiListID);
}

UINT DPIACL_GetListByName(IN DPIACL_HANDLE hDPIAcl, IN CHAR *pcListName)
{
    DPIACL_CTRL_S *pstDPIAcl = hDPIAcl;
    return ListRule_GetListIDByName(pstDPIAcl->hDPIAclHandle, pcListName);
}

UINT DPIACL_GetNextListID(IN DPIACL_HANDLE hDPIAcl, IN UINT uiCurrentListID/* 0表示获取第一个 */)
{
    DPIACL_CTRL_S *pstDPIAcl = hDPIAcl;
    return ListRule_GetNextListID(pstDPIAcl->hDPIAclHandle, uiCurrentListID);
}

CHAR * DPIACL_GetListNameByID(IN DPIACL_HANDLE hDPIAcl, IN UINT ulListID)
{
    DPIACL_CTRL_S *pstDPIAcl = hDPIAcl;
    return ListRule_GetListNameByID(pstDPIAcl->hDPIAclHandle, ulListID);
}

BS_ACTION_E DPIACL_GetDefaultActionByID(IN DPIACL_HANDLE hDPIAcl, IN UINT uiListID)
{
    DPIACL_CTRL_S *pstDPIAcl = hDPIAcl;
    return ListRule_GetDefaultActionByID(pstDPIAcl->hDPIAclHandle, uiListID);
}

BS_STATUS DPIACL_SetDefaultActionByID(IN DPIACL_HANDLE hDPIAcl, IN UINT uiListID, BS_ACTION_E enAction)
{
    DPIACL_CTRL_S *pstDPIAcl = hDPIAcl;
    return ListRule_SetDefaultActionByID(pstDPIAcl->hDPIAclHandle, uiListID, enAction);
}

BS_STATUS DPIACL_AddRule2List(IN DPIACL_HANDLE hDPIAcl, IN DPIACL_LIST_HANDLE hDPIAclList, 
                             IN UINT uiRuleID, IN DPIACL_RULE_CFG_S *pstRuleCfg)
{
    DPIACL_CTRL_S *pstDPIAcl = hDPIAcl;
    _DPIACL_RULE_DESC_S *pstRule;
    BS_STATUS eRet;

    pstRule = MemCap_ZMalloc(pstDPIAcl->memcap, sizeof(_DPIACL_RULE_DESC_S));
    if (NULL == pstRule)
    {
        return BS_NO_MEMORY;
    }
    pstRule->stRule.stRuleCfg = *pstRuleCfg;

    eRet = ListRule_AddRule2List(hDPIAclList, uiRuleID, &pstRule->stRuleListNode);
    if (BS_OK != eRet)
    {
        MemCap_Free(pstDPIAcl->memcap, pstRule);
        return eRet;
    }

    DPIACL_IncPoolReferedNumber(&pstRuleCfg->stKey.pools);
    return BS_OK;
}

BS_STATUS DPIACL_AddRule(IN DPIACL_HANDLE hDPIAcl, IN UINT uiListID, IN UINT uiRuleID, IN DPIACL_RULE_CFG_S *pstRuleCfg)
{
    DPIACL_CTRL_S *pstDPIAcl = hDPIAcl;
    _DPIACL_RULE_DESC_S *pstRule;
    BS_STATUS eRet;

    pstRule = MemCap_ZMalloc(pstDPIAcl->memcap, sizeof(_DPIACL_RULE_DESC_S));
    if (NULL == pstRule)
    {
        return BS_NO_MEMORY;
    }
    pstRule->stRule.stRuleCfg = *pstRuleCfg;

    eRet = ListRule_AddRule(pstDPIAcl->hDPIAclHandle, uiListID, uiRuleID, &pstRule->stRuleListNode);
    if (BS_OK != eRet)
    {
        MemCap_Free(pstDPIAcl->memcap, pstRule);
        return eRet;
    }

    DPIACL_IncPoolReferedNumber(&pstRuleCfg->stKey.pools);
    return BS_OK;
}

VOID DPIACL_DelRule(IN DPIACL_HANDLE hDPIAcl, IN UINT uiListID, IN UINT uiRuleID)
{
    DPIACL_CTRL_S *pstDPIAcl = hDPIAcl;
    _DPIACL_RULE_DESC_S *pstRule;
    
    pstRule = (VOID*) ListRule_DelRule(pstDPIAcl->hDPIAclHandle, uiListID, uiRuleID);
    if (NULL == pstRule)
    {
        return;
    }
    DPIACL_DecPoolReferedNumber(&pstRule->stRule.stRuleCfg.stKey.pools);

    MemCap_Free(pstDPIAcl->memcap, pstRule);
}

DPIACL_RULE_S * DPIACL_GetRule(IN DPIACL_HANDLE hDPIAcl, IN UINT uiListID, UINT uiRuleID)
{
    DPIACL_CTRL_S *pstDPIAcl = hDPIAcl;
    _DPIACL_RULE_DESC_S *pstRule;
    

    pstRule = (VOID*) ListRule_GetRule(pstDPIAcl->hDPIAclHandle, uiListID, uiRuleID);
    if (NULL == pstRule)
    {
        return NULL;
    }

    return &pstRule->stRule;
}

UINT DPIACL_GetLastRuleID(IN DPIACL_HANDLE hDPIAcl, IN UINT uiListID)
{
    DPIACL_CTRL_S *pstDPIAcl = hDPIAcl;
    _DPIACL_RULE_DESC_S *pstRule;
    
    pstRule = (VOID*) ListRule_GetLastRule(pstDPIAcl->hDPIAclHandle, uiListID);
    if (NULL == pstRule)
    {
        return 0;
    }

    return pstRule->stRuleListNode.uiRuleID;
}

VOID DPIACL_UpdateRule(IN DPIACL_RULE_S *pstRule, IN DPIACL_RULE_CFG_S *pstRuleCfg)
{
    //DPIACL_DecPoolReferedNumber(&pstRule->stRuleCfg.stKey.pools);
    pstRule->stRuleCfg = *pstRuleCfg;
    //DPIACL_IncPoolReferedNumber(&pstRule->stRuleCfg.stKey.pools);
    return;
}

BS_STATUS DPIACL_MoveRule(IN DPIACL_HANDLE hDPIAcl, IN UINT uiListID, IN UINT uiOldRuleID, IN UINT uiNewRuleID)
{
    DPIACL_CTRL_S *pstDPIAcl = hDPIAcl;
    return ListRule_MoveRule(pstDPIAcl->hDPIAclHandle, uiListID, uiOldRuleID, uiNewRuleID);
}


/*每个规则的ID都递增step，另一种方案是每两条规则的ID相差step，暂时未实现*/
BS_STATUS DPIACL_RebaseID(IN DPIACL_HANDLE hDPIAcl, IN UINT uiListID, IN UINT uiStep)
{
    DPIACL_CTRL_S *pstDPIAcl = hDPIAcl;
    return ListRule_ResetID(pstDPIAcl->hDPIAclHandle, uiListID, uiStep);
}


BS_STATUS  DPIACL_IncreaseID(IN DPIACL_HANDLE hDPIAcl, IN UINT uiListID, IN UINT uiStart, IN UINT uiEnd, IN UINT uiStep)
{
    DPIACL_CTRL_S *pstDPIAcl = hDPIAcl;
    return ListRule_IncreaseID(pstDPIAcl->hDPIAclHandle, uiListID, uiStart, uiEnd, uiStep);
}

UINT DPIACL_GetNextRuleID(IN DPIACL_HANDLE hDPIAcl, IN UINT uiListID, IN UINT uiCurrentRuleID)
{
    DPIACL_CTRL_S *pstDPIAcl = hDPIAcl;
    RULE_NODE_S *pstNode;

    pstNode = ListRule_GetNextRule(pstDPIAcl->hDPIAclHandle, uiListID, uiCurrentRuleID);
    if (NULL == pstNode)
    {
        return 0;
    }

    return pstNode->uiRuleID;
}

DPIACL_RULE_S * DPIACL_GetNextRule(IN DPIACL_HANDLE hDPIAcl, IN UINT uiListID, IN UINT uiCurrentRuleID)
{
    DPIACL_CTRL_S *pstDPIAcl = hDPIAcl;
    _DPIACL_RULE_DESC_S *pstRuleDesc;

    pstRuleDesc = (VOID*)ListRule_GetNextRule(pstDPIAcl->hDPIAclHandle, uiListID, uiCurrentRuleID);

    if (NULL == pstRuleDesc)
    {
        return NULL;
    }
    
    return &pstRuleDesc->stRule;
}

VOID DPIACL_ScanRule(IN DPIACL_HANDLE hDPIAcl, IN UINT uiListID, PF_DPIACL_RULE_SCAN pfFunc, IN VOID *pUserHandle)
{
    DPIACL_CTRL_S *pstDPIAcl = hDPIAcl;
    LIST_RULE_LIST_S* pstList;
    _DPIACL_SCAN_DESC_S stScanDesc = {0};

    stScanDesc.pfScan = pfFunc;
    stScanDesc.pUseData = pUserHandle;

    pstList = ListRule_GetListByID(pstDPIAcl->hDPIAclHandle, uiListID);
    if (NULL != pstList) {
        RuleList_ScanRule(&pstList->stRuleList, _dpiacl_RuleSacnOuter, &stScanDesc);
    }

    return;
}

BS_ACTION_E DPIACL_Match(IN DPIACL_HANDLE hDPIAcl, IN UINT uiListID, IN DPIACL_MATCH_INFO_S *pstMatchInfo)
{
    DPIACL_CTRL_S *pstDPIAcl = hDPIAcl;
    LIST_RULE_LIST_S* pstList;
    _DPIACL_MATCH_DESC_S stMatchDesc;

    Mem_Zero(&stMatchDesc, sizeof(_DPIACL_MATCH_DESC_S));
    stMatchDesc.pstMatchInfo = pstMatchInfo;
    stMatchDesc.bIsMatch = BOOL_FALSE;
    stMatchDesc.enAction = BS_ACTION_UNDEF;

    pstList = ListRule_GetListByID(pstDPIAcl->hDPIAclHandle, uiListID);
    if (NULL != pstList) {
        /* 按rule匹配 */
        RuleList_ScanRule(&pstList->stRuleList, _dpiacl_MatchRuleScan, &stMatchDesc);

        if (!stMatchDesc.bIsMatch) {
            stMatchDesc.enAction = pstList->default_action;
        }
    }

    return stMatchDesc.enAction;
}

void DPIACL_IncPoolReferedNumber(INOUT DPIACL_POOL_KEY_S *pstPoolKeys){
    DBGASSERT(NULL != pstPoolKeys);
    if (NULL != pstPoolKeys->pstSipPool){
        ATOM_INC_FETCH(&pstPoolKeys->pstSipPool->uiRefCount);
    }
    if (NULL != pstPoolKeys->pstDportPool){
        ATOM_INC_FETCH(&pstPoolKeys->pstDportPool->uiRefCount);
    }
    if (NULL != pstPoolKeys->pstDomainGroup){
        ATOM_INC_FETCH(&pstPoolKeys->pstDomainGroup->uiRefCount);
    }
    return;
}

void DPIACL_DecPoolReferedNumber(INOUT DPIACL_POOL_KEY_S *pstPoolKeys){
    DBGASSERT(NULL != pstPoolKeys);
    if (NULL != pstPoolKeys->pstSipPool){
        ATOM_DEC_FETCH(&pstPoolKeys->pstSipPool->uiRefCount);
    }
    if (NULL != pstPoolKeys->pstDportPool){
        ATOM_DEC_FETCH(&pstPoolKeys->pstDportPool->uiRefCount);
    }
    if (NULL != pstPoolKeys->pstDomainGroup){
        ATOM_DEC_FETCH(&pstPoolKeys->pstDomainGroup->uiRefCount);
    }
    return;
}

void DPIACL_Reset(DPIACL_HANDLE hDPIAcl)
{
    DPIACL_CTRL_S *pstDPIAcl = hDPIAcl;
    ListRule_Reset(pstDPIAcl->hDPIAclHandle, _dpiacl_DeleteRule, pstDPIAcl);
    return;
}
