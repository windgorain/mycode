/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/rule_list.h"
#include "utl/txt_utl.h"
#include "utl/pkt_policy.h"

static void pktpolicy_FreeNode(void *rule, void *pUserHandle)
{
    PKT_POLICY_NODE_S *pstNode = (void*)rule;

    MEM_Free(pstNode->policy_rule.action);
    MEM_Free(pstNode->policy_rule.host);
    MEM_Free(pstNode->policy_rule.protocol);
    MEM_Free(pstNode->policy_rule.dport);
    MEM_Free(pstNode);
}

static PKT_POLICY_NODE_S * pktpolicy_Rule2Node(PKT_POLICY_RULE_S *rule)
{
    PKT_POLICY_NODE_S *pstNode;

    pstNode = MEM_ZMalloc(sizeof(PKT_POLICY_NODE_S));
    if (NULL == pstNode) {
        return NULL;
    }

    pstNode->policy_rule.action = TXT_Strdup(rule->action);
    pstNode->policy_rule.host = TXT_Strdup(rule->host);
    pstNode->policy_rule.protocol = TXT_Strdup(rule->protocol);
    pstNode->policy_rule.dport = TXT_Strdup(rule->dport);

    return pstNode;
}

int PKTPolicy_Init(PKT_POLICY_S *pkt_policy)
{
    RuleList_Init(&pkt_policy->rule_list);
    return 0;
}

void PKTPolicy_Finit(PKT_POLICY_S *pkt_policy)
{
    RuleList_Finit(&pkt_policy->rule_list, pktpolicy_FreeNode, NULL);
}

int PKTPolicy_AddRule(PKT_POLICY_S *pkt_policy, UINT rule_id, PKT_POLICY_RULE_S *rule)
{
    PKT_POLICY_NODE_S *pstNode;

    if (RuleList_Count(&pkt_policy->rule_list) >= PKT_POLICY_RULE_MAX) {
        RETURN(BS_NO_RESOURCE);
    }

    pstNode = pktpolicy_Rule2Node(rule);
    if (NULL == pstNode) {
        RETURN(BS_NO_MEMORY);
    }

    RuleList_Add(&pkt_policy->rule_list, rule_id, &pstNode->rule_node);
    RuleList_ResetRuleID(&pkt_policy->rule_list, 1);

    return 0;
}

void PKTPolicy_DelRule(PKT_POLICY_S *pkt_policy, UINT rule_id)
{
    PKT_POLICY_NODE_S *pstNode;

    pstNode = (void*)RuleList_Del(&pkt_policy->rule_list, rule_id);
    if (pstNode) {
        pktpolicy_FreeNode(pstNode, NULL);
        RuleList_ResetRuleID(&pkt_policy->rule_list, 1);
    }
}

void PKTPolicy_DelRuleBatch(PKT_POLICY_S *pkt_policy, UINT *rule_ids, UINT count)
{
    PKT_POLICY_NODE_S *pstNode;
    int i;

    for (i=0; i<count; i++) {
        pstNode = (void*)RuleList_Del(&pkt_policy->rule_list, rule_ids[i]);
        if (pstNode) {
            pktpolicy_FreeNode(pstNode, NULL);
        }
    }

    RuleList_ResetRuleID(&pkt_policy->rule_list, 1);
}

PKT_POLICY_RULE_S * PKTPolicy_GetByID(PKT_POLICY_S *pkt_policy, UINT id)
{
    PKT_POLICY_NODE_S *pstNode;

    pstNode = (void*)RuleList_GetRule(&pkt_policy->rule_list, id);
    if (!pstNode) {
        return NULL;
    }

    return &pstNode->policy_rule;
}

