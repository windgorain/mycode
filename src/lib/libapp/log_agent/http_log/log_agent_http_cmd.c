/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/log_utl.h"
#include "utl/socket_utl.h"
#include "utl/ip_utl.h"
#include "utl/txt_utl.h"
#include "utl/exec_utl.h"
#include "utl/http_log.h"
#include "utl/match_utl.h"

#include "../h/log_agent_http.h"
#include "../h/log_agent_conf.h"


PLUG_API int LOGAGENT_HTTP_CmdHttpLogEnable(int argc, char **argv)
{
    HTTP_LOG_S *log = LOGAGENT_HTTP_GetCtrl();

    if (argv[0][0] == 'n') {
        log->log_enable = 0;
    } else {
        log->log_enable = 1;
    }

    return 0;
}


PLUG_API int LOGAGENT_HTTP_CmdHttpLogCookie(int argc, char **argv)
{
    HTTP_LOG_S *log = LOGAGENT_HTTP_GetCtrl();

    if (argv[0][0] == 'n') {
        log->log_cookie = 0;
    } else {
        log->log_cookie = 1;
    }

    return 0;
}


PLUG_API int LOGAGENT_HTTP_CmdHttpLogSetCookie(int argc, char **argv)
{
    HTTP_LOG_S *log = LOGAGENT_HTTP_GetCtrl();

    if (argv[0][0] == 'n') {
        log->log_setcookie = 0;
    } else {
        log->log_setcookie = 1;
    }

    return 0;
}


PLUG_API int LOGAGENT_HTTP_CmdHttpLogRequestBody(int argc, char **argv)
{
    HTTP_LOG_S *log = LOGAGENT_HTTP_GetCtrl();
    int len = TXT_Str2Ui(argv[3]);

    log->log_request_body = len;

    return 0;
}


PLUG_API int LOGAGENT_HTTP_CmdHttpLogResponseBody(int argc, char **argv)
{
    HTTP_LOG_S *log = LOGAGENT_HTTP_GetCtrl();
    int len = TXT_Str2Ui(argv[3]);

    log->log_response_body = len;

    return 0;
}


PLUG_API int LOGAGENT_HTTP_CmdMatchIP(int argc, char **argv)
{
    MATCH_HANDLE hMatch = LOGAGENT_HTTP_GetMatch();
    int index = TXT_Str2Ui(argv[2]);
    int prefix = TXT_Str2Ui(argv[6]);
    IP_MATCH_PATTERN_S *pattern;

    pattern = Match_GetPattern(hMatch, index);
    if (NULL == pattern) {
        RETURN(BS_OUT_OF_RANGE);
    }

    if (argv[3][0] == 's') { 
        pattern->sip = Socket_NameToIpNet(argv[4]);
        pattern->sip_mask = PREFIX_2_MASK(prefix);
        pattern->sip_mask = htonl(pattern->sip_mask);
        pattern->sip &= pattern->sip_mask;
    } else {
        pattern->dip = Socket_NameToIpNet(argv[4]);
        pattern->dip_mask = PREFIX_2_MASK(prefix);
        pattern->dip_mask = htonl(pattern->dip_mask);
        pattern->dip &= pattern->dip_mask;
    }

    return 0;
}


PLUG_API int LOGAGENT_HTTP_CmdMatchPort(int argc, char **argv)
{
    MATCH_HANDLE hMatch = LOGAGENT_HTTP_GetMatch();
    int index = TXT_Str2Ui(argv[2]);
    USHORT port = TXT_Str2Ui(argv[4]);
    IP_MATCH_PATTERN_S *pattern;

    pattern = Match_GetPattern(hMatch, index);
    if (NULL == pattern) {
        RETURN(BS_OUT_OF_RANGE);
    }

    if (argv[3][0] == 's') { 
        pattern->sport = htons(port);
    } else {
        pattern->dport = htons(port);
    }

    return 0;
}


PLUG_API int LOGAGENT_HTTP_CmdMatchEnable(int argc, char **argv)
{
    MATCH_HANDLE hMatch = LOGAGENT_HTTP_GetMatch();
    int index = TXT_Str2Ui(argv[2]);
    IP_MATCH_PATTERN_S *pattern = Match_GetPattern(hMatch, index);

    if (pattern) {
        pattern->protocol = IP_PROTO_TCP;
    }

    Match_Enable(hMatch, index);
    return 0;
}


PLUG_API int LOGAGENT_HTTP_CmdMatchDisable(int argc, char **argv)
{
    MATCH_HANDLE hMatch = LOGAGENT_HTTP_GetMatch();
    int index = TXT_Str2Ui(argv[3]);
    Match_Disable(hMatch, index);
    return 0;
}

static void logagent_http_ShowMatchedCountOne(MATCH_HANDLE hMatch, int index)
{
    if (index >= LOGAGENT_HTTP_MACTCH_COUNT) {
        return;
    }

    IP_MATCH_PATTERN_S *pattern = Match_GetPattern(hMatch, index);
    UINT64 count = Match_GetMatchedCount(hMatch, index);

    char sip[32];
    char dip[32];
    Socket_Ip2Name(pattern->sip, sip, sizeof(sip));
    Socket_Ip2Name(pattern->dip, dip, sizeof(dip));
    UCHAR s_prefix = MASK_2_PREFIX(ntohl(pattern->sip_mask));
    UCHAR d_prefix = MASK_2_PREFIX(ntohl(pattern->dip_mask));

    char saddr[64];
    char daddr[64];
    snprintf(saddr, sizeof(saddr), "%s/%u:%u",
            sip, s_prefix, ntohs(pattern->sport));
    snprintf(daddr, sizeof(daddr), "%s/%u:%u",
            dip, d_prefix, ntohs(pattern->dport));

    int enable = Match_IsEnable(hMatch, index);

    EXEC_OutInfo("%-3d %-24s %-24s %-8u %-6u %llu \r\n",
            index, saddr, daddr, pattern->protocol, enable, count);
}


PLUG_API int LOGAGENT_HTTP_CmdShowMatchCount(int argc, char **argv)
{
    MATCH_HANDLE hMatch = LOGAGENT_HTTP_GetMatch();
    int i;

    EXEC_OutString("id  source                   destination              "
            "protocol enable count\r\n");

    if (argc >= 5) {
        i = TXT_Str2Ui(argv[4]);
        logagent_http_ShowMatchedCountOne(hMatch, i);
    } else {
        for (i=0; i<LOGAGENT_HTTP_MACTCH_COUNT; i++) {
            if (Match_IsEnable(hMatch, i)) {
                logagent_http_ShowMatchedCountOne(hMatch, i);
            }
        }
    }

    return 0;
}


PLUG_API int LOGAGENT_HTTP_CmdResetMatchCount(int argc, char **argv)
{
    MATCH_HANDLE hMatch = LOGAGENT_HTTP_GetMatch();
    int i;

    if (argc >= 5) {
        i = TXT_Str2Ui(argv[4]);
        Match_ResetMatchedCount(hMatch, i);
    } else {
        Match_ResetAllMatchedCount(hMatch);
    }

    return 0;
}

void LOGAGENT_HTTP_Save(HANDLE hFile)
{
    HTTP_LOG_S *log = LOGAGENT_HTTP_GetCtrl();
    MATCH_HANDLE hMatch = LOGAGENT_HTTP_GetMatch();
    int i;
    IP_MATCH_PATTERN_S *pattern;

    if (log->log_cookie) {
        CMD_EXP_OutputCmd(hFile, "http log cookie");
    }

    if (log->log_setcookie) {
        CMD_EXP_OutputCmd(hFile, "http log setcookie");
    }

    if (log->log_request_body) {
        CMD_EXP_OutputCmd(hFile, "http log request-body %u",
                log->log_request_body);
    }

    if (log->log_response_body) {
        CMD_EXP_OutputCmd(hFile, "http log response-body %u",
                log->log_response_body);
    }

    if (log->bloomfilter_size > 0) {
        CMD_EXP_OutputCmd(hFile, "http log bloomfilter %u",
                log->bloomfilter_size);
    }

    if (log->white_host_enable) {
        CMD_EXP_OutputCmd(hFile, "http log filter host-white-list");
    }

    if (log->only_dip_enable) {
        CMD_EXP_OutputCmd(hFile, "http log filter only-dip-list");
    }

    for (i=0; i<LOGAGENT_HTTP_MACTCH_COUNT; i++) {
        pattern = Match_GetPattern(hMatch, i);
        if (! pattern) {
            continue;
        }

        if (pattern->sip) {
            CMD_EXP_OutputCmd(hFile, "http match %d sip %s mask %u",
                    i, Socket_IpToName(ntohl(pattern->sip)),
                    MASK_2_PREFIX(ntohl(pattern->sip_mask)));
        }
        if (pattern->dip) {
            CMD_EXP_OutputCmd(hFile, "http match %d dip %s mask %u",
                    i, Socket_IpToName(ntohl(pattern->dip)),
                    MASK_2_PREFIX(ntohl(pattern->dip_mask)));
        }
        if (pattern->sport) {
            CMD_EXP_OutputCmd(hFile, "http match %d sport %u",
                    i, ntohs(pattern->sport));
        }
        if (pattern->dport) {
            CMD_EXP_OutputCmd(hFile, "http match %d dport %u",
                    i, ntohs(pattern->dport));
        }
        if (Match_IsEnable(hMatch, i)) {
            CMD_EXP_OutputCmd(hFile, "http match %d enable", i);
        }
    }

    if (log->log_enable) {
        CMD_EXP_OutputCmd(hFile, "http log enable");
    }
}

