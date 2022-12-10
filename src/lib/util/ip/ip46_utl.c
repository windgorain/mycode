/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/ip_utl.h"
#include "utl/ip_string.h"

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
        offset = scnprintf(buf + offset, bufsize - offset, 
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
        offset = scnprintf(buf + offset, bufsize - offset,
                ",\"ip6_head_len\":%u,\"ip6_info\":%u,\"ip6_total_length\":%u,\"ip6_label\":%u,"
                "\"ip_ttl\":%u,\"ip_protocol\":%u, \"sip\":\"%s\",\"dip\":\"%s\"",
                len, IP6_HEAD_FLOW_INFO(iph6), ntohs(iph6->len), IP6_HEAD_FLOW_LABLE(iph6),
                iph6->hop_lmt, iph6->next, ip6_str_src, ip6_str_dst);
    }

    if (dump_hex) {
        offset += scnprintf(buf + offset, bufsize - offset, ",\"ipheaderhex\":\"");
        DH_Data2HexString(ippkt, len, buf + offset);
        offset += 2 * len;
        buf[offset++] = '"';
    }

    return offset;
}

int IP46_GetIPHeader(IP46_HEAD_S *pstIpHead, UCHAR *pucData, UINT uiDataLen, NET_PKT_TYPE_E enPktType)
{
    NET_PKT_TYPE_E enPktTypeTmp = enPktType;
    ETH_PKT_INFO_S stPktInfo;
    UINT uiHeadLen = 0;

    if (enPktTypeTmp == NET_PKT_TYPE_ETH) {
        if (BS_OK != ETH_GetEthHeadInfo(pucData, uiDataLen, &stPktInfo)) {
            RETURN(BS_ERR);
        }

        if ((stPktInfo.usType != ETH_P_IP) && (stPktInfo.usType != ETH_P_IP6)) {
            RETURN(BS_ERR);
        }

        if (stPktInfo.usType != ETH_P_IP ) {
            enPktTypeTmp = NET_PKT_TYPE_IP6;
        } else {
            enPktTypeTmp = NET_PKT_TYPE_IP;
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

