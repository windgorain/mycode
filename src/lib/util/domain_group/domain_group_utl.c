/*================================================================
*   Description: Domain地址薄
*
================================================================*/
#include "bs.h"
#include "bs/rcu_engine.h"
#include "utl/mutex_utl.h"
#include "utl/atomic_utl.h"
#include "utl/socket_utl.h"
#include "utl/txt_utl.h"
#include "utl/lstr_utl.h"
#include "utl/ip_utl.h"
#include "utl/mem_cap.h"
#include "utl/ip_string.h"
#include "utl/file_utl.h"
#include "utl/domain_group_utl.h"
#include "utl/args_utl.h"
#include "utl/trie_utl.h"

#define DOMAIN_GROUP_FORMAT_FILE_CFG_LINE_MAX 512 

static void _domain_group_free_List(IN void *pstRcuNode)
{
    DOMAIN_GROUP_LIST_S *pstList = container_of(pstRcuNode, DOMAIN_GROUP_LIST_S, rcu_node);
	Trie_Destroy(pstList->hTrie, NULL);
	MEM_Free(pstList);
    return;
}

static int _domain_group_check_domain(IN CHAR* pcDomain)
{
    int length = strlen(pcDomain);
    int i = 0;

    if (length > DOMAIN_NAME_MAX_LEN) {
        RETURN(BS_BAD_PARA);
    }

    for(i=1; i<length; i++) {
        if(pcDomain[i] == '*') {
            RETURN(BS_BAD_PARA);
        }
    }

    return BS_OK;
    
}

static int _domain_group_del_list(IN LIST_RULE_HANDLE pool, LIST_RULE_LIST_S *list)
{
    DOMAIN_GROUP_LIST_S *domain_list;

    if (list->uiRefCount > 0) {
        RETURN(BS_REF_NOT_ZERO);
    }

    domain_list = ListRule_GetUserHandle(list, 0);

    ListRule_DelList(pool, list, NULL, NULL);

    void *memcap = ListRule_GetMemcap(pool);

    MemCap_Call(memcap, &domain_list->rcu_node, _domain_group_free_List);

    return 0;
}

DOMAIN_GROUP_HANDLE DomainGroup_Create(void *memcap)
{
    return ListRule_Create(memcap);
}

void DomainGroup_Destroy(DOMAIN_GROUP_HANDLE pool)
{
    LIST_RULE_HEAD_S *head;

    while ((head = ListRule_GetNextList(pool, NULL))) {
        int ret = _domain_group_del_list(pool, head->pstListRule);
        if (ret < 0) {
            BS_DBGASSERT(ret == 0);
        }
    }

    ListRule_Destroy(pool, NULL, NULL);
}

LIST_RULE_LIST_S * DomainGroup_FindListByName(IN DOMAIN_GROUP_HANDLE pool, IN CHAR *pcListName)
{
    return ListRule_GetListByName(pool, pcListName);
}

int DomainGroup_AddList(DOMAIN_GROUP_HANDLE pool, IN char *list_name)
{
    LIST_RULE_LIST_S *list;
    DOMAIN_GROUP_LIST_S *node;
    UINT list_id;

    node = MEM_ZMalloc(sizeof(DOMAIN_GROUP_LIST_S));
    if (! node) {
        RETURN(BS_NO_MEMORY);
    }

    node->hTrie = Trie_Create(TRIE_TYPE_4BITS);
    if (! node->hTrie) {
        MEM_Free(node);
        RETURN(BS_NO_MEMORY); 
    }

    list_id = ListRule_AddList(pool, list_name, sizeof(void *));
    if (list_id == NAP_INVALID_ID) {
        Trie_Destroy(node->hTrie, NULL);
        MEM_Free(node);
        RETURN(BS_ERR);
    }

    list = ListRule_GetListByID(pool, list_id);
    BS_DBGASSERT(list);

    list->user_handle[0] = node;

    return BS_OK;
}

int DomainGroup_DelList(IN DOMAIN_GROUP_HANDLE pool, IN char *pcName)
{
    LIST_RULE_LIST_S *list = ListRule_GetListByName(pool, pcName);
    if (! list) {
        return 0;
    }
    return _domain_group_del_list(pool, list);
}

DOMAIN_GROUP_NODE_S * DomainGroup_FindNode(IN LIST_RULE_LIST_S *pstList, char *pcDomain)
{
    RULE_NODE_S *rule_node = NULL;
    DOMAIN_GROUP_NODE_S *node;

    while ((rule_node = RuleList_GetNextByNode(&pstList->stRuleList, rule_node))) {
        node = container_of(rule_node, DOMAIN_GROUP_NODE_S, rule_node);
        if (strcmp(node->szDomain, pcDomain) == 0) {
            return node;
        }
    }

    return NULL;
}

int DomainGroup_AddDomain(IN LIST_RULE_LIST_S *pstList, char *pcDomain)
{
    DOMAIN_GROUP_NODE_S *pstNode = NULL;
    DOMAIN_GROUP_LIST_S *domain_list;
    int ret;
    CHAR szDomain[DOMAIN_NAME_MAX_LEN+1];

    BS_DBGASSERT(pstList);

    if((ret = _domain_group_check_domain(pcDomain)) < 0) {
        return ret;
    }

    if (DomainGroup_FindNode(pstList, pcDomain)) {
        return 0;
    }

    pstNode = MEM_ZMalloc(sizeof(DOMAIN_GROUP_NODE_S));
    if (! pstNode){
        RETURN(BS_NO_MEMORY);
    }

    domain_list = ListRule_GetUserHandle(pstList, 0);

    int len = strlen(pcDomain);
    MEM_Invert(pcDomain, len, szDomain);
    szDomain[len] = '\0';
    MEM_ReplaceOneChar(szDomain, len, '*', '\0');

    if (! Trie_Insert(domain_list->hTrie, (UCHAR*)szDomain, strlen(szDomain), TRIE_NODE_FLAG_WILDCARD, NULL)) {
        MEM_Free(pstNode);
        RETURN(BS_NO_MEMORY);
    }

    strlcpy(pstNode->szDomain, pcDomain, sizeof(pstNode->szDomain));

    RuleList_Add(&pstList->stRuleList, 0, &pstNode->rule_node);

    return BS_OK;
}

int DomainGroup_DelDomain(IN LIST_RULE_LIST_S *pstList, char *pcDomain)
{
    DOMAIN_GROUP_NODE_S *pstNode;
    DOMAIN_GROUP_LIST_S *domain_list;
    CHAR szDomain[DOMAIN_NAME_MAX_LEN+1];

    pstNode = DomainGroup_FindNode(pstList, pcDomain);
    if (! pstNode){
        return BS_OK;
    }

    RuleList_DelByNode(&pstList->stRuleList, &pstNode->rule_node);
    MEM_Free(pstNode);

    domain_list = ListRule_GetUserHandle(pstList, 0);

    int len = strlen(pcDomain);
    MEM_Invert(pcDomain, len, szDomain);
    szDomain[len] = '\0';
    MEM_ReplaceOneChar(szDomain, len, '*', '\0');

    Trie_Del(domain_list->hTrie, (UCHAR*)szDomain, strlen(szDomain));

    return BS_OK;
}

int DomainGroup_LoadCfgFromFile(IN LIST_RULE_LIST_S *pstList, IN CHAR *pcFileName)
{
    FILE *fp;
    int ret = BS_OK;
    CHAR line[512];

    DBGASSERT(NULL != pstList);
    
    if(! FILE_IsFileExist(pcFileName)) {
        RETURN(BS_NO_SUCH);
    }

    fp = fopen(pcFileName, "rb");
    if (NULL == fp) {
        RETURN(BS_CAN_NOT_OPEN);
    }

    // 读取每一行的ip地址信息,格式为domain xxxx这种形式
    while (1) {
        int len = FILE_ReadLine(fp, line, sizeof(line), '\n');
        if (len <=0) {
            break;
        }
        if (len >= sizeof(line)) {
            continue;
        }

        ret = DomainGroup_AddDomain(pstList, line);
        if (ret < 0){
            break;
        }
    }

    fclose(fp);

    return ret;
}

BOOL_T DomainGroup_Match(IN LIST_RULE_LIST_S *pstList, IN CHAR* pcDomain)
{
    CHAR szDomain[DOMAIN_NAME_MAX_LEN + 1];
    TRIE_COMMON_S* pstNode;
    DOMAIN_GROUP_LIST_S *domain_list = ListRule_GetUserHandle(pstList, 0);

    DBGASSERT(NULL != pstList);

    int len = strlen(pcDomain);
    MEM_Invert(pcDomain, len, szDomain);
    szDomain[len] = '\0';
    MEM_ReplaceOneChar(szDomain, len, '*', '\0');

    pstNode = Trie_Match(domain_list->hTrie, (UCHAR*)szDomain, strlen(szDomain), TRIE_MATCH_WILDCARD);
    if(pstNode) {
        return BOOL_TRUE;
    }

    return BOOL_FALSE;
}

