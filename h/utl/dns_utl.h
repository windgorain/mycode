#ifndef __DNS_UTL_H_
#define __DNS_UTL_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

#define DNS_MAX_DOMAIN_NAME_LEN    253
#define DNS_MAX_DNS_NAME_SIZE   (DNS_MAX_DOMAIN_NAME_LEN + 2) 
#define DNS_MAX_LABEL_LEN     63
#define DNS_MAX_LABEL_NUM     ((DNS_MAX_DOMAIN_NAME_LEN + 1) / 2)
#define DNS_MAX_UDP_PKT_SIZE  512

#define DNS_IP_MAX            100 
#define DNS_DFT_SERVER_PORT   53 


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


#define DNS_FLAG_RESPONES           1
#define DNS_FLAG_OPCODE_STANDARY    0
#define DNS_FLAG_REPLY_CODE         0



#define DNS_CLASS_IN 1

#define DNS_RRTYPE_QUERY 0
#define DNS_RRTYPE_ANSWER 1
#define DNS_RRTYPE_AUTHORITY 2
#define DNS_RRTYPE_ADDITIONAL 3

#pragma pack(1)
typedef struct tagDNS_Header
{
    USHORT    usTransID;    
    
#if BS_BIG_ENDIAN
    
    USHORT    usQR: 1;          
    USHORT    usOpcode: 4;      
    USHORT    usAA: 1;          
    USHORT    usTC: 1;          
    USHORT    usRD: 1;          
    
    USHORT    usRA: 1;          
    USHORT    usUnused :1;      
    USHORT    usAD: 1;          
    USHORT    usCD: 1;          
    USHORT    usRcode :4;       
#else
    
    USHORT    usRD :1;          
    USHORT    usTC :1;          
    USHORT    usAA :1;          
    USHORT    usOpcode :4;      
    USHORT    usQR :1;          
    
    USHORT    usRcode :4;       
    USHORT    usCD: 1;          
    USHORT    usAD: 1;          
    USHORT    usUnused :1;      
    USHORT    usRA :1;          
#endif
    
    USHORT    usQdCount;        
    USHORT    usAnCount;        
    USHORT    usNsCount;        
    USHORT    usArCount;        
}DNS_HEADER_S;


typedef struct tagDNS_Question
{
    USHORT  usType;                       
    USHORT  usClass;                      
}DNS_QST_S; 


typedef struct
{
    USHORT  usType;
    USHORT  usClass;
    UINT    uiTTL;
    USHORT  usRDLen;
} DNS_RR_S;


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


UINT DNS_DomainName2Labels(IN CHAR *pcDomainName, OUT DNS_LABELS_S *pstLabels);


UINT DNS_GetDnsNameSize(IN UCHAR *pucData, IN UINT uiMaxSize);


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


UINT DNS_BuildRR
(
    IN CHAR *pcDomainName,
    IN USHORT usType,
    IN USHORT usClass,
    IN UINT uiTTL,  
    IN USHORT usRDLen,
    IN UCHAR *pucRDData,
    OUT UCHAR *pucData,
    IN UINT uiDataSize
);


UINT DNS_BuildRRCompress
(
    IN USHORT usDomainNameOffset,
    IN USHORT usType,
    IN USHORT usClass,
    IN UINT uiTTL,  
    IN USHORT usRDLen,
    IN UCHAR *pucRDData,
    OUT UCHAR *pucData,
    IN UINT uiDataSize
);

UINT DNS_GetHostDnsServer();

int DNS_GetRRDomainName(UCHAR *dns_pkt, int pkt_len, void *rr, OUT char domain_name[DNS_MAX_DOMAIN_NAME_LEN + 1]);
int DNS_GetQueryNameByPacket(UCHAR *pucDnsPkt, int pkt_len, OUT char szDomainName[DNS_MAX_DOMAIN_NAME_LEN + 1]);


UCHAR * DNS_GetFirstRR(IN DNS_HEADER_S *pstDnsHeader, IN UINT uiDataLen);

UCHAR * DNS_GetRDByRR(IN UCHAR *pucRRWithDomain, IN UINT uiDataLen, OUT USHORT *pusRdLen);

CHAR * DNS_Header2String(IN VOID *dnspkt, IN int pktlen, OUT CHAR *info, IN UINT infosize);

typedef struct {
    DNS_HEADER_S *dns_header; 
    void *rr_data; 
    void *rr_struct;  
    UCHAR rrtype;  
    USHORT rr_size; 
    USHORT dns_pkt_len; 
    USHORT domain_size; 
}DNS_WALK_QR_S;

typedef void (*PF_DNS_WALK_RR)(DNS_WALK_QR_S *qr, void *ud);
void DNS_WalkQR(DNS_HEADER_S *pstDnsHeader, UINT uiDataLen, PF_DNS_WALK_RR func, void *ud);

int DNS_ParseDomainNameIP(DNS_HEADER_S *pstDnsHeader, UINT uiDataLen, OUT DNS_IP_DOMAINNAME_S *domain_ip);

#ifdef __cplusplus
    }
#endif 

#endif 

