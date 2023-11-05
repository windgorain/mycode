#ifndef __DHCP_UTL_H_
#define __DHCP_UTL_H_

#include "udp_utl.h"

#ifdef __cplusplus
    extern "C" {
#endif 

#define DHCP_OP_REQUEST    1
#define DHCP_OP_REPLY      2
#define DHCP_TYPE_REQUEST  3
#define DHCP_TYPE_ACK      5
#define DHCP_OPT_HOSTNAME  12
#define DHCP_OPT_REQIP     50
#define DHCP_OPT_REQLIST   55
#define DHCP_OPT_MSGTYPE   53
#define DHCP_OPT_VENDORID  60
#define DHCP_OPT_CLIID     61
#define DHCP_OPT_SERVERIP  54
#define DHCP_OPT_END       255
#define DHCP_CHADDR_LEN    16
#define SERVERNAME_LEN     64
#define CLIENTNAME_LEN     256
#define BOOTFILE_LEN       128
#define DHCP_HDR_LEN       240
#define DHCP_OPT_HDR_LEN   2

#if 1
#define DHCP_DFT_SERVER_PORT 67
#define DHCP_DFT_CLIENT_PORT 68
#endif

#if 1



#define DHCP_DISCOVER 1
#define DHCP_OFFER    2
#define DHCP_REQUEST  3
#define DHCP_DECLINE  4
#define DHCP_ACK      5
#define DHCP_NACK     6
#define DHCP_RELEASE  7
#define DHCP_INFORM   8
#endif

#if 1



#define DHCP_MSG_TYPE               53  
#define DHCP_PARM_REQ               55  
#define DHCP_CLIENT_ID              61  
#define DHCP_REQUEST_IP             50  
#define DHCP_NETMASK                1  
#define DHCP_LEASE_TIME             51  
#define DHCP_RENEW_TIME             58  
#define DHCP_REBIND_TIME            59  
#define DHCP_SERVER_ID              54  
#define DHCP_GATEWAY                3
#define DHCP_DOMAIN_NAME_SERVER     6
#define DHCP_PAD                    0
#define DHCP_END        255
#endif

#define IP_HDR_LEN 20
#define UDP_HDR_LEN 8

#if 1

#pragma pack(1)

typedef struct{
    UCHAR ucOp;          

    UCHAR  ucHType;      
    UCHAR  ucHLen;       
    UCHAR  ucHops;       
    UINT  ulXid;        
    USHORT usSecs;       
    USHORT usFlags;
    UINT  ulClientIp;     
    UINT  ulYourIp;     
    UINT  ulServerIp;     
    UINT  ulRelayIp;     
    UCHAR  aucClientHaddr[16]; 
    UCHAR  aucServerName[64];  
    UCHAR  aucBootFile[128];  
    UINT  ulMagic;      
}DHCP_HEAD_S;

typedef struct {
  UCHAR type;
}VNET_DHCP_OPT0;

typedef struct {
  UCHAR type;
  UCHAR len;
  UCHAR data;
}VNET_DHCP_OPT8;

typedef struct {
  UCHAR type;
  UCHAR len;
  UINT data;
} VNET_DHCP_OPT32;

typedef struct dhcp_request {
    char hostname[CLIENTNAME_LEN];
    char client_mac[32];
    char client_id[256];
    char req_param[256];
    char vendor_id[256];
    char client_ip[16];
    char server_ip[16];
    int32_t client_type;
    int32_t opt_flags;
} DHCP_Request_t;

#define IPV4_FORMAT  "%u.%u.%u.%u"
#define ETH_MAC_FORMAT   "%02x:%02x:%02x:%02x:%02x:%02x"

#define NIPQUAD(addr) \
           ((uint8_t *)&addr)[0], \
           ((uint8_t *)&addr)[1], \
           ((uint8_t *)&addr)[2], \
           ((uint8_t *)&addr)[3]

#pragma pack()

#endif

INT DHCP_GetDHCPMsgType (IN DHCP_HEAD_S *pstDhcpHead, IN UINT ulOptLen);

UINT DHCP_GetRequestIpByDhcpRequest (IN DHCP_HEAD_S *pstDhcpHead, IN UINT ulOptLen);
UINT DHCP_SetOpt0 (UCHAR *pucOpt, int type, IN UINT ulLen);
UINT DHCP_SetOpt8 (UCHAR *pucOpt, int type, UCHAR data, IN UINT ulLen);
UINT DHCP_SetOpt32 (UCHAR *pucOpt, int type, UINT data, IN UINT ulLen);
BS_STATUS DHCP_GetDhcpRequest(IN UDP_HEAD_S *udp_hdr, IN UINT pktlen, OUT DHCP_Request_t *dh_req, OUT char *req_info, IN int req_len);

#ifdef __cplusplus
    }
#endif 

#define MAX_LOOP_TIMES     32


#define HAS_OPT_MSGTYPE    (1 << 0)
#define HAS_OPT_REQIP      (1 << 1)
#define HAS_OPT_SERVERIP   (1 << 2) 
#define HAS_OPT_END        (1 << 3)
#define HAS_OPT_HOSTNAME   (1 << 4)
#define HAS_OPT_CLIID      (1 << 5)
#define HAS_OPT_REQLIST    (1 << 6)
#define HAS_OPT_VENDORID   (1 << 7)
#define HAS_OPT_LEASTTIME  (1 << 8)

#define HAS_ALL_NEEDING_OPTS   (HAS_OPT_HOSTNAME | HAS_OPT_MSGTYPE | HAS_OPT_REQIP | HAS_OPT_SERVERIP | HAS_OPT_END)

struct dhcp_hdr {
    uint8_t op;
    uint8_t hwtype;
    uint8_t hwlen;
    uint8_t hwopcount;
    uint32_t xid;
    uint16_t secs;
    uint16_t flags;
    uint32_t ciaddr;
    uint32_t yiaddr;
    uint32_t siaddr;
    uint32_t giaddr;
    uint8_t chaddr[DHCP_CHADDR_LEN];
    uint8_t servername[SERVERNAME_LEN];
    uint8_t bootfile[BOOTFILE_LEN];
    uint32_t cookie;
} __attribute__((__packed__));

struct dhcp_opt {
    uint8_t opt;
    uint8_t len;
} __attribute__((__packed__));

#endif 
