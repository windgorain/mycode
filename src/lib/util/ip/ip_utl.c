/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/net.h"
#include "utl/eth_utl.h"
#include "utl/ip_utl.h"
#include "utl/in_checksum.h"
#include "utl/ip_string.h"

/* 返回网络序的校验和 */
USHORT IP_CheckSum (IN UCHAR *pucBuf/* IP头 */, IN UINT ulLen/* IP头长度 */)
{
    return IN_CHKSUM_CheckSum(pucBuf, ulLen);
}


IP_HEAD_S * IP_GetIPHeader(IN UCHAR *pucData, IN UINT uiDataLen, IN NET_PKT_TYPE_E enPktType)
{
    NET_PKT_TYPE_E enPktTypeTmp = enPktType;
    ETH_PKT_INFO_S stPktInfo;
    UINT uiHeadLen = 0;
    IP_HEAD_S *pstIpHead = NULL;

    if (enPktTypeTmp == NET_PKT_TYPE_ETH)
    {
        if (BS_OK != ETH_GetEthHeadInfo(pucData, uiDataLen, &stPktInfo))
        {
            return NULL;
        }

        if (stPktInfo.usType != ETH_P_IP)
        {
            return NULL;
        }

        enPktTypeTmp = NET_PKT_TYPE_IP;
        uiHeadLen += stPktInfo.usHeadLen;
    }

    if (enPktTypeTmp == NET_PKT_TYPE_IP)
    {
        if (uiHeadLen + sizeof(IP_HEAD_S) > uiDataLen)
        {
            return NULL;
        }

        pstIpHead = (IP_HEAD_S*)(pucData + uiHeadLen);

        if (uiHeadLen + IP_HEAD_LEN(pstIpHead) > uiDataLen)
        {
            return NULL;
        }
    }

    return pstIpHead;
}

int IP46_GetIPHeader(IP46_HEAD_S *pstIpHead, IN UCHAR *pucData,
        IN UINT uiDataLen, IN NET_PKT_TYPE_E enPktType)
{
    NET_PKT_TYPE_E enPktTypeTmp = enPktType;
    ETH_PKT_INFO_S stPktInfo;
    UINT uiHeadLen = 0;

    if (enPktTypeTmp == NET_PKT_TYPE_ETH) {
        if (BS_OK != ETH_GetEthHeadInfo(pucData, uiDataLen, &stPktInfo)) {
            RETURN(BS_ERR);
        }

        if (stPktInfo.usType != ETH_P_IP ||stPktInfo.usType != ETH_P_IP6) {
            RETURN(BS_ERR);
        }

        if (stPktInfo.usType != ETH_P_IP ) {
            enPktTypeTmp = NET_PKT_TYPE_IP;
        } else {
            enPktTypeTmp = NET_PKT_TYPE_IP6;
        }

        uiHeadLen += stPktInfo.usHeadLen;
    }

    if (enPktTypeTmp == NET_PKT_TYPE_IP) {
        if (uiHeadLen + sizeof(IP_HEAD_S) > uiDataLen) {
            RETURN(BS_ERR);
        }
        pstIpHead->family = ETH_P_IP;
        pstIpHead->iph.ip4 = (IP_HEAD_S*)(pucData + uiHeadLen);

        if (uiHeadLen + IP_HEAD_LEN(pstIpHead->iph.ip4) > uiDataLen) {
            RETURN(BS_ERR);
        }
    } else if (enPktTypeTmp == NET_PKT_TYPE_IP6) { 
        if (uiHeadLen + sizeof(IP6_HEAD_S) > uiDataLen) {
            RETURN(BS_ERR);
        }

        pstIpHead->family = ETH_P_IP6;
        pstIpHead->iph.ip6 = (IP6_HEAD_S*)(pucData + uiHeadLen);

        if (uiHeadLen + IP6_HDR_LEN > uiDataLen) {
            RETURN(BS_ERR);
        }
    }

    return 0;
}

BOOL_T IPUtl_IsExistInIpArry(IN IP_MAKS_S *pstIpMask, IN UINT uiNum, IN UINT uiIP, IN UINT uiMask)
{
    UINT i;

    for(i=0; i<uiNum; i++)
    {
        if ((pstIpMask[i].uiIP == uiIP) && (pstIpMask[i].uiMask == uiMask))
        {
            return TRUE;
        }
    }

    return FALSE;
}

static const char * inet_ntop4(const struct in_addr *addr, char *buf, socklen_t len)
{
  const u_int8_t *addr_p = (const u_int8_t *)&addr->s_addr;
  char tmp[INET_ADDRSTRLEN]; /* max length of ipv4 addr string */
  int fulllen;

  fulllen = snprintf(tmp, sizeof(tmp), "%d.%d.%d.%d",
             addr_p[0], addr_p[1], addr_p[2], addr_p[3]);
  if (fulllen >= (int)len) {
    return NULL;
  }

  bcopy(tmp, buf, fulllen + 1);

  return buf;
}

const char * inet_ntop6_full(const struct in6_addr *addr, char *dst, socklen_t size)
{
    char tmp[INET6_ADDRSTRLEN];
    size_t len = 0;
    int32_t i,j;
    uint8_t x8 =0;
    struct in_addr addr4;

    if (addr == NULL) return NULL;

    bzero(tmp, sizeof(tmp));

    /*  check for mapped or compat addresses */
    i = IN6_IS_ADDR_V4MAPPED(addr);
    j = IN6_IS_ADDR_V4COMPAT(addr);
    if ((i != 0) || (j != 0)) {
        char tmp2[16]; /* max length of ipv4 addr string */
        addr4.s_addr = addr->s6_addr32[3];
        len = snprintf(tmp, sizeof(tmp), "::%s%s", (i != 0) ? "ffff:" : "",
                inet_ntop4(&addr4, tmp2, sizeof(tmp2)));
        if (len >= size) return NULL;
        bcopy(tmp, dst, len + 1);
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

    /* trailing NULL */
    if (++len > size) {
        return NULL;
    }

    bcopy(tmp, dst, len);

    return dst;
}

char * IPUTL_IP2String(UINT ip/*net order*/, OUT char *buf, IN int buf_size)
{
    UCHAR *ch = (void*)&ip;

    snprintf(buf, buf_size, "%u.%u.%u.%u", ch[0], ch[1], ch[2], ch[3]);

    return buf;
}

int IP46_Header2String(IP46_HEAD_S *iph, IN VOID *ippkt, IN CHAR *buf, IN UINT bufsize, IN CHAR dump_hex)
{
    char sip[32];
    char dip[32];
    int offset = 0;
    int len =0;
    char ip6_str_src[INET6_ADDRSTRLEN];
    char ip6_str_dst[INET6_ADDRSTRLEN];

    if (iph->family == ETH_P_IP) {
        IP_HEAD_S *iph4 = iph->iph.ip4;
        len =  iph4->ucHLen*4;
        offset = snprintf(buf + offset, bufsize - offset, 
                ",\"ip_head_len\":%u,\"ip_tos\":%u,\"ip_total_length\":%u,\"ip_pkt_id\":%d,"
                "\"ip_flag\":%u,\"ip_frag_offset\":%u,\"ip_ttl\":%u,\"ip_protocol\":%u,"
                "\"ip_head_chksum\":\"%04x\", \"sip\":\"%s\",\"dip\":\"%s\"",
                len, iph4->ucTos, ntohs(iph4->usTotlelen), ntohs(iph4->usIdentification),
                IP_HEAD_FLAG(iph4), IP_HEAD_FRAG_OFFSET(iph4), iph4->ucTtl, iph4->ucProto, iph4->usCrc,
                IPString_IP2String(iph4->unSrcIp.uiIp, sip), IPString_IP2String(iph4->unDstIp.uiIp, dip));
    } else if(iph->family == ETH_P_IP6) {
        IP6_HEAD_S *iph6 = iph->iph.ip6;
        len =  IP6_HDR_LEN;
        inet_ntop6_full(&iph6->ip6_src, ip6_str_src, sizeof(ip6_str_src));
        inet_ntop6_full(&iph6->ip6_dst, ip6_str_dst, sizeof(ip6_str_dst));
        offset = snprintf(buf + offset, bufsize - offset,
                ",\"ip6_head_len\":%u,\"ip6_info\":%u,\"ip6_total_length\":%u,\"ip6_label\":%u,"
                "\"ip_ttl\":%u,\"ip_protocol\":%u, \"sip\":\"%s\",\"dip\":\"%s\"",
                len, iph6->vcl & IPV6_FLOWINFO_MASK, ntohs(iph6->len), iph6->vcl & IPV6_FLOWLABEL_MASK,
                iph6->hop_lmt, iph6->next, ip6_str_src, ip6_str_dst);
    }

    if (dump_hex) {
        offset += snprintf(buf + offset, bufsize - offset, ",\"ipheaderhex\":\"");
        DH_Data2HexString(ippkt, len, buf + offset);
        offset += 2 * len;
        buf[offset++] = '"';
    }

    return offset;
}

