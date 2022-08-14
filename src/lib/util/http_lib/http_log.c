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
#include "utl/time_utl.h"
#include "utl/txt_utl.h"
#include "utl/cuckoo_hash.h"
#include "utl/box_utl.h"
#include "utl/http_lib.h"
#include "utl/http_log.h"
#include "utl/syslog_utl.h"
#ifdef USE_SLS
#include "utl/itoa.h"
#include <apr_uuid.h>
#include "utl/log_producer_api.h"
#endif
#ifdef USE_REDIS
#include "utl/redis_utl.h"
#endif

#define WHITE_HOST_NODES_BUCKETS (1024*256)
#define WHITE_HOST_NODES_DEPTH (8)
#define WHITE_IP_NODES_BUCKETS (1024*4)
#define WHITE_IP_NODES_DEPTH (8)
#define ONLY_IP_NODES_BUCKETS (1024*4)
#define ONLY_IP_NODES_DEPTH (8)

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

static int _ua_bloomfilter_test(HTTP_LOG_S *config, UINT sip, int ua_len, const uint8_t *ua)
{
    if (!ua_len) return 0;

    int len = ua_len + 4;
    uint8_t ip_ua[len + 1];
    uint8_t *ip = (void*)&sip;
    int i;

    for (i = 0; i < sizeof(UINT); i++) {
        ip_ua[i] = ip[i];
    }
    memcpy(ip_ua + sizeof(UINT), ua, strlen((void*)ua));
    
    if (0 == BloomFilter_TrySet(&config->ua_filter, ip_ua, len)) {
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
    } else if ((LSTR_StrCmp(pstKey, "redis_ua") == 0)
            && (pstValue->uiLen > 0)) {
        config->useragent_db_enable = 1;
        config->ua_filter_size = 1000000;
        BloomFilter_Init(&config->ua_filter, config->ua_filter_size);
        BloomFilter_SetStepsToClearAll(&config->ua_filter, 60);
        BloomFilter_SetAutoStep(&config->ua_filter, 1);
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
    }else if ((LSTR_StrCmp(pstKey, "ip_white_list") ==0)
            && (pstValue->uiLen > 0)) {
        LSTR_Strlcpy(pstValue, sizeof(buf), buf);
        file = FILE_ToAbsPath(config->config_base_dir, buf,
                filename, sizeof(filename));
        if (file != NULL) {
            config->white_ip_enable = 1;
            IntBox_Init(&config->white_ip_list, NULL,
                    WHITE_IP_NODES_BUCKETS, WHITE_IP_NODES_DEPTH);
            httplog_LoadIPFile(&config->white_ip_list, file);
        }
    } else if ((LSTR_StrCmp(pstKey, "http_log_enable") == 0)
            && (pstValue->uiLen > 0) && (pstValue->pcData[0] == '1')) {
            config->log_enable = 1;
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

    return 0;
}

static int _httplog_HeadRawScan(char *field, int field_len, char *value, int value_len, void *ud)
{
    BUFFER_S *buffer = ud;

    if (field == NULL) { /* first line */
        field = "First-Line";
        field_len = strlen(field);
    }

    BUFFER_WriteString(buffer, ",\"");
    BUFFER_Write(buffer, field, field_len);
    BUFFER_WriteString(buffer, "\":\"");
    if (value) {
        BUFFER_Write(buffer, value, value_len);
    }
    BUFFER_WriteString(buffer, "\"");

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

    if ((config->white_host_enable) && head_info->host.start) {
        char host[256];
        int len = MIN(head_info->host.len, sizeof(host) - 1);
        memcpy(host, head_info->host.start, len);
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

static int _httplog_head_input(HTTP_LOG_S *config,
        HTTP_LOG_HEAD_INFO_S *head_info, BUFFER_S *buffer)
{
    UCHAR *src, *dst;
    UINT head_len = head_info->head_end - head_info->head_start;
    char *line;
    UINT line_len;
    char ip6_str_src[INET6_ADDRSTRLEN];
    char ip6_str_dst[INET6_ADDRSTRLEN];
    char buf[32];

    BUFFER_Print(buffer, "{\"type\":\"httplog\",\"log_ver\":1,\"time\":\"%s\"",
            TM_Utc2Acstime(time(0), buf));

    if (head_info->ether_type == NET_PKT_TYPE_IP) {
        src = (void*)&head_info->sip;
        dst = (void*)&head_info->dip;
        BUFFER_Print(buffer,
                ",\"address\":\"%d.%d.%d.%d:%d-%d.%d.%d.%d:%d\"",
                src[0], src[1], src[2], src[3], ntohs(head_info->sport),
                dst[0], dst[1], dst[2], dst[3], ntohs(head_info->dport));
    } else if (head_info->ether_type == NET_PKT_TYPE_IP6) {
        inet_ntop6_full(&head_info->ip6_src, ip6_str_src, sizeof(ip6_str_src));
        inet_ntop6_full(&head_info->ip6_dst, ip6_str_dst, sizeof(ip6_str_dst));
        BUFFER_Print(buffer, ",\"address\":\"%s:%d-%s:%d\"",
                ip6_str_src, ntohs(head_info->sport), ip6_str_dst,
                ntohs(head_info->dport));
    }

    if (config->head_format == HTTP_LOG_HEAD_FMT_HEX) {
        BUFFER_WriteString(buffer, ",\"head_hex\":\"");
        BUFFER_WriteByHex(buffer, head_info->head_start, head_len);
        BUFFER_WriteString(buffer, "\"");
    } else if (config->head_format == HTTP_LOG_HEAD_FMT_JSON) {
        HTTP_HeadRawScan(head_info->head_start, head_len, _httplog_HeadRawScan, buffer);
    }

    BUFFER_WriteString(buffer, "}\n");

    if (config->head_format == HTTP_LOG_HEAD_FMT_RAW) {
        TXT_SCAN_N_LINE_BEGIN(head_info->head_start, head_len, line, line_len) {
            if (line_len && _httplog_IsPermitLogLine(config, line, line_len)) {
                BUFFER_Write(buffer, line, line_len + 2);
            }
        }TXT_SCAN_LINE_END();
        BUFFER_Write(buffer, "\r\n", 2);
    }

    return 0;
}

static void _httplog_output_log(void *data, UINT len, USER_HANDLE_S *ud)
{
    char *str = data;
    HTTP_LOG_S *config = ud->ahUserHandle[0];
    str[len] = '\0';
    LOG_Output(&config->log_utl, data, len);
}

static int _httplog_process_head(HTTP_LOG_S *config, HTTP_LOG_HEAD_INFO_S *head_info)
{
    char buf[2048];
    BUFFER_S buffer;
    USER_HANDLE_S ud;

    ud.ahUserHandle[0] = config;

    BUFFER_Init(&buffer);
    BUFFER_AttachBuf(&buffer, buf, sizeof(buf)-1);
    BUFFER_SetOutputFunc(&buffer, _httplog_output_log, &ud);
    int ret = _httplog_head_input(config, head_info, &buffer);
    BUFFER_Flush(&buffer);
    BUFFER_Fini(&buffer);

    return ret;
}

int HTTPLOG_HeadInput(HTTP_LOG_S *config, HTTP_LOG_HEAD_INFO_S *head_info)
{
    if (config->log_enable == 0) {
        return BS_NOT_INIT;
    }

    int ret = httplog_Filter(config, head_info);
    if (ret != 0) {
        return ret;
    }

    return _httplog_process_head(config, head_info);
}

static int _httplog_body_input(HTTP_LOG_S *config,
        HTTP_LOG_HEAD_INFO_S *body_info, int log_len, BUFFER_S *buffer)
{
    char ip6_str_src[INET6_ADDRSTRLEN];
    char ip6_str_dst[INET6_ADDRSTRLEN];
    UCHAR *src, *dst;
    char buf[32];

    BUFFER_Print(buffer, "{\"type\":\"httpbody\",\"log_ver\":1,\"time\":\"%s\"",
            TM_Utc2Acstime(time(0), buf));

    if (body_info->ether_type == NET_PKT_TYPE_IP) {
        src = (void*)&body_info->sip;
        dst = (void*)&body_info->dip;
        BUFFER_Print(buffer,
                ",\"address\":\"%d.%d.%d.%d:%d-%d.%d.%d.%d:%d\"",
                src[0], src[1], src[2], src[3], ntohs(body_info->sport),
                dst[0], dst[1], dst[2], dst[3], ntohs(body_info->dport));
    }else if (body_info->ether_type == NET_PKT_TYPE_IP6) {
        inet_ntop6_full(&body_info->ip6_src, ip6_str_src, sizeof(ip6_str_src));
        inet_ntop6_full(&body_info->ip6_dst, ip6_str_dst, sizeof(ip6_str_dst));
        BUFFER_Print(buffer, ",\"address\":\"%s:%d-%s:%d\"",
                ip6_str_src, ntohs(body_info->sport), ip6_str_dst,
                ntohs(body_info->dport));
    }

    BUFFER_Print(buffer,  ",\"dir\":\"%d\",\"body\":\"", body_info->is_request ? 0:1);
    UINT len = body_info->body_end - body_info->body_start;
    len = MIN(log_len, len);
    BUFFER_WriteByHex(buffer, (CHAR*)body_info->body_start, len);
    BUFFER_WriteString(buffer, "\"");
    body_info->log_printed_len += len;

    BUFFER_WriteString(buffer, "}\n");

    return 0;
}

static int _httplog_process_body(HTTP_LOG_S *config, HTTP_LOG_HEAD_INFO_S *body_info, int len)
{
    char buf[2048];
    BUFFER_S buffer;
    USER_HANDLE_S ud;

    ud.ahUserHandle[0] = config;

    BUFFER_Init(&buffer);
    BUFFER_AttachBuf(&buffer, buf, sizeof(buf)-1);
    BUFFER_SetOutputFunc(&buffer, _httplog_output_log, &ud);
    int ret = _httplog_body_input(config, body_info, len, &buffer);
    BUFFER_Flush(&buffer);
    BUFFER_Fini(&buffer);

    return ret;
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

    return _httplog_process_body(config, body_info, log_len);
}

int _HTTPLOG_GetUa_Msg(HTTP_LOG_S *config, HTTP_LOG_HEAD_INFO_S *http_info)
{
    time_t seconds = time(NULL);
    char info[1024];
    memset(info,0,sizeof(info));

    UCHAR *sip = (void*)&http_info->sip;
    
    CHAR *key = "ua";

    int offset = scnprintf(info, sizeof(info),
            "{\"type\":\"%s\",\"log_ver\":%d,\"time\":\"%ld\","
            "\"client_ip\":\"%d.%d.%d.%d\", \"ua\":\"",
            key, 1, seconds, sip[0], sip[1], sip[2], sip[3]);
    
    memcpy(info + offset, (unsigned char *)http_info->user_agent.start, http_info->user_agent.len);    
    offset += http_info->user_agent.len;
    offset += scnprintf(info + offset, sizeof(info) - offset, "\"}");

#ifdef USE_REDIS
    void *redisHandle = redisGetRedisHandle();
    redisWriteDataToListTail(redisHandle,key,info);
#endif

    return 0;
}

#if USE_SLS
int _HTTPLOG_Input(HTTP_LOG_S *config, HTTP_LOG_HEAD_INFO_S *http_info)
{
    char ip6_str_src[INET6_ADDRSTRLEN];
    char ip6_str_dst[INET6_ADDRSTRLEN];
    char sport[16], dport[16];

    if (http_info->ether_type == NET_PKT_TYPE_IP) {
        IPString_IP2String(http_info->sip, ip6_str_src);
        IPString_IP2String(http_info->dip, ip6_str_dst);
    } else if (http_info->ether_type == NET_PKT_TYPE_IP6) {
        inet_ntop6_full(&http_info->ip6_src, ip6_str_src, sizeof(ip6_str_src));
        inet_ntop6_full(&http_info->ip6_dst, ip6_str_dst, sizeof(ip6_str_dst));
    }

    log_client_t *client = (log_client_t*) config->log_utl.sls_client;
    if (!client) return BS_ERR;

    LogSls_Start(client);

    if (http_info->is_request || !http_info->uuid) {
#ifdef USE_SLS
        apr_uuid_t g;
        memset(http_info->uuid, 0, sizeof(http_info->uuid));
        apr_uuid_get(&g);
        apr_uuid_format((char*)http_info->uuid, &g);
#endif
    }

    LOG_PRODUCER_CLIENT_SEND_LOG_NKV(client, "type", "httplog", "logver", "2", "uuid", http_info->uuid);
    LOG_PRODUCER_CLIENT_SEND_LOG_NKV(client, "sip", ip6_str_src, "dip", ip6_str_dst,
            "sport", stoa(sport, http_info->sport), "dport", stoa(dport, http_info->dport));
    if (http_info->is_request == 0) {
        if (http_info->response_code.len) {
            LogSls_AddKvLen(client, "rspcode", sizeof("rspcode") - 1,
                    (void*)http_info->response_code.start, http_info->response_code.len);
        }
        if (http_info->location.len) {
            LogSls_AddKvLen(client, "location", sizeof("location") -1,
                    (void*)http_info->location.start, http_info->location.len);
        }
        if (http_info->content_type.len) {
            LogSls_AddKvLen(client, "contenttype", sizeof("contenttype") -1,
                    (void*)http_info->content_type.start, http_info->content_type.len);
        }

        U_COOKIE_PTR *ptr = http_info->cookie;
        while(ptr) {
            if (http_info->is_request) {
                LogSls_AddKvLen(client, "setcookie", sizeof("setcookie") -1,
                        (void*)ptr->cookie, ptr->cookie_end - ptr->cookie);
            } else {
                LogSls_AddKvLen(client, "cookie", sizeof("cookie") -1,
                        ptr->cookie, ptr->cookie_end - ptr->cookie);
            }
            ptr = ptr->next;
        }
        LogSls_AddKv(client, "dir", "2");
    } else {
        if (http_info->method.len) {
            LogSls_AddKvLen(client, "method", sizeof("method") -1,
                    http_info->method.start, http_info->method.len);
        }
        if (http_info->uri.len) {
            LogSls_AddKvLen(client, "uri", sizeof("uri") -1,
            http_info->uri.start, http_info->uri.len);
        }
        if (http_info->host.len) {
            LogSls_AddKvLen(client, "host", sizeof("host") -1,
                    http_info->host.start, http_info->host.len);
        }

        U_COOKIE_PTR *ptr = http_info->cookie;
        while(ptr) {
            LogSls_AddKvLen(client, "cookie", sizeof("cookie") -1,
                    ptr->cookie, ptr->cookie_end - ptr->cookie);
            ptr = ptr->next;
        }
        if (http_info->content_type.len) {
            LogSls_AddKvLen(client, "contenttype", sizeof("contenttype") -1,
                    http_info->content_type.start, http_info->content_type.len);
        }
        if (http_info->proxy_connect.len) {
            LogSls_AddKvLen(client, "proxyconnection", sizeof("proxyconnetion"),
                    http_info->proxy_connect.start, http_info->proxy_connect.len);
        }
        if (http_info->proxy_authorization.len) {
            LogSls_AddKvLen(client, "proxyauthorization", sizeof("proxyauthoriation"),
                    http_info->proxy_authorization.start, http_info->proxy_authorization.len);
        }

        if (http_info->x_forward_for.len) {
            LogSls_AddKvLen(client, "xforwardfor", sizeof("xforwardfor"),
                    http_info->x_forward_for.start, http_info->x_forward_for.len);
        }
        if (http_info->referer.len) {
            LogSls_AddKvLen(client, "referer", sizeof("referer"),
                    http_info->referer.start, http_info->referer.len);
        }
        if (http_info->user_agent.len) {
            LogSls_AddKvLen(client, "useragent", sizeof("useragent"),
                    http_info->user_agent.start, http_info->user_agent.len);
        }
        LogSls_AddKv(client, "dir", "1");
    }
    UINT len = http_info->head_end - http_info->head_start;
    if (len && HTTP_LOG_HEAD_FMT_RAW) {
        LogSls_AddKvLen(client, "head", sizeof("head"), http_info->head_start, len);
    }

    int log_len = 0;
    if (http_info->is_request) {
        if (config->log_request_body != 0) {
            log_len = config->log_request_body - http_info->log_printed_len;
        }
    } else {
        if (config->log_response_body == 0) {
        }
        log_len = config->log_response_body - http_info->log_printed_len;
    }

    len = http_info->body_end - http_info->body_start;
    log_len = log_len >= len ? len  : log_len;
    if (log_len) {
        //LOG_OutputByHex(&config->log_utl, (CHAR*)http_info->body_start, len);
        LogSls_AddKvLen(client, "body", sizeof("body"), http_info->body_start, log_len);
        http_info->log_printed_len += log_len;
    }

    LogSls_End(client);

    return 0;
}
#endif

int HTTPLOG_Input(HTTP_LOG_S *config, HTTP_LOG_HEAD_INFO_S *http_info)
{
    if (config->log_enable == 0) {
        return BS_NOT_INIT;
    }

    if(config->useragent_db_enable && http_info->is_request &&
                _ua_bloomfilter_test(config, http_info->sip,
                http_info->user_agent.len, http_info->user_agent.start)){
        _HTTPLOG_GetUa_Msg(config, http_info);
    }

    if (config->white_host_enable && (http_info->match_white_list == 1)){
        return BS_NO_PERMIT;
    }

    if (config->only_dip_enable) {
        if (Box_Find(&config->only_dip_list, UINT_HANDLE(http_info->dip)) < 0) {
            http_info->match_white_list = 1;
            return BS_NO_PERMIT;
        }
    }

    if ((config->white_host_enable) && http_info->host.start && http_info->is_request) {
        char host[256];
        int len = MIN(http_info->host.len, sizeof(host) - 1);
        memcpy(host, http_info->host.start, len);
        host[len] = 0;
        if (Box_Find(&config->white_host_list, host) >= 0) {
            http_info->match_white_list = 1;
            return BS_NO_PERMIT;
        }
    }

    if ((config->bloomfilter_size != 0)
            && (0 == _httplog_bloomfilter_test(config, http_info->sip,
                    http_info->dip, http_info->sport, http_info->dport))) {
        return BS_ALREADY_EXIST;
    }

    int ret =  0;

    if (config->log_utl.file) {
        if (http_info->with_head) {
            ret |= HTTPLOG_HeadInput(config, http_info);
        }
        if (http_info->with_body) {
            ret |= HTTPLOG_BodyInput(config, http_info);
        }
    }
#ifdef USE_SLS
    if (config->log_utl.sls) {
        ret |= _HTTPLOG_Input(config, http_info);
    }
#endif

    return ret;
}
