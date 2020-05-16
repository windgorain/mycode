/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/net.h"
#include "utl/ip_utl.h"
#include "utl/tcp_utl.h"
#include "utl/tcp_log.h"
#include "utl/match_utl.h"
#include "../h/log_agent_conf.h"
#include "../h/log_agent_tcp.h"

static TCP_LOG_S g_logagent_tcp_log;
static MATCH_HANDLE g_logagent_tcp_match;

static void logagent_tcp_DoMatch(void *ippkt, UINT pktlen, NET_PKT_TYPE_E type)
{
    IP_HEAD_S *ip_head = IP_GetIPHeader(ippkt, pktlen, type);
    if (NULL == ip_head) {
        return;
    }

    int offset = (char*)ip_head - (char*)ippkt;
    TCP_HEAD_S *tcp_head = TCP_GetTcpHeader((void*)ip_head,
            pktlen - offset, NET_PKT_TYPE_IP);
    if (tcp_head == NULL) {
        return;
    }

    IP_MATCH_KEY_S key = {0};
    key.sip = ip_head->unSrcIp.uiIp;
    key.dip = ip_head->unDstIp.uiIp;
    key.sport = tcp_head->usSrcPort;
    key.dport = tcp_head->usDstPort;
    key.protocol = IP_PROTO_TCP;

    Match_Do(g_logagent_tcp_match, &key);
}

int LOGAGENT_TCP_Init()
{
    g_logagent_tcp_match = IPMatch_Create(LOGAGENT_TCP_MACTCH_COUNT);
    return 0;
}

TCP_LOG_S * LOGAGENT_TCP_GetCtrl()
{
    return &g_logagent_tcp_log;
}

MATCH_HANDLE LOGAGENT_TCP_GetMatch()
{
    return g_logagent_tcp_match;
}

BS_STATUS LOGAGENT_TCP_ParseConfig(char *conf_string)
{
    TCPLOG_Init(&g_logagent_tcp_log, LOGAGENT_CONF_GetConfDir(),
            LOGAGENT_CONF_GetLogDir());
    return TCPLOG_ParseConfig(&g_logagent_tcp_log, conf_string);
}

int LOGAGENT_TCP_Input(void *ippkt, UINT pktlen, NET_PKT_TYPE_E pkt_type)
{
    int ret = TCPLOG_Input(&g_logagent_tcp_log, ippkt, pktlen, pkt_type);

    if (ret == BS_OK) {
        logagent_tcp_DoMatch(ippkt, pktlen, pkt_type);
    }

    return ret;
}


