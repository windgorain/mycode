/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-6-8
* Description: 
* History:     
******************************************************************************/

#ifndef __URI_ACL_H_
#define __URI_ACL_H_

#include "utl/list_rule.h"
#include "utl/socket_in.h"

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */


typedef HANDLE ACL_HANDLE;

#define URI_ACL_RULE_MAX_LEN 255

#define URI_ACL_KEY_MATCH_ALL    0x01  /* 匹配所有,相当于配置了 "permit/deny *" */
#define URI_ACL_KEY_PROTOCOL     0x02
#define URI_ACL_KEY_IPADDR       0x04
#define URI_ACL_KEY_DOMAIN       0x08
#define URI_ACL_KEY_PORT         0x10
#define URI_ACL_KEY_PATH         0x20
#define URI_ACL_KEY_URI          0x40

#define URI_ACL_MAX_DOMAIN_LEN      255
#define URI_ACL_MAX_PATH_LEN        255


typedef enum
{
    URI_ACL_PROTOCOL_HTTP,
    URI_ACL_PROTOCOL_HTTPS,
    URI_ACL_PROTOCOL_TCP,
    URI_ACL_PROTOCOL_UDP,
    URI_ACL_PROTOCOL_ICMP,
    URI_ACL_PROTOCOL_IP,
    URI_ACL_PROTOCOL_BUTT
}URI_ACL_PROTOCOL_E;

typedef struct tagURI_ACL_MATCH_INFO
{
    UINT   uiFlag;                               /* 有效比较位，如URI_ACL_KEY_PORT */
    URI_ACL_PROTOCOL_E enProtocol;               /* 协议 */
    INET_ADDR_S stAddr;                          /* 地址 */
    UCHAR szDomain[URI_ACL_MAX_DOMAIN_LEN + 1];  /* 域名 */
    USHORT usPort;                               /* 端口 */
    UCHAR szPath[URI_ACL_MAX_PATH_LEN + 1];      /* 路径 */
}URI_ACL_MATCH_INFO_S;


#define URI_ACL_LIST_NAME_MAX_LEN 63
#define URI_ACL_PORTNUM_MAX_LEN     5

#define URI_ACL_MAX_PATTERN_LEN     URI_ACL_RULE_MAX_LEN

#define URI_ACL_PATTERN_BUF_MAX      1024UL

typedef enum
{
    URI_ACL_CHAR_WILDCARD,
    URI_ACL_CHAR_NEEDTRANSFER,
    URI_ACL_CHAR_NORMAL,
    URI_ACL_CHAR_BUTT
}URI_ACL_CHAR_TYPE_E;

typedef enum enUriAcl_PATTERN
{
    URI_ACL_PATTERN_STRING,    /* 精确字符串 */
    URI_ACL_PATTERN_PCRE,      /* 正则字符串 */
    URI_ACL_PATTERN_BUTT
}URI_ACL_PATTERN_E;

typedef struct
{
    VOID *pstReg;       /* 正则表达式编译结果 */
    VOID *pstExtraData; /* 正则表达式，学习数据 */
}URI_ACL_PCRE_S;

typedef enum enUriAcl_HOST
{
    URI_ACL_HOST_IPADDR,   /* ip地址方式 */
    URI_ACL_HOST_DOMAIN,   /* 域名方式 */
    URI_ACL_HOST_BUTT
}URI_ACL_HOST_E;

typedef struct tagUriAcl_PATTERN
{
    URI_ACL_PATTERN_E enType;       /*简单字符串、正则表达式*/
    UCHAR szPattern[URI_ACL_MAX_PATTERN_LEN + 1];
    CHAR *pcPcreStr;
    URI_ACL_PCRE_S stPcre;           
}URI_ACL_PATTERN_S;

typedef struct tagUriAcl_IPGROUP
{
    DLL_NODE_S stNode;      /* ip group list */
    INET_ADDR_S stAddrStart;
    INET_ADDR_S stAddrStop;
}URI_ACL_IPGROUP_S;

typedef struct tagUriAcl_PORTGROUP
{
    DLL_NODE_S stNode;     /* Port group list */
    USHORT usPortStart;    /* start port */
    USHORT usPortEnd;      /* end port */
}URI_ACL_PORTGROUP_S;

typedef struct tagUriAcl_HOST
{
    URI_ACL_HOST_E enType;                /* ip/host-name */
    union {
        DLL_HEAD_S stIpList;             /* URI_ACL_IPGROUP_S list*/
        URI_ACL_PATTERN_S stHostDomain;
    }un_host;
#define uIpList un_host.stIpList
#define uHostDomain  un_host.stHostDomain
}URI_ACL_HOST_S;

typedef struct tagURIAcl_Rule
{
    RULE_NODE_S stListRuleNode;
    UINT action;
    URI_ACL_PATTERN_S stPattern; 
    UINT match_count;
}URI_ACL_RULE_S;

LIST_RULE_HANDLE URI_ACL_Create(void *memcap);
VOID URI_ACL_Destroy(IN LIST_RULE_HANDLE hCtx);
UINT URI_ACL_AddList(IN LIST_RULE_HANDLE hCtx, IN CHAR *pcListName);
VOID URI_ACL_DelList(IN LIST_RULE_HANDLE hCtx, IN UINT uiListID);
UINT URI_ACL_FindListByName(IN LIST_RULE_HANDLE hCtx, IN CHAR *pcListName);
LIST_RULE_LIST_S* URI_ACL_GetListByName(IN LIST_RULE_HANDLE hURIHandle, IN CHAR *pcListName);
LIST_RULE_LIST_S* URI_ACL_GetListByID(IN LIST_RULE_HANDLE hURIHandle, UINT list_id);
BS_STATUS URI_ACL_AddListRef(IN LIST_RULE_HANDLE hURIAcl, IN UINT uiListID);
BS_STATUS URI_ACL_DelListRef(IN LIST_RULE_HANDLE hURIAcl, IN UINT uiListID);
BS_STATUS URI_ACL_AddRule(IN LIST_RULE_HANDLE hCtx, IN LIST_RULE_LIST_S* rule_list, IN UINT uiRuleID, IN CHAR *pcRule, IN CHAR* Action);
BS_STATUS URI_ACL_AddRuleToListID(IN LIST_RULE_HANDLE hCtx, UINT uiListID, IN UINT uiRuleID, IN CHAR *pcRule, IN CHAR* Action);
VOID URI_ACL_DelRule(IN LIST_RULE_LIST_S*pstList, IN UINT uiRuleID);
URI_ACL_RULE_S * URI_ACL_GetRule(IN LIST_RULE_HANDLE hURIAcl, IN UINT uiListID, IN UINT uiRuleID);
BS_STATUS URI_ACL_Match(LIST_RULE_HANDLE hCtx, UINT uiListID, URI_ACL_MATCH_INFO_S *pstMatchInfo, OUT BS_ACTION_E *penAction);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__URI_ACL_H_*/


