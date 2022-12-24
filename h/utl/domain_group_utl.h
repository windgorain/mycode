/******************************************************************************
* Description: Domain地址薄
* History:     
******************************************************************************/
#ifndef __DOMAIN_GROUP_UTL_H_
#define __DOMAIN_GROUP_UTL_H_

#include "utl/trie_utl.h"
#include "utl/list_rule.h"

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

#define DOMAIN_GROUP_LIST_NAME_SIZE 64

#define DOMAIN_GROUP_TYPE     "domain"
#define DOMAIN_NAME_MAX_LEN 255

typedef LIST_RULE_HANDLE DOMAIN_GROUP_HANDLE;

typedef struct {
    RCU_NODE_S rcu_node;
    TRIE_HANDLE hTrie;
}DOMAIN_GROUP_LIST_S;

typedef struct {
    RULE_NODE_S rule_node;
    CHAR szDomain[DOMAIN_NAME_MAX_LEN + 1];
}DOMAIN_GROUP_NODE_S;

DOMAIN_GROUP_HANDLE DomainGroup_Create(void *memcap);
void DomainGroup_Destroy(DOMAIN_GROUP_HANDLE pool);

LIST_RULE_LIST_S * DomainGroup_FindListByName(IN DOMAIN_GROUP_HANDLE pool, IN CHAR *pcListName);
int DomainGroup_AddList(DOMAIN_GROUP_HANDLE pool, IN char *list_name);
int DomainGroup_DelList(IN DOMAIN_GROUP_HANDLE pool, IN char *pcName);
DOMAIN_GROUP_NODE_S * DomainGroup_FindNode(IN LIST_RULE_LIST_S *pstList, char *pcDomain);
int DomainGroup_DelDomain(IN LIST_RULE_LIST_S *pstList, char *pcDomain);
int DomainGroup_AddDomain(IN LIST_RULE_LIST_S *pstList, char *pcDomain);
int DomainGroup_LoadCfgFromFile(IN LIST_RULE_LIST_S *pstList, IN CHAR *pcFileName);
BOOL_T DomainGroup_Match(IN LIST_RULE_LIST_S *pstList, IN CHAR* pcDomain);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__DOMAIN_GROUP_UTL_H_*/
