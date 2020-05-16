#include "bs.h"
#include "utl/net.h"
#include "utl/ip_utl.h"
#include "utl/udp_utl.h"
#include "utl/udp_log.h"
#include "utl/dhcp_utl.h"
#include "utl/dhcp_log.h"

#define IP_HDR_LEN 20
#define UDP_HDR_LEN 8

#define IPV4_FORMAT  "%u.%u.%u.%u"
#define ETH_MAC_FORMAT   "%02x%02x%02x%02x%02x%02x"

#define NIPQUAD(addr) \
           ((uint8_t *)&addr)[0], \
           ((uint8_t *)&addr)[1], \
           ((uint8_t *)&addr)[2], \
           ((uint8_t *)&addr)[3]


void DhcpLog_Process(UDP_LOG_S *config, UDP_HEAD_S *udp_hdr, UINT pktlen)
{
    struct dhcp_hdr *dhcp_h;
    struct dhcp_opt *dhcp_opt;
   
    char hostname[CLIENTNAME_LEN] = {0}; 
    uint8_t msg_type = 0;
    uint32_t request_addr = 0, server_addr = 0;
    uint32_t opt_flags = 0;
    uint32_t udp_len = 0;
    int i = 0;
    char info[1024];
    
    /*Not request pkt*/
    if (ntohs(udp_hdr->usSrcPort) != DHCP_DFT_CLIENT_PORT || ntohs(udp_hdr->usDstPort) != DHCP_DFT_SERVER_PORT) {
        return;
    }

     /* packet too short */
    if (pktlen < IP_HDR_LEN + UDP_HDR_LEN + DHCP_HDR_LEN + DHCP_OPT_HDR_LEN) {
        return;
    }

    udp_len = pktlen - IP_HDR_LEN;

    dhcp_h = (struct dhcp_hdr *) ((char *) udp_hdr + UDP_HDR_LEN);
    dhcp_opt = (struct dhcp_opt *) ((char *) dhcp_h + DHCP_HDR_LEN);

    /* only care about REQUEST opcodes */
    if (dhcp_h->op != DHCP_OP_REQUEST) {
        return;
    }

    /*Can't use while(1), because there are illgeal packets coming in.*/
    for (i = 0; i < MAX_LOOP_TIMES; ++i) {
        if (dhcp_opt->opt == DHCP_OPT_MSGTYPE) {
            if (dhcp_opt->len != 1) {
                return;
            }
            memcpy(&msg_type, (char *) dhcp_opt + DHCP_OPT_HDR_LEN, dhcp_opt->len);
            /* only care about REQUEST msg types */
            if (msg_type != DHCP_TYPE_REQUEST) {
                return;
            }
            opt_flags |= HAS_OPT_MSGTYPE;
        } else if (dhcp_opt->opt == DHCP_OPT_REQIP) {
            if (dhcp_opt->len != 4) {
                return;
            }
            memcpy(&request_addr, (char *) dhcp_opt + DHCP_OPT_HDR_LEN, dhcp_opt->len);
            opt_flags |= HAS_OPT_REQIP;
        } else if (dhcp_opt->opt == DHCP_OPT_HOSTNAME) {
            if (dhcp_opt->len < 1) {
                return;
            }
            memcpy(hostname, (char *) dhcp_opt + DHCP_OPT_HDR_LEN, dhcp_opt->len);
            opt_flags |= HAS_OPT_HOSTNAME;
        } else if (dhcp_opt->opt == DHCP_OPT_SERVERIP) {
            if (dhcp_opt->len != 4) {
                return;
            }
            memcpy(&server_addr, (char *) dhcp_opt + DHCP_OPT_HDR_LEN, dhcp_opt->len);
            opt_flags |= HAS_OPT_SERVERIP;
        } else if (dhcp_opt->opt == DHCP_OPT_END) {
            opt_flags |= HAS_OPT_END;
            break;
        }

        if (((char *) dhcp_opt - (char *)udp_hdr) + DHCP_OPT_HDR_LEN + dhcp_opt->len > udp_len) {
            break;
        }
        dhcp_opt = (struct dhcp_opt *) ((char *) dhcp_opt + DHCP_OPT_HDR_LEN + dhcp_opt->len);
    }

    if ((opt_flags & 0x1f) != HAS_ALL_NEEDING_OPTS) {
        return;
    }

    int offset = Log_FillHead("dhcplog", 1, info, sizeof(info));

    offset += snprintf(info + offset, sizeof(info) - offset,
            ",\"client_mac\":\""ETH_MAC_FORMAT"\",\"client_ip\":\""IPV4_FORMAT"\","
            "\"client_name\":\"%s\",\"server_ip\":\""IPV4_FORMAT"\"",
            dhcp_h->chaddr[0], dhcp_h->chaddr[1], dhcp_h->chaddr[2], dhcp_h->chaddr[3], dhcp_h->chaddr[4],
            dhcp_h->chaddr[5], NIPQUAD(request_addr), hostname, NIPQUAD(server_addr));

    offset += Log_FillTail(info + offset, sizeof(info) - offset);

    LOG_Output(&config->log_dhcp_utl, info, offset);
}

