#include "bs.h"
#include "utl/net.h"
#include "utl/eth_utl.h"
#include "utl/data2hex_utl.h"
#include "utl/txt_utl.h"
#include "utl/time_utl.h"
#include "utl/file_utl.h"
#include "utl/lstr_utl.h"
#include "utl/ip_utl.h"
#include "utl/ip_string.h"
#include "utl/tcp_finger.h"
#include "utl/tcp_utl.h"
#include "utl/tcp_log.h"
#include "utl/itoa.h"
#ifdef USE_SLS
#include "utl/ip_log_sls.h"
#include "utl/tcp_log_sls.h"
#endif

#include "utl/lpm_utl.h"

#define WHITE_IP_NODES_BUCKETS (1024*256)
#define WHITE_IP_NODES_DEPTH (8)


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

    scnprintf(file_path, sizeof(file_path), "%s/%s", config->config_base_dir, "etter.finger.os");
    TcpFinger_ReadFingerFile(&config->tcp_finger, file_path, 0);

    return 0;
}

static int tcplog_LoadIPFile(BOX_S *box, char *filename)
{
    FILE *fp;
    char buf[256];
    ULONG len;
    UINT ip;

    fp = fopen(filename, "rb");
    if (NULL == fp) {
        RETURN(BS_CAN_NOT_OPEN);
    }

    while(NULL != fgets(buf, sizeof(buf), fp)) {
        TXT_Strim(buf);
        len = TXT_StrimTail(buf, strlen(buf), "\r\n");
        buf[len] = '\0';

        ip = inet_addr(buf);
        if (ip == 0) {
            continue;
        }
        if (Box_Find(box, UINT_HANDLE(ip)) >= 0) {
            continue;
        }
        Box_Add(box, UINT_HANDLE(ip));
    }

    fclose(fp);

    return 0;
}

static int _tcplog_process_kv(LSTR_S *pstKey, LSTR_S *pstValue, void *pUserHandle)
{
    TCP_LOG_S *config = pUserHandle;
    char filename[FILE_MAX_PATH_LEN + 1];
    char buf[256];
    char *file;
    if ((LSTR_StrCmp(pstKey, "log_utl") == 0) && (pstValue->uiLen > 0)) {
        LOG_ParseConfig(&config->log_utl, pstValue->pcData, pstValue->uiLen);
    } else if ((LSTR_StrCmp(pstKey, "log_all_pkt") == 0) && (pstValue->uiLen > 0) && (pstValue->pcData[0] == '1')) {
        config->log_all_pkt = 1;
    } else if ((LSTR_StrCmp(pstKey, "ip_header_hex") == 0) && (pstValue->uiLen > 0) && (pstValue->pcData[0] == '1')) {
        config->ip_header_hex = 1;
    } else if ((LSTR_StrCmp(pstKey, "tcp_header_hex") == 0) && (pstValue->uiLen > 0) && (pstValue->pcData[0] == '1')) {
        config->tcp_header_hex = 1;
    } else if ((LSTR_StrCmp(pstKey, "payload_hex") == 0) && (pstValue->uiLen > 0)) {
        config->tcp_payload_hex = TXT_Str2Ui(pstValue->pcData); /* 输出多长的Data,0表示不输出,如果想输出全部,设置为65535 */
    } else if ((LSTR_StrCmp(pstKey, "tcp_log_enable") == 0) && (pstValue->uiLen > 0)) {
        if (pstValue->pcData[0] == '1') {
            config->tcp_log_enable = 1;
        } else if (pstValue->pcData[0] == '0') {
            config->tcp_log_enable = 0;
        }
    } else if ((LSTR_StrCmp(pstKey, "os_finger") == 0) && (pstValue->uiLen > 0) && (pstValue->pcData[0] == '1')) {
        if (BS_OK == _tcplog_osfinger_init(config)) {
            config->os_finger = 1;
        }
    } else if ((LSTR_StrCmp(pstKey, "only_sip_list") == 0) && (pstValue->uiLen > 0)) {
        LSTR_Strlcpy(pstValue, sizeof(buf), buf);
        file = FILE_ToAbsPath(config->config_base_dir, buf, 
                config->sip_monitor.filename, sizeof(config->sip_monitor.filename));
        if (file != NULL) {
            config->only_sip_enable = 1;
            TCP_LoadIPFile(&(config->sip_monitor));
        }
    } else if ((LSTR_StrCmp(pstKey, "bloomfilter") == 0) && (pstValue->uiLen > 0)) {
        config->bloomfilter_size = LSTR_A2ui(pstValue);
        if (config->bloomfilter_size > 0) {
            BloomFilter_Init(&config->bloomfilter, config->bloomfilter_size);
            BloomFilter_SetStepsToClearAll(&config->bloomfilter, 60);
            BloomFilter_SetAutoStep(&config->bloomfilter, 1);
        }
    }else if ((LSTR_StrCmp(pstKey, "tcp_ip_white_list") ==0)
            && (pstValue->uiLen > 0)) {
        LSTR_Strlcpy(pstValue, sizeof(buf), buf);
        file = FILE_ToAbsPath(config->config_base_dir, buf,
                filename, sizeof(filename));
        if (file != NULL) {
            config->white_ip_enable = 1;
            IntBox_Init(&config->white_ip_list, NULL,
                    WHITE_IP_NODES_BUCKETS, WHITE_IP_NODES_DEPTH);
            tcplog_LoadIPFile(&config->white_ip_list, file);
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

    offset +=scnprintf(buf, buflen,
            ",\"sport\":%u,\"dport\":%u,\"sequence\":%u, \"ack\":%u,"
            "\"tcp_head_len\":%u,\"tcp_flags\":\"%s\",\"tcp_wsize\":%u,"
            "\"tcp_chksum\":\"%02x\",\"tcp_urg\":%u",
            ntohs(tcph->usSrcPort), ntohs(tcph->usDstPort), ntohl(tcph->ulSequence), ntohl(tcph->ulAckSequence),
            len, TCP_GetFlagsString(tcph->ucFlag, szFlags), ntohs(tcph->usWindow),
            ntohs(tcph->usCrc), ntohs(tcph->usUrg));

    offset += TCP_GetOptString(tcph, buf + offset, buflen - offset);

    if (dump_hex) {
        offset += scnprintf(buf + offset, buflen - offset, ",\"tcpheaderhex\":\"");
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

    return scnprintf(buf, buflen, ",\"OS\":\"%s\"", node->os);
}

static inline int Get_TCPPayload_len(IP46_HEAD_S *ipheader, int pktlen, TCP_HEAD_S *tcpheader) 
{
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

    if (tcpheader && payload_len >= TCP_HEAD_LEN(tcpheader)) {
        payload_len -= TCP_HEAD_LEN(tcpheader);
    }
    return payload_len;
}

void TCPLOG_Init(TCP_LOG_S *config)
{
    memset(config, 0, sizeof(TCP_LOG_S));
    LOG_Init(&config->log_utl);
}

void TCPLOG_SetDir(TCP_LOG_S *config, char *conf_base_dir, char *log_base_dir)
{
    config->config_base_dir = conf_base_dir;
    LOG_SetBaseDir(&config->log_utl, log_base_dir);
}

void TCPLOG_Final(TCP_LOG_S *config)
{
    LOG_Fini(&config->log_utl);
    if (config->bloomfilter_size > 0) {
        BloomFilter_Final(&config->bloomfilter);
    }

    if (config->white_ip_enable) {
        Box_Fini(&config->white_ip_list, NULL, NULL);
    }

    memset(config, 0, sizeof(TCP_LOG_S));
}

void TCPLOG_SetOutputFunc(TCP_LOG_S *config, PF_LOG_OUT_FUNC output_func)
{
    LOG_SetOutputFunc(&config->log_utl, output_func);
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

    if (config->tcp_log_enable == 0 && config->only_sip_enable == 0) {
        config->log_enable = 0;
    } else {
        config->log_enable = 1;
    }

    return 0;
}

static void _tcplog_logutl(TCP_LOG_S *config, BUFFER_S *buffer, VOID *ippkt, UINT pktlen,
        NET_PKT_TYPE_E pkt_type, int dir_s2c, IP46_HEAD_S *ipheader, TCP_HEAD_S *tcpheader)
{
    char log_buf[512];
    int len;
    int payload_len = 0;
    UINT tcp_payload_hex = config->tcp_payload_hex;
    BOOL_T ip_header_hex = config->ip_header_hex;
    BOOL_T tcp_header_hex = config->tcp_header_hex;

    BUFFER_Print(buffer, "{\"type\":\"tcplog\",\"log_ver\":1,\"time\":\"%s\"",
            TM_Utc2Acstime(time(0), log_buf));

    len = IP46_Header2String(ipheader, ippkt, log_buf, sizeof(log_buf), ip_header_hex);
    BUFFER_Write(buffer, log_buf, len);

    len = Log_Fill_TCPHead(tcpheader, log_buf, sizeof(log_buf), tcp_header_hex);
    BUFFER_Write(buffer, log_buf, len);

    payload_len = Get_TCPPayload_len(ipheader, pktlen, tcpheader);
    payload_len = MIN(payload_len, tcp_payload_hex);
    if (payload_len && tcpheader) {
        BUFFER_WriteString(buffer, ",\"payloadhex\":\"");
        BUFFER_WriteByHex(buffer, tcpheader + TCP_HEAD_LEN(tcpheader), payload_len);
        BUFFER_WriteString(buffer, "\"");
    }

    if ((config->os_finger) && (ipheader->family == ETH_P_IP)) {
        len = tcplog_Fill_OSFinger(config, ipheader->iph.ip4, tcpheader, log_buf, sizeof(log_buf));
        BUFFER_Write(buffer, log_buf, len);
    }

    BUFFER_WriteString(buffer, "}\n");
}

static void _tcplog_sls(TCP_LOG_S *config, VOID *ippkt, UINT pktlen,
        NET_PKT_TYPE_E pkt_type, int dir_s2c, IP46_HEAD_S *ipheader, TCP_HEAD_S *tcpheader)
{

#ifdef USE_SLS
    if (config->log_utl.sls && config->log_utl.sls_client) {
        
        log_client_t *client = (log_client_t*) config->log_utl.sls_client;

        LogSls_Start(client);

        LOG_PRODUCER_CLIENT_SEND_LOG_NKV(client, "logtype", "tcplog", "logver", "2");
        SLS_Log_Fill_IP_Head(client, ipheader, ippkt, config->ip_header_hex);
        SLS_Log_Fill_TCPHead(client, tcpheader, config->tcp_header_hex);

        if (config->tcp_payload_hex) {
            SLS_Log_Fill_TCPPayload(client, ipheader, pktlen, tcpheader);
        }
        if (config->os_finger) {
            if (ipheader->family == ETH_P_IP) {
                SLS_Log_Fill_TCPOSFinger(client, &config->tcp_finger, ipheader->iph.ip4, tcpheader);
            }
        }
        LogSls_End(client);
    }
#endif
}

static BOOL_T _tcplog_filter(TCP_LOG_S *config, VOID *ippkt, UINT pktlen,
        NET_PKT_TYPE_E pkt_type, int dir_s2c, IP46_HEAD_S *ipheader, TCP_HEAD_S *tcpheader)
{
    /*check the ip monitor for src */
    BOOL_T is_sip_match = FALSE;
    int ret, src;
    UINT64 nexthop;

    SIP_MONITOR_S *sip_monitor = &(config->sip_monitor);
    if (config->only_sip_enable) {
        if (sip_monitor->need_reload) {
            sip_monitor->lpm_handle[0] = sip_monitor->lpm_handle[1];
            sip_monitor->need_reload = 0;
        }
        if (dir_s2c == 0) {
            src = ntohl(ipheader->iph.ip4->unSrcIp.uiIp);
        } else {
            src = ntohl(ipheader->iph.ip4->unDstIp.uiIp);
        }
        ret = LPM_Lookup(sip_monitor->lpm_handle[0], src, &nexthop);
        if (ret == 0) {
            is_sip_match = TRUE;
        }
    }

    if (is_sip_match) {
        return FALSE;
    }

    if (! config->tcp_log_enable) {
        return TRUE;
    }

    /*check the tcp log enable */
    if ((config->log_all_pkt == 0) && 
            (!(tcpheader->ucFlag & (TCP_FLAG_SYN | TCP_FLAG_FIN | TCP_FLAG_RST))))
    {
        return TRUE;
    }
    /* white ip list dip-addr */
    if (config->white_ip_enable) {
        if (Box_Find(&config->white_ip_list, UINT_HANDLE(ipheader->iph.ip4->unDstIp.uiIp)) >= 0) {
            //head_info->match_white_list = 1; //http_log 中存在该变量，但没有使用
            return TRUE;
        }
    }

    if (ipheader->family == ETH_P_IP &&(config->bloomfilter_size != 0)
            && (0 == _tcplog_bloomfilter_test(config, ipheader->iph.ip4->unSrcIp.uiIp, 
                    ipheader->iph.ip4->unDstIp.uiIp,
                    tcpheader->usSrcPort, tcpheader->usDstPort))) {
        return TRUE; 
    }

    return FALSE;
}

static void _tcplog_output_log(void *data, UINT len, USER_HANDLE_S *ud)
{
    char *str = data;
    TCP_LOG_S *config = ud->ahUserHandle[0];
    str[len] = '\0';
    LOG_Output(&config->log_utl, data, len);
}

static void _tcplog_process(TCP_LOG_S *config, VOID *ippkt, UINT pktlen,
        NET_PKT_TYPE_E pkt_type, int dir_s2c, IP46_HEAD_S *ipheader, TCP_HEAD_S *tcpheader)
{
    BUFFER_S buffer;
    char info[2048];
    USER_HANDLE_S ud;
    ud.ahUserHandle[0] = config;

    BUFFER_Init(&buffer);
    BUFFER_AttachBuf(&buffer, info, sizeof(info)-1);
    BUFFER_SetOutputFunc(&buffer, _tcplog_output_log, &ud);
    _tcplog_logutl(config, &buffer, ippkt, pktlen, pkt_type, dir_s2c, ipheader, tcpheader);
    BUFFER_Flush(&buffer);
    BUFFER_Fini(&buffer);
}

int TCPLOG_Input(TCP_LOG_S *config, VOID *ippkt, UINT pktlen, NET_PKT_TYPE_E pkt_type, int dir_s2c)
{
    IP46_HEAD_S ipheader = {0};
    TCP_HEAD_S *tcpheader= NULL;

    if (!config->log_enable) {
        return BS_NOT_INIT;
    }

    if (0 != IP46_GetIPHeader(&ipheader, ippkt, pktlen, pkt_type)) {
        return BS_ERR;
    }

    if (ipheader.family == ETH_P_IP) {
        tcpheader = TCP_GetTcpHeader(((UCHAR*)ipheader.iph.ip4) + IP_HEAD_LEN(ipheader.iph.ip4), 
                pktlen - IP_HEAD_LEN(ipheader.iph.ip4), NET_PKT_TYPE_TCP);
    }else if (ipheader.family == ETH_P_IP6) {
        tcpheader = TCP_GetTcpHeader(((UCHAR*)ipheader.iph.ip6) + IP6_HDR_LEN, 
                pktlen - IP6_HDR_LEN, NET_PKT_TYPE_TCP);
    }

    if (tcpheader == NULL) {
        return BS_ERR;
    }

    if (_tcplog_filter(config, ippkt, pktlen, pkt_type, dir_s2c, &ipheader, tcpheader)) {
        return BS_NO_PERMIT;
    }

    if (config->log_utl.file) {
        _tcplog_process(config, ippkt, pktlen, pkt_type, dir_s2c, &ipheader, tcpheader);
    }
    _tcplog_sls(config, ippkt, pktlen, pkt_type, dir_s2c, &ipheader, tcpheader);

    return 0;
}

