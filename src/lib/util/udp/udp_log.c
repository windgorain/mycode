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
#include "utl/udp_log.h"
#include "utl/dhcp_utl.h"
#include "utl/dhcp_log.h"
#include "utl/itoa.h"

typedef struct {
    UINT sip;
    UINT dip;
    USHORT sport;
    USHORT dport;
}UDPLOG_KEY_S;

static int _udplog_process_kv(LSTR_S *pstKey, LSTR_S *pstValue, void *pUserHandle)
{
    UDP_LOG_S *config = pUserHandle;

    if ((LSTR_StrCmp(pstKey, "log_utl") == 0) && (pstValue->uiLen > 0)) {
        LOG_ParseConfig(&config->log_utl, pstValue->pcData, pstValue->uiLen);
    } else if ((LSTR_StrCmp(pstKey, "log_dhcp_utl") == 0) && (pstValue->uiLen > 0)) {
        LOG_ParseConfig(&config->log_dhcp_utl, pstValue->pcData, pstValue->uiLen);
        config->dhcp_log_enable = 1;
    }else if ((LSTR_StrCmp(pstKey, "ip_header_hex") == 0) && (pstValue->uiLen > 0) && (pstValue->pcData[0] == '1')) {
        config->ip_header_hex = 1;
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

static int _udplog_bloomfilter_test(UDP_LOG_S *config, UINT sip, UINT dip, USHORT sport, UINT dport)
{
    UDPLOG_KEY_S key = {0};

    key.sip = sip;
    key.dip = dip;
    key.sport = sport;
    key.dport = dport;

    if (0 == BloomFilter_TrySet(&config->bloomfilter, &key, sizeof(key))) {
        return 1;
    }

    return 0;
}

static inline int Log_Fill_UDPHead(UDP_HEAD_S *udph, char *buf, int buflen)
{
    int offset = 0;

    offset += snprintf(buf, buflen,
            ",\"sport\":%u,\"dport\":%u,"
            "\"udp_total_len\":%u,\"udp_chksum\":\"%02x\"",
            ntohs(udph->usSrcPort), ntohs(udph->usDstPort),
            ntohs(udph->usDataLength), ntohs(udph->usCrc));

    return offset;
}

static inline int Log_Fill_UDPPayload(IP46_HEAD_S *ipheader, int pktlen, UDP_HEAD_S *udph, char *buf, int buflen)
{
    int payload_len = 0, offset = 0;
    void *payload;

    payload_len = IP46_GetIPDataLen(ipheader, pktlen);

    if (payload_len > sizeof(UDP_HEAD_S)) {
        payload_len -= sizeof(UDP_HEAD_S);
    }

    if (payload_len > 0) {
        payload = udph + 1;

        payload_len = MIN(payload_len, 50);
        if (payload_len <= 0) {
            return 0;
        }

        offset += snprintf(buf + offset, buflen, ",\"payloadhex\":\"");
        DH_Data2HexString(payload, payload_len, buf +offset);
        offset += 2 *payload_len;
        buf[offset++ ] = '"';
    }

    return offset;
}

void UDPLOG_Init(UDP_LOG_S *config, char * conf_base_dir, char *log_base_dir)
{
    memset(config, 0, sizeof(UDP_LOG_S));
    config->config_base_dir = conf_base_dir;
    LOG_Init(&config->log_utl);
    LOG_Init(&config->log_dhcp_utl);
    LOG_SetBaseDir(&config->log_utl, log_base_dir);
    LOG_SetBaseDir(&config->log_dhcp_utl, log_base_dir);
}

BS_STATUS UDPLOG_ParseConfig(UDP_LOG_S *config, char *conf_string)
{
    char *start, *end;
    LSTR_S stConfig;

    if (0 != TXT_FindBracket(conf_string, strlen(conf_string), "{}", &start, &end)) {
        return BS_ERR;
    }

    start ++;

    stConfig.pcData = start;
    stConfig.uiLen = end - start;

    LSTR_ScanMultiKV(&stConfig, ',', ':', _udplog_process_kv, config);

    config->log_enable = 1;

    return 0;
}

void UDPLOG_Input(UDP_LOG_S *config, VOID *ippkt, UINT pktlen, NET_PKT_TYPE_E pkt_type)
{
    IP46_HEAD_S ipheader={0};
    UDP_HEAD_S *udph = NULL;

    if(0 != IP46_GetIPHeader(&ipheader, ippkt, pktlen, pkt_type)) {
        return;
    }

    if (ipheader.family == ETH_P_IP) {
        udph =
            UDP_GetUDPHeader(((UCHAR*)ipheader.iph.ip4) + IP_HEAD_LEN(ipheader.iph.ip4),
                    pktlen - IP_HEAD_LEN(ipheader.iph.ip4), NET_PKT_TYPE_UDP);
    }else if (ipheader.family == ETH_P_IP6) {
        udph = UDP_GetUDPHeader(((UCHAR*)ipheader.iph.ip6) + IP6_HDR_LEN,
                pktlen - IP6_HDR_LEN, NET_PKT_TYPE_UDP);
    }

    if (udph == NULL) {
        return;
    }


    if (config->dhcp_log_enable) {
        DhcpLog_Process(config, udph, pktlen);    
    }

    if (! config->log_enable) {
        return;
    }

    if ((config->bloomfilter_size == 0) || 
            ((ipheader.family == ETH_P_IP) && 
             _udplog_bloomfilter_test(config, 
                                      ipheader.iph.ip4->unSrcIp.uiIp, 
                                      ipheader.iph.ip4->unDstIp.uiIp,
                                      udph->usSrcPort, 
                                      udph->usDstPort)) ||
            (ipheader.family == ETH_P_IP6)) {

        if (config->log_utl.file) {

            char log_buf[MAX_LOG_BUF];
            char *ptr = log_buf;
            int  offset = 0, buf_len = MAX_LOG_BUF;

            offset = Log_FillHead("udplog", 1, ptr, buf_len);
            buf_len -= offset;
            ptr += offset;

            offset = IP46_Header2String(&ipheader, ippkt, ptr, buf_len, config->ip_header_hex);
            buf_len -= offset;
            ptr += offset;

            offset = Log_Fill_UDPHead(udph, ptr, buf_len);
            buf_len -= offset;
            ptr += offset;

            if (config->payload_hex) {
                offset = Log_Fill_UDPPayload(&ipheader, pktlen, udph, ptr, buf_len);
                buf_len -= offset;
                ptr += offset;

            }

            offset = Log_FillTail(ptr, buf_len);
            ptr += offset;
            buf_len -= offset;

            LOG_Output(&config->log_utl, log_buf, ptr - log_buf);
            LOG_Flush(&config->log_utl);
        }
    }

    return;
}
