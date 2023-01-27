/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#ifndef _IP4_UTL_H
#define _IP4_UTL_H
#ifdef __cplusplus
extern "C"
{
#endif

#ifndef IN_CLASSA
    #define IN_CLASSA(a)        ((((UINT)(a)) & 0x80000000) == 0)
    #define IN_CLASSA_NET       0xff000000
    #define IN_CLASSA_NSHIFT    24
    #define IN_CLASSA_HOST      (0xffffffff & ~IN_CLASSA_NET)
    #define IN_CLASSA_MAX       128
    
    #define IN_CLASSB(a)        ((((UINT)(a)) & 0xc0000000) == 0x80000000)
    #define IN_CLASSB_NET       0xffff0000
    #define IN_CLASSB_NSHIFT    16
    #define IN_CLASSB_HOST      (0xffffffff & ~IN_CLASSB_NET)
    #define IN_CLASSB_MAX       65536
    
    #define IN_CLASSC(a)        ((((UINT)(a)) & 0xe0000000) == 0xc0000000)
    #define IN_CLASSC_NET       0xffffff00
    #define IN_CLASSC_NSHIFT    8
    #define IN_CLASSC_HOST      (0xffffffff & ~IN_CLASSC_NET)
    
    #define IN_CLASSD(a)        (((UINT)(a) & 0xf0000000) == 0xe0000000)
    #define IN_CLASSD_NET       0xf0000000  /* These ones aren't really */
    #define IN_CLASSD_NSHIFT    28      /* net and host fields, but */
    #define IN_CLASSD_HOST      0x0fffffff  /* routing needn't know.    */
    #define IN_MULTICAST(a)     IN_CLASSD(a)
    
    /* Address to accept any incoming messages.  */
    #define INADDR_ANY      ((UINT) 0x00000000)
    
    /* Address to send to all hosts.  */
    #define INADDR_BROADCAST    ((UINT) 0xffffffff)
    
    /* Address indicating an error return.  */
    #ifndef INADDR_NONE
        #define INADDR_NONE     ((UINT) 0xffffffff)
    #endif
    
    /* Network number for local host loopback.  */
    #define IN_LOOPBACKNET      127

    /* Address to loopback in software to local host.  */
    #define INADDR_LOOPBACK    ((UINT) 0x7f000001) /* Inet 127.0.0.1.  */
#endif /* IN_CLASSA */

#define IP_IN_LOOPBACK(a)      (((UINT) (a) & 0xff000000) == 0x7f000000)
#define IP_IN_EXPERIMENTAL(a)  ((((UINT)(a)) & 0xe0000000) == 0xe0000000)
#define IP_IN_BADCLASS(a)      ((((UINT)(a)) & 0xf0000000) == 0xf0000000)
#define IP_IN_LINKLOCAL(a)     (((UINT)(a) & 0xffff0000) == 0xa9fe0000)
#define IP_IN_PRIVATE(a)   ((((UINT)(a) & 0xff000000) == 0x0a000000) || \
                         (((UINT)(a) & 0xfff00000) == 0xac100000) || \
                         (((UINT)(a) & 0xffff0000) == 0xc0a80000))
#define IP_IN_LOCAL_GROUP(a)   (((UINT)(a) & 0xffffff00) == 0xe0000000)
#define IP_IN_ANY_LOCAL(a)     (IP_IN_LINKLOCAL(a) || IP_IN_LOCAL_GROUP(a))

#define MAX_IPHEADLEN    60

/* 子网地址 */
#define IP_IS_SUB_NET(uiIp, uiMask) ((uiIp) & (uiMask) == (uiIp))
/* 子网广播 */
#define IP_IS_SUB_BROADCAST(uiIp, uiMask) (((uiIp) & (~(uiMask))) == (~(uiMask)))

#define IP_PROTO_HOPOPTS        0   /* Hop-by-hop option header */
#define IP_PROTO_ICMP  1   /* ICMP protocol */
#define IP_PROTO_IGMP  2   /* IGMP protocol */
#define IP_PROTO_TCP   6   /* TCP protocol */
#define IP_PROTO_UDP   17  /* UDP protocol */
#define IP_PROTO_ROUTE 43  /* Routing header */
#define IP_PROTO_FRAGMENT 44  /* fragmentheader */
#define IP_PROTO_ICMP6 58  /* icmp6*/
#define IP_PROTO_NONE  59  /* no next header */

#define IP_VERSION      4       /* IPv4 version */
#define IP6_VERSION     6       /* IPv6 version */

#define IP_HEAD_VER(pstIpHead) ((pstIpHead)->ucVer)
#define IP_HEAD_LEN(pstIpHead) (((pstIpHead)->ucHLen) << 2)
#define IP_HEAD_DSCP(pstIpHead) (((pstIpHead)->ucTos) >> 2)
#define IP_SET_HEAD_LEN(pstIpHead, usLen) (((pstIpHead)->ucHLen) = ((usLen) >> 2))
#define IP_HEAD_FLAG(pstIpHead) ((ntohs((pstIpHead)->usOff) & 0xe000) >> 13)
#define IP_HEAD_FRAG_OFFSET(pstIpHead) ((ntohs((pstIpHead)->usOff) & 0x1fff))

/* ip fragment flag */
#define IP_DF           0x4000  /* dont fragment flag */
#define IP_MF           0x2000  /* more fragments flag */
#define IP_OFFMASK      0x1fff  /* mask for fragmenting bits */

#define IP_MAXPACKET    65535       /* maximum packet size */

#pragma pack(1)
/* IPv4 头的定义 */
typedef struct 
{
#if BS_BIG_ENDIAN
    UCHAR    ucVer:4;              /* version */
    UCHAR    ucHLen:4;             /* header length */
#else
    UCHAR    ucHLen:4;
    UCHAR    ucVer:4;
#endif
	UCHAR 		ucTos; // TOS类型
	USHORT 		usTotlelen; // 总长度
	USHORT 		usIdentification; // Identification
	USHORT 		usOff; // Flags (3 bits) + Fragment offset (13 bits)
	UCHAR  		ucTtl; // 生存期
	UCHAR  		ucProto;  /* 报文协议类型 */
	USHORT 		usCrc; // 首部校验和
	IP_ADDR_U   unSrcIp; // 源IP
	IP_ADDR_U   unDstIp; // 目的IP
}IP_HEAD_S;
#pragma pack()

char * inet_ntop4(const struct in_addr *addr, char *buf, socklen_t len);

#ifdef __cplusplus
}
#endif
#endif //IP4_UTL_H_
