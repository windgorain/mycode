/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-6-8
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "pcre.h"

#include "utl/txt_utl.h"
#include "utl/socket_utl.h"
#include "utl/eth_utl.h"
#include "utl/lstr_utl.h"
#include "utl/nap_utl.h"
#include "utl/bit_opt.h"
#include "utl/uri_acl.h"


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
    union
    {
        DLL_HEAD_S stIpList;             /* URI_ACL_IPGROUP_S list*/
        URI_ACL_PATTERN_S stHostDomain;
    }un_host;
#define uIpList un_host.stIpList
#define uHostDomain  un_host.stHostDomain
}URI_ACL_HOST_S;

typedef struct tagUriAcl_KRULE
{
    RULE_NODE_S stListRuleNode;

    CHAR szRule[URI_ACL_RULE_MAX_LEN + 1];
    URI_ACL_PROTOCOL_E enProtocol;   /* http, https, tcp …etc. */
    UINT uiCfgFlag;
    URI_ACL_HOST_S stHostPattern;    /* URI_ACL_HOST_S */
    DLL_HEAD_S stPortGroupList;      /* URI_ACL_PORTGROUP_S */
    URI_ACL_PATTERN_S stPathPattern;    /* URI_ACL_PATTERN_S */
    URI_ACL_ACTION_E enAction;       /* URI ACL Action */
    UINT stMatchCount;         /* match count */
}URI_ACL_RULE_S;

STATIC CHAR *g_apcUriAclProtocol[URI_ACL_PROTOCOL_BUTT] =  
{
    "http://",
    "https://",
    "tcp://",
    "udp://",
    "icmp://",
    "ip://"    
};

#if 1
STATIC BS_STATUS uri_acl_ParseAction(INOUT LSTR_S *pstPattern, INOUT URI_ACL_RULE_S *pstRule)
{
    BS_STATUS eRet = BS_ERR;
    CHAR *pcFound;
    UINT uiLen;
    
    pstRule->enAction = URI_ACL_ACTION_DENY;

    if ((pstPattern->pcData[0] == 'p') || (pstPattern->pcData[0] == 'P'))
    {
        pstRule->enAction = URI_ACL_ACTION_PERMIT;
        eRet = BS_OK;
    }
    else if ((pstPattern->pcData[0] == 'd') || (pstPattern->pcData[0] == 'D'))
    {
        pstRule->enAction = URI_ACL_ACTION_DENY;
        eRet = BS_OK;
    }

    pcFound = LSTR_Strchr(pstPattern, ' ');
    uiLen = pcFound - pstPattern->pcData;

    pstPattern->pcData = pcFound;
    pstPattern->uiLen -= uiLen;

    LSTR_Strim(pstPattern, " \t", pstPattern);

    return eRet;
}

STATIC ULONG url_acl_ParseProtocol(INOUT LSTR_S *pstPattern, INOUT URI_ACL_RULE_S *pstRule)
{
    CHAR *pcFindString;
    URI_ACL_PROTOCOL_E enTmp;
    ULONG ulRet = ERROR_FAILED;
    UINT uiOffset;

    pstRule->enProtocol = URI_ACL_PROTOCOL_BUTT;
    for (enTmp = URI_ACL_PROTOCOL_HTTP; enTmp < URI_ACL_PROTOCOL_BUTT; enTmp++)
    {
        pcFindString = TXT_Strnstr(pstPattern->pcData, g_apcUriAclProtocol[enTmp], pstPattern->uiLen);
        if (NULL != pcFindString)
        {
            pstRule->enProtocol = enTmp; 
            pstRule->uiCfgFlag |= URI_ACL_KEY_PROTOCOL;
            uiOffset = pcFindString - pstPattern->pcData;
            uiOffset += strlen(g_apcUriAclProtocol[enTmp]);
            pstPattern->pcData += uiOffset;
            pstPattern->uiLen -= uiOffset;
            ulRet = ERROR_SUCCESS;
            break;
        }
    }

    return ulRet;    
}

/*****************************************************************************
  Description: 是否IPv6地址形式
      Caution: IPv6地址方式一定是已[开始的。形式如下:
*****************************************************************************/
STATIC inline BOOL_T uri_acl_HostIsAddr6Mode(IN CHAR cChar)
{
    BOOL_T bIsIPv6AddrMode = BOOL_FALSE;

    if (cChar == '[')
    {
        bIsIPv6AddrMode = BOOL_TRUE;
    }
    
    return bIsIPv6AddrMode;
}

STATIC inline ULONG uri_acl_HostGetAddr6Len(IN LSTR_S *pstString)
{
    const CHAR *pcHostCur = pstString->pcData;
    const CHAR *pcStrEnd = pstString->pcData + pstString->uiLen;
    ULONG ulHostLen = 0;
    BOOL_T bExistEnd = BOOL_FALSE;

    /* 外面已经判断第一个字符为'[',这里直接从后面一个开始 */
    pcHostCur++;

    /* 结尾处一定是']' */
    while (pcHostCur < pcStrEnd)
    {
        if (']' == *pcHostCur)
        {
            bExistEnd = BOOL_TRUE;
            break;
        }        
        pcHostCur++;
        ulHostLen++;
    }

    /* 存在结束符*/
    if (BOOL_TRUE == bExistEnd)
    {
        pcHostCur++;
        /* ']'后面一定是':'或'/'或'\0' */
        if ((':' == *pcHostCur) || ('/' == *pcHostCur) || ('\0' == *pcHostCur))
        {
            return ulHostLen;
        }        
    }
    
    return 0;
}

/*****************************************************************************
  Description: 将地址字符串转换为IP地址结构
      Caution: 转换后的地址为网络序
*****************************************************************************/
STATIC ULONG uri_acl_HostTransStr2Addr(IN USHORT usFamily,
                                        IN const CHAR *pcIPStr, 
                                        IN ULONG ulLen,
                                        OUT INET_ADDR_S *pstAddr)
{
    ULONG ulMaxLen;
    ULONG ulRet = ERROR_FAILED;
    CHAR szIPStr[INET6_ADDRSTRLEN + 1];

    if (AF_INET == usFamily)
    {
        ulMaxLen = INET_ADDRSTRLEN;
    }
    else
    {
        ulMaxLen = INET6_ADDRSTRLEN;
    }

    if (ulLen > ulMaxLen)
    {
        return ERROR_FAILED;
    }    

    memcpy(szIPStr, pcIPStr, ulLen);
    szIPStr[ulLen] = '\0';
    memset(pstAddr, 0, sizeof(INET_ADDR_S)); 
    ulRet = INET_ADDR_Str2IP(usFamily, szIPStr, pstAddr); 

    return ulRet;
}

/*****************************************************************************
  Description: 将掩码字符串转换为数值
*****************************************************************************/
STATIC inline ULONG uri_acl_HostTransStr2Mask(IN USHORT usFamily,
                                        IN const CHAR *pcMaskStr, 
                                        IN ULONG ulLen,
                                        OUT ULONG *pulMask)
{
    LONG lRet;
    CHAR szTmp[4];
    ULONG ulMaskNum;
    ULONG ulMaskLen;

    if (AF_INET == usFamily)
    {
        ulMaskLen = 2;
    }
    else
    {
        ulMaskLen = 3;
    }
    
    if (ulLen > ulMaskLen)
    {
        return ERROR_FAILED;
    }    

    memcpy(szTmp, pcMaskStr, ulLen);
    szTmp[ulLen] = '\0';
    lRet = sscanf(szTmp, "%lu", &ulMaskNum);
    if (1 > lRet)
    {
        return ERROR_FAILED;
    }
    
    *pulMask = ulMaskNum;
    return ERROR_SUCCESS;
}

/*****************************************************************************
  Description: 获取地址对应的地址
*****************************************************************************/
STATIC inline VOID uri_acl_HostTransIPv6Mask(IN const IN6ADDR_S *pstAddr, 
                                       IN const IN6ADDR_S *pstMask, 
                                       OUT IN6ADDR_S *pstOutAddr)
{
    UINT *puiPrefix;
    const UINT *puiAddr;
    const UINT *puiMask;
    
    puiPrefix = pstOutAddr->net_s6_addr32;
    puiAddr = pstAddr->net_s6_addr32;
    puiMask = pstMask->net_s6_addr32;

    puiPrefix[0] = puiAddr[0] | (~puiMask[0]);
    puiPrefix[1] = puiAddr[1] | (~puiMask[1]);
    puiPrefix[2] = puiAddr[2] | (~puiMask[2]);
    puiPrefix[3] = puiAddr[3] | (~puiMask[3]);
    
    return;
}

/*****************************************************************************
  Description: 根据地址和掩码计算地址段
*****************************************************************************/
STATIC inline VOID uri_acl_HostMaskTrans2AddrGrp(IN const INET_ADDR_S *pstAddr, 
                                           IN ULONG ulMaskNum,
                                           OUT INET_ADDR_S *pstStartAddr,
                                           OUT INET_ADDR_S *pstEndAddr)
{    
    UINT uiV4Mask; 
    UINT uiIPv4Addr;
    UINT uiIPv4StartAddr;
    UINT uiIPv4EndAddr;
    USHORT usFamily;
    INET_ADDR_S stAddrMask;
    

    *pstStartAddr = *pstAddr;
    *pstEndAddr = *pstAddr;

    usFamily = pstAddr->usFamily;
    if (AF_INET == usFamily)
    {
        /* 根据长度获取掩码 */
        INADDR_LenToMask((UINT)ulMaskNum, &uiV4Mask);
        uiIPv4Addr = ntohl(INET_ADDR_IP4ADDRUINT(pstAddr));

        /* 转换起始地址 */                 
        uiIPv4StartAddr = (uiIPv4Addr & uiV4Mask);
        INET_ADDR_IP4ADDRUINT(pstStartAddr) = htonl(uiIPv4StartAddr);
        
        /* 转换终止地址 */
        uiIPv4EndAddr = (uiIPv4Addr | (~uiV4Mask));
        INET_ADDR_IP4ADDRUINT(pstEndAddr) = htonl(uiIPv4EndAddr);
    }
    else
    {
        /* 根据长度获取掩码 */
        memset(&stAddrMask, 0, sizeof(stAddrMask));
        IN6ADDR_LenToMask((UINT)ulMaskNum, &INET_ADDR_IP6ADDR(&stAddrMask));

        IN6ADDR_GetPrefix(&INET_ADDR_IP6ADDR(pstAddr),
                          &INET_ADDR_IP6ADDR(&stAddrMask),
                          &INET_ADDR_IP6ADDR(pstStartAddr));

        uri_acl_HostTransIPv6Mask(&INET_ADDR_IP6ADDR(pstAddr),
                            &INET_ADDR_IP6ADDR(&stAddrMask),
                            &INET_ADDR_IP6ADDR(pstEndAddr));
    }
            
    return;
}

/*****************************************************************************
  Description: 创建ip地址组节点
*****************************************************************************/
STATIC inline URI_ACL_IPGROUP_S *uri_acl_HostCreateAddrGrpNode(IN const INET_ADDR_S *pstAddrStart, 
                                                         IN const INET_ADDR_S *pstAddrEnd)
{
    URI_ACL_IPGROUP_S *pstAddrGrp;

    /* 分配URI_ACL_IPGROUP_S */
    pstAddrGrp = MEM_ZMalloc(sizeof(URI_ACL_IPGROUP_S));
    if (NULL != pstAddrGrp)
    {
        pstAddrGrp->stAddrStart = *pstAddrStart;
        pstAddrGrp->stAddrStop  = *pstAddrEnd;
    }

    return pstAddrGrp;
}

/****************************************************************************
  Description: 保存ip地址组
*****************************************************************************/
STATIC inline ULONG uri_acl_HostSaveAddrGrp(IN const INET_ADDR_S *pstAddrStart, 
                                      IN const INET_ADDR_S *pstAddrEnd,
                                      INOUT URI_ACL_HOST_S *pstHost)
{
    URI_ACL_IPGROUP_S *pstAddrGrp;
    DLL_HEAD_S* pstList;
    ULONG ulRet = ERROR_FAILED;
    
    /* 创建IP group节点 */
    pstAddrGrp = uri_acl_HostCreateAddrGrpNode(pstAddrStart, pstAddrEnd);
    if (NULL != pstAddrGrp)
    {
        /* 加入到rule下地址组链表中 */
        pstList = &(pstHost->uIpList);
        DLL_ADD(pstList, &(pstAddrGrp->stNode));
        ulRet = ERROR_SUCCESS;
    }

    return ulRet;
}

/*****************************************************************************
   1)单个地址 192.168.1.1
   2)地址段   192.168.3.3-192.168.4.4
   3)掩码地址 192.168.2.0/24      
*****************************************************************************/
STATIC inline ULONG uri_acl_HostProcAddrGrpStr
(
    IN USHORT usFamily, 
    IN const CHAR *pcIPGrp, 
    IN ULONG ulLen,
    INOUT URI_ACL_HOST_S * pstHost
)
{
    const CHAR *pcIPStrCur = pcIPGrp;
    const CHAR *pcIPStrEnd = pcIPGrp + ulLen;    
    BOOL_T bIsIPGrp = BOOL_FALSE;
    BOOL_T bIsIPMask = BOOL_FALSE;
    ULONG ulRet = ERROR_SUCCESS;
    ULONG ulCurLen;
    ULONG ulStartAddrLen;
    ULONG ulEndAddrLen;
    ULONG ulMaskLen;
    INET_ADDR_S stStartAddr;
    INET_ADDR_S stEndAddr; 
    ULONG ulMaskNum = 0;
    
    /* 是地址段或者掩码 */
    memset(&stStartAddr, 0, sizeof(stStartAddr));
    memset(&stEndAddr, 0, sizeof(stEndAddr));
    ulCurLen = 0;
    ulStartAddrLen = ulLen;
    while (pcIPStrCur < pcIPStrEnd)
    {
        if ('-' == *pcIPStrCur)
        {
            bIsIPGrp = BOOL_TRUE;            
            /* 转换终止地址 */
            ulStartAddrLen = ulCurLen;
            pcIPStrCur++;
            ulEndAddrLen = ulLen - (ulCurLen + 1);
            ulRet = uri_acl_HostTransStr2Addr(usFamily, pcIPStrCur, ulEndAddrLen, &stEndAddr);            
            break;
        } 
        else if ('/' == *pcIPStrCur)
        {
            bIsIPMask = BOOL_TRUE;
            /* 转换掩码数值 */
            ulStartAddrLen = ulCurLen;
            pcIPStrCur++;
            ulMaskLen = ulLen - (ulCurLen + 1);
            ulRet = uri_acl_HostTransStr2Mask(usFamily, pcIPStrCur, ulMaskLen, &ulMaskNum);
            break;
        }
        pcIPStrCur++;
        ulCurLen++;
    }

    if (ERROR_SUCCESS != ulRet)
    {
        return ERROR_FAILED;
    }

    /* 转换起始地址 */
    ulRet = uri_acl_HostTransStr2Addr(usFamily, pcIPGrp, ulStartAddrLen, &stStartAddr);
    if (ERROR_SUCCESS != ulRet)
    {
        return ERROR_FAILED;
    }

    /* 掩码方式 */
    if (BOOL_TRUE == bIsIPMask)
    {
        /* 根据地址和掩码计算地址范围 */
        uri_acl_HostMaskTrans2AddrGrp(&stStartAddr, ulMaskNum, &stStartAddr, &stEndAddr);
    }
    else
    {
        /* 单一地址 */
        if (BOOL_TRUE != bIsIPGrp)
        {
            stEndAddr = stStartAddr;
        }
    }

    /* 创建节点并添加到链表中 */
    ulRet = uri_acl_HostSaveAddrGrp(&stStartAddr, &stEndAddr, pstHost);         

    return ulRet;
}

/*****************************************************************************
  Description: 解析IP地址
      Caution: 配置的IPv4地址有如下形式及组合
               192.168.1.1;
               192.168.2.2,192.3.4.4;
               192.168.3.3-192.168.4.4;
               192.168.2.0/24;
               192.168.2.0/24,192.168.3.0/24;
               [1234:5678::2]
*****************************************************************************/
STATIC ULONG uri_acl_HostParseAddr(IN USHORT usFamily,
                             IN const CHAR *pcIPGrp, 
                             IN ULONG ulHostNameLen,
                             INOUT URI_ACL_HOST_S * pstHost)
{      
    ULONG ulRet = ERROR_SUCCESS;
    ULONG ulCurLen;
    const CHAR *pcIPGrpCur = pcIPGrp;
    const CHAR *pcIPGrpEnd = pcIPGrp + ulHostNameLen;
    const CHAR *pcIPGrpTmp;

    DLL_INIT(&(pstHost->uIpList));

    /* 根据分隔符判断地址组 */
    ulCurLen = 0;
    pcIPGrpTmp  = pcIPGrp;
    while (pcIPGrpCur < pcIPGrpEnd)
    {
        if (',' == *pcIPGrpCur)
        {
            /* 解析分隔符之前的地址组 */
            ulRet = uri_acl_HostProcAddrGrpStr(usFamily, pcIPGrpTmp, ulCurLen, pstHost);
            if (ERROR_SUCCESS != ulRet)
            {
                break;
            }
            ulCurLen = 0; /* 下一组地址长度先清零 */
            pcIPGrpCur++; /* 跳过分隔符 */
            pcIPGrpTmp = pcIPGrpCur; /* 更新下一组的起始位置 */
        } 
        else
        {
            pcIPGrpCur++;
            ulCurLen++;
        }        
    }

    if (ERROR_SUCCESS != ulRet)
    {
        return ERROR_FAILED;
    }

    /* 解析最后一组地址 */
    ulRet = uri_acl_HostProcAddrGrpStr(usFamily, pcIPGrpTmp, ulCurLen, pstHost);
    
    return ulRet;
}

/*****************************************************************************
  Description: 是否是地址字符串的终止处
      Caution: ip://192.168.100.123/24/path2
                                   ^
               ip://192.168.100.123/abc/path2   
                                   ^
*****************************************************************************/
STATIC inline BOOL_T uri_acl_HostIsStrEnd(IN const CHAR *pcStr)
{
    const CHAR *pcNext;
    CHAR szTmp[4];
    ULONG ulMaskNum;
    BOOL_T bIsEnd = BOOL_TRUE;

    /* 1、判断'/'后的第一个字符 */
    /* '/'后面一个字符只可能是数字、字符、结束符,非数字时一定是地址段的终止 */
    pcNext = pcStr + 1;
    if (!isdigit(*pcNext))
    {
        return bIsEnd;
    }

    /* '/'后第一个字符为数字,保存 */
    szTmp[0] = *pcNext; 

    /* 2、判断'/'后的第二个字符 */
    pcNext++;
    /* 有可能为数字、字符、结束符 */
    /* 如果为'/'或者'\0'，则说明'/'后面接着的是掩码 */
    if (('/' == *pcNext) || ('\0' == *pcNext))
    {
        return BOOL_FALSE;
    }
    /* 如果为':'或者',',则说明'/'后面是掩码 */
    else if ((':' == *pcNext) || (',' == *pcNext))
    {
        return BOOL_FALSE;      
    }
    /* 如果为普通字符，则说明'/'是分隔符，后面接着的是path */
    else if (!isdigit(*pcNext))
    {
        return bIsEnd;
    }
    
    /* 如果为数字，先保存第二个数字，然后继续判断 */
    szTmp[1] = *pcNext; 
    szTmp[2] = '\0';

    /* 3、判断'/'后的第三个字符 */
    pcNext++;
    /* 有可能为数字、字符、结束符 */     
    /* 如果为数字，则说明'/'后面接着的是path */
    if (isdigit(*pcNext))
    {
        return bIsEnd;
    } 
    /* 如果为':'或者',',则说明'/'后面是掩码 */
    else if ((':' == *pcNext) || (',' == *pcNext))
    {
        return BOOL_FALSE;      
    }
    /* 如果为'\0'或者'/'，可能有两种情况:1、数值>32的认为是path; 2、否则认为是掩码 */
    else if (('\0' == *pcNext) || ('/' == *pcNext))
    {
        (VOID)sscanf(szTmp, "%lu", &ulMaskNum);
        if (32 < ulMaskNum)
        {
            return bIsEnd;
        }
        else
        {
            return BOOL_FALSE;
        }       
    }
    /* 其他字符, 则说明'/'后面接着的是path */
    else
    {
        return bIsEnd;
    }          
}

/*****************************************************************************
  Description: 获取hostname字符串长度
      Caution: ip://192.168.100.123/21/path2
               Q:如何区分'/'后面是一个地址掩码长度，还是路径呢
               A:地址后面如果是/1-32/的数字就认为是子网掩码,否则认为是路径
               如果确实想在地址后面直接跟1-32数字的路径，
               需要把端口号加上,或者使用地址范围，
                  例如 http://192.168.100.123:80/21/path2
                       http://192.168.100.1-192.168.100.254/21/path2
*****************************************************************************/
STATIC inline ULONG uri_acl_HostGetNameLen(IN LSTR_S *pstPattern)
{
    const CHAR *pcHostPre = pstPattern->pcData - 1;
    const CHAR *pcHostCur = pstPattern->pcData;
    const CHAR *pcStrEnd = pstPattern->pcData + pstPattern->uiLen;
    ULONG ulHostLen = 0;
    BOOL_T bIsEnd;
    BOOL_T bIsScope = FALSE;/* 是否范围方式 */

    if (*pcHostCur == '/')
    {
        return 0;
    }

    /* hostnaem结尾处一定是':'、'/'或者结束符 */
    while (pcHostCur <= pcStrEnd)
    {
        if ((':' == *pcHostCur) || ('\0' == *pcHostCur))
        {
            break;
        }
        else if ('-' == *pcHostCur)
        {
            bIsScope = TRUE;
        }
        else if (',' == *pcHostCur)
        {
            bIsScope = FALSE;
        }
        /* 需要判断'/'后面的数值 */
        else if ('/' == *pcHostCur)
        {
            if (bIsScope == TRUE)
            {
                break;
            }
            if (!isdigit(*pcHostPre))
            {
                break;
            }
            bIsEnd = uri_acl_HostIsStrEnd(pcHostCur);
            if (BOOL_TRUE == bIsEnd)
            {
                break;
            }
        }
        
        pcHostCur++;
        pcHostPre++;
        ulHostLen++;
    }
    
    return ulHostLen;
}

/*****************************************************************************
  Description: 是否是简单字符串
      Caution: 通配符包括"*"、"?"、"%" 
*****************************************************************************/
STATIC inline BOOL_T uri_acl_IsSimpStr(IN const CHAR *pcStrStart, IN const CHAR *pcStrEnd)
{
    BOOL_T bIsSimpStr = BOOL_TRUE;

    while (pcStrStart < pcStrEnd)
    {
        if ((*pcStrStart == '*') || (*pcStrStart == '?') || (*pcStrStart == '%'))
        {
            bIsSimpStr = BOOL_FALSE;
            break;
        }
        pcStrStart++;
    }
    
    return bIsSimpStr;
}

/*****************************************************************************
  Description: 解析输入的字符类型
*****************************************************************************/
STATIC inline URI_ACL_CHAR_TYPE_E uri_acl_GetCharType(IN CHAR cChar)
{
    URI_ACL_CHAR_TYPE_E enType;
    
    switch(cChar)
    {
        /* 通配符 */
        case '*':
        case '?':
        case '%':
        {
            enType = URI_ACL_CHAR_WILDCARD;
            break;
        }
        /* 需要转义的字符 */
        case '.':
        {
            enType = URI_ACL_CHAR_NEEDTRANSFER;
            break;
        }
        /* 普通字符 */
        default:
        {
            enType = URI_ACL_CHAR_NORMAL;
            break;
        }
    }
    
    return enType;
}

/*****************************************************************************
  Description: 转换通配符为正则的形式
*****************************************************************************/
STATIC inline CHAR *uri_acl_TransWildCard2Reg(IN CHAR cChar)
{
    CHAR *pcString = "";
    
    if (cChar == '*')
    {
        pcString = ".*";
    }
    else if (cChar == '?')
    {
        pcString = ".";
    }
    else /* cChar == '%' */
    {
        pcString = "[^./]*";
    }
    
    return pcString;
}

/*****************************************************************************
  Description: pcre 编译处理
*****************************************************************************/
STATIC VOID uri_acl_pcre_Compile(IN const CHAR *pcPattern, IN INT iOptions, OUT URI_ACL_PCRE_S *pstPcre)
{
    const CHAR* pcError = NULL;
    INT iErrOffset = 0;

    if ((NULL == pcPattern) || (0 == pcPattern[0]))
    {
        return;
    }

    pstPcre->pstReg = pcre_compile(pcPattern, iOptions, &pcError, &iErrOffset, NULL);
    if (NULL != pstPcre->pstReg)
    {

        #ifndef PCRE_STUDY_EXTRA_NEEDED
        #define PCRE_STUDY_EXTRA_NEEDED 0
        #endif

#if (PCRE_MAJOR >= 8)
        pstPcre->pstExtraData = pcre_study((pcre*)pstPcre->pstReg, PCRE_STUDY_EXTRA_NEEDED, &pcError);
#endif

    }
    return;
}

/*****************************************************************************
  Description: pcre 高级编译不区分大小写
*****************************************************************************/
VOID URI_ACL_KPCRE_Compile2(IN const CHAR* pcPattern, OUT URI_ACL_PCRE_S *pstPcre)
{
    uri_acl_pcre_Compile(pcPattern, PCRE_NEWLINE_ANY | PCRE_DOTALL | PCRE_CASELESS, pstPcre);
    return;
}

/*****************************************************************************
  Description: 正则释放
*****************************************************************************/
VOID URI_ACL_KPCRE_Free(IN URI_ACL_PCRE_S *pstPcre)
{
    if (NULL != pstPcre->pstReg)
    {
        pcre_free((pcre*)pstPcre->pstReg);
        pstPcre->pstReg = NULL;
    }

#if (PCRE_MAJOR >= 8)
    if (NULL != pstPcre->pstExtraData)
    {
        pcre_free_study((pcre_extra*)pstPcre->pstExtraData);
        pstPcre->pstExtraData = NULL;
    }
#endif

    return;
}

/*****************************************************************************
  Description: 正则查找
       Return: 匹配的个数
*****************************************************************************/
INT URI_ACL_KPCRE_Exec(IN const URI_ACL_PCRE_S *pstPcre, 
                       IN const UCHAR* pucStr, 
                       IN INT iLeng, 
                       IN INT iOffsetCount, 
                       OUT INT *piOffsets)
{
    return pcre_exec((pcre*)pstPcre->pstReg, 
                     (pcre_extra*)pstPcre->pstExtraData, 
                     (CHAR*)pucStr, iLeng, 0, PCRE_NEWLINE_ANY, 
                     piOffsets, iOffsetCount);
}

/*****************************************************************************
  Description: 解析模式字符串
*****************************************************************************/
STATIC ULONG uri_acl_ProcPattern(IN const CHAR *pcStrCur, IN const CHAR *pcStrEnd, OUT URI_ACL_PATTERN_S *pstPattern)
{
    CHAR *pcRegString; 
    CHAR *pcPattern;
    UINT uiOffset;
    URI_ACL_CHAR_TYPE_E enCharType;  
    URI_ACL_PCRE_S *pstPcre = &(pstPattern->stPcre);

    pcPattern = MEM_ZMalloc(URI_ACL_PATTERN_BUF_MAX);
    if (NULL == pcPattern)
    {
        return ERROR_FAILED;
    }
    
    /* 通配符模式 */
    uiOffset = 0;
    while (pcStrCur < pcStrEnd)
    {
        /* 解析输入的字符类型 */
        enCharType = uri_acl_GetCharType(*pcStrCur);
        
        /* 通配符 */
        if (URI_ACL_CHAR_WILDCARD == enCharType)
        {            
            /* 转换为正则形式 */
            pcRegString = uri_acl_TransWildCard2Reg(*pcStrCur);
            uiOffset += (UINT)snprintf(pcPattern + uiOffset, URI_ACL_PATTERN_BUF_MAX - uiOffset, "%s", pcRegString);
        }        
        /* 需要转义的字符 */
        else if (URI_ACL_CHAR_NEEDTRANSFER == enCharType) 
        {
            uiOffset += (UINT)snprintf(pcPattern + uiOffset, URI_ACL_PATTERN_BUF_MAX - uiOffset, "\\%c", *pcStrCur);
        }  
        /* 普通字符 */
        else 
        {
            uiOffset += (UINT)snprintf(pcPattern + uiOffset, URI_ACL_PATTERN_BUF_MAX - uiOffset, "%c", *pcStrCur);
        }
        pcStrCur++;
    }

    pstPattern->pcPcreStr = pcPattern;
    
    URI_ACL_KPCRE_Compile2(pcPattern, pstPcre); 
        
    return ERROR_SUCCESS;    
}

/*****************************************************************************
  Description: 解析主机名
      Caution: 域名包含如下形式
               www.abc.com
               www.%.com/path1/
               *.domain.com/path1/%/path3/
               www.domain?.com:80,8080/
*****************************************************************************/
STATIC ULONG uri_acl_HostParseHostName(IN const CHAR *pcHost, 
                                 IN ULONG ulHostNameLen,
                                 INOUT URI_ACL_HOST_S *pstHost)
{
    const CHAR *pcDomainStart = pcHost;
    const CHAR *pcDomainEnd = pcDomainStart + ulHostNameLen; 
    URI_ACL_PATTERN_S *pstHostDomain;    
    BOOL_T bIsSimpStr; 
    ULONG ulRet = ERROR_SUCCESS;
    
    pstHostDomain = &(pstHost->uHostDomain);    
    bIsSimpStr = uri_acl_IsSimpStr(pcDomainStart, pcDomainEnd); 
    /* 简单字符串方式，精确匹配 */
    if (BOOL_TRUE == bIsSimpStr)
    {
        pstHostDomain->enType = URI_ACL_PATTERN_STRING;
        memcpy(pstHostDomain->szPattern, pcDomainStart, ulHostNameLen);
        pstHostDomain->szPattern[ulHostNameLen] = '\0';
    }
    /* 通配符模式 */
    else
    {        
        pstHostDomain->enType = URI_ACL_PATTERN_PCRE;        
        ulRet = uri_acl_ProcPattern(pcDomainStart, pcDomainEnd, pstHostDomain);          
    }                   
    
    return ulRet;
}

STATIC ULONG uri_acl_ParseHost(INOUT LSTR_S *pstPattern, INOUT URI_ACL_RULE_S *pstRule)
{    
    CHAR *pcHostStart = pstPattern->pcData;
    BOOL_T bIsIPv6AddrMode;
    BOOL_T bIsIPv4AddrMode;
    ULONG ulHostNameLen;
    URI_ACL_HOST_S * pstHost = &(pstRule->stHostPattern);
    ULONG ulRet = ERROR_FAILED;

    /* hostname是否为IPv6地址形式 */
    bIsIPv6AddrMode = uri_acl_HostIsAddr6Mode(*pstPattern->pcData);
    if (BOOL_TRUE == bIsIPv6AddrMode)
    {
        /* ipv6地址方式处理 */
        ulHostNameLen = uri_acl_HostGetAddr6Len(pstPattern);
        
        /* 跳过ipv6地址起始处的'[' */
        pcHostStart++;
        ulRet = uri_acl_HostParseAddr(AF_INET6, pcHostStart, ulHostNameLen, pstHost);
        
        /* 增加'['、']'的长度 */
        ulHostNameLen += 2;
        pstRule->uiCfgFlag |= URI_ACL_KEY_IPADDR;
        pstHost->enType = URI_ACL_HOST_IPADDR;
    }
    else
    {        
        ulHostNameLen = uri_acl_HostGetNameLen(pstPattern);        
        bIsIPv4AddrMode = Socket_N_IsIPv4(pcHostStart, ulHostNameLen);
       
        /* ipv4地址方式处理 */
        if (BOOL_TRUE == bIsIPv4AddrMode)
        {
            ulRet = uri_acl_HostParseAddr(AF_INET, pcHostStart, ulHostNameLen, pstHost);
            pstRule->uiCfgFlag |= URI_ACL_KEY_IPADDR;
            pstHost->enType = URI_ACL_HOST_IPADDR;
        }
        else /* 主机名方式处理 */
        {
            ulRet = uri_acl_HostParseHostName(pcHostStart, ulHostNameLen, pstHost); 
            pstRule->uiCfgFlag |= URI_ACL_KEY_DOMAIN;
            pstHost->enType = URI_ACL_HOST_DOMAIN;
        }
    }

    if (ERROR_SUCCESS != ulRet)
    {
        return ERROR_FAILED;
    }

    pstPattern->pcData += ulHostNameLen;
    pstPattern->uiLen -= ulHostNameLen;

    return ERROR_SUCCESS;    
}

/*****************************************************************************
  Description: 获取port字符串长度
*****************************************************************************/
STATIC inline ULONG uri_acl_PortGetPortStrLen(IN LSTR_S *pstPattern)
{
    CHAR *pcTmp = pstPattern->pcData;
    CHAR *pcEnd = pstPattern->pcData + pstPattern->uiLen;
    UINT uiLen = 0;

    /* port结尾处一定是'/'或者结束符 */
    while (pcTmp < pcEnd)
    {
        if (('/' == *pcTmp) || ('\0' == *pcTmp))
        {
            break;
        }
        uiLen++;
        pcTmp ++;
    }
    
    return uiLen;
}

/*****************************************************************************
  Description: 将端口字符串转换为数值
*****************************************************************************/
STATIC inline ULONG uri_acl_PortTransStr2PortNum(IN const CHAR *pcPortStr, 
                                           IN ULONG ulLen,
                                           OUT USHORT *pusPortNum)
{
    ULONG ulRet = ERROR_FAILED;
    ULONG ulNum;
    CHAR szTmp[URI_ACL_PORTNUM_MAX_LEN + 1];
    LONG lRet;

    if (URI_ACL_PORTNUM_MAX_LEN >= ulLen)
    {
        memcpy(szTmp, pcPortStr, ulLen);
        szTmp[ulLen] = '\0';
        lRet = sscanf(szTmp, "%lu", &ulNum);
        if (1 > lRet)
        {
            return ulRet;
        }
        *pusPortNum = (USHORT)ulNum;
        ulRet = ERROR_SUCCESS;
    }

    return ulRet;
}

/*****************************************************************************
  Description: 创建port组节点
*****************************************************************************/
STATIC inline URI_ACL_PORTGROUP_S *uri_acl_PortCreatePortGrpNode(IN USHORT usPortStart, IN USHORT usPortEnd)
{
    URI_ACL_PORTGROUP_S *pstPortGrp;

    /* 分配URI_ACL_PORTGROUP_S */
    pstPortGrp = MEM_ZMalloc(sizeof(URI_ACL_PORTGROUP_S));
    if (NULL != pstPortGrp)
    {
        pstPortGrp->usPortStart = usPortStart;
        pstPortGrp->usPortEnd   = usPortEnd;
    }

    return pstPortGrp;
}

/*****************************************************************************
  Description: 保存port组
*****************************************************************************/
STATIC inline ULONG uri_acl_PortSavePortGrp(IN USHORT usPortStart, 
                                      IN USHORT usPortEnd,
                                      INOUT URI_ACL_RULE_S *pstRule)
{
    URI_ACL_PORTGROUP_S *pstPortGrp;
    DLL_HEAD_S* pstList;
    ULONG ulRet = ERROR_FAILED;
    
    /* 创建port group节点 */
    pstPortGrp = uri_acl_PortCreatePortGrpNode(usPortStart, usPortEnd);
    if (NULL != pstPortGrp)
    {
        /* 加入到rule下port组链表中 */
        pstList = &(pstRule->stPortGroupList);
        DLL_ADD(pstList, &(pstPortGrp->stNode));
        ulRet = ERROR_SUCCESS;
    }

    return ulRet;
}

/*****************************************************************************
  Description: 解析一段端口字符串
      Caution: 存在如下情况:
               1)单个端口 8080
               2)端口范围 8080-8088   
*****************************************************************************/
STATIC inline ULONG uri_acl_PortProcPortGrpStr(IN const CHAR *pcPortGrp, 
                                         IN ULONG ulLen,
                                         INOUT URI_ACL_RULE_S *pstRule)
{
    const CHAR *pcPortCur = pcPortGrp;
    const CHAR *pcPortEnd = pcPortGrp + ulLen;    
    BOOL_T bIsPortGrp = BOOL_FALSE;
    ULONG ulRet = ERROR_SUCCESS;
    ULONG ulPortCurLen;
    ULONG ulPortStartLen;
    ULONG ulPortEndLen;
    USHORT usStartPort = 0;
    USHORT usEndPort = 0;

    ulPortCurLen = 0;
    ulPortStartLen = ulLen;
    while (pcPortCur < pcPortEnd)
    {
        if ('-' == *pcPortCur)
        {                        
            /* 转换终止端口 */
            bIsPortGrp = BOOL_TRUE;
            ulPortStartLen = ulPortCurLen;
            pcPortCur++;
            ulPortEndLen = ulLen - (ulPortCurLen + 1);
            ulRet = uri_acl_PortTransStr2PortNum(pcPortCur, ulPortEndLen, &usEndPort);            
            break;
        } 
        pcPortCur++;
        ulPortCurLen++;
    }

    if (ERROR_SUCCESS != ulRet)
    {
        return ERROR_FAILED;
    }

    /* 转换起始端口 */
    ulRet = uri_acl_PortTransStr2PortNum(pcPortGrp, ulPortStartLen, &usStartPort);
    if (ERROR_SUCCESS == ulRet)
    {
        /* 单一端口 */
        if (BOOL_TRUE != bIsPortGrp)
        {
            usEndPort = usStartPort; 
        }
        /* 创建节点并添加到链表中 */
        ulRet = uri_acl_PortSavePortGrp(usStartPort, usEndPort, pstRule);
    }       

    return ulRet;
}

/*****************************************************************************
  Description: 解析端口
*****************************************************************************/
STATIC ULONG uri_acl_ParsePort(IN LSTR_S *pstPattern, INOUT URI_ACL_RULE_S *pstRule)
{    
    const CHAR *pcPortStr = pstPattern->pcData;
    const CHAR *pcPortEnd;
    const CHAR *pcPortTmp;
    ULONG ulPortNameLen;
    ULONG ulRet = ERROR_SUCCESS;
    ULONG ulCurLen;

    /* 获取端口字符串长度 */
    ulPortNameLen = uri_acl_PortGetPortStrLen(pstPattern);

    /* 根据分隔符判断端口组 */
    ulCurLen = 0;
    pcPortTmp = pcPortStr;
    pcPortEnd = pcPortStr + ulPortNameLen;
    while (pcPortStr < pcPortEnd)
    {
        if (',' == *pcPortStr)
        {
            /* 解析分隔符之前的port */
            ulRet = uri_acl_PortProcPortGrpStr(pcPortTmp, ulCurLen, pstRule);
            if (ERROR_SUCCESS != ulRet)
            {
                break;
            }
            ulCurLen = 0; /* 下一组长度先清零 */
            pcPortStr++;  /* 跳过分隔符 */
            pcPortTmp = pcPortStr; /* 更新下一组的起始位置 */
        } 
        else
        {
            pcPortStr++;
            ulCurLen++;
        }        
    }

    if (ERROR_SUCCESS != ulRet)
    {
        return ERROR_FAILED;
    }

    /* 解析最后一组port */
    ulRet = uri_acl_PortProcPortGrpStr(pcPortTmp, ulCurLen, pstRule);
    if (ERROR_SUCCESS == ulRet)
    {
        pstPattern->pcData += ulPortNameLen;
        pstPattern->uiLen -= ulPortNameLen;
        pstRule->uiCfgFlag |= URI_ACL_KEY_PORT;
    }

    return ulRet;    
}

/*****************************************************************************
  Description: 解析path
      Caution: path是从'/'开始的。如:
               http://www.%.com/path1/
                               ^ 
*****************************************************************************/
STATIC ULONG uri_acl_ParsePath(IN LSTR_S *pstPattern, INOUT URI_ACL_RULE_S *pstRule)
{    
    ULONG ulPathLen = pstPattern->uiLen; /* path字符串长度 */
    BOOL_T bIsSimpStr;
    const CHAR *pcPathStr = pstPattern->pcData;
    const CHAR *pcPathEnd = pcPathStr + ulPathLen;      
    URI_ACL_PATTERN_S *pstPath;    
    ULONG ulRet = ERROR_SUCCESS;
    ULONG ulLen;

    /* 只存在'/' */
    if (pstPattern->uiLen == 1)
    {
        return ulRet;
    }

    pstPath = &(pstRule->stPathPattern);
    bIsSimpStr = uri_acl_IsSimpStr(pcPathStr, pcPathEnd); 
    /* 简单字符串方式，精确匹配 */
    if (BOOL_TRUE == bIsSimpStr)
    {
        pstPath->enType = URI_ACL_PATTERN_STRING;
        memcpy(pstPath->szPattern, pcPathStr, ulPathLen);
        ulLen = ulPathLen;
        if ('/' != pcPathStr[ulPathLen - 1])
        {
            pstPath->szPattern[ulLen] = '/';
            ulLen++;
        }
        pstPath->szPattern[ulLen] = '\0';
    }
    /* 通配符模式 */
    else
    {        
        pstPath->enType = URI_ACL_PATTERN_PCRE; 
        ulRet = uri_acl_ProcPattern(pcPathStr, pcPathEnd, pstPath);       
    }

    pstRule->uiCfgFlag |= URI_ACL_KEY_PATH;
    
    return ulRet;    
}

static BS_STATUS uri_acl_ParsePattern(IN CHAR *pcPattern, INOUT URI_ACL_RULE_S *pstRule)
{
    ULONG ulRet;
    LSTR_S stPattern;

    /* 合法性检查 */
    if ((NULL == pcPattern) || (NULL == pstRule))
    {
        return BS_NULL_PARA;
    }

    stPattern.pcData = pcPattern;
    stPattern.uiLen = strlen(pcPattern);

    /* 解析Action */
    if (BS_OK != uri_acl_ParseAction(&stPattern, pstRule))
    {
        return BS_ERR;
    }

    /* 解析是否全网资源 "*" */
    if (stPattern.uiLen == 1 && (stPattern.pcData[0] == '*'))
    {
        pstRule->uiCfgFlag |= URI_ACL_KEY_MATCH_ALL;
        return BS_OK;
    }

    /* 解析协议 */
    ulRet = url_acl_ParseProtocol(&stPattern, pstRule);
    if (ERROR_SUCCESS != ulRet)
    {
        return BS_ERR;
    }

    /* 解析主机 */
    ulRet = uri_acl_ParseHost(&stPattern, pstRule);
    if (ERROR_SUCCESS != ulRet)
    {
        return BS_ERR;
    }
    
    /* 端口和路径不是必须有的 */
    if (stPattern.uiLen == 0)
    {
        return BS_OK;
    }

    if (':' == *stPattern.pcData)
    {
        /* 解析端口 */
        stPattern.pcData ++;
        stPattern.uiLen --;
        ulRet = uri_acl_ParsePort(&stPattern, pstRule);
        if (ERROR_SUCCESS != ulRet)
        {
            return BS_ERR;
        }        
        if (stPattern.uiLen == 0)
        {
            return BS_OK;
        }
    }    
    
    /* 解析路径 */
    ulRet = uri_acl_ParsePath(&stPattern, pstRule);
    
    return ulRet; 
}

STATIC URI_ACL_RULE_S * uri_acl_AllocRule()
{
    URI_ACL_RULE_S *pstRule;
    
    /* 分配URI_ACL_KNODE_S */
    pstRule = MEM_ZMalloc(sizeof(URI_ACL_RULE_S));
    if (NULL != pstRule)
    {
        DLL_INIT(&(pstRule->stPortGroupList));
        DLL_INIT(&(pstRule->stHostPattern.uIpList));
    }          
    
    return pstRule;
}

STATIC VOID uri_acl_FreeHost(IN URI_ACL_RULE_S *pstRule)
{
    URI_ACL_HOST_S *pstHost;
    URI_ACL_PATTERN_S *pstHostDomain;
    DLL_HEAD_S *pstList;
    URI_ACL_IPGROUP_S *pstNextNode = NULL;
    URI_ACL_IPGROUP_S *pstIpGrp;
        
    DBGASSERT(NULL != pstRule);
        
    /* 域名方式 */
    pstHost = &(pstRule->stHostPattern);
    if (URI_ACL_HOST_DOMAIN == pstHost->enType)
    {                
        /* 正则方式 */
        pstHostDomain = &(pstHost->uHostDomain);
        if (URI_ACL_PATTERN_PCRE == pstHostDomain->enType)
        {
            if (NULL != pstHostDomain->pcPcreStr)
            {
                MEM_Free(pstHostDomain->pcPcreStr);
            }
            URI_ACL_KPCRE_Free(&(pstHostDomain->stPcre));
        }
    }
    else /* ip地址方式 */
    {                
        /* 删除rule下的所有ip node */
        pstList = &(pstHost->uIpList);
        DLL_SAFE_SCAN(pstList, pstIpGrp, pstNextNode)
        {
            /* 摘链 */
            DLL_DEL(pstList, &(pstIpGrp->stNode));
            /* 删除 */
            MEM_Free(pstIpGrp);
        }
    }
        
    return;
}

STATIC VOID uri_acl_FreePortGroup(IN URI_ACL_RULE_S *pstRule)
{
    DLL_HEAD_S *pstList;
    URI_ACL_PORTGROUP_S *pstNode = NULL;
    URI_ACL_PORTGROUP_S *pstNextNode = NULL;
    
    DBGASSERT(NULL != pstRule);

    /* 删除rule下的所有port node */
    pstList = &(pstRule->stPortGroupList);
    DLL_SAFE_SCAN(pstList, pstNode, pstNextNode)
    {
        /* 摘链 */
        DLL_DEL(pstList, &(pstNode->stNode));
        /* 删除 */
        MEM_Free(pstNode);
    }

    return;
}

STATIC VOID uri_acl_FreePath(IN URI_ACL_RULE_S *pstRule)
{
    URI_ACL_PATTERN_S *pstPath;
    
    DBGASSERT(NULL != pstRule);

    /* 正则方式 */
    pstPath = &(pstRule->stPathPattern);
    if (URI_ACL_PATTERN_PCRE == pstPath->enType)
    {
        if (NULL != pstPath->pcPcreStr)
        {
            MEM_Free(pstPath->pcPcreStr);
        }
        URI_ACL_KPCRE_Free(&(pstPath->stPcre));
    }
    
    return;
}

STATIC VOID uri_acl_FreeRule(IN URI_ACL_RULE_S *pstRule)
{
    DBGASSERT(NULL != pstRule);

    /* 释放host */
    uri_acl_FreeHost(pstRule);

    /* 释放portgroup */
    uri_acl_FreePortGroup(pstRule);

    /* 释放path */
    uri_acl_FreePath(pstRule);
    
    /* 释放Rule */
    MEM_Free(pstRule);

    return;
}

/*****************************************************************************
  Description: 匹配协议
       Return: BOOL_TRUE   匹配
               BOOL_FALSE  不匹配
*****************************************************************************/
STATIC BOOL_T uri_acl_MatchProtocol(IN const URI_ACL_MATCH_INFO_S *pstMatchInfo, IN const URI_ACL_RULE_S *pstRule)
{
    BOOL_T bIsMatch = BOOL_FALSE;  

    if ((URI_ACL_PROTOCOL_IP == pstRule->enProtocol) ||
        (pstMatchInfo->enProtocol == pstRule->enProtocol))
    {
        bIsMatch = BOOL_TRUE;
    }    
    return bIsMatch;
}

/*****************************************************************************
  Description: 匹配地址
       Return: BOOL_TRUE   匹配
               BOOL_FALSE  不匹配
*****************************************************************************/
STATIC BOOL_T uri_acl_MatchAddr(IN const URI_ACL_MATCH_INFO_S *pstMatchInfo, IN URI_ACL_RULE_S *pstRule)
{
    BOOL_T bIsMatch = BOOL_FALSE; 
    URI_ACL_IPGROUP_S *pstIPGrp;
    const INET_ADDR_S *pstAddr;
    INET_ADDR_S *pstAddrStart;
    INET_ADDR_S *pstAddrEnd;
    INT iRet1;
    INT iRet2;

    pstAddr = &(pstMatchInfo->stAddr);
    
    /* 遍历地址group链表 */
    DLL_SCAN(&(pstRule->stHostPattern.uIpList), pstIPGrp)
    {
        pstAddrStart = &(pstIPGrp->stAddrStart);
        pstAddrEnd   = &(pstIPGrp->stAddrStop);
        if (((AF_INET == pstAddr->usFamily) && (AF_INET == pstIPGrp->stAddrStart.usFamily)) ||
            ((AF_INET6 == pstAddr->usFamily) && (AF_INET6 == pstIPGrp->stAddrStart.usFamily)))
        {
            iRet1 = INET_ADDR_Cmp(pstAddr, pstAddrStart);
            iRet2 = INET_ADDR_Cmp(pstAddr, pstAddrEnd);
            if ((iRet1 >= 0) && (iRet2 <= 0))
            {
                bIsMatch = BOOL_TRUE;
                break;
            }
        }        
    }
        
    return bIsMatch;
}

/*****************************************************************************
  Description: 匹配域名字符串
*****************************************************************************/
STATIC BOOL_T uri_acl_MatchDomainStr(IN const URI_ACL_MATCH_INFO_S *pstMatchInfo, IN const URI_ACL_RULE_S *pstRule)
{
    BOOL_T bIsMatch = BOOL_FALSE;
    const URI_ACL_PATTERN_S *pstPattern;
    const UCHAR *pucDnStr;
    INT iRet;
    INT aiOvector[2] = {-1,-1};
    
    pucDnStr = pstMatchInfo->szDomain;
    pstPattern = &(pstRule->stHostPattern.uHostDomain);

    /* 简单字符串 */
    if (URI_ACL_PATTERN_STRING == pstPattern->enType)
    {
        iRet = stricmp((CHAR *)pucDnStr, (CHAR *)pstPattern->szPattern);
        if (0 == iRet)
        {
            bIsMatch = BOOL_TRUE;
        }
    }    
    else /* 正则 */
    {
        iRet = URI_ACL_KPCRE_Exec(&(pstPattern->stPcre), 
                                  pucDnStr, 
                                  (INT)strlen((CHAR *)pucDnStr), 
                                  2, aiOvector);
        /* 能够匹配且从起始处完全匹配 */
        if ((iRet >= 0) && (0 == aiOvector[0]))
        {
            bIsMatch = BOOL_TRUE;
        }    
    }
        
    return bIsMatch;
}

/*****************************************************************************
  Description: 匹配主机
       Return: BOOL_TRUE   匹配
               BOOL_FALSE  不匹配
      Caution: URI-ACL不负责进行域名和ip地址的转换匹配
               如果有需要，业务转换后分别匹配
*****************************************************************************/
STATIC BOOL_T uri_acl_MatchHost(IN const URI_ACL_MATCH_INFO_S *pstMatchInfo, IN const URI_ACL_RULE_S *pstRule)
{
    BOOL_T bIsAddrMatch = BOOL_FALSE;
    BOOL_T bIsHostMatch = BOOL_FALSE;

    /* 地址方式 */
    if (BIT_MATCH(pstMatchInfo->uiFlag, URI_ACL_KEY_IPADDR))
    {
        /* 配置也为地址方式 */
        if (BIT_MATCH(pstRule->uiCfgFlag, URI_ACL_KEY_IPADDR))
        {
            bIsAddrMatch = uri_acl_MatchAddr(pstMatchInfo, (void*)pstRule);
        }
    } 
    
    /* 域名方式 */
    if (BIT_MATCH(pstMatchInfo->uiFlag, URI_ACL_KEY_DOMAIN))
    {
        /* 配置也为域名方式 */
        if (BIT_MATCH(pstRule->uiCfgFlag, URI_ACL_KEY_DOMAIN))
        {
            bIsHostMatch = uri_acl_MatchDomainStr(pstMatchInfo, pstRule);
        }
    }

    if ((BOOL_TRUE == bIsAddrMatch) || (BOOL_TRUE == bIsHostMatch))
    {
        return BOOL_TRUE;
    }
           
    return BOOL_FALSE;
}

/*****************************************************************************
  Description: 匹配端口
       Return: BOOL_TRUE   匹配
               BOOL_FALSE  不匹配
*****************************************************************************/
STATIC BOOL_T uri_acl_MatchPort(IN const URI_ACL_MATCH_INFO_S *pstMatchInfo, IN URI_ACL_RULE_S *pstRule)
{
    BOOL_T bIsMatch = BOOL_FALSE; 
    USHORT usPort;
    URI_ACL_PORTGROUP_S *pstPortGrp;

    usPort = pstMatchInfo->usPort;
    
    /* 遍历port group链表 */
    DLL_SCAN(&pstRule->stPortGroupList, pstPortGrp)
    {
        if ((pstPortGrp->usPortStart <= usPort) &&
            (pstPortGrp->usPortEnd   >= usPort))
        {
            bIsMatch = BOOL_TRUE;
            break;
        }
    }
        
    return bIsMatch;
}

/*****************************************************************************
  Description: 匹配字符串
       Return: BOOL_TRUE   匹配
               BOOL_FALSE  不匹配
      Caution: 1、 usr: http://1.2.3.1/path1/path2
                  rule: http://1.2.3.1/path1/
                    ----matched
               2、 usr: http://1.2.3.1/path1/
                  rule: http://1.2.3.1/path1
                    ----not-matched
               3、 usr: http://1.2.3.1/path313/ 或者
                        http://1.2.3.1/path313
                  rule: http://1.2.3.1/path
                    ----not-matched
*****************************************************************************/
STATIC BOOL_T uri_acl_MatchUsrStrGtPtnStr(IN const UCHAR *pucUsrString, IN const UCHAR *pucPtnString)
{
    ULONG ulPtnStrLen;
    INT iRet;

    ulPtnStrLen = strlen((CHAR *)pucPtnString);

    iRet = strncmp((CHAR *)pucUsrString, (CHAR *)pucPtnString, ulPtnStrLen);
    if (0 != iRet)
    {
        return FALSE;
    }

    if (pucPtnString[ulPtnStrLen - 1] != '/')
    {
        return FALSE;
    }
    
    return TRUE;
}

/*****************************************************************************
  Description: 匹配字符串
       Return: BOOL_TRUE   匹配
               BOOL_FALSE  不匹配
*****************************************************************************/
STATIC BOOL_T uri_acl_MatchSimpStr(IN const UCHAR *pucUsrString, IN const URI_ACL_PATTERN_S *pstPattern)
{
    BOOL_T bIsMatch = BOOL_FALSE;
    INT iRet;
    const UCHAR *pucPatStr;

    pucPatStr = pstPattern->szPattern;

    /* 要求path区分大小写 */
    iRet = strcmp((CHAR *)pucUsrString, (CHAR *)pucPatStr);
    if (0 == iRet)
    {
        bIsMatch = BOOL_TRUE;
    }
    else if (0 < iRet) /* pucUsrString长于pattern*/
    {   
        bIsMatch = uri_acl_MatchUsrStrGtPtnStr(pucUsrString, pucPatStr);
    }
    else /* pucUsrString短于pattern */
    {
        bIsMatch = BOOL_FALSE;
    }
    
    return bIsMatch;
}

/*****************************************************************************
  Description: 匹配字符串
       Return: BOOL_TRUE   匹配
               BOOL_FALSE  不匹配
*****************************************************************************/
STATIC BOOL_T uri_acl_MatchPath(IN const URI_ACL_MATCH_INFO_S *pstMatchInfo, IN const URI_ACL_RULE_S *pstRule)
{
    BOOL_T bIsMatch = BOOL_FALSE;
    const URI_ACL_PATTERN_S *pstPattern;
    const UCHAR *pucStr;
    INT iRet;
    INT aiOvector[2] = {-1,-1};
       
    pucStr = pstMatchInfo->szPath;
    pstPattern = &(pstRule->stPathPattern);

    /* 简单字符串 */
    if (URI_ACL_PATTERN_STRING == pstPattern->enType)
    {
        bIsMatch = uri_acl_MatchSimpStr(pucStr, pstPattern);
    }
    /* 正则 */
    else
    {
        iRet = URI_ACL_KPCRE_Exec(&(pstPattern->stPcre), 
                                  pucStr, 
                                  (INT)strlen((CHAR *)pucStr), 
                                  2, aiOvector);
        /* 能够匹配且从起始处完全匹配 */
        if ((iRet >= 0) && (0 == aiOvector[0]))
        {
            bIsMatch = BOOL_TRUE;
        }    
    }
        
    return bIsMatch;
}

/*****************************************************************************
  Description: 匹配rule
       Return: BOOL_TRUE   匹配
               BOOL_FALSE  不匹配
*****************************************************************************/
STATIC BOOL_T uri_acl_MatchByRule(IN const URI_ACL_MATCH_INFO_S *pstMatchInfo, IN const URI_ACL_RULE_S *pstRule)
{
    BOOL_T bIsMatch;

    if (pstRule->uiCfgFlag & URI_ACL_KEY_MATCH_ALL)
    {
        return BOOL_TRUE;
    }
    
    /* 匹配协议 */
    bIsMatch = uri_acl_MatchProtocol(pstMatchInfo, pstRule);
    if (BOOL_TRUE != bIsMatch)
    {
        return BOOL_FALSE;
    }

    /* 匹配主机 */
    bIsMatch = uri_acl_MatchHost(pstMatchInfo, pstRule);
    if (BOOL_TRUE != bIsMatch)
    {
        return BOOL_FALSE;
    }

    /* 匹配端口 */
    if (BIT_MATCH(pstRule->uiCfgFlag, URI_ACL_KEY_PORT))
    {
        if (!BIT_MATCH(pstMatchInfo->uiFlag, URI_ACL_KEY_PORT))
        {
            return BOOL_FALSE;
        }
        bIsMatch = uri_acl_MatchPort(pstMatchInfo, (void*)pstRule);
        if (BOOL_TRUE != bIsMatch)
        {
            return BOOL_FALSE;
        }
    }

    /* 匹配路径 */
    if (BIT_MATCH(pstRule->uiCfgFlag, URI_ACL_KEY_PATH))
    {
        if (!BIT_MATCH(pstMatchInfo->uiFlag, URI_ACL_KEY_PATH))
        {
            return BOOL_FALSE;
        }
        bIsMatch = uri_acl_MatchPath(pstMatchInfo, pstRule);
        if (BOOL_TRUE != bIsMatch)
        {
            return BOOL_FALSE;
        }
    }
    
    return BOOL_TRUE;
}

static VOID uri_acl_DeleteRule(IN void *pstRule, IN VOID *pUserHandle)
{
    URI_ACL_RULE_S *pstUriAclRule = container_of(pstRule, URI_ACL_RULE_S, stListRuleNode);

    uri_acl_FreeRule(pstUriAclRule);
}

LIST_RULE_HANDLE URI_ACL_Create()
{
    return ListRule_Create();
}

VOID URI_ACL_Destroy(IN LIST_RULE_HANDLE hCtx)
{
    ListRule_Destroy(hCtx, uri_acl_DeleteRule, NULL);

    return;
}

UINT URI_ACL_AddList(IN LIST_RULE_HANDLE hCtx, IN CHAR *pcListName)
{
    return ListRule_AddList(hCtx, pcListName);
}

VOID URI_ACL_DelList(IN LIST_RULE_HANDLE hCtx, IN UINT uiListID)
{
    ListRule_DelList(hCtx, uiListID, uri_acl_DeleteRule, NULL);
}

UINT URI_ACL_FindListByName(IN LIST_RULE_HANDLE hCtx, IN CHAR *pcListName)
{
    return ListRule_FindListByName(hCtx, pcListName);
}

BS_STATUS URI_ACL_AddRule(IN LIST_RULE_HANDLE hCtx, IN UINT uiListID, IN UINT uiRuleID, IN CHAR *pcRule)
{
    URI_ACL_RULE_S *pstRule;
    ULONG ulRet;

    if (strlen(pcRule) > URI_ACL_RULE_MAX_LEN)
    {
        return BS_OUT_OF_RANGE;
    }
    
    /* 创建rule */
    pstRule = uri_acl_AllocRule();
    if (NULL == pstRule)
    {
        return BS_NO_MEMORY;
    }

    TXT_Strlcpy(pstRule->szRule, pcRule, sizeof(pstRule->szRule));

    /* 把pattern解析为rule结构 */
    ulRet = uri_acl_ParsePattern(pcRule, pstRule);
    if (BS_OK != ulRet)
    {
        /* 统一释放rule */
        uri_acl_FreeRule(pstRule);
        return ulRet;
    }
    
    /* 把rule添加到链表中 */
    if (BS_OK != ListRule_AddRule(hCtx, uiListID, uiRuleID, &pstRule->stListRuleNode))
    {
        uri_acl_FreeRule(pstRule);
        return BS_ERR;
    }
    
    return BS_OK;
}

VOID URI_ACL_DelRule(IN LIST_RULE_HANDLE hCtx, IN UINT uiListID, IN UINT uiRuleID)
{
    RULE_NODE_S *pstListRuleNode;
    
    pstListRuleNode = ListRule_DelRule(hCtx, uiListID, uiRuleID);
    if (NULL == pstListRuleNode)
    {
        return;
    }

    uri_acl_DeleteRule(pstListRuleNode, NULL);
}

BS_STATUS URI_ACL_Match
(
    IN LIST_RULE_HANDLE hCtx,
    IN UINT uiListID,
    IN URI_ACL_MATCH_INFO_S *pstMatchInfo, 
    OUT URI_ACL_ACTION_E *penAction
)
{
    RULE_NODE_S *pstListRuleNode;
    URI_ACL_RULE_S *pstRule;
    BOOL_T bIsMatch = BOOL_TRUE;
    BS_STATUS eRet = BS_NOT_FOUND;
    UINT uiRuleID = RULE_ID_INVALID;

    *penAction = URI_ACL_ACTION_DENY;

    while ((pstListRuleNode = ListRule_GetNextRule(hCtx, uiListID, uiRuleID)) != NULL)
    {
        uiRuleID = pstListRuleNode->uiRuleID;
        pstRule = container_of(pstListRuleNode, URI_ACL_RULE_S, stListRuleNode);
        /* 按rule匹配 */
        bIsMatch = uri_acl_MatchByRule(pstMatchInfo, pstRule);
        if (BOOL_TRUE == bIsMatch)
        {
            *penAction = pstRule->enAction;
            eRet = ERROR_SUCCESS;
            break;
        }   
    }

    return eRet;
}

#else

URI_ACL_CTX_HANDLE URI_ACL_Create()
{
	return NULL;
}

VOID URI_ACL_Destroy(IN URI_ACL_CTX_HANDLE hCtx)
{
}

UINT URI_ACL_AddList(IN URI_ACL_CTX_HANDLE hCtx, IN CHAR *pcListName)
{
	return 0;
}

VOID URI_ACL_DelList(IN URI_ACL_CTX_HANDLE hCtx, IN UINT uiListID)
{
}

UINT URI_ACL_FindListByName(IN URI_ACL_CTX_HANDLE hCtx, IN CHAR *pcListName)
{
	return 0;
}

BS_STATUS URI_ACL_AddRule(IN URI_ACL_CTX_HANDLE hCtx, IN UINT uiListID, IN UINT uiRuleID, IN CHAR *pcRule)
{
	return 0;
}

VOID URI_ACL_DelRule(IN URI_ACL_CTX_HANDLE hCtx, IN UINT uiListID, IN UINT uiRuleID)
{
}

BS_STATUS URI_ACL_Match
(
    IN URI_ACL_CTX_HANDLE hCtx,
    IN UINT uiListID,
    URI_ACL_MATCH_INFO_S *pstMatchInfo, 
    OUT URI_ACL_ACTION_E *penAction
)
{
	return 0;
}

#endif

