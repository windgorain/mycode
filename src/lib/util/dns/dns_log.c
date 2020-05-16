#include "bs.h"
#include "utl/net.h"
#include "utl/eth_utl.h"
#include "utl/udp_utl.h"
#include "utl/ip_utl.h"
#include "utl/txt_utl.h"
#include "utl/file_utl.h"
#include "utl/lstr_utl.h"
#include "utl/ip_string.h"
#include "utl/bloomfilter_utl.h"
#include "utl/data2hex_utl.h"
#include "utl/dns_utl.h"
#include "utl/dns_log.h"
#include "utl/itoa.h"

typedef struct {
    UINT sip;
    UINT dip;
    USHORT sport;
    USHORT dport;
}DNSLOG_KEY_S;

static int _dnslog_bloomfilter_test(DNS_LOG_S *config, UINT sip, UINT dip, USHORT sport, UINT dport)
{
    DNSLOG_KEY_S key = {0};

    key.sip = sip;
    key.dip = dip;
    key.sport = sport;
    key.dport = dport;

    if (0 == BloomFilter_TrySet(&config->bloomfilter, &key, sizeof(key))) {
        return 1;
    }

    return 0;
}


static int _dnslog_process_kv(LSTR_S *pstKey, LSTR_S *pstValue, void *pUserHandle)
{
    DNS_LOG_S *config = pUserHandle;

    if ((LSTR_StrCmp(pstKey, "log_utl") == 0) && (pstValue->uiLen > 0)) {
        LOG_ParseConfig(&config->log_utl, pstValue->pcData, pstValue->uiLen);
    } else if ((LSTR_StrCmp(pstKey, "payload_hex") == 0) && (pstValue->uiLen > 0) && (pstValue->pcData[0] == '1')) {
        config->payload_hex = 1;
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

static int Log_Fill_DNSPayload(void *payload, UINT payload_len, char *buf, int buflen)
{
    int len = 0;

    len = MIN(payload_len, 500);
    if (len <= 0) {
        return 0;
    }

    int offset = snprintf(buf, buflen, ",\"payloadhex\":\"");

    DH_Data2HexString(payload, len, buf + offset);
    offset += 2*len;
    buf[offset++] = '"';

    return offset;
}

void DNSLOG_Init(DNS_LOG_S *config, char * conf_base_dir, char *log_base_dir)
{
    memset(config, 0, sizeof(DNS_LOG_S));
    config->config_base_dir = conf_base_dir;
    LOG_Init(&config->log_utl);
    LOG_SetBaseDir(&config->log_utl, log_base_dir);
}

BS_STATUS DNSLOG_ParseConfig(DNS_LOG_S *config, char *conf_string)
{
    char *start, *end;
    LSTR_S stConfig;

    if (0 != TXT_FindBracket(conf_string, strlen(conf_string), "{}", &start, &end)) {
        return BS_ERR;
    }

    start ++;

    stConfig.pcData = start;
    stConfig.uiLen = end - start;

    LSTR_ScanMultiKV(&stConfig, ',', ':', _dnslog_process_kv, config);

    config->log_enable = 1;

    return 0;
}

int Log_Fill_DNSHead(IN VOID *dnspkt, IN int pktlen, OUT CHAR *info, IN UINT infosize)
{
    DNS_HEADER_S *dns = dnspkt;

    *info = 0;

    if (pktlen < sizeof(DNS_HEADER_S)) {
        return 0;
    }

    return snprintf(info, infosize,
            ",\"ID\":%u,\"QR\":%u,\"opcode\":%u,\"AA\":%u,\"TC\":%u,"
            "\"RD\":%u,\"RA\":%u,\"rcode\":%u,"
            "\"question\":%u, \"answer_rr\":%u,"
            "\"authority_rr\":%u,\"additional_rr\":%u",
            ntohs(dns->usTransID), dns->usQR, dns->usOpcode, dns->usAA, dns->usTC,
            dns->usRD, dns->usRA, dns->usRcode,
            ntohs(dns->usQdCount), ntohs(dns->usAnCount),
            ntohs(dns->usNsCount), ntohs(dns->usArCount));

   return 0;
}

void DNSLOG_Input(DNS_LOG_S *config, void *ippkt, UINT pktlen, NET_PKT_TYPE_E pkt_type)
{
    IP46_HEAD_S  ipheader = {0};
    UDP_HEAD_S *udpheader = NULL;
    UINT total_len;
    UINT payload_len = 0;
    void *payload;
    char domain_name[DNS_MAX_DOMAIN_NAME_LEN + 1];
    if (config->log_enable == 0) {
        return;
    }

    if (0 != IP46_GetIPHeader(&ipheader, ippkt, pktlen, pkt_type)) {
        return;
    }

    if (ipheader.family == ETH_P_IP) {
        udpheader = UDP_GetUDPHeader(((UCHAR*)ipheader.iph.ip4) + IP_HEAD_LEN(ipheader.iph.ip4), 
                    pktlen - IP_HEAD_LEN(ipheader.iph.ip4), NET_PKT_TYPE_UDP);
    } else if (ipheader.family == ETH_P_IP6) {
        udpheader = UDP_GetUDPHeader(((UCHAR*)ipheader.iph.ip6) + IP6_HDR_LEN, 
                pktlen - IP6_HDR_LEN, NET_PKT_TYPE_UDP);
    }
    if (udpheader == NULL) {
        return;
    }

    if ((config->bloomfilter_size != 0) && (ipheader.family == ETH_P_IP)
            && (0 == _dnslog_bloomfilter_test(config,
                    ipheader.iph.ip4->unSrcIp.uiIp, ipheader.iph.ip4->unDstIp.uiIp,
                    udpheader->usSrcPort, udpheader->usDstPort))) {
        return;
    }

    if (ipheader.family == ETH_P_IP) {
        total_len = ntohs(ipheader.iph.ip4->usTotlelen);
        total_len = MIN(total_len, pktlen);
        payload_len = (total_len - IP_HEAD_LEN(ipheader.iph.ip4)) - sizeof(UDP_HEAD_S);
    } else if (ipheader.family ==ETH_P_IP6) {
        total_len = ntohs(ipheader.iph.ip6->len);
        total_len = MIN(total_len, pktlen);
        payload_len = (total_len - IP6_HDR_LEN) - sizeof(UDP_HEAD_S);
    }

    if (payload_len < sizeof(DNS_HEADER_S)) {
        return;
    }

    payload = udpheader + 1;

    if (BS_OK != DNS_GetDomainNameByPacket(payload, payload_len, domain_name)) {
        domain_name[0] = 0;
    }

    if (config->log_utl.file) {
        char log_buf[MAX_LOG_BUF];
        int offset = 0;

        offset = Log_FillHead("dnslog", 1, log_buf, MAX_LOG_BUF);

        offset += IP46_Header2String(&ipheader, ippkt, log_buf + offset, MAX_LOG_BUF - offset, 0);

        offset += snprintf(log_buf + offset, MAX_LOG_BUF - offset,
                ",\"sport\":%u,\"dport\":%u,\"length\":%u,\"domain_name\":\"%s\"",
                ntohs(udpheader->usSrcPort), ntohs(udpheader->usDstPort), payload_len, domain_name);

        offset += Log_Fill_DNSHead(payload, payload_len, log_buf + offset, MAX_LOG_BUF - offset);

        if (config->payload_hex) {
            offset += Log_Fill_DNSPayload(payload, payload_len, log_buf + offset, MAX_LOG_BUF - offset);
        }

        offset += Log_FillTail(log_buf + offset, MAX_LOG_BUF - offset);

        LOG_Output(&config->log_utl, log_buf, offset);
        LOG_Flush(&config->log_utl);
    }

    return;
}
