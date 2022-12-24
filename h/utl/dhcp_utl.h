#ifndef __DHCP_UTL_H_
#define __DHCP_UTL_H_

#include "udp_utl.h"

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

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
//====================
// DHCP Messages types
//====================
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
//==================
// DHCP Option types
//==================
#define DHCP_MSG_TYPE               53  /* message type (u8) */
#define DHCP_PARM_REQ               55  /* parameter request list: c1 (u8), ... */
#define DHCP_CLIENT_ID              61  /* client ID: type (u8), i1 (u8), ... */
#define DHCP_REQUEST_IP             50  /* requested IP addr (u32) */
#define DHCP_NETMASK                1  /* subnet mask (u32) */
#define DHCP_LEASE_TIME             51  /* lease time sec (u32) */
#define DHCP_RENEW_TIME             58  /* renewal time sec (u32) */
#define DHCP_REBIND_TIME            59  /* rebind time sec (u32) */
#define DHCP_SERVER_ID              54  /* server ID: IP addr (u32) */
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
    UCHAR ucOp;          /* message op */

    UCHAR  ucHType;      /* hardware address type (e.g. '1' = 10Mb Ethernet) */
    UCHAR  ucHLen;       /* hardware address length (e.g. '6' for 10Mb Ethernet) */
    UCHAR  ucHops;       /* client sets to 0, may be used by relay agents */
    UINT  ulXid;        /* transaction ID, chosen by client */
    USHORT usSecs;       /* seconds since request process began, set by client */
    USHORT usFlags;
    UINT  ulClientIp;     /* client IP address, client sets if known */
    UINT  ulYourIp;     /* 'your' IP address -- server's response to client */
    UINT  ulServerIp;     /* server IP address */
    UINT  ulRelayIp;     /* relay agent IP address */
    UCHAR  aucClientHaddr[16]; /* client hardware address */
    UCHAR  aucServerName[64];  /* optional server host name */
    UCHAR  aucBootFile[128];  /* boot file name */
    UINT  ulMagic;      /* must be 0x63825363 (network order) */
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
/* 得到要申请的IP地址, 网络序 */
UINT DHCP_GetRequestIpByDhcpRequest (IN DHCP_HEAD_S *pstDhcpHead, IN UINT ulOptLen);
UINT DHCP_SetOpt0 (UCHAR *pucOpt, int type, IN UINT ulLen);
UINT DHCP_SetOpt8 (UCHAR *pucOpt, int type, UCHAR data, IN UINT ulLen);
UINT DHCP_SetOpt32 (UCHAR *pucOpt, int type, UINT data, IN UINT ulLen);
BS_STATUS DHCP_GetDhcpRequest(IN UDP_HEAD_S *udp_hdr, IN UINT pktlen, OUT DHCP_Request_t *dh_req, OUT char *req_info, IN int req_len);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

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

#endif //DHCP_H_
