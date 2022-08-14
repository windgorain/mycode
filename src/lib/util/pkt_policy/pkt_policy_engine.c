/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/net.h"
#include "utl/list_stq.h"
#include "utl/num_list.h"
#include "utl/ip_list.h"
#include "utl/ip_protocol.h"
#include "utl/rule_list.h"
#include "utl/dnsname_trie.h"
#include "utl/pkt_policy.h"

/* 一个维度命中的rule */
typedef struct {
    STQ_NODE_S stLinkNode;
    UINT rule_id;
}PKT_POLICY_DIM_NODE_S;

typedef struct {
    IPMASKTBL_NODE_S node;
    STQ_HEAD_S rule_id_list;
}PKT_POLICY_IP_DIM_NODE_S;


static PKT_POLICY_DIM_NODE_S * pktpolicy_engine_FindInList(STQ_HEAD_S *list, UINT rule_id)
{
    STQ_NODE_S *pNode;
    PKT_POLICY_DIM_NODE_S *pstNode;

    STQ_FOREACH(list, pNode) {
        pstNode = (void*)pNode;
        if (rule_id == pstNode->rule_id) {
            return pstNode;
        }
    }

    return NULL;
}

static int pktpolicy_engine_Insert2List(STQ_HEAD_S *list, UINT rule_id)
{
    PKT_POLICY_DIM_NODE_S *node;

    node = MEM_ZMalloc(sizeof(PKT_POLICY_DIM_NODE_S));
    if (node == NULL) {
        return -1;
    }
    node->rule_id = rule_id;

    STQ_AddTail(list, &node->stLinkNode);

    return 0;
}

static void pktpolicy_engine_MergeList(STQ_HEAD_S *to, STQ_HEAD_S *from)
{
    STQ_NODE_S *pNode;
    PKT_POLICY_DIM_NODE_S *pstNode;

    STQ_FOREACH(from, pNode) {
        pstNode = (void*)pNode;
        if (NULL == pktpolicy_engine_FindInList(to, pstNode->rule_id)) {
            pktpolicy_engine_Insert2List(to, pstNode->rule_id);
        }
    }
}

static int pktpolicy_engine_AddProtocolDimNode(PKT_POLICY_PROTOCOL_DIM_S *protocol_dim, UINT rule_id, UCHAR protocol)
{
    PKT_POLICY_DIM_NODE_S *node;

    node = MEM_ZMalloc(sizeof(PKT_POLICY_DIM_NODE_S));
    if (node == NULL) {
        RETURN(BS_NO_MEMORY);
    }

    node->rule_id = rule_id;

    STQ_AddTail(&protocol_dim->protocol_dim[protocol], &node->stLinkNode);

    return 0;
}

static int pktpolicy_engine_AddProcotolList(PKT_POLICY_PROTOCOL_DIM_S *protocol_dim, char *protocol, UINT rule_id)
{
    NUM_LIST_S num_list;
    LSTR_S lstr;
    int i;
    BS_STATUS eRet = 0;
    char tmp[1024];

    if (strlen(protocol) >= sizeof(tmp)) {
        RETURN(BS_BAD_PARA);
    }

    strcpy(tmp, protocol);
    IPProtocol_NameList2Protocols(tmp);

    lstr.pcData = tmp;
    lstr.uiLen = strlen(tmp);

    NumList_Init(&num_list);
    NumList_ParseLstr(&num_list, &lstr);

    for (i=0; i<=255; i++) {
        if (NumList_IsNumInTheList(&num_list, i)) {
            if (0 != pktpolicy_engine_AddProtocolDimNode(protocol_dim, rule_id, i)) {
                eRet = BS_ERR;
                break;
            }
        }
    }

    NumList_Finit(&num_list);

    return eRet;
}

static int pktpolicy_engine_AddPortDimNode(PKT_POLICY_PORT_DIM_S *port_dim, UINT rule_id, USHORT port)
{
    PKT_POLICY_DIM_NODE_S *node;

    node = MEM_ZMalloc(sizeof(PKT_POLICY_DIM_NODE_S));
    if (node == NULL) {
        RETURN(BS_NO_MEMORY);
    }

    node->rule_id = rule_id;
    STQ_AddTail(&port_dim->port_dim[port], &node->stLinkNode);

    return 0;
}

/* 将端口维度添加到端口表 */
static int pktpolicy_engine_AddPortList(PKT_POLICY_PORT_DIM_S *port_dim, char *port, UINT rule_id)
{
    NUM_LIST_S num_list;
    LSTR_S lstr;
    int i;
    BS_STATUS eRet = 0;

    lstr.pcData = port;
    lstr.uiLen = strlen(port);

    NumList_Init(&num_list);
    NumList_ParseLstr(&num_list, &lstr);

    for (i=0; i<=65535; i++) {
        if (NumList_IsNumInTheList(&num_list, i)) {
            if (0 != pktpolicy_engine_AddPortDimNode(port_dim, rule_id, i)) {
                eRet = BS_ERR;
                break;
            }
        }
    }

    NumList_Finit(&num_list);

    return eRet;
}

static void pktpolicy_engine_MergeIPTblUserData(IPMASKTBL_NODE_S *node_to, IPMASKTBL_NODE_S *node_from)
{
    PKT_POLICY_IP_DIM_NODE_S *node1 = (void*)node_to;
    PKT_POLICY_IP_DIM_NODE_S *node2 = (void*)node_from;

    pktpolicy_engine_MergeList(&node1->rule_id_list, &node2->rule_id_list);
}

static int pktpolicy_engine_AddIPDimNode(PKT_POLICY_IP_DIM_S *ip_dim, UINT rule_id, UINT ip, UCHAR depth)
{
    PKT_POLICY_DIM_NODE_S *node;
    PKT_POLICY_IP_DIM_NODE_S *pstIpDimNode;

    node = MEM_ZMalloc(sizeof(PKT_POLICY_DIM_NODE_S));
    if (node == NULL) {
        RETURN(BS_NO_MEMORY);
    }
    node->rule_id = rule_id;

    pstIpDimNode = (void*)IPMASKTBL_Get(&ip_dim->ipmasktbl, ip, depth);
    if (NULL == pstIpDimNode) {
        pstIpDimNode = MEM_ZMalloc(sizeof(PKT_POLICY_IP_DIM_NODE_S));
        if (NULL == pstIpDimNode) {
            MEM_Free(node);
            RETURN(BS_NO_MEMORY);
        }
        pstIpDimNode->node.ip = ip;
        pstIpDimNode->node.depth = depth;
        IPMASKTBL_Add(&ip_dim->ipmasktbl, &pstIpDimNode->node);
        IPMASKTBL_BfSet(&ip_dim->bf, ip, depth);
    }

    STQ_AddTail(&pstIpDimNode->rule_id_list, &node->stLinkNode);

    return 0;
}

/* 将地址维度添加到地址表 */
static int pktpolicy_engine_AddIPList(PKT_POLICY_IP_DIM_S *ip_dim, char *ip, UINT rule_id)
{
    IPLIST_S ip_list;
    LSTR_S lstr;
    UINT ip_begin;
    UINT ip_end;
    UINT mask;
    UCHAR prefix;
    BS_STATUS eRet = BS_OK;

    lstr.pcData = ip;
    lstr.uiLen = strlen(ip);

    IPList_Init(&ip_list);
    IPList_ParseString(&ip_list, &lstr);

    IPLIST_SCAN_BEGIN(&ip_list, ip_begin, ip_end) {
        mask = IP_GetMiniMask(ip_begin, ip_end);
        prefix = MASK_2_PREFIX(mask);
        if (0 != pktpolicy_engine_AddIPDimNode(ip_dim, rule_id, htonl(ip_begin), prefix)) {
            eRet = BS_ERR;
            break;
        }
    }IPLIST_SCAN_END();

    IPList_Finit(&ip_list);

    return eRet;
}

static void* pktpolicy_engine_HostnameTrieSetUserData(void *user_data, void *user_handle)
{
    PKT_POLICY_DIM_NODE_S *node;
    USER_HANDLE_S *pstUserHandle = user_handle;
    UINT rule_id = HANDLE_UINT(pstUserHandle->ahUserHandle[0]);

    node = MEM_ZMalloc(sizeof(PKT_POLICY_DIM_NODE_S));
    if (node == NULL) {
        pstUserHandle->ahUserHandle[1] = UINT_HANDLE(BS_NO_MEMORY);
        return user_data;
    }
    node->rule_id = rule_id;

    if (user_data == NULL) {
        user_data = MEM_ZMalloc(sizeof(STQ_HEAD_S));
        if (user_data == NULL) {
            MEM_Free(node);
            pstUserHandle->ahUserHandle[1] = UINT_HANDLE(BS_NO_MEMORY);
            return NULL;
        }
    }

    STQ_AddTail(user_data, &node->stLinkNode);

    return user_data;
}

static int pktpolicy_engine_AddHostnameTrie(TRIE_HANDLE trie, char *hostname, UINT rule_id)
{
    USER_HANDLE_S stUserHandle;

    stUserHandle.ahUserHandle[0] = UINT_HANDLE(rule_id);
    stUserHandle.ahUserHandle[1] = 0;

    if (0 != DnsNameTrie_InsertExt(trie, hostname, strlen(hostname),
                pktpolicy_engine_HostnameTrieSetUserData, &stUserHandle)) {
        RETURN(BS_ERR);
    }

    if (stUserHandle.ahUserHandle[1]) {
        RETURN(BS_NO_MEMORY);
    }

    return 0;
}

static int pktpolicy_ParseAction(char *action)
{
    if (action[0] == 'p') {
        return PKT_POLICY_ACTION_PERMIT;
    } else if (action[0] == 'd') {
        return PKT_POLICY_ACTION_PERMIT;
    } else if (action[0] == 'b') {
        return PKT_POLICY_ACTION_BYPASS;
    }

    return -1;
}

static STQ_HEAD_S* pktpolicy_engine_MatchProtocol(PKT_POLICY_PROTOCOL_DIM_S *protocol_dim, UCHAR protocol)
{
    return &protocol_dim->protocol_dim[protocol];
}

static STQ_HEAD_S* pktpolicy_engine_MatchPort(PKT_POLICY_PORT_DIM_S *port_dim, USHORT port/*netorder*/)
{
    port = ntohs(port);
    return &port_dim->port_dim[port];
}

static STQ_HEAD_S* pktpolicy_engine_MatchIP(PKT_POLICY_IP_DIM_S *ip_dim, UINT ip/*netorder*/)
{
    PKT_POLICY_IP_DIM_NODE_S *pstNode;

    pstNode = (void*)IPMASKTBL_BfMatch(&ip_dim->ipmasktbl, &ip_dim->bf, ip);
    if (NULL == pstNode) {
        return NULL;
    }

    return &pstNode->rule_id_list;
}

static STQ_HEAD_S* pktpolicy_engine_MatchHost(TRIE_HANDLE trie, LSTR_S *host)
{
    return DnsNameTrie_Match(trie, host->pcData, host->uiLen, TRIE_MATCH_WILDCARD);
}

typedef struct {
    STQ_NODE_S line_node;
    UINT rule_id;
    UINT mask;
}PKT_POLICY_MATCHED_NODE_S;

typedef struct {
    PKT_POLICY_MATCHED_NODE_S nodes[PKT_POLICY_RULE_MAX];
    int count;
    UINT masked;
    UINT min_rule_id_matched; /* 已经被完全匹配的最小rule id */
}PKT_POLICY_MATCHED_S;

static void pktpolicy_engine_MergeMatched(PKT_POLICY_ENGINE_S *engine, STQ_HEAD_S *list, UINT mask, OUT PKT_POLICY_MATCHED_S *matched)
{
    STQ_NODE_S *pNode;
    PKT_POLICY_DIM_NODE_S *pstNode;
    int i;

    STQ_FOREACH(list, pNode) {
        pstNode = (void*)pNode;

        if (pstNode->rule_id >= matched->min_rule_id_matched) {
            continue;
        }

        for (i=0; i<matched->count; i++) {
            /* 如果找到了,则增加对应的mask位 */
            if (pstNode->rule_id == matched->nodes[i].rule_id) {
                matched->nodes[i].mask |= mask;
                if (engine->rule_desc[pstNode->rule_id].rule_dim_mask == matched->nodes[i].mask) {
                    matched->min_rule_id_matched = i;
                }
                continue;
            }
        }
        /* 如果没有找到,则如果rule有masked的维度,则对应的这些维度是没有匹配的,不需要加入matched */
        if (engine->rule_desc[pstNode->rule_id].rule_dim_mask & matched->masked) {
            continue;
        }

        matched->nodes[matched->count].rule_id = pstNode->rule_id;
        matched->nodes[matched->count].mask = mask;
        matched->count ++;

        if (engine->rule_desc[pstNode->rule_id].rule_dim_mask == mask) {
            matched->min_rule_id_matched = pstNode->rule_id;
        }
    }

    matched->masked |= mask;
    for (i=0; i<matched->count; i++) {
        if ((matched->nodes[i].rule_id > matched->min_rule_id_matched)
                || ((engine->rule_desc[matched->nodes[i].rule_id].rule_dim_mask & matched->masked) != matched->nodes[i].mask)) {
            matched->nodes[i] = matched->nodes[matched->count - 1];
            matched->count --;
            i--;
        }
    }
}

int PKTPolicy_EngineInit(PKT_POLICY_ENGINE_S *engine)
{
    memset(engine, 0, sizeof(PKT_POLICY_ENGINE_S));

    IPMASKTBL_Init(&engine->sip_dim.ipmasktbl);
    IPMASKTBL_Init(&engine->dip_dim.ipmasktbl);
    IPMASKTBL_BfInit(&engine->sip_dim.bf);
    IPMASKTBL_BfInit(&engine->dip_dim.bf);
    engine->trie = Trie_Create(TRIE_TYPE_4BITS);
    if (! engine->trie) {
        RETURN(BS_NO_MEMORY);
    }

    return 0;
}

void PKTPolicy_EngineSetDefaultAction(PKT_POLICY_ENGINE_S *engine, int action)
{
    engine->default_action = action;
}

int PKTPolicy_EngineCompile(PKT_POLICY_ENGINE_S *engine, PKT_POLICY_S *pkt_policy)
{
    UINT rule_id;
    PKT_POLICY_NODE_S *pstRuleNode = NULL;

    while (NULL != (pstRuleNode = (void*)RuleList_GetNextByNode(&pkt_policy->rule_list, (void*)pstRuleNode))) {
        rule_id = pstRuleNode->rule_node.uiRuleID;
        engine->rule_desc[rule_id].rule_dim_mask = pstRuleNode->dim_mask;
        engine->rule_desc[rule_id].action = pktpolicy_ParseAction(pstRuleNode->policy_rule.action);
        pktpolicy_engine_AddProcotolList(&engine->protocol_dim, pstRuleNode->policy_rule.protocol, rule_id);
        pktpolicy_engine_AddPortList(&engine->sport_dim, pstRuleNode->policy_rule.sport, rule_id);
        pktpolicy_engine_AddPortList(&engine->dport_dim, pstRuleNode->policy_rule.dport, rule_id);
        pktpolicy_engine_AddIPList(&engine->sip_dim, pstRuleNode->policy_rule.sip, rule_id);
        pktpolicy_engine_AddIPList(&engine->dip_dim, pstRuleNode->policy_rule.dip, rule_id);
        pktpolicy_engine_AddHostnameTrie(engine->trie, pstRuleNode->policy_rule.host, rule_id);
    }

    IPMASKTBL_MergeUserData(&engine->sip_dim.ipmasktbl, pktpolicy_engine_MergeIPTblUserData);
    IPMASKTBL_MergeUserData(&engine->dip_dim.ipmasktbl, pktpolicy_engine_MergeIPTblUserData);

    return 0;
}

/* return <0: 没找到
   >=0: PKT_POLICY_ACTION_E
 */
int PKTPolicy_EngineMatch(PKT_POLICY_ENGINE_S *engine, PKT_POLICY_PKTINFO_S *pktinfo)
{
    STQ_HEAD_S *protocol_match_list;
    STQ_HEAD_S *sport_match_list, *dport_match_list;
    STQ_HEAD_S *sip_match_list, *dip_match_list;
    STQ_HEAD_S *host_match_list;
    PKT_POLICY_MATCHED_S matched;

    matched.count = 0;
    matched.min_rule_id_matched = RULE_ID_INVALID;

    protocol_match_list = pktpolicy_engine_MatchProtocol(&engine->protocol_dim, pktinfo->protocol);
    sport_match_list = pktpolicy_engine_MatchPort(&engine->sport_dim, pktinfo->sport);
    dport_match_list = pktpolicy_engine_MatchPort(&engine->dport_dim, pktinfo->dport);
    sip_match_list = pktpolicy_engine_MatchIP(&engine->sip_dim, pktinfo->sip);
    dip_match_list = pktpolicy_engine_MatchIP(&engine->dip_dim, pktinfo->dip);
    host_match_list = pktpolicy_engine_MatchHost(engine->trie, &pktinfo->host);

    pktpolicy_engine_MergeMatched(engine, protocol_match_list, PKT_POLICY_DIM_MASK_PROTOCOL, &matched);
    pktpolicy_engine_MergeMatched(engine, sport_match_list, PKT_POLICY_DIM_MASK_SPORT, &matched);
    pktpolicy_engine_MergeMatched(engine, dport_match_list, PKT_POLICY_DIM_MASK_DPORT, &matched);
    pktpolicy_engine_MergeMatched(engine, sip_match_list, PKT_POLICY_DIM_MASK_SIP, &matched);
    pktpolicy_engine_MergeMatched(engine, dip_match_list, PKT_POLICY_DIM_MASK_DIP, &matched);
    pktpolicy_engine_MergeMatched(engine, host_match_list, PKT_POLICY_DIM_MASK_HOST, &matched);

    if (matched.min_rule_id_matched == RULE_ID_INVALID) {
        return engine->default_action;
    }

    return engine->rule_desc[matched.min_rule_id_matched].action;
}

