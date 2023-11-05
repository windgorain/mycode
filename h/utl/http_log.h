#ifndef _HTTP_LOG_H
#define _HTTP_LOG_H

#include "utl/bloomfilter_utl.h"
#include "utl/box_utl.h"
#include "utl/log_utl.h"
#include "utl/eth_utl.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define MAX_SPACE_LEN 40
#define MAX_RN_LEN  2850


#define HTTP_LOG_HEAD_FMT_RAW  0
#define HTTP_LOG_HEAD_FMT_JSON 1
#define HTTP_LOG_HEAD_FMT_HEX  2

typedef struct {
    UINT log_enable: 1;
    UINT log_cookie: 1;
    UINT log_setcookie: 1;
    UINT white_host_enable: 1;
    UINT white_ip_enable: 1;
    UINT only_dip_enable: 1;
    UINT useragent_db_enable: 1;
    UINT head_format:2;
    UINT log_request_body;
    UINT log_response_body;
    char *config_base_dir;
    LOG_UTL_S log_utl;
    BLOOM_FILTER_S bloomfilter;
    BLOOM_FILTER_S ua_filter;
    UINT bloomfilter_size; 
    UINT ua_filter_size; 
    BOX_S white_host_list;
    BOX_S white_ip_list;
    BOX_S only_dip_list;
}HTTP_LOG_S;

typedef struct HEADER_FIELD_LOCATION_S {
    uint8_t *start;
    int     len;
} HEADER_FIELD_LOCATION_PTR;

typedef struct u_COOKIE_PTR
{
    char *cookie;
    char *cookie_end;
    struct u_COOKIE_PTR *next;
} U_COOKIE_PTR;

typedef struct {
    USHORT ether_type;
    UINT sip;
    UINT dip;
    struct in6_addr ip6_src;
    struct in6_addr ip6_dst;
    USHORT sport;
    USHORT dport;
    USHORT is_request:1; 
    USHORT match_white_list:1; 
    USHORT reuse_parsed_value :1;
    USHORT is_chunked: 1;
    USHORT with_body: 1;
    USHORT with_head: 1;
    USHORT log_printed_len;
	char *head_start;
	char *head_end;
	char *body_start;
	char *body_end;
	#define HEADER_CONTENT_SET(A,B) A.start = B.start; A.len = B.len;
	HEADER_FIELD_LOCATION_PTR host, user_agent, referer, uri, method, proxy_connect, proxy_authorization, x_forward_for;
	HEADER_FIELD_LOCATION_PTR content_type;
	HEADER_FIELD_LOCATION_PTR response_code, location;
	U_COOKIE_PTR *cookie;
	int32_t content_len;
	unsigned char uuid[37];
}HTTP_LOG_HEAD_INFO_S;

char * httplog_FindHeadEnd(char *data, int len);
void HTTPLOG_Init(HTTP_LOG_S *config);
void HTTPLOG_Final(HTTP_LOG_S *config);
int HTTPLOG_SetConfDir(HTTP_LOG_S *config, char *base_dir, char *log_dir);
BS_STATUS HTTPLOG_ParseConfig(HTTP_LOG_S *config, char *conf_string);
int HTTPLOG_Input(HTTP_LOG_S *config, HTTP_LOG_HEAD_INFO_S *http_info);
int HTTPLOG_HeadInput(HTTP_LOG_S *config, HTTP_LOG_HEAD_INFO_S *head_info);
int HTTPLOG_BodyInput(HTTP_LOG_S *config, HTTP_LOG_HEAD_INFO_S *body_info);

#ifdef __cplusplus
}
#endif
#endif 
