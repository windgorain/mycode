
#ifndef __ADDRESS_POOL_UTL_H_
#define __ADDRESS_POOL_UTL_H_

#ifdef __cplusplus
    extern "C" {
#endif 

#include "bs.h"
#include "utl/lpm_utl.h"
#include "utl/rcu_utl.h"
#include "utl/list_rule.h"

#define ADDRESS_POOL_TYPE "ip"
#define ADDRESS_POOL_LIST_NAME_SIZE 64

typedef LIST_RULE_HANDLE ADDRESS_POOL_HANDLE;

typedef struct address_pool_cfg_node {
    RULE_NODE_S rule_node;
    UINT ip; 
    UCHAR prefix;
}ADDRESS_POOL_NODE_S;

typedef struct {
    RCU_NODE_S rcu_node;
    LPM_S stLpm;
}ADDRESS_POOL_LIST_S;

ADDRESS_POOL_HANDLE AddressPool_Create(void *memcap);
void AddressPool_Destroy(ADDRESS_POOL_HANDLE hList);
LIST_RULE_LIST_S * AddressPool_FindListByName(IN ADDRESS_POOL_HANDLE pool, IN CHAR *pcListName);
int AddressPool_AddIpList(IN ADDRESS_POOL_HANDLE hList, char *pcListName);
BS_STATUS AddressPool_DelIpList(IN ADDRESS_POOL_HANDLE hList, IN CHAR *pcName);
ADDRESS_POOL_NODE_S * AddressPool_FindIpNode(IN LIST_RULE_LIST_S *pstList, UINT ip, UCHAR prefix);
int AddressPool_DelIp(IN LIST_RULE_LIST_S *pstList, UINT ip, UCHAR prefix);
int AddressPool_AddIp(IN LIST_RULE_LIST_S *pstList, UINT ip, UCHAR prefix);
int AddressPool_LoadCfgFromFile(IN LIST_RULE_LIST_S *pstList, IN CHAR *pcFileName);
BOOL_T AddressPool_MatchIpPool(IN LIST_RULE_LIST_S *pstList, IN UINT uiIp);

#ifdef __cplusplus
    }
#endif 

#endif 
