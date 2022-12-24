/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      lixingang  Version: 1.0  Date: 2007-2-3
* Description: 
* History:     
******************************************************************************/
#ifndef __NET_H_
#define __NET_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* ---define--- */
#define LOOPBACK_IP 0x7f000001    /* 127.0.0.1, 主机序 */

#ifdef IN_WINDOWS
#define NET_GET_ADDR_BY_SOCKADDR(pstSockAddr) (((struct sockaddr_in *)(pstSockAddr))->sin_addr.S_un.S_addr)
#endif

#ifdef IN_UNIXLIKE
#define NET_GET_ADDR_BY_SOCKADDR(pstSockAddr) (((struct sockaddr_in *)(pstSockAddr))->sin_addr.s_addr)
#endif

#define  PRINT_HIP(x)\
    (((x) >>  24) & 0xFF),\
    (((x) >>  16) & 0xFF),\
    (((x) >> 8) & 0xFF),\
    (((x) >> 0) & 0xFF)

#define PREFIX_2_COUNT(depth) (1UL << (depth)) /* 整个地址段可以有多少个prefix的网段 */

/* 3字节数据转网络序 */
#ifdef BS_BIG_ENDIAN
#define ntoh3B(x) (x)
#define hton3B(x) (x)
#else
#define ntoh3B(x) ((((x) & 0xff) << 16) | ((x) & 0x00ff00) | (((x) >> 16) & 0xff))
#define hton3B(x) ntoh3B(x)
#endif

#ifndef htonll
static inline uint64_t __my_htonll(uint64_t val)
{
	if (1 == htonl(1))
		return val;
	return (((uint64_t)htonl(val)) << 32) | htonl(val >> 32);
}
 
static inline uint64_t __my_ntohll(uint64_t val)
{
    return __my_htonll(val);
}

#define htonll __my_htonll
#define ntohll __my_ntohll
#endif

/* 将Prefix转换成主机序的Mask */
static inline UINT PREFIX_2_MASK(IN UCHAR ucPrefixLen)
{
    /* 很多CPU只取右移的低5bit, 所以右移32相当于右移0了. 所以32要特殊处理 */
    if (ucPrefixLen == 32) {
        return 0xffffffff;
    }

    return ~(((UINT)0xFFFFFFFF) >> ucPrefixLen);
}

/* 根据Prefix得到容量 */
static inline UINT PREFIX_2_CAPACITY(IN UCHAR ucPrefixLen)
{
	return 1 << (32 - ucPrefixLen);
}

static inline UCHAR MASK_2_PREFIX(IN UINT uiMask/* 主机序 */)
{
    UINT i;

    for (i=0; i<32; i++) {
        if ((uiMask & (1 << (31 - i))) == 0) {
            return i;
        }
    }

    return 32;
}

/* 判断是否一个合法Mask */
static inline BOOL_T MASK_IS_VALID(IN UINT uiMask/* 主机序 */)
{
    if (uiMask == PREFIX_2_MASK(MASK_2_PREFIX(uiMask))) {
        return TRUE;
    }

    return FALSE;
}

/* 
    根据地址和子网掩码转换为地址范围
    uiBeginIp和uiEndIp的字节序和uiIp+uiMask是一致的
*/
static inline VOID IpMask_2_Range(IN UINT uiIp, IN UINT uiMask, OUT UINT *puiBeginIP, OUT UINT *puiEndIP)
{
    *puiBeginIP = uiIp & uiMask;
    *puiEndIP = uiIp | (~uiMask);
}

/* 判断IP是否是子网边界 */
static inline BOOL_T IP_IsSubNetEdge(IN UINT uiIP, IN UINT uiMask)
{
    if ((uiIP & uiMask) == uiIP) {
        return TRUE;
    }
    if ((uiIP | (~uiMask)) == uiIP) {
        return TRUE;
    }
    return FALSE;
}

/* 
    根据子网的自身地址(即起始地址)和广播地址(即结束地址)得到子网掩码
    得到的掩码字节序和输入情况一致
*/
static inline UINT SubNet_2_Mask(IN UINT uiSubNetSelf, IN UINT uiSubNetBroadIP)
{
    return ~(uiSubNetSelf ^ uiSubNetBroadIP);
}

/* 判断两个地址是否正好能构成一个网段的两个边界 */
static inline BOOL_T IP_IsSubNetEdge2(IN UINT uiStart/* 主机序 */, IN UINT uiEnd/* 主机序 */)
{
    UINT uiMask = SubNet_2_Mask(uiStart, uiEnd);
    if(MASK_IS_VALID(uiMask)) {
        return TRUE;
    }
    return FALSE;
}

/* 获取能包下这个网段的最小的掩码,主机序 */
static inline UINT IP_GetMiniMask(IN UINT uiStartIP/* 主机序 */, IN UINT uiEndIP/* 主机序 */)
{
    UINT uiPrefix;
    UINT uiMask;

    for (uiPrefix=31; uiPrefix>0; uiPrefix--) {
        uiMask = PREFIX_2_MASK(uiPrefix);
        if ((uiStartIP & uiMask) == (uiEndIP & uiMask)) {
            return uiMask;
        }
    }

    return 0;
}

/* 
    根据地址范围, 可以将其分割为多个子网, 此函数获得第一个子网的主机序掩码
*/
static inline UINT Range_GetFirstMask(IN UINT uiBeginIP/* 主机序 */, IN UINT uiEndIP/* 主机序 */)
{
    UINT uiPrefix;
    UINT uiMask = 0;
    UINT uiStart;
    UINT uiStop;

    for (uiPrefix=0; uiPrefix<=32; uiPrefix++) {
        uiMask = PREFIX_2_MASK(uiPrefix);
        IpMask_2_Range(uiBeginIP, uiMask, &uiStart, &uiStop);
        if ((uiStart >= uiBeginIP) && (uiStop <= uiEndIP)) {
            break;
        }
    }

    return uiMask;
}

/* 获取共同前缀的位数 */
static inline UINT IP_GetCommonPrefix(IN UINT uiIP1/* 主机序 */, IN UINT uiIP2/* 主机序 */)
{
    UINT i;

    for (i=0; i<32; i++) {
        if ((uiIP1 & (0x1 << (32- i))) != (uiIP2 & (0x1 << (32- i)))) {
            return i;
        }
    }

    return i;
}

/* 根据掩码将地址段分成多个段 */
#define IP_SPLIT_SUBNET_BY_MASK_BEGIN(_uiStartIP, _uiEndIP, _uiMask, _uiNetStart, _uiNetEnd) \
    do { \
        UINT _uiBeginTmp = _uiStartIP;  \
        UINT _uiNetStartTmp, _uiNetEndTmp;  \
        while (_uiBeginTmp <= _uiEndIP) {   \
            IpMask_2_Range(_uiBeginTmp, _uiMask, &_uiNetStartTmp, &_uiNetEndTmp);   \
            if (_uiNetStartTmp < _uiBeginTmp){ _uiNetStartTmp = _uiBeginTmp;} \
            if (_uiNetEndTmp > _uiEndIP){_uiNetEndTmp = _uiEndIP;} \
            _uiNetStart = _uiNetStartTmp;   \
            _uiNetEnd = _uiNetEndTmp;  \
            {

#define IP_SPLIT_SUBNET_BY_MASK_END() } _uiBeginTmp = _uiNetEndTmp + 1; }} while(0)


typedef struct
{
    UCHAR aucMac[6];
}MAC_ADDR_S;

typedef union
{
    MAC_ADDR_S stMacAddr;
    USHORT ausMacAddr[3];
}MAC_ADDR_U;

typedef union
{
    UCHAR aucIp[4];
    UINT uiIp;
}IP_ADDR_U;

#define IP_ADDR_4_CHAR(ip_addr) (ip_addr)->aucIp[0], (ip_addr)->aucIp[1], (ip_addr)->aucIp[2], (ip_addr)->aucIp[3]
#define IP_4_CHAR(aucIp) (aucIp)[0], (aucIp)[1], (aucIp)[2], (aucIp)[3]

#define IPv4(a, b, c, d) ((uint32_t)(((a) & 0xff) << 24) | \
				(((b) & 0xff) << 16) |     \
				(((c) & 0xff) << 8)  |     \
				((d) & 0xff))

#define NS_IN6ADDRSZ     16
#define IN6ADDR_SIZE8    NS_IN6ADDRSZ
#define IN6ADDR_SIZE16   8
#define IN6ADDR_SIZE32   4

typedef struct net_in6_addr {
    union
    {
        unsigned char   __u6_addr8[IN6ADDR_SIZE8];
        unsigned short  __u6_addr16[IN6ADDR_SIZE16];
        unsigned int    __u6_addr32[IN6ADDR_SIZE32];
    } __u6_addr;            /* 128-bit IP6 address */
    
    #define net_s6_addr   __u6_addr.__u6_addr8
    #define net_s6_addr16 __u6_addr.__u6_addr16
    #define net_s6_addr32 __u6_addr.__u6_addr32
}IN6ADDR_S;

typedef struct
{
    UINT S_addr;
}INADDR_S;

typedef enum
{
    NET_PKT_TYPE_ETH = 0,
    NET_PKT_TYPE_IP,
    NET_PKT_TYPE_IP6,
    NET_PKT_TYPE_UDP,
    NET_PKT_TYPE_TCP,
    NET_PKT_TYPE_ICMP,
    NET_PKT_TYPE_VXLAN,

    NET_PKT_TYPE_MAX
}NET_PKT_TYPE_E;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /*__NET_H_*/

