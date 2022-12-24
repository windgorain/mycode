/******************************************************************************
* Copyright (C), LiXingang
* Author:      LiXingang  Date: 2016-6-15
* Description: 
* History:     
******************************************************************************/

#ifndef __SOCKET_IN_H_
#define __SOCKET_IN_H_

#include "utl/net.h"

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */


#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN 15    /* IPv4地址的字符串形式的长度 */
#endif

#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 46   /* IPv6地址的字符串形式的长度 */
#endif

/* 地址通用结构中存储的地址转成字符串的最大长度，包括'\0' */
#define INET_ADDR_STR_LEN      INET6_ADDRSTRLEN

/* IPv4和IPv6使用的通用IP地址结构. 其中的地址建议存储为网络序 */
typedef struct
{
    USHORT   usFamily;             /* 地址协议族(AF_INET/AF_INET6) */
    USHORT   usReserved;           /* 保留字段 */
    union
    {
        IN6ADDR_S stIP6Addr;
        INADDR_S  stIP4Addr;
    } un_addr;                      /* IP地址字段 */

    #define uIP6_Addr       un_addr.stIP6Addr
    #define uIP4_Addr       un_addr.stIP4Addr
}INET_ADDR_S;

/* 地址通用结构成员相关宏 */
#define INET_ADDR_FAMILY(pstAddrM)      ((pstAddrM)->usFamily)
#define INET_ADDR_IP4ADDR(pstAddrM)     ((pstAddrM)->uIP4_Addr)
#define INET_ADDR_IP4ADDRUINT(pstAddrM) INET_ADDR_IP4ADDR(pstAddrM).S_addr
#define INET_ADDR_IP6ADDR(pstAddrM)     ((pstAddrM)->uIP6_Addr)
#define INET_ADDR_IP6ADDRUINT(pstAddrM) INET_ADDR_IP6ADDR(pstAddrM).s6_addr32

static inline VOID INADDR_LenToMask(IN UINT uiLen, OUT UINT *puiMask)
{
    if (0 == uiLen)
    {
        *puiMask = 0;
    }
    else
    {
        *puiMask = INADDR_NONE<<(32-uiLen);
    }

    return;
}

/*****************************************************************************
  Description: 根据掩码长度计算得到掩码
*****************************************************************************/
static inline VOID IN6ADDR_LenToMask(IN UINT uiLen, OUT IN6ADDR_S *pstMask)
{
    UINT uiUintLen;
    UINT uiBitLen;
    UINT uiLoop;
    UINT *puiMask;

    puiMask = pstMask->net_s6_addr32;
    
    puiMask[0] = 0;
    puiMask[1] = 0;
    puiMask[2] = 0;
    puiMask[3] = 0;

    uiUintLen = uiLen >> 5;     /* uiLen除以32 */
    uiBitLen = uiLen & 31;      /* uiLen除以32的余数 */
    for (uiLoop = 0; uiLoop < uiUintLen; uiLoop++)
    {
        puiMask[uiLoop] = 0xffffffff;
    }
    
    if (uiBitLen != 0)
    {
        puiMask[uiUintLen] = htonl(((UINT)0xffffffff) << (32-uiBitLen));
    }

    return;
}

/*****************************************************************************
  Description: 获取地址对应的前缀
*****************************************************************************/
static inline VOID IN6ADDR_GetPrefix(IN const IN6ADDR_S *pstAddr, IN const IN6ADDR_S *pstMask, 
                                            OUT IN6ADDR_S *pstPrefix)
{
    UINT *puiPrefix;
    const UINT *puiAddr;
    const UINT *puiMask;
    
    puiPrefix = pstPrefix->net_s6_addr32;
    puiAddr = pstAddr->net_s6_addr32;
    puiMask = pstMask->net_s6_addr32;

    puiPrefix[0] = puiAddr[0] & puiMask[0];
    puiPrefix[1] = puiAddr[1] & puiMask[1];
    puiPrefix[2] = puiAddr[2] & puiMask[2];
    puiPrefix[3] = puiAddr[3] & puiMask[3];
    
    return;
}

/*****************************************************************************
  Description: 比较IPv6地址大小
        Input: pstAddr1: 待比较地址1
               pstAddr2: 待比较地址2
       Return: 大于0: 地址1大于地址2
               小于0: 地址1小于地址2
               等于0: 地址1等于地址2
      Caution: IPV6地址按网络序输入
*****************************************************************************/
static inline INT IN6ADDR_Cmp(IN const IN6ADDR_S *pstAddr1, IN const IN6ADDR_S *pstAddr2)
{
    UINT i;
    INT iRet;

    for (i = 0; i < NS_IN6ADDRSZ; i++)
    {
        iRet = pstAddr1->net_s6_addr[i] - pstAddr2->net_s6_addr[i];
        if (0 != iRet)
        {
            break;
        }
    }
    
    return iRet;
}

/*****************************************************************************
  函数描述: 比较IP地址通用结构中包含的地址字段的大小(网络序)
    返回值: 大于0: 地址1大于地址2
            小于0: 地址1小于地址2
            等于0: 地址1等于地址2
    注意点: 只比较相同地址族的地址通用结构
*****************************************************************************/
static inline INT INET_ADDR_Cmp(IN const INET_ADDR_S *pstAddr1, IN const INET_ADDR_S *pstAddr2)
{
    INT iRet = 0;
    
    if(AF_INET == INET_ADDR_FAMILY(pstAddr1))
    {
        UINT uiIP4Addr1 = ntohl(INET_ADDR_IP4ADDR(pstAddr1).S_addr);
        UINT uiIP4Addr2 = ntohl(INET_ADDR_IP4ADDR(pstAddr2).S_addr);

        if (uiIP4Addr1 > uiIP4Addr2)
        {
            iRet = 1;
        }
        else if(uiIP4Addr1 < uiIP4Addr2)
        {
            iRet = -1;
        }
        else
        {
            iRet = 0;
        }
    }
    else
    {
        iRet = IN6ADDR_Cmp(&(INET_ADDR_IP6ADDR(pstAddr1)), &(INET_ADDR_IP6ADDR(pstAddr2)));
    }

    return iRet;
}

/*****************************************************************************
  将字符格式转换为IP地址通用结构
  本函数只支持转换为网络序地址
*****************************************************************************/
BS_STATUS INET_ADDR_Str2IP(IN USHORT usFamily, IN const CHAR *pcStr, OUT INET_ADDR_S *pstAddr);
BS_STATUS INET_ADDR_N_Str2IP(IN USHORT usFamily, IN const CHAR *pcStr, IN UINT uiStrLen, OUT INET_ADDR_S *pstAddr);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__SOCKET_IN_H_*/


