#include "bs.h"
#include "utl/net.h"
#include "utl/eth_utl.h"
#include "utl/data2hex_utl.h"
#include "utl/txt_utl.h"
#include "utl/file_utl.h"
#include "utl/lstr_utl.h"
#include "utl/ip_utl.h"
#include "utl/ip_string.h"
#include "utl/tcp_finger.h"
#include "utl/tcp_utl.h"
#include "utl/tcp_log.h"
#include "utl/itoa.h"

typedef struct {
    UINT sip;
    UINT dip;
    USHORT sport;
    USHORT dport;
}TCPLOG_KEY_S;

static BS_STATUS _tcplog_osfinger_init(TCP_LOG_S *config)
{
    char file_path[FILE_MAX_PATH_LEN + 1];

    if (BS_OK != TcpFinger_Init(&config->tcp_finger)) {
        return BS_ERR;
    }

    snprintf(file_path, sizeof(file_path), "%s/%s", config->config_base_dir, "etter.finger.os");
    TcpFinger_ReadFingerFile(&config->tcp_finger, file_path);

    return 0;
}

static int _tcplog_process_kv(LSTR_S *pstKey, LSTR_S *pstValue, void *pUserHandle)
{
    TCP_LOG_S *config = pUserHandle;

    if ((LSTR_StrCmp(pstKey, "log_utl") == 0) && (pstValue->uiLen > 0)) {
        LOG_ParseConfig(&config->log_utl, pstValue->pcData, pstValue->uiLen);
    } else if ((LSTR_StrCmp(pstKey, "log_all_pkt") == 0) && (pstValue->uiLen > 0) && (pstValue->pcData[0] == '1')) {
        config->log_all_pkt = 1;
    } else if ((LSTR_StrCmp(pstKey, "ip_header_hex") == 0) && (pstValue->uiLen > 0) && (pstValue->pcData[0] == '1')) {
        config->ip_header_hex = 1;
    } else if ((LSTR_StrCmp(pstKey, "tcp_header_hex") == 0) && (pstValue->uiLen > 0) && (pstValue->pcData[0] == '1')) {
        config->tcp_header_hex = 1;
    } else if ((LSTR_StrCmp(pstKey, "payload_hex") == 0) && (pstValue->uiLen > 0) && (pstValue->pcData[0] == '1')) {
        config->tcp_payload_hex = 1;
    } else if ((LSTR_StrCmp(pstKey, "os_finger") == 0) && (pstValue->uiLen > 0) && (pstValue->pcData[0] == '1')) {
        if (BS_OK == _tcplog_osfinger_init(config)) {
            config->os_finger = 1;
        }
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

static int _tcplog_bloomfilter_test(TCP_LOG_S *config, UINT sip, UINT dip, USHORT sport, UINT dport)
{
    TCPLOG_KEY_S key = {0};

    key.sip = sip;
    key.dip = dip;
    key.sport = sport;
    key.dport = dport;

    if (0 == BloomFilter_TrySet(&config->bloomfilter, &key, sizeof(key))) {
        return 1;
    }

    return 0;
}

static int Log_Fill_TCPHead(TCP_HEAD_S *tcph, char *buf, int buflen, char dump_hex)
{
    int offset = 0;
    int len = TCP_HEAD_LEN(tcph);
    char szFlags[16];

    offset +=snprintf(buf, buflen,
            ",\"sport\":%u,\"dport\":%u,\"sequence\":%u, \"ack\":%u,"
            "\"tcp_head_len\":%u,\"tcp_flags\":\"%s\",\"tcp_wsize\":%u,"
            "\"tcp_chksum\":\"%02x\",\"tcp_urg\":%u",
            ntohs(tcph->usSrcPort), ntohs(tcph->usDstPort), ntohl(tcph->ulSequence), ntohl(tcph->ulAckSequence),
            len, TCP_GetFlagsString(tcph->ucFlag, szFlags), ntohs(tcph->usWindow),
            ntohs(tcph->usCrc), ntohs(tcph->usUrg));

    offset += TCP_GetOptString(tcph, buf + offset, buflen - offset);

    if (dump_hex) {
        offset += snprintf(buf + offset, buflen - offset, ",\"tcpheaderhex\":\"");
        DH_Data2HexString((void*)tcph, len, buf + offset);
        offset += 2 * len;
        buf[offset++] = '"';
    }

    return offset;
}

static int tcplog_Fill_OSFinger(TCP_LOG_S *config, IP_HEAD_S *ipheader, TCP_HEAD_S *tcpheader, char *buf, int buflen)
{
    TCP_FINGER_NODE_S  *node;

    node = TcpFinger_Match(&config->tcp_finger, ipheader, tcpheader);
    if (NULL == node) {
        return 0;
    }

    return snprintf(buf, buflen, ",\"OS\":\"%s\"", node->os);
}

static inline int Log_Fill_TCPPayload(IP46_HEAD_S *ipheader, int pktlen, TCP_HEAD_S *tcpheader, char *buf, int buflen)
{
    void *payload;
    int offset = 0;
    int payload_len = 0;

    if (ipheader->family == ETH_P_IP) {
        payload_len = MIN(ntohs(ipheader->iph.ip4->usTotlelen), pktlen);
        if (payload_len > IP_HEAD_LEN(ipheader->iph.ip4)) {
            payload_len -= IP_HEAD_LEN(ipheader->iph.ip4);
        } else {
            return offset;
        }
    }else if (ipheader->family == ETH_P_IP6) {
        payload_len = MIN(ntohs(ipheader->iph.ip6->len), pktlen);
    }

    if (tcpheader && payload_len > TCP_HEAD_LEN(tcpheader)) {
        payload_len -= TCP_HEAD_LEN(tcpheader);
    }
    if (payload_len > 0) {
        offset+=snprintf(buf, buflen, ",\"payloadhex\":\"");

        payload = (char*)tcpheader + TCP_HEAD_LEN(tcpheader);
        payload_len = MIN(payload_len, 50);

        DH_Data2HexString(payload, payload_len, buf + offset);
        offset += 2*payload_len;
        buf[offset++] = '"';
    }

    return offset;
}

void TCPLOG_Init(TCP_LOG_S *config, char * conf_base_dir, char *log_base_dir)
{
    memset(config, 0, sizeof(TCP_LOG_S));
    config->config_base_dir = conf_base_dir;
    LOG_Init(&config->log_utl);
    LOG_SetBaseDir(&config->log_utl, log_base_dir);
}

BS_STATUS TCPLOG_ParseConfig(TCP_LOG_S *config, char *conf_string)
{
    char *start, *end;
    LSTR_S stConfig;

    if (0 != TXT_FindBracket(conf_string, strlen(conf_string), "{}", &start, &end)) {
        return BS_ERR;
    }

    start ++;

    stConfig.pcData = start;
    stConfig.uiLen = end - start;

    LSTR_ScanMultiKV(&stConfig, ',', ':', _tcplog_process_kv, config);

    config->log_enable = 1;

    return 0;
}

int TCPLOG_Input(TCP_LOG_S *config, VOID *ippkt, UINT pktlen, NET_PKT_TYPE_E pkt_type)
{
    IP46_HEAD_S ipheader = {0};
    TCP_HEAD_S *tcpheader= NULL;

    if (! config->log_enable) {
        return BS_NOT_INIT;
    }

    if (0 != IP46_GetIPHeader(&ipheader, ippkt, pktlen, pkt_type)) {
        return BS_ERR;
    }

    if (ipheader.family == ETH_P_IP) {
        tcpheader = TCP_GetTcpHeader(
                ((UCHAR*)ipheader.iph.ip4) + IP_HEAD_LEN(ipheader.iph.ip4), 
                pktlen - IP_HEAD_LEN(ipheader.iph.ip4), NET_PKT_TYPE_TCP);
    }else if (ipheader.family == ETH_P_IP6) {
        tcpheader = TCP_GetTcpHeader(
                ((UCHAR*)ipheader.iph.ip6) + IP6_HDR_LEN, 
                pktlen - IP6_HDR_LEN, NET_PKT_TYPE_TCP);
    }
    if (tcpheader == NULL) {
        return BS_ERR;
    }

    if ((config->log_all_pkt == 0) && 
            (! (tcpheader->ucFlag & (TCP_FLAG_SYN | TCP_FLAG_FIN | TCP_FLAG_RST))))
    {
        return BS_NOT_MATCHED;
    }

    if (ipheader.family == ETH_P_IP &&(config->bloomfilter_size != 0)
       && (0 == _tcplog_bloomfilter_test(config, ipheader.iph.ip4->unSrcIp.uiIp, 
               ipheader.iph.ip4->unDstIp.uiIp,
               tcpheader->usSrcPort, tcpheader->usDstPort))) {
        return BS_ALREADY_EXIST;
    }

    if (config->log_utl.file) {
        char log_buf[MAX_LOG_BUF];
        char *ptr = log_buf;
        int offset = 0, buf_len = MAX_LOG_BUF;

        offset = Log_FillHead("tcplog", 1, ptr, buf_len);
        buf_len -= offset;
        ptr += offset;

        offset = IP46_Header2String(&ipheader, ippkt, ptr, buf_len, config->ip_header_hex);
        buf_len -= offset;
        ptr += offset;

        offset = Log_Fill_TCPHead(tcpheader, ptr, buf_len, config->tcp_header_hex);
        buf_len -= offset;
        ptr += offset;

        if (config->tcp_payload_hex) {
            offset = Log_Fill_TCPPayload(&ipheader, pktlen, tcpheader, ptr, buf_len);
            buf_len -= offset;
        }
        if (config->os_finger) {
            if (ipheader.family == ETH_P_IP) {
                offset += tcplog_Fill_OSFinger(config, ipheader.iph.ip4,
                        tcpheader, ptr, buf_len);
            }
        }

        offset = Log_FillTail(ptr, buf_len);
        ptr += offset;
        buf_len -= offset;

        LOG_Output(&config->log_utl, log_buf, ptr - log_buf);
        LOG_Flush(&config->log_utl);
    } 

    return 0;
}
