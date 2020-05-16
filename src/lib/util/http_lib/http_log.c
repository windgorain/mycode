#include "bs.h"
#include "utl/net.h"
#include "utl/ip_utl.h"
#include "utl/eth_utl.h"
#include "utl/data2hex_utl.h"
#include "utl/txt_utl.h"
#include "utl/file_utl.h"
#include "utl/lstr_utl.h"
#include "utl/ip_string.h"
#include "utl/tcp_utl.h"
#include "utl/txt_utl.h"
#include "utl/cuckoo_hash.h"
#include "utl/box_utl.h"
#include "utl/http_lib.h"
#include "utl/http_log.h"

typedef struct {
    UINT sip;
    UINT dip;
    USHORT sport;
    USHORT dport;
}HTTPLOG_KEY_S;

static int _httplog_IsHeadField(char *line, UINT line_len, char *head_field)
{
    UINT len = strlen(head_field);

    if (line_len < len) {
        return 0;
    }

    if (0 == strncmp(line, head_field, len)) {
        return 1;
    }

    return 0;
}

static int _httplog_IsPermitLogLine(HTTP_LOG_S *config, char *line,
        UINT line_len)
{
    if (config->log_cookie == 0) {
        if (_httplog_IsHeadField(line, line_len, "cookie:")) {
            return 0;
        }
    }

    if (config->log_setcookie == 0) {
        if (_httplog_IsHeadField(line, line_len, "set-cookie:")) {
            return 0;
        }
    }

    return 1;
}

static int _httplog_bloomfilter_test(HTTP_LOG_S *config, UINT sip,
        UINT dip, USHORT sport, UINT dport)
{
    HTTPLOG_KEY_S key = {0};

    key.sip = sip;
    key.dip = dip;
    key.sport = sport;
    key.dport = dport;

    if (0 == BloomFilter_TrySet(&config->bloomfilter, &key, sizeof(key))) {
        return 1;
    }

    return 0;
}

char * httplog_FindHeadEnd(char *data, int len)
{
    char *ptr = NULL;
    int space_len = len < MAX_SPACE_LEN ?  len : MAX_SPACE_LEN;

    if (memchr(data, ' ', space_len) == NULL) {
        return NULL;
    }

    if (len <= MAX_RN_LEN) {
        return TXT_Strnstr(data, "\r\n\r\n", len);
    }

    ptr = TXT_Strnstr(data, "\r\n", len - 4);
    if (ptr == NULL) {
        return NULL;
    }

    return TXT_Strnstr(ptr, "\r\n\r\n",  data + len - ptr);
}

/* 从文件加载字符串,每行一条 */
static int httplog_LoadStrFile(BOX_S *box, char *filename)
{
    FILE *fp;
    char buf[256];
    char *str;
    ULONG len;

    fp = fopen(filename, "rb");
    if (NULL == fp) {
        RETURN(BS_CAN_NOT_OPEN);
    }

    while(NULL != fgets(buf, sizeof(buf), fp)) {
        TXT_Strim(buf);
        len = TXT_StrimTail(buf, strlen(buf), "\r\n");
        buf[len] = '\0';

        if (Box_Find(box, buf) >= 0) {
            continue;
        }
        str = strdup(buf);
        if (str) {
            Box_Add(box, str);
        }
    }

    fclose(fp);

    return 0;
}

static int httplog_LoadIPFile(BOX_S *box, char *filename)
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

static int _httplog_process_kv(LSTR_S *pstKey, LSTR_S *pstValue,
        void *pUserHandle)
{
    HTTP_LOG_S *config = pUserHandle;
    char filename[FILE_MAX_PATH_LEN + 1];
    char buf[256];
    char *file;

    if ((LSTR_StrCmp(pstKey, "log_utl") == 0) && (pstValue->uiLen > 0)) {
        LOG_ParseConfig(&config->log_utl, pstValue->pcData, pstValue->uiLen);
    } else if ((LSTR_StrCmp(pstKey, "log_cookie") == 0)
            && (pstValue->uiLen > 0) && (pstValue->pcData[0] == '1')) {
        config->log_cookie = 1;
    } else if ((LSTR_StrCmp(pstKey, "log_setcookie") == 0)
            && (pstValue->uiLen > 0) && (pstValue->pcData[0] == '1')) {
        config->log_setcookie = 1;
    } else if ((LSTR_StrCmp(pstKey, "log_request_body") == 0)
            && (pstValue->uiLen > 0)) {
        config->log_request_body = LSTR_A2ui(pstValue);
    } else if ((LSTR_StrCmp(pstKey, "log_response_body") == 0)
            && (pstValue->uiLen > 0)) {
        config->log_response_body = LSTR_A2ui(pstValue);
    } else if ((LSTR_StrCmp(pstKey, "head_format") == 0)
            && (pstValue->uiLen > 0)) {
        config->head_format = HTTP_LOG_HEAD_FMT_RAW;
        if (LSTR_StrCmp(pstValue, "json") == 0) {
            config->head_format = HTTP_LOG_HEAD_FMT_JSON;
        } else if (LSTR_StrCmp(pstValue, "hex") == 0) {
            config->head_format = HTTP_LOG_HEAD_FMT_HEX;
        }
    } else if ((LSTR_StrCmp(pstKey, "bloomfilter") == 0)
            && (pstValue->uiLen > 0)) {
        config->bloomfilter_size = LSTR_A2ui(pstValue);
        if (config->bloomfilter_size > 0) {
            BloomFilter_Init(&config->bloomfilter, config->bloomfilter_size);
            BloomFilter_SetStepsToClearAll(&config->bloomfilter, 60);
            BloomFilter_SetAutoStep(&config->bloomfilter, 1);
        }
    } else if ((LSTR_StrCmp(pstKey, "host_white_list") ==0)
            && (pstValue->uiLen > 0)) {
        LSTR_Strlcpy(pstValue, sizeof(buf), buf);
        file = FILE_ToAbsPath(config->config_base_dir, buf,
                filename, sizeof(filename));
        if (file != NULL) {
            config->white_host_enable = 1;
            StrBox_Init(&config->white_host_list, NULL,
                    WHITE_HOST_NODES_BUCKETS, WHITE_HOST_NODES_DEPTH);
            httplog_LoadStrFile(&config->white_host_list, file);
        }
    } else if ((LSTR_StrCmp(pstKey, "ip_white_list") ==0)
            && (pstValue->uiLen > 0)) {
        LSTR_Strlcpy(pstValue, sizeof(buf), buf);
        file = FILE_ToAbsPath(config->config_base_dir, buf,
                filename, sizeof(filename));
        if (file != NULL) {
            config->white_ip_enable = 1;
            StrBox_Init(&config->white_host_list, NULL,
                    WHITE_IP_NODES_BUCKETS, WHITE_IP_NODES_DEPTH);
            httplog_LoadIPFile(&config->white_ip_list, file);
        }
    } else if ((LSTR_StrCmp(pstKey, "only_dip_list") ==0)
            && (pstValue->uiLen > 0)) {
        LSTR_Strlcpy(pstValue, sizeof(buf), buf);
        file = FILE_ToAbsPath(config->config_base_dir, buf,
                filename, sizeof(filename));
        if (file != NULL) {
            config->only_dip_enable = 1;
            IntBox_Init(&config->only_dip_list, NULL,
                    ONLY_IP_NODES_BUCKETS, ONLY_IP_NODES_DEPTH);
            httplog_LoadIPFile(&config->only_dip_list, file);
        }
    }

    return BS_OK;
}

void HTTPLOG_Init(HTTP_LOG_S *config)
{
    memset(config, 0, sizeof(HTTP_LOG_S));
    LOG_Init(&config->log_utl);
}

static void httplog_FreeWhiteListNode(BOX_S *box, void *data, void *ud)
{
    if (data) {
        free(data);
    }
}

void HTTPLOG_Final(HTTP_LOG_S *config)
{
    LOG_Fini(&config->log_utl);
    if (config->bloomfilter_size > 0) {
        BloomFilter_Final(&config->bloomfilter);
    }

    if (config->white_host_enable) {
        Box_Fini(&config->white_host_list, httplog_FreeWhiteListNode, NULL);
    }

    if (config->white_ip_enable) {
        Box_Fini(&config->white_ip_list, NULL, NULL);
    }

    if (config->only_dip_enable) {
        Box_Fini(&config->only_dip_list, NULL, NULL);
    }

    memset(config, 0, sizeof(HTTP_LOG_S));
}

int HTTPLOG_SetConfDir(HTTP_LOG_S *config, char *conf_dir, char *log_dir)
{
    config->config_base_dir = conf_dir;
    LOG_SetBaseDir(&config->log_utl, log_dir);

    return 0;
}

BS_STATUS HTTPLOG_ParseConfig(HTTP_LOG_S *config, char *conf_string)
{
    char *start, *end;
    LSTR_S stConfig;

    if (0 != TXT_FindBracket(conf_string, strlen(conf_string),
                "{}", &start, &end)) {
        return BS_ERR;
    }

    start ++;

    stConfig.pcData = start;
    stConfig.uiLen = end - start;

    LSTR_ScanMultiKV(&stConfig, ',', ':', _httplog_process_kv, config);

    config->log_enable = 1;

    return 0;
}

static int _httplog_HeadRawScan(char *field, int field_len,
        char *value, int value_len, void *ud)
{
    HTTP_LOG_S *config = ud;

    if (field == NULL) { /* first line */
        field = "First-Line";
        field_len = strlen(field);
    }

    LOG_OutputString(&config->log_utl, ",\"");
    LOG_Output(&config->log_utl, field, field_len);
    LOG_OutputString(&config->log_utl, "\":\"");
    if (value) {
        LOG_Output(&config->log_utl, value, value_len);
    }
    LOG_OutputString(&config->log_utl, "\"");

    return 0;
}

static int httplog_Filter(HTTP_LOG_S *config, HTTP_LOG_HEAD_INFO_S *head_info)
{
    if (config->only_dip_enable) {
        if (Box_Find(&config->only_dip_list, UINT_HANDLE(head_info->dip)) < 0) {
            head_info->match_white_list = 1;
            return BS_NO_PERMIT;
        }
    }

    if ((config->white_host_enable) && head_info->host) {
        char host[256];
        int len = MIN(head_info->host_len, sizeof(host) - 1);
        memcpy(host, head_info->host, len);
        host[len] = 0;
        if (Box_Find(&config->white_host_list, host) >= 0) {
            head_info->match_white_list = 1;
            return BS_NO_PERMIT;
        }
    }

    if (config->white_ip_enable) {
        if (Box_Find(&config->white_ip_list, UINT_HANDLE(head_info->dip)) >=0) {
            head_info->match_white_list = 1;
            return BS_NO_PERMIT;
        }
    }

    if ((config->bloomfilter_size != 0)
            && (0 == _httplog_bloomfilter_test(config, head_info->sip,
                    head_info->dip, head_info->sport, head_info->dport))) {
        return BS_ALREADY_EXIST;
    }

    return 0;
}

int HTTPLOG_HeadInput(HTTP_LOG_S *config, HTTP_LOG_HEAD_INFO_S *head_info)
{
    UCHAR *src, *dst;
    UINT head_len = head_info->end - head_info->data;
    char *line;
    UINT line_len;
    char ip6_str_src[INET6_ADDRSTRLEN];
    char ip6_str_dst[INET6_ADDRSTRLEN];

    if (config->log_enable == 0) {
        return BS_NOT_INIT;
    }

    int ret = httplog_Filter(config, head_info);
    if (ret != 0) {
        return ret;
    }

    LOG_OutputHead(&config->log_utl, "httplog", 1);

    if (head_info->ether_type == NET_PKT_TYPE_IP) {
        src = (void*)&head_info->sip;
        dst = (void*)&head_info->dip;
        LOG_OutputArgs(&config->log_utl,
                ",\"address\":\"%d.%d.%d.%d:%d-%d.%d.%d.%d:%d\"",
                src[0], src[1], src[2], src[3], ntohs(head_info->sport),
                dst[0], dst[1], dst[2], dst[3], ntohs(head_info->dport));
    } else if (head_info->ether_type == NET_PKT_TYPE_IP6) {
        inet_ntop6_full(&head_info->ip6_src, ip6_str_src, sizeof(ip6_str_src));
        inet_ntop6_full(&head_info->ip6_dst, ip6_str_dst, sizeof(ip6_str_dst));
        LOG_OutputArgs(&config->log_utl, ",\"address\":\"%s:%d-%s:%d\"",
                ip6_str_src, ntohs(head_info->sport), ip6_str_dst,
                ntohs(head_info->dport));
    }

    if (config->head_format == HTTP_LOG_HEAD_FMT_HEX) {
        LOG_OutputString(&config->log_utl, ",\"head_hex\":\"");
        LOG_OutputByHex(&config->log_utl, head_info->data, head_len);
        LOG_OutputString(&config->log_utl, "\"");
    } else if (config->head_format == HTTP_LOG_HEAD_FMT_JSON) {
        HTTP_HeadRawScan(head_info->data, head_len,
                _httplog_HeadRawScan, config);
    }

    LOG_OutputTail(&config->log_utl);

    if (config->head_format == HTTP_LOG_HEAD_FMT_RAW) {
        TXT_SCAN_N_LINE_BEGIN(head_info->data, head_len, line, line_len) {
            if (_httplog_IsPermitLogLine(config, line, line_len)) {
                LOG_Output(&config->log_utl, line, line_len + 2);
            }
        }TXT_SCAN_LINE_END();
        LOG_Output(&config->log_utl, "\r\n", 2);
    }

    LOG_Flush(&config->log_utl);

    return 0;
}

int HTTPLOG_BodyInput(HTTP_LOG_S *config, HTTP_LOG_HEAD_INFO_S *body_info)
{
    if (!config->log_enable) {
        return BS_NOT_INIT;
    }

    int log_len = 0;
    if (body_info->is_request) {
        if (config->log_request_body == 0) {
            return BS_NOT_SUPPORT;
        }
        log_len = config->log_request_body - body_info->log_printed_len;
    } else {
        if (config->log_response_body == 0) {
            return BS_NOT_SUPPORT;
        }
        log_len = config->log_response_body - body_info->log_printed_len;
    }

    if (log_len == 0) {
        return BS_EMPTY;
    }

    char ip6_str_src[INET6_ADDRSTRLEN];
    char ip6_str_dst[INET6_ADDRSTRLEN];
    UCHAR *src, *dst;

    LOG_OutputHead(&config->log_utl, "httpbody", 1);

    if (body_info->ether_type == NET_PKT_TYPE_IP) {
        src = (void*)&body_info->sip;
        dst = (void*)&body_info->dip;
        LOG_OutputArgs(&config->log_utl,
                ",\"address\":\"%d.%d.%d.%d:%d-%d.%d.%d.%d:%d\"",
                src[0], src[1], src[2], src[3], ntohs(body_info->sport),
                dst[0], dst[1], dst[2], dst[3], ntohs(body_info->dport));
    }else if (body_info->ether_type == NET_PKT_TYPE_IP6) {
        inet_ntop6_full(&body_info->ip6_src, ip6_str_src, sizeof(ip6_str_src));
        inet_ntop6_full(&body_info->ip6_dst, ip6_str_dst, sizeof(ip6_str_dst));
        LOG_OutputArgs(&config->log_utl, ",\"address\":\"%s:%d-%s:%d\"",
                ip6_str_src, ntohs(body_info->sport), ip6_str_dst,
                ntohs(body_info->dport));
    }

    LOG_OutputArgs(&config->log_utl,  ",\"dir\":\"%d\",\"body\":\"", 
            body_info->is_request == 0);
    uint32_t len = body_info->end - body_info->data;
    len = MIN(log_len, len);
    LOG_OutputByHex(&config->log_utl, (CHAR*)body_info->data, len);
    body_info->log_printed_len += len;

    LOG_OutputTail(&config->log_utl);
    LOG_Flush(&config->log_utl);

    return 0;
}

