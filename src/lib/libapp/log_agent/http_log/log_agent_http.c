/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/log_utl.h"
#include "utl/socket_utl.h"
#include "utl/txt_utl.h"
#include "utl/exec_utl.h"
#include "utl/ip_utl.h"
#include "utl/match_utl.h"
#include "utl/http_log.h"
#include "../h/log_agent_conf.h"

#define LOGAGENT_HTTP_MACTCH_COUNT 128

static HTTP_LOG_S g_logagent_httplog;
static MATCH_HANDLE g_logagent_http_match;

static void logagent_http_DoMatch(HTTP_LOG_HEAD_INFO_S *head_info)
{
    if (head_info->ether_type != NET_PKT_TYPE_IP) {
        return;
    }

    IP_MATCH_KEY_S key = {0};
    key.sip = head_info->sip;
    key.dip = head_info->dip;
    key.sport = head_info->sport;
    key.dport = head_info->dport;
    key.protocol = IP_PROTO_TCP;

    Match_Do(g_logagent_http_match, &key);
}

int LOGAGENT_HTTP_Init()
{
    g_logagent_http_match = IPMatch_Create(LOGAGENT_HTTP_MACTCH_COUNT);
    HTTPLOG_Init(&g_logagent_httplog);

    return 0;
}

HTTP_LOG_S * LOGAGENT_HTTP_GetCtrl()
{
    return &g_logagent_httplog;
}

HTTP_LOG_S * LOGAGENT_HTTP_GetMatch()
{
    return g_logagent_http_match;
}

PLUG_API int LOGAGENT_HTTP_ParseConfig(char *config_buf, UCHAR *is_enable)
{
    HTTPLOG_Final(&g_logagent_httplog);
    HTTPLOG_Init(&g_logagent_httplog);
    HTTPLOG_SetConfDir(&g_logagent_httplog, LOGAGENT_CONF_GetConfDir(),
            LOGAGENT_CONF_GetLogDir());

    int ret = HTTPLOG_ParseConfig(&g_logagent_httplog, config_buf);

	*is_enable = g_logagent_httplog.log_enable;

    return ret;
}

PLUG_API int LOGAGENT_HTTP_HeadInput(HTTP_LOG_HEAD_INFO_S *head_info)
{
    int ret = HTTPLOG_HeadInput(&g_logagent_httplog, head_info);

    if (ret == BS_OK) {
        logagent_http_DoMatch(head_info);
    }

    return ret;
}

PLUG_API int LOGAGENT_HTTP_BodyInput(HTTP_LOG_HEAD_INFO_S *body_info)
{
    return HTTPLOG_BodyInput(&g_logagent_httplog, body_info);
}


PLUG_API int LOGAGENT_HTTP_Input(HTTP_LOG_HEAD_INFO_S *http_info)
{
    return HTTPLOG_Input(&g_logagent_httplog, http_info);
}
