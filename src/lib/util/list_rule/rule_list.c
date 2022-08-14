/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"

#include "utl/rule_list.h"

void RuleList_Init(RULE_LIST_S * rulelist)
{
    memset(rulelist, 0, sizeof(RULE_LIST_S));
    DLL_INIT(&rulelist->stRuleList);
}

void RuleList_Finit(RULE_LIST_S *pstList, PF_RULE_FREE pfFunc, IN VOID *pUserHandle)
{
    RULE_NODE_S *pstRule;
    RULE_NODE_S *pstRuleNext;

    DLL_SAFE_SCAN(&pstList->stRuleList, pstRule, pstRuleNext) {
        DLL_DEL(&pstList->stRuleList, pstRule);
        if (NULL != pfFunc) {
            pfFunc(pstRule, pUserHandle);
        }
    }
}

void RuleList_ScanRule(RULE_LIST_S *pstList, PF_RULE_SCAN pfFunc, IN VOID *pUserHandle)
{
    RULE_NODE_S *pstRule;
    BOOL_T bIsContinue;

    if (NULL != pfFunc)
    {
        DLL_SCAN(&pstList->stRuleList, pstRule) {
            bIsContinue = pfFunc(pstRule, pUserHandle);
            if (!bIsContinue)
            {
                break;
            }
        }
    }
    return;
}

void RuleList_Add(RULE_LIST_S * pstList, UINT uiRuleID, RULE_NODE_S *pstRule)
{
    DLL_HEAD_S *listhead;
    RULE_NODE_S *pstCur;
    RULE_NODE_S *pstLast;
    RULE_NODE_S *pstTemp = NULL;

    listhead = &pstList->stRuleList;
    pstRule->uiRuleID = uiRuleID;

    /* 添加到链表中, 需要按照rule id排序 */
    if (DLL_COUNT(listhead) == 0) {
        /* 没有rule,则加在队列头 */
        DLL_ADD(listhead, &(pstRule->stLinkNode));
        return;
    }

    /* 大于最大id插在最后 */
    pstLast = DLL_LAST(&pstList->stRuleList);
    if (NULL == pstLast || uiRuleID > pstLast->uiRuleID) {
        DLL_ADD(listhead, &(pstRule->stLinkNode));
        return;
    }

    /* 遍历链表 */
    DLL_SCAN(listhead, pstCur) {
        if (uiRuleID<= pstCur->uiRuleID) {
            pstTemp = pstCur;
            break;
        }
    }

    if (NULL != pstTemp) {
        /* 找到节点则加在节点前面 */
        DLL_INSERT_BEFORE(listhead, pstRule, pstTemp);
    } else {
        /* 没找到则加在节点最后面 */
        DLL_ADD(listhead, pstRule);
    }

    return ;
}

RULE_NODE_S * RuleList_Del(RULE_LIST_S *pstList, IN UINT uiRuleID)
{
    RULE_NODE_S *pstRule = NULL;

    DLL_SCAN(&pstList->stRuleList, pstRule) {
        if (pstRule->uiRuleID == uiRuleID) {
            break;
        }
    }

    if (NULL != pstRule) {
        DLL_DEL(&pstList->stRuleList, pstRule);
    }

    return pstRule;
}

void RuleList_ResetRuleID(RULE_LIST_S *pstList, IN UINT step)
{
    RULE_NODE_S *pstRule = NULL;
    UINT i = 0;

    if (step == 0) {
        step = 1;
    }

    DLL_SCAN(&pstList->stRuleList, pstRule) {
        pstRule->uiRuleID = i;
        i += step;
    }
}

RULE_NODE_S * RuleList_FindRule(RULE_LIST_S *pstList, PF_RULE_CMP pfCmp, RULE_NODE_S *pstToFind)
{
    RULE_NODE_S *pstRule = NULL;
    int ret;

    DLL_SCAN(&pstList->stRuleList, pstRule) {
        ret = pfCmp(pstRule, pstToFind);
        if (ret == 0) {
            return pstRule;
        }
    }

    return NULL;
}

RULE_NODE_S * RuleList_GetRule(RULE_LIST_S *pstList, IN UINT uiRuleID)
{
    RULE_NODE_S *pstRule;

    DLL_SCAN(&pstList->stRuleList, pstRule) {
        if (pstRule->uiRuleID == uiRuleID) {
            return pstRule;
        }

        if (pstRule->uiRuleID > uiRuleID) {
            break;
        }
    }

    return NULL;
}

RULE_NODE_S * RuleList_GetLastRule(RULE_LIST_S *pstList)
{
    return DLL_LAST(&pstList->stRuleList);
}

BS_STATUS RuleList_IncreaseID(RULE_LIST_S *pstList, IN UINT uiStart, IN UINT uiEnd, IN UINT uiStep)
{
    UINT uiRuleID;
    RULE_NODE_S *pstRule;
    RULE_NODE_S *pstTmp;

    DLL_SAFE_SCAN_REVERSE(&pstList->stRuleList, pstRule, pstTmp)
    {
        uiRuleID = pstRule->uiRuleID;
        if (uiRuleID >= uiStart && uiRuleID <= uiEnd)
        {
            pstRule->uiRuleID = uiRuleID + uiStep;
        }
        
        if (uiRuleID <= uiStart)
        {
            break;
        }
    }

    return BS_OK;
}

/* 移动rule */
BS_STATUS RuleList_MoveRule(RULE_LIST_S *pstList, UINT uiOldRuleID, UINT uiNewRuleID)
{
    RULE_NODE_S *pstNode;
    RULE_NODE_S* pstNodeTmp;
    RULE_NODE_S* pstOldNode;
    
    pstOldNode = RuleList_Del(pstList, uiOldRuleID);
    if (NULL == pstOldNode) {
        return BS_NOT_FOUND;
    }
    pstNode = DLL_LAST(&pstList->stRuleList);
    if (NULL == pstNode || pstNode->uiRuleID < uiNewRuleID) {
        pstOldNode->uiRuleID = uiNewRuleID;
        DLL_ADD(&pstList->stRuleList, &pstOldNode->stLinkNode);
        return BS_OK;
    }

    DLL_SAFE_SCAN(&pstList->stRuleList, pstNode, pstNodeTmp) {
        if (pstNode->uiRuleID > uiNewRuleID) {
            DLL_INSERT_BEFORE(&pstList->stRuleList, &pstOldNode->stLinkNode, &pstNode->stLinkNode);
            pstOldNode->uiRuleID = uiNewRuleID;
            break;
        }
    }
    
    return BS_OK;
}

RULE_NODE_S * RuleList_GetNextByNode(RULE_LIST_S *pstList, RULE_NODE_S *pstCurr/* NULL表示从头开始 */)
{
    if (pstCurr == NULL) {
        return DLL_FIRST(&pstList->stRuleList);
    }

    return DLL_NEXT(&pstList->stRuleList, pstCurr);
}

RULE_NODE_S * RuleList_GetNextByID(RULE_LIST_S *pstList, UINT uiCurrentRuleID /* INVALID表示从头开始 */)
{
    RULE_NODE_S *pstRule;

    if (uiCurrentRuleID == RULE_ID_INVALID) {
        return DLL_FIRST(&pstList->stRuleList);
    }

    DLL_SCAN(&pstList->stRuleList, pstRule) {
        if (pstRule->uiRuleID > uiCurrentRuleID) {
            return pstRule;
        }
    }

    return NULL;
}

UINT RuleList_Count(RULE_LIST_S *pstList)
{
    return DLL_COUNT(&pstList->stRuleList);
}

UINT RuleList_ResetID(RULE_LIST_S* pstList, UINT uiStep)
{
    RULE_NODE_S *pstNode;
    RULE_NODE_S* pstNodeTmp;

    DLL_SAFE_SCAN_REVERSE(&pstList->stRuleList, pstNode, pstNodeTmp) {
        pstNode->uiRuleID += uiStep;
    }

    return BS_OK;
}