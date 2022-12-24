/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-6-8
* Description: 
* History:     
******************************************************************************/

#ifndef __URL_ACL_H_
#define __URL_ACL_H_

#include "utl/list_rule.h"
#include "utl/socket_in.h"
#include "utl/re_match.h"

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */


typedef LIST_RULE_HANDLE URL_ACL_HANDLE;
typedef HANDLE URL_ACL_LIST_HANDLE;

#define URL_ACL_RULE_MAX_LEN 255
#define COMMON_URL_MAX_LEN   255

#define URL_ACL_KEY_MATCH_ALL    0x01  /* 匹配所有,相当于配置了 "permit/deny *" */
#define URL_ACL_KEY_PROTOCOL     0x02
#define URL_ACL_KEY_IPADDR       0x04
#define URL_ACL_KEY_DOMAIN       0x08
#define URL_ACL_KEY_PORT         0x10
#define URL_ACL_KEY_PATH         0x20
#define URL_ACL_KEY_URL          0x40

#define URL_ACL_MAX_DOMAIN_LEN      255
#define URL_ACL_MAX_PATH_LEN        255

#define ACL_INVALID_LIST_ID 0
//#define ACL_POOL_NAME_LEN_MAX 64

#define URL_ACL_RULE_ID_MAX 10000000

typedef enum
{
    URL_ACL_PROTOCOL_HTTP,
    URL_ACL_PROTOCOL_HTTPS,
    URL_ACL_PROTOCOL_TCP,
    URL_ACL_PROTOCOL_UDP,
    URL_ACL_PROTOCOL_ICMP,
    URL_ACL_PROTOCOL_IP,
    URL_ACL_PROTOCOL_BUTT
}URL_ACL_PROTOCOL_E;

typedef struct tagURL_ACL_MATCH_INFO
{
    UINT   uiFlag;                               /* 有效比较位，如URL_ACL_KEY_PORT */
    URL_ACL_PROTOCOL_E enProtocol;               /* 协议 */
    INET_ADDR_S stAddr;                          /* 地址 */
    UCHAR szDomain[URL_ACL_MAX_DOMAIN_LEN + 1];  /* 域名 */
    USHORT usPort;                               /* 端口 */
    UCHAR szPath[URL_ACL_MAX_PATH_LEN + 1];      /* 路径 */
}URL_ACL_MATCH_INFO_S;

#define URL_ACL_LIST_NAME_MAX_LEN 63
#define URL_ACL_PORTNUM_MAX_LEN     5

#define URL_ACL_MAX_PATTERN_LEN     URL_ACL_RULE_MAX_LEN

#define URL_ACL_PATTERN_BUF_MAX      1024UL


typedef enum
{
    URL_ACL_CHAR_WILDCARD,
    URL_ACL_CHAR_NEEDTRANSFER,
    URL_ACL_CHAR_NORMAL,
    URL_ACL_CHAR_BUTT
}URL_ACL_CHAR_TYPE_E;

typedef enum enUrlAcl_PATTERN
{
    URL_ACL_PATTERN_STRING,    /* 精确字符串 */
    URL_ACL_PATTERN_PCRE,      /* 正则字符串 */
    URL_ACL_PATTERN_BUTT
}URL_ACL_PATTERN_E;

typedef struct
{
    VOID *pstReg;       /* 正则表达式编译结果 */
    VOID *pstExtraData; /* 正则表达式，学习数据 */
}URL_ACL_PCRE_S;

typedef enum enUrlAcl_HOST
{
    URL_ACL_HOST_IPADDR,   /* ip地址方式 */
    URL_ACL_HOST_DOMAIN,   /* 域名方式 */
    URL_ACL_HOST_BUTT
}URL_ACL_HOST_E;

typedef struct tagUrlAcl_PATTERN
{
    URL_ACL_PATTERN_E enType;       /*简单字符串、正则表达式*/
    UCHAR szPattern[URL_ACL_MAX_PATTERN_LEN + 1];
    CHAR *pcPcreStr;
	RE_HDL pstPcre;
}URL_ACL_PATTERN_S;

typedef struct tagUrlAcl_IPGROUP
{
    DLL_NODE_S stNode;      /* ip group list */
    INET_ADDR_S stAddrStart;
    INET_ADDR_S stAddrStop;
}URL_ACL_IPGROUP_S;

typedef struct tagUrlAcl_PORTGROUP
{
    DLL_NODE_S stNode;     /* Port group list */
    USHORT usPortStart;    /* start port */
    USHORT usPortEnd;      /* end port */
}URL_ACL_PORTGROUP_S;

typedef struct tagUrlAcl_HOST
{
    URL_ACL_HOST_E enType;                /* ip/host-name */
    union
    {
        DLL_HEAD_S stIpList;             /* URL_ACL_IPGROUP_S list*/
        URL_ACL_PATTERN_S stHostDomain;
    }un_host;
#define uIpList un_host.stIpList
#define uHostDomain  un_host.stHostDomain
}URL_ACL_RULE_CFG_S;


typedef struct
{
    /* 统计计数 */
    volatile ULONG ulMatchCount;    /* 命中次数 */
    UINT uiLatestMatchTime;         /* 规则命中最新时间 */
}URL_ACL_RULE_STATISTICS_S;

typedef struct tagURLAcl_Rule
{
    BS_ACTION_E enAction;       /* URL ACL Action */
	URL_ACL_RULE_CFG_S stRuleCfg;
	URL_ACL_RULE_STATISTICS_S statistics;
}URL_ACL_RULE_S;

typedef BOOL_T (*PF_URL_ACL_RULE_SCAN)(UINT rule_id, URL_ACL_RULE_S *pstRule, void *ud);
URL_ACL_HANDLE URL_ACL_Create(void *memcap);
void URL_ACL_Destroy(URL_ACL_HANDLE acl);
URL_ACL_LIST_HANDLE URL_ACL_CreateList(URL_ACL_HANDLE acl, char *list_name);
void URL_ACL_DestroyList(URL_ACL_HANDLE acl, URL_ACL_LIST_HANDLE list);
UINT URL_ACL_AddList(URL_ACL_HANDLE acl, char *list_name);
void URL_ACL_DelList(URL_ACL_HANDLE acl, UINT list_id);
int URL_ACL_ReplaceList(URL_ACL_HANDLE acl, UINT list_id, URL_ACL_LIST_HANDLE new_list);
int URL_ACL_AddListRef(URL_ACL_HANDLE acl, UINT list_id);
int URL_ACL_DelListRef(URL_ACL_HANDLE acl, UINT list_id);
UINT URL_ACL_ListGetRef(URL_ACL_HANDLE acl, UINT list_id);
UINT URL_ACL_GetListByName(URL_ACL_HANDLE acl, char *list_name);
UINT URL_ACL_GetNextListID(URL_ACL_HANDLE acl, UINT curr_list_id/* 0表示获取第一个 */);
char * URL_ACL_GetListNameByID(URL_ACL_HANDLE acl, UINT list_id);
BS_ACTION_E URL_ACL_GetDefaultActionByID(URL_ACL_HANDLE acl, UINT list_id);
int URL_ACL_SetDefaultActionByID(URL_ACL_HANDLE acl, UINT list_id, BS_ACTION_E enAction);
int URL_ACL_AddRuleToList(LIST_RULE_HANDLE acl, URL_ACL_LIST_HANDLE list,
        UINT rule_id, URL_ACL_RULE_CFG_S *rule_cfg);
int URL_ACL_AddRule(URL_ACL_HANDLE acl, UINT list_id, UINT rule_id, URL_ACL_RULE_CFG_S *rule_cfg);
void URL_ACL_DelRule(URL_ACL_HANDLE acl, UINT list_id, UINT rule_id);
URL_ACL_RULE_S * URL_ACL_GetRule(LIST_RULE_HANDLE acl, UINT list_id, UINT rule_id);
int URL_ACL_MoveRule(URL_ACL_HANDLE acl, UINT list_id, UINT old_rule_id, UINT new_rule_id);
int URL_ACL_RebaseID(URL_ACL_HANDLE acl, UINT list_id, UINT step);
int URL_ACL_IncreaseID(URL_ACL_HANDLE acl, UINT list_id, UINT start, UINT end, UINT step);
UINT URL_ACL_GetNextRuleID(URL_ACL_HANDLE acl, UINT list_id, UINT curr_rule_id);
void URL_ACL_ScanRule(URL_ACL_HANDLE acl, UINT list_id, PF_URL_ACL_RULE_SCAN pfFunc, void *ud);
void URL_ACL_UpdateRule(URL_ACL_RULE_S *pstRule, URL_ACL_RULE_CFG_S *rule_config);
void URL_ACL_Reset(URL_ACL_HANDLE acl);
BS_ACTION_E URL_ACL_Match(URL_ACL_HANDLE acl, UINT list_id, URL_ACL_MATCH_INFO_S *pstMatchInfo);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__URL_ACL_H_*/


