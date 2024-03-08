/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2012-9-23
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/mac_table.h"
#include "utl/ippool_utl.h"
#include "utl/udp_utl.h"
#include "utl/dhcp_utl.h"
#include "utl/eth_def.h"

INT DHCP_GetDHCPMsgType (IN DHCP_HEAD_S *pstDhcpHead, IN UINT ulOptLen)
{
    const UCHAR *p = (UCHAR *) (pstDhcpHead + 1);
    UINT i;

    for (i = 0; i < ulOptLen; ++i)
    {
        const UCHAR type = p[i];
        const int room = ulOptLen - i - 1;
      
        if (type == DHCP_END)           
        {
            return -1;
        }
        else if (type == DHCP_PAD)      
        {
            ;
        }
        else if (type == DHCP_MSG_TYPE) 
        {
            if (room >= 2)
            {
                if (p[i+1] == 1)        
                {
                    return p[i+2];        
                }
            }
            return -1;
        }
        else                            
        {
            if (room >= 1)
            {
                const int len = p[i+1]; 
                i += (len + 1);         
            }
        }
    }

    return -1;
}


UINT DHCP_GetRequestIpByDhcpRequest (IN DHCP_HEAD_S *pstDhcpHead, IN UINT ulOptLen)
{
    const UCHAR *p = (UCHAR *) (pstDhcpHead + 1);
    UINT i;
    UINT ulIp;

    if (pstDhcpHead->ulClientIp != 0)
    {
        return pstDhcpHead->ulClientIp;
    }

    for (i = 0; i < ulOptLen; ++i)
    {
        const UCHAR type = p[i];
        const int room = ulOptLen - i - 1;
      
        if (type == DHCP_END)           
        {
            return 0;
        }
        else if (type == DHCP_PAD)      
        {
            ;
        }
        else if (type == DHCP_REQUEST_IP) 
        {
            if (room >= 5)
            {
                if (p[i+1] == 4)        
                {
                    ulIp = *(UINT*) (&p[i+2]);
                    return ulIp;
                }
            }
            return 0;
        }
        else                            
        {
            if (room >= 1)
            {
                const int len = p[i+1]; 
                i += (len + 1);         
            }
        }
    }

    return 0;
}

UINT DHCP_SetOpt0 (UCHAR *pucOpt, int type, IN UINT ulLen)
{
    VNET_DHCP_OPT0 stOpt;

    if (sizeof(stOpt) > ulLen)
    {
        BS_DBGASSERT (0);
        return 0;
    }

    stOpt.type = (UCHAR) type;

    MEM_Copy (pucOpt, &stOpt, sizeof(stOpt));

    return sizeof (stOpt);
}


UINT DHCP_SetOpt8 (UCHAR *pucOpt, int type, UCHAR data, IN UINT ulLen)
{
  VNET_DHCP_OPT8 stOpt;

  if (sizeof(stOpt) > ulLen)
  {
      BS_DBGASSERT (0);
      return 0;
  }

  
  stOpt.type = (UCHAR) type;
  stOpt.len = sizeof (stOpt.data);
  stOpt.data = (UCHAR) data;
  MEM_Copy (pucOpt, &stOpt, sizeof(stOpt));

  return sizeof (stOpt);
}

UINT DHCP_SetOpt32 (UCHAR *pucOpt, int type, UINT data, IN UINT ulLen)
{
  VNET_DHCP_OPT32 stOpt;

  if (sizeof(stOpt) > ulLen)
  {
      BS_DBGASSERT (0);
      return 0;
  }

  stOpt.type = (UCHAR) type;
  stOpt.len = sizeof (stOpt.data);
  stOpt.data = data;
  MEM_Copy (pucOpt, &stOpt, sizeof(stOpt));

  return sizeof (stOpt);
}

static BS_STATUS _DHCP_GetDhcpRequest(IN UDP_HEAD_S *udp_hdr, int pktlen, DHCP_Request_t *req)
{
    char hostname[CLIENTNAME_LEN] = {0}, client_id_buf[256] = "";
    char req_param_buf[256] = "", vendor_id[256] = "";
    uint8_t msg_type = 0, client_type = 0, client_id_len = 0;
    uint32_t request_addr = 0, server_addr = 0;
    uint32_t opt_flags = 0, len = 0, client_len = 0;
    int i = 0, offset = 0;
    struct dhcp_opt *dhcp_opt;
    struct dhcp_hdr *dhcp_h;

    memset(req, 0, sizeof(DHCP_Request_t));

    if (ntohs(udp_hdr->usSrcPort) != DHCP_DFT_CLIENT_PORT 
            || ntohs(udp_hdr->usDstPort) != DHCP_DFT_SERVER_PORT) {
        return BS_ERR;
    }

    
    if (pktlen < IP_HDR_LEN + UDP_HDR_LEN + DHCP_HDR_LEN + DHCP_OPT_HDR_LEN) {
        return BS_ERR;
    }

    
    len = pktlen - IP_HDR_LEN;

    dhcp_h = (struct dhcp_hdr *) ((char *) udp_hdr + UDP_HDR_LEN);
    dhcp_opt = (struct dhcp_opt *) ((char *) dhcp_h + DHCP_HDR_LEN);

    
    if (dhcp_h->op != DHCP_OP_REQUEST) {
        return BS_ERR;
    }

    
    for (i = 0; i < MAX_LOOP_TIMES; ++i) {
        if (dhcp_opt->opt == DHCP_OPT_MSGTYPE) {
            if (dhcp_opt->len != 1) {
                return BS_ERR;
            }
            memcpy(&msg_type, (char *) dhcp_opt + DHCP_OPT_HDR_LEN, dhcp_opt->len);
            
            if (msg_type != DHCP_TYPE_REQUEST) {
                return BS_ERR;
            }
            opt_flags |= HAS_OPT_MSGTYPE;
        } else if (dhcp_opt->opt == DHCP_OPT_REQIP) {
            if (dhcp_opt->len != 4) {
                return BS_ERR;
            }
            memcpy(&request_addr, (char *) dhcp_opt + DHCP_OPT_HDR_LEN, dhcp_opt->len);
            opt_flags |= HAS_OPT_REQIP;
        } else if (dhcp_opt->opt == DHCP_LEASE_TIME) {
            
            opt_flags |= HAS_OPT_LEASTTIME;
         }else if (dhcp_opt->opt == DHCP_OPT_HOSTNAME) {
            if (dhcp_opt->len < 1) {
                return BS_ERR;
            }
            memcpy(hostname, (char *) dhcp_opt + DHCP_OPT_HDR_LEN, dhcp_opt->len);
            opt_flags |= HAS_OPT_HOSTNAME;
        } else if (dhcp_opt->opt == DHCP_OPT_SERVERIP) {
            if (dhcp_opt->len != 4) {
                return BS_ERR;
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
            for (i = 0; req_list && i < req_list_len; i++) {
                offset += scnprintf(req_param_buf + offset, sizeof(req_param_buf) - offset, 
                        "%d,", (unsigned char)req_list[i]);
            }
            req_param_buf[offset - 1] = '\0';
            opt_flags |= HAS_OPT_REQLIST;
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

    scnprintf(req->client_mac, sizeof(req->client_mac),
            ETH_MAC_FMT, ETH_MAC_CHAR(dhcp_h->chaddr));

    memcpy(req->hostname, hostname, strlen(hostname));

    scnprintf(req->client_ip, sizeof(req->client_ip), 
            IPV4_FORMAT, NIPQUAD(request_addr));

    scnprintf(req->server_ip, sizeof(req->server_ip), 
            IPV4_FORMAT, NIPQUAD(server_addr));

    if (opt_flags & HAS_OPT_CLIID) {
        memcpy(req->client_id, client_id_buf, strlen(client_id_buf));
    }

    if (opt_flags & HAS_OPT_REQLIST) {
        memcpy(req->req_param, req_param_buf, strlen(req_param_buf));
    }

    if (opt_flags & HAS_OPT_VENDORID) {
        memcpy(req->vendor_id, vendor_id, strlen(vendor_id));
    }

    req->client_type = client_type;
    req->opt_flags = opt_flags;

    return BS_OK;
}

BS_STATUS DHCP_GetDhcpRequest(IN UDP_HEAD_S *udp_hdr, IN UINT pktlen, OUT DHCP_Request_t *dh_req, OUT char *req_info, IN int req_len)
{
    BOOL_T bRet = _DHCP_GetDhcpRequest(udp_hdr, pktlen, dh_req);

    if (bRet != BS_OK) return BS_ERR;

    if (!req_info || !req_len) return BS_OK;

    int offset = 0;
    offset += scnprintf(req_info, req_len, "{\"type\":\"dhcplog\",\"client_mac\":\"%s\",\"client_ip\":\"%s\",\"client_name\":\"%s\",\"server_ip\":\"%s\",\"client_id_type\":\"%02x\"",
            dh_req->client_mac, dh_req->client_ip, dh_req->hostname, 
            dh_req->server_ip, dh_req->client_type);

    if (dh_req->opt_flags & HAS_OPT_CLIID) {
        offset += scnprintf(req_info + offset, req_len - offset,
                ",\"client_id_val\":\"%s\"", dh_req->client_id);
    }
    if (dh_req->opt_flags & HAS_OPT_REQLIST) {
        offset += scnprintf(req_info + offset, req_len - offset,
                ",\"req_param_list\":\"%s\"", dh_req->req_param);
    }
    if (dh_req->opt_flags & HAS_OPT_VENDORID) {
        offset += scnprintf(req_info + offset, req_len - offset, 
                ",\"vendor_id\":\"%s\"", dh_req->vendor_id);
    }

    scnprintf(req_info + offset, req_len - offset, "%s", "}");

    return BS_OK;
}
