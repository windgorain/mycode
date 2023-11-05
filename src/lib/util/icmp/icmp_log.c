
#include "bs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include "utl/data2hex_utl.h"
#include "utl/txt_utl.h"
#include "utl/file_utl.h"
#include "utl/lstr_utl.h"
#include "utl/net.h"
#include "utl/eth_utl.h"
#include "utl/ip_utl.h"
#include "utl/ip_string.h"
#include "utl/icmp_utl.h"
#include "utl/icmp_log.h"
#include "utl/itoa.h"

typedef struct {
    UINT sip;
    UINT dip;
    UINT id;
    UINT seq;
}ICMPLOG_KEY_S;

static int _icmplog_process_kv(LSTR_S *pstKey, LSTR_S *pstValue, void *pUserHandle)
{
    ICMP_LOG_S *config = pUserHandle;

    if ((LSTR_StrCmp(pstKey, "log_utl") == 0) && (pstValue->uiLen > 0)) {
        LOG_ParseConfig(&config->log_utl, pstValue->pcData, pstValue->uiLen);
    } else if ((LSTR_StrCmp(pstKey, "log_all_pkt") == 0) && (pstValue->uiLen > 0) && (pstValue->pcData[0] == '1')) {
        config->log_all_pkt = 1;
    } else if ((LSTR_StrCmp(pstKey, "ip_header_hex") == 0) && (pstValue->uiLen > 0) && (pstValue->pcData[0] == '1')) {
        config->ip_header_hex = 1;
    } else if ((LSTR_StrCmp(pstKey, "icmp_header_hex") == 0) && (pstValue->uiLen > 0) && (pstValue->pcData[0] == '1')) {
        config->icmp_header_hex = 1;
    } else if ((LSTR_StrCmp(pstKey, "payload_hex") == 0) && (pstValue->uiLen > 0) && (pstValue->pcData[0] == '1')) {
        config->icmp_payload_hex = 1;
    } else if ((LSTR_StrCmp(pstKey, "bloomfilter") == 0) && (pstValue->uiLen > 0)) {
        config->bloomfilter_size = LSTR_A2ui(pstValue);
        if (config->bloomfilter_size > 0) {
            BloomFilter_Init(&config->bloomfilter, config->bloomfilter_size);
            BloomFilter_SetStepsToClearAll(&config->bloomfilter, 60);
            BloomFilter_SetAutoStep(&config->bloomfilter, 1);
        }
    }

    return BS_OK;
}

static int _icmplog_bloomfilter_test(ICMP_LOG_S *config, UINT sip, UINT dip, USHORT id, USHORT seq)
{
    ICMPLOG_KEY_S key = {0};

    key.sip = sip;
    key.dip = dip;
    key.id = id;
    key.seq = seq;

    if (0 == BloomFilter_TrySet(&config->bloomfilter, &key, sizeof(key))) {
        return 1;
    }

    return 0;
}

CHAR * ICMP_Header2String(IN VOID *tcp, OUT CHAR *info, IN UINT infosize)
{
    ICMP_ECHO_HEAD_S *icmph = tcp;

    scnprintf(info, infosize,
            "\"type\":%u,\"code\":%u,\"id\":%u, \"seq\":%u",
            icmph->stIcmpHeader.ucType, icmph->stIcmpHeader.ucCode,
            ntohs(icmph->stEcho.usIcmpId), ntohs(icmph->stEcho.usSn)); 

    return info;
}

int Log_Fill_IcmpHead(ICMP_ECHO_HEAD_S *icmph, char *buf, int buflen, char dump_hex)
{
    if (!icmph) return 0;

    int offset = scnprintf(buf, buflen,
            ",\"type\":%u,\"code\":%u,\"id\":%u, \"seq\":%u",
            icmph->stIcmpHeader.ucType, icmph->stIcmpHeader.ucCode,
            ntohs(icmph->stEcho.usIcmpId), ntohs(icmph->stEcho.usSn)); 

    if (dump_hex) {
        offset += scnprintf(buf + offset, buflen - offset, ",\"icmpheaderhex\":\"");
        DH_Data2HexString((void*)icmph, sizeof(ICMP_HEAD_S), buf + offset);
        offset += 2 * sizeof(ICMP_HEAD_S);
        buf[offset++]= '"';
    }

    return offset;
}

int fill_log_icmpbody(IP46_HEAD_S *ipheader, int pktlen, ICMP_ECHO_HEAD_S *icmpheader, char *buf, int buflen)
{
    UINT payload_len = 0;
    void *payload = NULL;;
    int offset = 0;

    payload_len = IP46_GetIPDataLen(ipheader, pktlen);

    if (icmpheader && payload_len > sizeof(ICMP_HEAD_S)) {
        payload_len -= sizeof(ICMP_HEAD_S);
        payload = icmpheader + 1;
    }
    if (payload_len > 0 && payload) {
        offset+=scnprintf(buf, buflen, ",\"payloadhex\":\"");
        payload_len = MIN(payload_len, 50);
        DH_Data2HexString(payload, payload_len, buf + offset);
        offset += 2*payload_len;
        buf[offset++] = '"';
    }

    return offset;
}

void ICMPLOG_Init(ICMP_LOG_S *config, char * conf_base_dir, char *log_base_dir)
{
    memset(config, 0, sizeof(ICMP_LOG_S));
    config->config_base_dir = conf_base_dir;
    LOG_Init(&config->log_utl);
    LOG_SetBaseDir(&config->log_utl, log_base_dir);
}

BS_STATUS ICMPLOG_ParseConfig(ICMP_LOG_S *config, char *conf_string)
{
    char *start, *end;
    LSTR_S stConfig;

    if (0 != TXT_FindBracket(conf_string, strlen(conf_string), "{}", &start, &end)) {
        return BS_ERR;
    }

    start ++;

    stConfig.pcData = start;
    stConfig.uiLen = end - start;

    LSTR_ScanMultiKV(&stConfig, ',', ':', _icmplog_process_kv, config);

    config->log_enable = 1;

    return 0;
}

void ICMPLOG_Input(ICMP_LOG_S *config, VOID *ippkt, UINT pktlen, NET_PKT_TYPE_E pkt_type)
{
    IP46_HEAD_S ipheader;
    ICMP_ECHO_HEAD_S *icmpheader;

    if (0 != IP46_GetIPHeader(&ipheader,ippkt, pktlen, pkt_type)) {
        return;
    }

    
    if (ipheader.family == ETH_P_IP) {
        IP_HEAD_S *ip4 = ipheader.iph.ip4;
        if (IP_HEAD_FRAG_OFFSET(ip4) == 0) {
            icmpheader = ICMP_GetEchoHeader(((UCHAR*)ip4) + IP_HEAD_LEN(ip4),
                    pktlen - IP_HEAD_LEN(ip4), NET_PKT_TYPE_ICMP);
            if (icmpheader == NULL) {
                return;
            }

            if (config->bloomfilter_size > 0) {
                if ((icmpheader->stIcmpHeader.ucType == 0 || icmpheader->stIcmpHeader.ucType == 8)
                        && (_icmplog_bloomfilter_test(config, ip4->unSrcIp.uiIp, ip4->unDstIp.uiIp,
                                icmpheader->stEcho.usIcmpId,  icmpheader->stEcho.usSn) == 0)) {
                    return;
                }
            }
        } else {
            icmpheader = NULL;
        }
    } else {
        IP6_HEAD_S *ip6 = ipheader.iph.ip6;
        icmpheader = ICMP_GetEchoHeader(((UCHAR*)ip6) + IP6_HDR_LEN, pktlen - IP6_HDR_LEN, NET_PKT_TYPE_ICMP);
        if (icmpheader == NULL) {
            return;
        }
    }

    if (config->log_utl.file) {
        char log_buf[MAX_LOG_BUF];
        int offset = 0;

        offset += Log_FillHead("icmplog", 1, log_buf + offset, MAX_LOG_BUF);

        offset += IP46_Header2String(&ipheader, ippkt, log_buf + offset, MAX_LOG_BUF - offset, config->ip_header_hex);

        offset += Log_Fill_IcmpHead(icmpheader, log_buf + offset, MAX_LOG_BUF - offset, config->icmp_header_hex);

        if (config->icmp_payload_hex) {
            offset += fill_log_icmpbody(&ipheader, pktlen, icmpheader, log_buf + offset, MAX_LOG_BUF - offset);
        }
        offset += Log_FillTail(log_buf + offset, MAX_LOG_BUF - offset);

        LOG_Output(&config->log_utl, log_buf, offset);
    }
}
