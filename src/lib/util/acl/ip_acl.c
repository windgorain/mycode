/******************************************************************************
* Copyright (C), 2000-2006,  LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2016-10-14
* Description: IP ACL
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/list_rule.h"
#include "utl/ip_acl.h"

typedef struct
{
    RULE_NODE_S stRuleListNode;

    IPACL_RULE_S stRule;
}_IPACL_RULE_DESC_S;

STATIC BOOL_T _ipacl_MatchByRule(IN IPACL_MATCH_INFO_S *pstMatchInfo, IN _IPACL_RULE_DESC_S *pstRule)
{
    IPACL_RULE_CFG_S *pstRuleCfg = &pstRule->stRule.stRuleCfg;
    UINT uiMask;

    if (pstRuleCfg->bEnable == FALSE)
    {
        return FALSE;
    }
    
    if ((pstMatchInfo->uiKeyMask & IPACL_KEY_SIP) && (pstRuleCfg->uiKeyMask & IPACL_KEY_SIP))
    {
        uiMask = ~(pstRuleCfg->stSIP.uiWildcard);
        if ((pstMatchInfo->uiSIP & uiMask) != (pstRuleCfg->stSIP.uiIP & uiMask))
        {
            return FALSE;
        }
    }

    if ((pstMatchInfo->uiKeyMask & IPACL_KEY_DIP) && (pstRuleCfg->uiKeyMask & IPACL_KEY_DIP))
    {
        uiMask = ~(pstRuleCfg->stDIP.uiWildcard);
        if ((pstMatchInfo->uiDIP & uiMask) != (pstRuleCfg->stDIP.uiIP & uiMask))
        {
            return FALSE;
        }
    }

    if ((pstMatchInfo->uiKeyMask & IPACL_KEY_SPORT) && (pstRuleCfg->uiKeyMask & IPACL_KEY_SPORT))
    {
        if ((pstMatchInfo->usSPort < pstRuleCfg->stSPort.usBegin)
            || (pstMatchInfo->usSPort > pstRuleCfg->stSPort.usEnd))
        {
            return FALSE;
        }
    }

    if ((pstMatchInfo->uiKeyMask & IPACL_KEY_DPORT) && (pstRuleCfg->uiKeyMask & IPACL_KEY_DPORT))
    {
        if ((pstMatchInfo->usDPort < pstRuleCfg->stDPort.usBegin)
            || (pstMatchInfo->usDPort > pstRuleCfg->stDPort.usEnd))
        {
            return FALSE;
        }
    }

    return TRUE;
}

static VOID _ipacl_DeleteRule(void *pstRule, void *pUserHandle)
{
    _IPACL_RULE_DESC_S *pstData = container_of(pstRule, _IPACL_RULE_DESC_S, stRuleListNode);

    MEM_Free(pstData);
}

IPACL_HANDLE IPACL_Create()
{
    return ListRule_Create();
}

VOID IPACL_Destroy(IN IPACL_HANDLE hIpAcl)
{
    ListRule_Destroy(hIpAcl, _ipacl_DeleteRule, NULL);

    return;
}

UINT IPACL_AddList(IN IPACL_HANDLE hIpAcl, IN CHAR *pcListName)
{
    return ListRule_AddList(hIpAcl, pcListName);
}

VOID IPACL_DelList(IN IPACL_HANDLE hIpAcl, IN UINT uiListID)
{
    ListRule_DelList(hIpAcl, uiListID, _ipacl_DeleteRule, NULL);
}

BS_STATUS IPACL_AddListRef(IN IPACL_HANDLE hIpAcl, IN UINT uiListID)
{
    return ListRule_AddListRef(hIpAcl, uiListID);
}

BS_STATUS IPACL_DelListRef(IN IPACL_HANDLE hIpAcl, IN UINT uiListID)
{
    return ListRule_DelListRef(hIpAcl, uiListID);
}

UINT IPACL_ListGetRef(IN IPACL_HANDLE hIpAcl, IN UINT uiListID)
{
    return ListRule_GetListRef(hIpAcl, uiListID);
}

UINT IPACL_GetListByName(IN IPACL_HANDLE hIpAcl, IN CHAR *pcListName)
{
    return ListRule_FindListByName(hIpAcl, pcListName);
}

UINT IPACL_GetNextList(IN IPACL_HANDLE hIpAcl, IN UINT uiCurrentListID/* 0表示获取第一个 */)
{
    return ListRule_GetNextList(hIpAcl, uiCurrentListID);
}

CHAR * IPACL_GetListNameByID(IN IPACL_HANDLE hIpAcl, IN UINT ulListID)
{
    return ListRule_GetListNameByID(hIpAcl, ulListID);
}

BS_STATUS IPACL_AddRule
(
    IN IPACL_HANDLE hIpAcl,
    IN UINT uiListID,
    IN UINT uiRuleID,
    IN IPACL_RULE_CFG_S *pstRuleCfg
)
{
    _IPACL_RULE_DESC_S *pstRule;
    BS_STATUS eRet;

    pstRule = MEM_ZMalloc(sizeof(_IPACL_RULE_DESC_S));
    if (NULL == pstRule)
    {
        return BS_NO_MEMORY;
    }

    pstRule->stRule.stRuleCfg = *pstRuleCfg;

    eRet = ListRule_AddRule(hIpAcl, uiListID, uiRuleID, &pstRule->stRuleListNode);
    if (BS_OK != eRet)
    {
        MEM_Free(pstRule);
        return eRet;
    }

    return BS_OK;
}

VOID IPACL_DelRule(IN IPACL_HANDLE hIpAcl, IN UINT uiListID, IN UINT uiRuleID)
{
    _IPACL_RULE_DESC_S *pstRule;
    
    pstRule = (VOID*) ListRule_DelRule(hIpAcl, uiListID, uiRuleID);
    if (NULL == pstRule)
    {
        return;
    }

    MEM_Free(pstRule);
}

IPACL_RULE_S * IPACL_GetRule(IN IPACL_HANDLE hIpAcl, IN UINT uiListID, IN UINT uiRuleID)
{
    _IPACL_RULE_DESC_S *pstRule;
    
    pstRule = (VOID*) ListRule_GetRule(hIpAcl, uiListID, uiRuleID);
    if (NULL == pstRule)
    {
        return NULL;
    }

    return &pstRule->stRule;
}

BS_STATUS IPACL_MoveRule(IN IPACL_HANDLE hIpAcl, IN UINT uiListID, IN UINT uiOldRuleID, IN UINT uiNewRuleID)
{
    return ListRule_MoveRule(hIpAcl, uiListID, uiOldRuleID, uiNewRuleID);
}

UINT IPACL_GetNextRuleID(IN IPACL_HANDLE hIpAcl, IN UINT uiListID, IN UINT uiCurrentRuleID)
{
    RULE_NODE_S *pstNode;

    pstNode = ListRule_GetNextRule(hIpAcl, uiListID, uiCurrentRuleID);
    if (NULL == pstNode)
    {
        return 0;
    }

    return pstNode->uiRuleID;
}

IPACL_RULE_S * IPACL_GetNextRule(IN IPACL_HANDLE hIpAcl, IN UINT uiListID, IN UINT uiCurrentRuleID)
{
    _IPACL_RULE_DESC_S *pstRuleDesc;

    pstRuleDesc = (VOID*)ListRule_GetNextRule(hIpAcl, uiListID, uiCurrentRuleID);

    if (NULL == pstRuleDesc)
    {
        return NULL;
    }
    
    return &pstRuleDesc->stRule;
}

BS_ACTION_E IPACL_Match
(
    IN IPACL_HANDLE hIpAcl,
    IN UINT uiListID,
    IN IPACL_MATCH_INFO_S *pstMatchInfo
)
{
    _IPACL_RULE_DESC_S *pstRuleDesc;
    BOOL_T bIsMatch = BOOL_TRUE;
    UINT uiRuleID = RULE_ID_INVALID;
    BS_ACTION_E eAction = BS_ACTION_MAX;

    while ((pstRuleDesc = (VOID*)ListRule_GetNextRule(hIpAcl, uiListID, uiRuleID)) != NULL)
    {
        uiRuleID = Rule_GetIDByNode(&pstRuleDesc->stRuleListNode);

        /* 按rule匹配 */
        bIsMatch = _ipacl_MatchByRule(pstMatchInfo, pstRuleDesc);
        if (BOOL_TRUE == bIsMatch)
        {
            eAction = pstRuleDesc->stRule.stRuleCfg.enAction;
            break;
        }   
    }

    return eAction;
}

