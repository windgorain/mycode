#include "bs.h"
#include "utl/net.h"
#include "utl/ip_utl.h"
#include "utl/udp_utl.h"
#include "utl/udp_log.h"
#include "utl/dhcp_utl.h"
#include "utl/dhcp_log.h"
#ifdef USE_REDIS
#include "utl/redis_utl.h"
#endif

void DhcpLog_Process(UDP_LOG_S *config, UDP_HEAD_S *udp_hdr, UINT pktlen)
{
    struct dhcp_hdr *dhcp_h;
    struct dhcp_opt *dhcp_opt;
   
    char hostname[CLIENTNAME_LEN] = "", client_id_buf[256] = "", param_req_buf[256] = "", vendor_id[256] = "";
    uint8_t msg_type = 0, client_type = 0, client_id_len = 0;
    uint32_t request_addr = 0, server_addr = 0;
    uint32_t opt_flags = 0, len = 0;
    uint32_t client_len = 0, req_len = 0;
    int i = 0;
    char info[1024];

    memset(info,0,sizeof(info));
    memset(client_id_buf,0,sizeof(client_id_buf));

    /*Not request pkt*/
    if (ntohs(udp_hdr->usSrcPort) != DHCP_DFT_CLIENT_PORT || ntohs(udp_hdr->usDstPort) != DHCP_DFT_SERVER_PORT) {
        return;
    }

     /* packet too short */
    if (pktlen < IP_HDR_LEN + UDP_HDR_LEN + DHCP_HDR_LEN + DHCP_OPT_HDR_LEN) {
        return;
    }

    len = pktlen - IP_HDR_LEN;

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
        } else if (dhcp_opt->opt == DHCP_LEASE_TIME) {
            /* do nothing */
            opt_flags |= HAS_OPT_LEASTTIME;
         }else if (dhcp_opt->opt == DHCP_OPT_HOSTNAME) {
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
        } else if (dhcp_opt->opt == DHCP_OPT_CLIID) {
            client_type = *((char*)dhcp_opt + DHCP_OPT_HDR_LEN);
            client_id_len = dhcp_opt->len - 1;
            char *client_id = ((char *) dhcp_opt + DHCP_OPT_HDR_LEN + 1);
            if (client_type == 1) {
                for (i = 0; client_id && i < client_id_len; i++) {
                    client_len += scnprintf(client_id_buf + client_len, sizeof(client_id_buf) - client_len, 
                            "%02x:", (unsigned char)client_id[i]);
                }
            } else {
                for (i = 0; client_id && i < client_id_len; i++) {
                    client_len += scnprintf(client_id_buf + client_len, sizeof(client_id_buf) - client_len, 
                            "%c", (unsigned char)client_id[i]);
                }
            }
            opt_flags |= HAS_OPT_CLIID;
        } else if (dhcp_opt->opt == DHCP_OPT_REQLIST) {
            char *req_list = ((char*)dhcp_opt + DHCP_OPT_HDR_LEN);
            int req_list_len = dhcp_opt->len;
            for (i = 0; req_list && i < req_list_len && i < sizeof(param_req_buf); i++) {
                req_len += scnprintf(param_req_buf + req_len, sizeof(param_req_buf)-req_len, 
                        "%d,", (unsigned char)req_list[i]);
            }
            if (req_len) {
                param_req_buf[req_len - 1] = '\0';
                opt_flags |= HAS_OPT_REQLIST;
            }   
        } else if (dhcp_opt->opt == DHCP_OPT_VENDORID) {
           memcpy(vendor_id, (char*)dhcp_opt + DHCP_OPT_HDR_LEN, dhcp_opt->len);
           vendor_id[dhcp_opt->len] = 0;
           opt_flags |= HAS_OPT_VENDORID;
        } else if (dhcp_opt->opt == DHCP_OPT_END) {
            opt_flags |= HAS_OPT_END;
            break;
        }

        if (((char *) dhcp_opt - (char *)udp_hdr) + DHCP_OPT_HDR_LEN + dhcp_opt->len > len) {
            break;
        }
        dhcp_opt = (struct dhcp_opt *) ((char *) dhcp_opt + DHCP_OPT_HDR_LEN + dhcp_opt->len);
    }
    #if 0
    if ((opt_flags & 0x1f) != HAS_ALL_NEEDING_OPTS) {
        return;
    }
    #endif

    if (config->log_dhcp_utl.file || config->dhcp_db_enable) {

        int offset = Log_FillHead("dhcplog", 1, info, sizeof(info));
        if (0 == request_addr) request_addr = dhcp_h->ciaddr;

        offset += scnprintf(info + offset, sizeof(info) - offset,
                ",\"client_mac\":\""ETH_MAC_FORMAT"\",\"client_ip\":\""IPV4_FORMAT"\","
                "\"client_name\":\"%s\",\"server_ip\":\""IPV4_FORMAT"\",\"client_id_type\":\"%02x\"",
                dhcp_h->chaddr[0], dhcp_h->chaddr[1], dhcp_h->chaddr[2], dhcp_h->chaddr[3], dhcp_h->chaddr[4],
                dhcp_h->chaddr[5], NIPQUAD(request_addr), hostname, NIPQUAD(server_addr), client_type);
        if (opt_flags & HAS_OPT_CLIID) {
            offset += scnprintf(info + offset, sizeof(info) - offset, ",\"client_id_val\":\"%s\"", client_id_buf);
        }
        if (opt_flags & HAS_OPT_REQLIST) {
            offset += scnprintf(info + offset, sizeof(info) - offset, ",\"param_req_list\":\"%s\"",param_req_buf);
        }
        if (opt_flags & HAS_OPT_VENDORID) {
            offset += scnprintf(info + offset, sizeof(info) - offset, ",\"vendor_id\":\"%s\"", vendor_id);
        }

        if (config->log_dhcp_utl.file) {
            offset += Log_FillTail(info + offset, sizeof(info) - offset);
            LOG_Output(&config->log_dhcp_utl, info, offset);
        }
#ifdef USE_REDIS
        if (config->dhcp_db_enable) {
            char *key = "dhcp";
            void *redisHandle = redisGetRedisHandle();
            info[offset - 1] = '\0';
            redisWriteDataToListTail(redisHandle,key,info);
        }
#endif
    }
}

