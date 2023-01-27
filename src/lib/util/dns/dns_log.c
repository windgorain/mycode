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
#include "utl/trie_utl.h"
#include "utl/data2hex_utl.h"
#include "utl/dns_utl.h"
#include "utl/dns_log.h"
#include "utl/itoa.h"

#define DOMAIN_NAME_MIN_LEN 3

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

int dnslog_matchRule_byDomain(TRIE_HANDLE trie, char *domainname)
{
    HANDLE hID;
    char invert_domain[255] = {0};

    MEM_Invert(domainname, strlen(domainname), invert_domain);
    TRIE_COMMON_S *trieComm = Trie_Match(trie, (UCHAR *)invert_domain,
            strlen(invert_domain), TRIE_MATCH_WILDCARD);
    if(!trieComm){
        return 0;
    }
    hID = trieComm->ud;

    return HANDLE_UINT(hID);
}


/* 从文件加载字符串,每行一条 */
static int dnslog_domainList_LoadFile_(TRIE_HANDLE trie, char *filename)
{
    FILE *fp;
    char buf[256];
    ULONG len;
    TRIE_COMMON_S *triComm = NULL;
    int i = 0;
    char c;
    char invert_domain[256] = {0};

    fp = fopen(filename, "rb");
    if (NULL == fp) {
        RETURN(BS_CAN_NOT_OPEN);
    }

    while(NULL != fgets(buf, sizeof(buf), fp)) {
        TXT_Strim(buf);
        len = TXT_StrimTail(buf, strlen(buf), "\r\n");
        buf[len] = '\0';
        MEM_Invert(buf, strlen(buf), invert_domain);

        for(i=0; invert_domain[i] != '\0'; i++) {
            c = invert_domain[i];
            if (c == '*') {
                invert_domain[i] = '\0';
            }
        }

        triComm = Trie_Insert(trie, (UCHAR *)invert_domain, strlen(invert_domain), TRIE_NODE_FLAG_WILDCARD, UINT_HANDLE(1));
        if(!triComm){
            continue; //已经存在
        }
    }

    fclose(fp);

    return 0;
}

static int _dnslog_process_kv(LSTR_S *pstKey, LSTR_S *pstValue, void *pUserHandle)
{
    DNS_LOG_S *config = pUserHandle;
    char filename[FILE_MAX_PATH_LEN + 1];
    char buf[256];
    char *file;
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
    } else if ((LSTR_StrCmp(pstKey, "domain_whitelist") == 0) && (pstValue->uiLen > 0)
                && (pstValue->pcData[0] == '1')) {
        //直接修改树类型TRIE_TYPE_4BITS 即可切换
        LSTR_Strlcpy(pstValue, sizeof(buf), buf);
        file = FILE_ToAbsPath(config->config_base_dir, buf,
                filename, sizeof(filename));
        if (file != NULL) {
            config->domain_whitelist_enable = 1;
            config->domain_trie = Trie_Create(TRIE_TYPE_4BITS);
            dnslog_domainList_LoadFile_(&config->domain_trie, file);
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

    int offset = scnprintf(buf, buflen, ",\"payloadhex\":\"");

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

    return scnprintf(info, infosize,
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

    if (BS_OK != DNS_GetQueryNameByPacket(payload, payload_len, domain_name)) {
        domain_name[0] = 0;
    }

    if ((config->domain_whitelist_enable != 0) && (strlen(domain_name) > DOMAIN_NAME_MIN_LEN)
            && (0 != dnslog_matchRule_byDomain(config->domain_trie,domain_name))) {
        return;
    }

    if (config->log_utl.file) {
        char log_buf[MAX_LOG_BUF];
        int offset = 0;

        offset = Log_FillHead("dnslog", 1, log_buf, MAX_LOG_BUF);

        offset += IP46_Header2String(&ipheader, ippkt, log_buf + offset, MAX_LOG_BUF - offset, 0);

        offset += scnprintf(log_buf + offset, MAX_LOG_BUF - offset,
                ",\"sport\":%u,\"dport\":%u,\"length\":%u,\"domain_name\":\"%s\"",
                ntohs(udpheader->usSrcPort), ntohs(udpheader->usDstPort), payload_len, domain_name);

        offset += Log_Fill_DNSHead(payload, payload_len, log_buf + offset, MAX_LOG_BUF - offset);

        if (config->payload_hex) {
            offset += Log_Fill_DNSPayload(payload, payload_len, log_buf + offset, MAX_LOG_BUF - offset);
        }

        offset += Log_FillTail(log_buf + offset, MAX_LOG_BUF - offset);

        LOG_Output(&config->log_utl, log_buf, offset);
    }

    return;
}
