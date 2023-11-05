/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/ip_utl.h"

char * inet_ntop6_full(const struct in6_addr *addr, char *dst, socklen_t size)
{
    char tmp[INET6_ADDRSTRLEN];
    size_t len = 0;
    int32_t i,j;
    uint8_t x8 =0;
    struct in_addr addr4;

    if (addr == NULL) return NULL;

    memset(tmp, 0, sizeof(tmp));

    
    i = IN6_IS_ADDR_V4MAPPED(addr);
    j = IN6_IS_ADDR_V4COMPAT(addr);
    if ((i != 0) || (j != 0)) {
        char tmp2[16]; 
        addr4.s_addr = addr->s6_addr32[3];
        len = scnprintf(tmp, sizeof(tmp), "::%s%s", (i != 0) ? "ffff:" : "",
                inet_ntop4(&addr4, tmp2, sizeof(tmp2)));
        if (len >= size) return NULL;
        memcpy(dst, tmp, len + 1);
        return dst;
    }

    for (i = 0; i < 16; i += 2) {
        x8 = addr->s6_addr[i];
        UCHAR_2_HEX(x8, tmp + len);
        len += 2;

        x8 = addr->s6_addr[i + 1];
        UCHAR_2_HEX(x8, tmp + len);
        len += 2;

        if (i != 14) tmp[len++] = ':';
    }

    
    if (++len > size) {
        return NULL;
    }

    memcpy(dst, tmp, len);

    return dst;
}

IP6_HEAD_S * IP6_GetIPHeader(UCHAR *pucData, UINT uiDataLen, NET_PKT_TYPE_E enPktType)
{
    NET_PKT_TYPE_E enPktTypeTmp = enPktType;
    ETH_PKT_INFO_S stPktInfo;
    UINT uiHeadLen = 0;
    IP6_HEAD_S *pstIpHead = NULL;

    if (enPktTypeTmp == NET_PKT_TYPE_ETH) {
        if (BS_OK != ETH_GetEthHeadInfo(pucData, uiDataLen, &stPktInfo)) {
            return NULL;
        }
        if (stPktInfo.usType != ETH_P_IP) {
            return NULL;
        }
        enPktTypeTmp = stPktInfo.usType;
        uiHeadLen += stPktInfo.usHeadLen;
    }

    if (enPktTypeTmp == NET_PKT_TYPE_IP6) {
        if (uiHeadLen + sizeof(IP_HEAD_S) > uiDataLen) {
            return NULL;
        }
        pstIpHead = (IP6_HEAD_S*)(pucData + uiHeadLen);
        if (uiHeadLen + IP6_HDR_LEN > uiDataLen) {
            return NULL;
        }
    }

    return pstIpHead;
}

static inline int ip6_is_up_layer_type(UCHAR protocol)
{
    if ((protocol == IP_PROTO_TCP) || (protocol == IP_PROTO_UDP) || (protocol == IP_PROTO_ICMP6)) {
        return 1;
    }
    
    return 0;
}

int IP6_GetUpLayer(IP6_HEAD_S *ip6_header, int len, OUT IP6_UPLAYER_S *uplayer)
{
    UCHAR protocol = ip6_header->next;
    UCHAR *opt = (void*)(ip6_header + 1);
    UCHAR opt_len;
    int reserved_len = len - IP6_HDR_LEN;

    if (reserved_len <= sizeof(IP6_OPT_S)) {
        return -1;
    }

    
    while (! ip6_is_up_layer_type(protocol)) {
        if (protocol == IP_PROTO_FRAGMENT) {
            opt_len = 8;
        } else {
            opt_len = *(opt + 1) + 1;
        }
        reserved_len -= opt_len;
        if (reserved_len <= sizeof(IP6_OPT_S)) {
            return -1;
        }
        protocol = *opt;
        opt += opt_len;
    }

    uplayer->protocol = protocol;
    uplayer->data = opt;
    uplayer->len = reserved_len;

    return 0;
}

