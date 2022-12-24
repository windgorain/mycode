/******************************************************************************
* Copyright (C), 2000-2006,  Xingang.Li
* Author:      LiXingang  Version: 1.0  Date: 2016-10-14
* Description: 
* History:     
******************************************************************************/
#ifndef __IP_ACL_H_
#define __IP_ACL_H_

#include "utl/list_rule.h"
#include "utl/address_pool_utl.h"
#include "utl/port_pool_utl.h"

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

typedef LIST_RULE_HANDLE IPACL_HANDLE;
typedef HANDLE IPACL_LIST_HANDLE;

#define ACL_INVALID_LIST_ID 0
#define ACL_POOL_NAME_LEN_MAX 64

#define IPACL_RULE_ID_MAX 10000000

/* IPV4基本和高级ACL: */
#define IPACL_KEY_SIP         0x01          /* 源IP地址 */
#define IPACL_KEY_DIP        (0x01 << 1)    /* 目的IP地址 */
#define IPACL_KEY_SPORT      (0x01 << 2)    /* 源端口号 */
#define IPACL_KEY_DPORT      (0x01 << 3)    /* 目的端口号 */
#define IPACL_KEY_PROTO      (0x01 << 4)    /* 协议号 */
#define IPACL_KEY_POOL_SIP   (0x01 << 5)    /* 源地址池 */
#define IPACL_KEY_POOL_DIP   (0x01 << 6)    /* 目的地址池 */
#define IPACL_KEY_POOL_SPORT (0x01 << 7)    /* 源端口池 */
#define IPACL_KEY_POOL_DPORT (0x01 << 8)    /* 源目的端口池 */

typedef struct {
    UINT uiIP;
    UINT uiWildcard;
}IPACL_IPKEY_S;

typedef struct {
    USHORT usBegin;
    USHORT usEnd;
}IPACL_PORTKEY_S;

typedef struct {
    IPACL_IPKEY_S stSIP;
    IPACL_IPKEY_S stDIP;
    IPACL_PORTKEY_S stSPort;
    IPACL_PORTKEY_S stDPort;
}IPACL_SINGLE_KEY_S;

typedef struct {
    LIST_RULE_LIST_S *pstSipPool;
    LIST_RULE_LIST_S *pstDipPool;
    LIST_RULE_LIST_S *pstSportPool;
    LIST_RULE_LIST_S *pstDportPool;
}IPACL_POOL_KEY_S;

typedef struct {
    IPACL_POOL_KEY_S pools;
    IPACL_SINGLE_KEY_S single;
} IPACL_KEY_S;

typedef struct {
    UINT bEnable : 1;
    BS_ACTION_E enAction;    /* 命中后的动作 */

    UINT uiKeyMask;
    IPACL_KEY_S stKey;
    UCHAR ucProto;
}IPACL_RULE_CFG_S;

typedef struct {
    /* 统计计数 */
    volatile ULONG ulMatchCount;    /* 命中次数 */
    UINT uiLatestMatchTime;         /* 规则命中最新时间 */
}IPACL_RULE_STATISTICS_S;

typedef struct {
    UINT uiKeyMask;
    UINT uiSIP;
    UINT uiDIP;
    USHORT usSPort;
    USHORT usDPort;
    UCHAR ucProto;
    UINT acl_id;
}IPACL_MATCH_INFO_S;

typedef struct {
    IPACL_RULE_CFG_S stRuleCfg;
    IPACL_RULE_STATISTICS_S stStatistics;
}IPACL_RULE_S;

typedef BOOL_T (*PF_IPACL_RULE_SCAN)(IN UINT uiRuleID, IN IPACL_RULE_S *pstRule, IN VOID *pUserHandle);

IPACL_HANDLE IPACL_Create(void *memcap);
void IPACL_Destroy(IPACL_HANDLE hIpAcl);
void IPACL_Reset(IPACL_HANDLE hIpAcl);
IPACL_LIST_HANDLE IPACL_CreateList(IN IPACL_HANDLE hIpAcl, IN CHAR *pcListName);
VOID IPACL_DestroyList(IN IPACL_HANDLE hIpAcl, IN IPACL_LIST_HANDLE hIpAclList);
UINT IPACL_AddList(IN IPACL_HANDLE hIpAcl, IN CHAR *pcListName);
VOID IPACL_DelList(IN IPACL_HANDLE hIpAcl, IN UINT ulListID);
BS_STATUS IPACL_ReplaceList(IN IPACL_HANDLE hIpAcl, IN UINT uiListID, IN IPACL_LIST_HANDLE hIpAclListNew);
BS_STATUS IPACL_AddListRef(IN IPACL_HANDLE hIpAcl, IN UINT uiListID);
BS_STATUS IPACL_DelListRef(IN IPACL_HANDLE hIpAcl, IN UINT uiListID);
UINT IPACL_ListGetRef(IN IPACL_HANDLE hIpAcl, IN UINT uiListID);
UINT IPACL_GetListByName(IN IPACL_HANDLE hIpAcl, IN CHAR *pcListName);
UINT IPACL_GetNextListID(IN IPACL_HANDLE hIpAcl, IN UINT uiCurrentListID/* 0表示获取第一个 */);
CHAR * IPACL_GetListNameByID(IN IPACL_HANDLE hIpAcl, IN UINT ulListID);
BS_ACTION_E IPACL_GetDefaultActionByID(IN IPACL_HANDLE hIpAcl, IN UINT uiListID);
BS_STATUS IPACL_SetDefaultActionByID(IN IPACL_HANDLE hIpAcl, IN UINT uiListID, BS_ACTION_E enAction);
BS_STATUS IPACL_AddRule2List(IN IPACL_HANDLE hIpAcl, IN IPACL_LIST_HANDLE hIpAclList, 
                             IN UINT uiRuleID, IN IPACL_RULE_CFG_S *pstRuleCfg);
BS_STATUS IPACL_AddRule(IN IPACL_HANDLE hIpAcl, IN UINT uiListID, IN UINT uiRuleID, IN IPACL_RULE_CFG_S *pstRuleCfg);
VOID IPACL_DelRule(IN IPACL_HANDLE hIpAcl, IN UINT uiListID, IN UINT uiRuleID);
VOID IPACL_UpdateRule(IN IPACL_RULE_S *pstRule, IN IPACL_RULE_CFG_S *pstRuleCfg);
IPACL_RULE_S * IPACL_GetRule(IN IPACL_HANDLE hIpAcl, IN UINT uiListID, IN UINT uiRuleID);
UINT IPACL_GetLastRuleID(IN IPACL_HANDLE hIpAcl, IN UINT uiListID);
BS_STATUS IPACL_MoveRule(IN IPACL_HANDLE hIpAcl, IN UINT uiListID, IN UINT uiOldRuleID, IN UINT uiNewRuleID);
BS_STATUS IPACL_RebaseID(IN IPACL_HANDLE hIpAcl, IN UINT uiListID, IN UINT uiStep);
BS_STATUS  IPACL_IncreaseID(IN IPACL_HANDLE hIpAcl, IN UINT uiListID, IN UINT uiStart, IN UINT uiEnd, IN UINT uiStep);
UINT IPACL_GetNextRuleID(IN IPACL_HANDLE hIpAcl, IN UINT uiListID, IN UINT uiCurrentRuleID);
VOID IPACL_ScanRule(IN IPACL_HANDLE hIpAcl, IN UINT uiListID, PF_IPACL_RULE_SCAN pfFunc, IN VOID *pUserHandle);
BS_ACTION_E IPACL_Match(IPACL_HANDLE hIpAcl, UINT uiListID, IPACL_MATCH_INFO_S *pstMatchInfo);



#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__IP_ACL_H_*/


