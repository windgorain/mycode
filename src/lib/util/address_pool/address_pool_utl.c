/*================================================================
*   Description: acl addresses
*      acl 地址薄处理
*
================================================================*/
#include "bs.h"
#include "bs/rcu_engine.h"
#include "utl/mutex_utl.h"
#include "utl/list_rule.h"
#include "utl/atomic_utl.h"
#include "utl/socket_utl.h"
#include "utl/txt_utl.h"
#include "utl/lstr_utl.h"
#include "utl/ip_utl.h"
#include "utl/mem_cap.h"
#include "utl/ip_string.h"
#include "utl/file_utl.h"
#include "utl/address_pool_utl.h"

#define ADDRESS_POOL_LPM_SIZE (1024*512)

static void _address_pool_free_List(void *rcu_node)
{
    ADDRESS_POOL_LIST_S *node = container_of(rcu_node, ADDRESS_POOL_LIST_S, rcu_node);
    LPM_Final(&node->stLpm);
    MEM_Free(node);
}

static int _address_pool_add_ip_string(IN LIST_RULE_LIST_S *pstList, char *ip_prefix_string)
{
    IP_PREFIX_S ip_prefix;

    int ret = IPString_ParseIpPrefix(ip_prefix_string, &ip_prefix);
    if (ret < 0) {
        return ret;
    }
 
    return AddressPool_AddIp(pstList, ip_prefix.uiIP, ip_prefix.ucPrefix);
}


static UCHAR _address_pool_get_up_prefix(IN LIST_RULE_LIST_S *pstList, IN ADDRESS_POOL_NODE_S *pstNode)
{
    ADDRESS_POOL_NODE_S *pstNodeTmp = NULL;
    RULE_NODE_S *node = NULL;
    UCHAR prefix = 0;
    DBGASSERT(NULL != pstList);

    while ((node = RuleList_GetNextByNode(&pstList->stRuleList, node))) {
        pstNodeTmp = container_of(node, ADDRESS_POOL_NODE_S, rule_node);

        if ((pstNodeTmp == pstNode) || (pstNode->prefix < pstNodeTmp->prefix)) {
            continue;
        }

        UINT mask = PREFIX_2_MASK(pstNodeTmp->prefix);

        
        if ((pstNode->ip & mask) == (pstNodeTmp->ip & mask)){
            if (prefix < pstNodeTmp->prefix){
                prefix = pstNodeTmp->prefix;
            }
        }
    }

    return prefix;
}

static int _address_pool_del_ip_list(IN ADDRESS_POOL_HANDLE pool, LIST_RULE_LIST_S *list)
{
    ADDRESS_POOL_LIST_S *node;

    if (list->uiRefCount > 0) {
        RETURN(BS_REF_NOT_ZERO);
    }

    node = list->user_handle[0];

    ListRule_DelList(pool, list, NULL, NULL);

    void *memcap = ListRule_GetMemcap(pool);

    MemCap_Call(memcap, &node->rcu_node, _address_pool_free_List);

    return 0;
}

ADDRESS_POOL_HANDLE AddressPool_Create(void *memcap)
{
    return ListRule_Create(memcap);
}

void AddressPool_Destroy(ADDRESS_POOL_HANDLE pool)
{
    LIST_RULE_HEAD_S *head;

    while ((head = ListRule_GetNextList(pool, NULL))) {
        int ret = _address_pool_del_ip_list(pool, head->pstListRule);
        if (ret < 0) {
            BS_DBGASSERT(0);
        }
    }

    ListRule_Destroy(pool, NULL, NULL);
}

LIST_RULE_LIST_S * AddressPool_FindListByName(IN ADDRESS_POOL_HANDLE pool, IN CHAR *pcListName)
{
    return ListRule_GetListByName(pool, pcListName);
}

int AddressPool_AddIpList(ADDRESS_POOL_HANDLE pool, char *list_name)
{
    LIST_RULE_LIST_S *list;
    ADDRESS_POOL_LIST_S *node;
    UINT list_id;

    node = MEM_ZMalloc(sizeof(ADDRESS_POOL_LIST_S));
    if (! node) {
        RETURN(BS_NO_MEMORY);
    }

    if (0 != LPM_Init(&node->stLpm, ADDRESS_POOL_LPM_SIZE, NULL)) {
        MEM_Free(node);
        RETURN(BS_NO_MEMORY);
    }

    LPM_SetLevel(&node->stLpm, 7, 8);

    list_id = ListRule_AddList(pool, list_name, sizeof(void *));
    if (list_id == NAP_INVALID_ID) {
        LPM_Final(&node->stLpm);
        MEM_Free(node);
        RETURN(BS_ERR);
    }

    list = ListRule_GetListByID(pool, list_id);
    BS_DBGASSERT(list);
    list->user_handle[0] = node;

    return BS_OK;
}

int AddressPool_DelIpList(IN ADDRESS_POOL_HANDLE pool, IN char *pcName)
{
    LIST_RULE_LIST_S *list = ListRule_GetListByName(pool, pcName);
    if (! list) {
        return 0;
    }
    return _address_pool_del_ip_list(pool, list);
}

ADDRESS_POOL_NODE_S * AddressPool_FindIpNode(IN LIST_RULE_LIST_S *pstList, UINT ip, UCHAR prefix)
{
    RULE_NODE_S *rule_node = NULL;
    ADDRESS_POOL_NODE_S *node;

    while ((rule_node = RuleList_GetNextByNode(&pstList->stRuleList, rule_node))) {
        node = container_of(rule_node, ADDRESS_POOL_NODE_S, rule_node);
        if ((node->ip == ip) && (node->prefix == prefix)) {
            return node;
        }
    }

    return NULL;
}

int AddressPool_AddIp(IN LIST_RULE_LIST_S *pstList, UINT ip, UCHAR prefix)
{
    ADDRESS_POOL_NODE_S *pstNode = NULL;
    ADDRESS_POOL_LIST_S *node;

    DBGASSERT(NULL != pstList);

    if (AddressPool_FindIpNode(pstList, ip, prefix)) {
        
        return BS_OK;
    }

    pstNode = MEM_ZMalloc(sizeof(ADDRESS_POOL_NODE_S));
    if (! pstNode){
        RETURN(BS_NO_MEMORY);
    }

    node = pstList->user_handle[0];

    if (0 < LPM_Add(&node->stLpm, ip, prefix, 1)) {
        MEM_Free(pstNode);
        RETURN(BS_NO_MEMORY);
    }

    pstNode->ip = ip;
    pstNode->prefix = prefix;

    RuleList_Add(&pstList->stRuleList, 0, &pstNode->rule_node);

    return BS_OK;
}

int AddressPool_DelIp(IN LIST_RULE_LIST_S *pstList, UINT ip, UCHAR prefix)
{
    ADDRESS_POOL_NODE_S *pstNode = NULL;
    ADDRESS_POOL_LIST_S *node;
    UCHAR up_prifix;

    pstNode = AddressPool_FindIpNode(pstList, ip, prefix);
    if (! pstNode){
        return BS_OK;
    }

    up_prifix = _address_pool_get_up_prefix(pstList, pstNode);

    RuleList_DelByNode(&pstList->stRuleList, &pstNode->rule_node);
    MEM_Free(pstNode);

    node = pstList->user_handle[0];
    LPM_Del(&node->stLpm, ip, prefix, up_prifix, 1);

    return BS_OK;
}

int AddressPool_LoadCfgFromFile(IN LIST_RULE_LIST_S *pstList, IN CHAR *pcFileName)
{
    FILE *fp;
    BS_STATUS ret = BS_OK;
    CHAR line[64];

    DBGASSERT(NULL != pstList);
    
    if (! FILE_IsFileExist(pcFileName)) {
        RETURN(BS_NO_SUCH);
    }

    fp = fopen(pcFileName, "rb");
    if (NULL == fp) {
        RETURN(BS_CAN_NOT_OPEN);
    }

    
    while (1) {
        int len = FILE_ReadLine(fp, line, sizeof(line), '\n');
        if (len <= 0) {
            
            break;
        }

        if (len >= sizeof(line)) {
            
            continue;
        }

        ret = _address_pool_add_ip_string(pstList, line);
        if (BS_OK != ret) {
            break;
        }
    }

    fclose(fp);

    return ret;
}

BOOL_T AddressPool_MatchIpPool(IN LIST_RULE_LIST_S *pstList, IN UINT uiIp)
{
    UINT64 nextHop = 0;
    ADDRESS_POOL_LIST_S *node;

    BS_DBGASSERT(NULL != pstList);

    node = pstList->user_handle[0];

    BS_DBGASSERT(node);

    return (BS_NOT_FOUND != LPM_Lookup(&node->stLpm, uiIp, &nextHop));
}

