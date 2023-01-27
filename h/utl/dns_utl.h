#ifndef __DNS_UTL_H_
#define __DNS_UTL_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

#define DNS_MAX_DOMAIN_NAME_LEN    253
#define DNS_MAX_DNS_NAME_SIZE   (DNS_MAX_DOMAIN_NAME_LEN + 2) /* max string length for host name, include '\0' */
#define DNS_MAX_LABEL_LEN     63
#define DNS_MAX_LABEL_NUM     ((DNS_MAX_DOMAIN_NAME_LEN + 1) / 2)
#define DNS_MAX_UDP_PKT_SIZE  512

#define DNS_IP_MAX            100 /* 一个DNS报文中最多解析多少个IP */
#define DNS_DFT_SERVER_PORT   53 /* 主机序 */

/* TYPE */
#define DNS_TYPE_NULL   0
#define DNS_TYPE_A      1
#define DNS_TYPE_AAAA   28
#define DNS_TYPE_NS     2
#define DNS_TYPE_CNAME  5
#define DNS_TYPE_PTR    12
#define DNS_TYPE_HINFO  13
#define DNS_TYPE_MX     15
#define DNS_TYPE_AXFR   252
#define DNS_TYPE_ANY    255

/* flag */
#define DNS_FLAG_RESPONES           1
#define DNS_FLAG_OPCODE_STANDARY    0
#define DNS_FLAG_REPLY_CODE         0


/* CLASS */
#define DNS_CLASS_IN 1

#define DNS_RRTYPE_QUERY 0
#define DNS_RRTYPE_ANSWER 1
#define DNS_RRTYPE_AUTHORITY 2
#define DNS_RRTYPE_ADDITIONAL 3

#pragma pack(1)
typedef struct tagDNS_Header
{
    USHORT    usTransID;    /* query identification number */
    
#if BS_BIG_ENDIAN
    /* fields in third byte */
    USHORT    usQR: 1;          /* response flag */
    USHORT    usOpcode: 4;      /* purpose of message */
    USHORT    usAA: 1;          /* authoritive answer */
    USHORT    usTC: 1;          /* truncated message */
    USHORT    usRD: 1;          /* recursion desired */
    /* fields in fourth byte */
    USHORT    usRA: 1;          /* recursion available */
    USHORT    usUnused :1;      /* unused bits (MBZ as of 4.9.3a3) */
    USHORT    usAD: 1;          /* authentic data from named */
    USHORT    usCD: 1;          /* checking disabled by resolver */
    USHORT    usRcode :4;       /* response code */
#else
    /* fields in third byte */
    USHORT    usRD :1;          /* recursion desired */
    USHORT    usTC :1;          /* truncated message */
    USHORT    usAA :1;          /* authoritive answer */
    USHORT    usOpcode :4;      /* purpose of message */
    USHORT    usQR :1;          /* response flag */
    /* fields in fourth byte */
    USHORT    usRcode :4;       /* response code */
    USHORT    usCD: 1;          /* checking disabled by resolver */
    USHORT    usAD: 1;          /* authentic data from named */
    USHORT    usUnused :1;      /* unused bits (MBZ as of 4.9.3a3) */
    USHORT    usRA :1;          /* recursion available */
#endif
    /* remaining bytes */
    USHORT    usQdCount;        /* number of question entries */
    USHORT    usAnCount;        /* number of answer entries */
    USHORT    usNsCount;        /* number of authority entries */
    USHORT    usArCount;        /* number of resource entries */
}DNS_HEADER_S;

/* Information of DNS query question */
typedef struct tagDNS_Question
{
    USHORT  usType;                       /* Query type */
    USHORT  usClass;                      /* Query class */
}DNS_QST_S; 

/* Information of DNS query answer */
typedef struct
{
    USHORT  usType;
    USHORT  usClass;
    UINT    uiTTL;
    USHORT  usRDLen;
} DNS_RR_S;

/* Information of DNS query answer */
typedef struct
{
    UCHAR aucData[DNS_MAX_UDP_PKT_SIZE];
} DNS_PKT_S; 

#pragma pack()

typedef struct
{
    CHAR *apcLabels[DNS_MAX_LABEL_NUM];
}DNS_LABELS_S;

typedef struct {
    char domain_name[DNS_MAX_DNS_NAME_SIZE + 1];
    UINT ips[DNS_IP_MAX];
    UCHAR ip_num;
}DNS_IP_DOMAINNAME_S;

/* 返回解析出了多少个Label, 如果返回0表示出错 */
UINT DNS_DomainName2Labels(IN CHAR *pcDomainName, OUT DNS_LABELS_S *pstLabels);

/* 获得DNS的数字分格式的域名的长度,包含开头的第一个数字和最后的'\0' . 返回0表示出错 */
UINT DNS_GetDnsNameSize(IN UCHAR *pucData, IN UINT uiMaxSize);

/* 将普通的点分格式的域名转成DNS的数字分格式的域名. 返回组装后的长度(含'\0'). 返回0为失败 */
UINT DNS_DomainName2DnsName
(
    IN CHAR *pcDomainName,
    OUT CHAR *pcDnsDomainName,
    IN UINT uiDnsDomainNameSize
);

UINT DNS_BuildQuestPacket
(
    IN CHAR *pcDomainName,
    IN USHORT usType,
    OUT DNS_PKT_S *pstPkt
);

/* 返回构造之后的数据长度 */
UINT DNS_BuildRR
(
    IN CHAR *pcDomainName,
    IN USHORT usType,
    IN USHORT usClass,
    IN UINT uiTTL,  /* 秒 */
    IN USHORT usRDLen,
    IN UCHAR *pucRDData,
    OUT UCHAR *pucData,
    IN UINT uiDataSize
);

/* 返回构造之后的数据长度. 返回0表示失败 */
UINT DNS_BuildRRCompress
(
    IN USHORT usDomainNameOffset,
    IN USHORT usType,
    IN USHORT usClass,
    IN UINT uiTTL,  /* 秒 */
    IN USHORT usRDLen,
    IN UCHAR *pucRDData,
    OUT UCHAR *pucData,
    IN UINT uiDataSize
);

UINT DNS_GetHostDnsServer();

int DNS_GetRRDomainName(UCHAR *dns_pkt, int pkt_len, void *rr, OUT char domain_name[DNS_MAX_DOMAIN_NAME_LEN + 1]);
int DNS_GetQueryNameByPacket(UCHAR *pucDnsPkt, int pkt_len, OUT char szDomainName[DNS_MAX_DOMAIN_NAME_LEN + 1]);

/* 返回第一个RR的指针, 包含这个RR的域名 */
UCHAR * DNS_GetFirstRR(IN DNS_HEADER_S *pstDnsHeader, IN UINT uiDataLen);
/* 根据包含域名的RR获得其RD的长度和指针, 返回值为RD指针 */
UCHAR * DNS_GetRDByRR(IN UCHAR *pucRRWithDomain, IN UINT uiDataLen, OUT USHORT *pusRdLen);

CHAR * DNS_Header2String(IN VOID *dnspkt, IN int pktlen, OUT CHAR *info, IN UINT infosize);

typedef struct {
    DNS_HEADER_S *dns_header; /* 指向dns报文头 */
    void *rr_data; /* 指向rr data, 包含Name部分 */
    void *rr_struct;  /* 指向rr结构体, 不包含Name部分 */
    UCHAR rrtype;  /* rr的类型 */
    USHORT rr_size; /* rr的size, 包含域名/rr结构体/资源 */
    USHORT dns_pkt_len; /* dns报文长度 */
    USHORT domain_size; /* 域名所占rr的size(不展开) */
}DNS_WALK_QR_S;

typedef void (*PF_DNS_WALK_RR)(DNS_WALK_QR_S *qr, void *ud);
void DNS_WalkQR(DNS_HEADER_S *pstDnsHeader, UINT uiDataLen, PF_DNS_WALK_RR func, void *ud);

int DNS_ParseDomainNameIP(DNS_HEADER_S *pstDnsHeader, UINT uiDataLen, OUT DNS_IP_DOMAINNAME_S *domain_ip);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /* __DNS_UTL_H_ */

