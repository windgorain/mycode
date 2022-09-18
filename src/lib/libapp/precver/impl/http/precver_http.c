/*================================================================
*   Created by LiXingang
*   Description: 产生随机报文
*
================================================================*/
#include "bs.h"
#include "utl/time_utl.h"
#include "utl/rand_utl.h"
#include "utl/getopt2_utl.h"
#include "comp/comp_precver.h"
#include "utl/stream2ip.h"
#include "../../h/precver_def.h"
#include "../../h/precver_ev.h"
#include "../precver_impl_common.h"

#define PRECVER_HTTP_SLOW 0x1

static struct timeval g_precver_http_ts;

static int precver_http_cb(void *eth_pkt, int pkt_len, void *ud)
{
    PRECVER_PKT_S pkt;
    PRECVER_RUNNER_S *runner = ud;

    pkt.data = (void*)eth_pkt;
    pkt.data_len = pkt_len;
    pkt.ts = g_precver_http_ts;

    PRecver_Ev_Publish(runner, &pkt);
    if (runner->need_stop) {
        return BS_STOP;
    }

    return 0;
}

static int precver_http_help(GETOPT2_NODE_S *opts)
{
    char buf[512];
    printf("%s\r\n", GETOPT2_BuildHelpinfo(opts, buf, sizeof(buf)));
    return 0;
}

static UINT precver_http_init(int argc, char **argv)
{
    UINT flag = 0;
    GETOPT2_NODE_S opts[] = {
        {'o', 'h', "help", 0, NULL, NULL, 0},
        {'o', 's', "slow", 0, NULL, "slow down", 0},
        {0}
    };

    if (BS_OK != GETOPT2_ParseFromArgv0(argc, argv, opts)) {
        precver_http_help(opts);
        return 0;
    }

    if (GETOPT2_IsOptSetted(opts, 'h', NULL)) {
        precver_http_help(opts);
        return 0;
    }

    if (GETOPT2_IsOptSetted(opts, 's', NULL)) {
        flag |= PRECVER_HTTP_SLOW;
    }

    return flag;
}

PLUG_API int PRecverImpl_Init(PRECVER_RUNNER_S *runner, int argc, char **argv)
{
    UINT flag = precver_http_init(argc, argv);
    runner->recver_handle = UINT_HANDLE(flag);
    return 0;
}

PLUG_API int PRecverImpl_Run(PRECVER_RUNNER_S *runner)
{
    S2IP_S s2ip_ctrl;
    char *request =
        "GET / HTTP/1.1\r\n"
        "Host: test.com\r\n\r\n";
    char *reply=
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 10\r\n\r\n"
        "0123456789";
    int request_len = strlen(request);
    int reply_len = strlen(reply);
    int i, j;
    int count = 10000;
    int count_data = 100;
    UINT flag;
    UINT seed = RAND_Get();

    memset(&s2ip_ctrl, 0, sizeof(s2ip_ctrl));

    flag = HANDLE_UINT(runner->recver_handle);

    if (flag & PRECVER_HTTP_SLOW) {
        count = 1;
        count_data = 1;
    }

    gettimeofday(&g_precver_http_ts, NULL);

    for (i=0; i<count; i++) {
        S2IP_Init(&s2ip_ctrl, RAND_FastGet(&seed), RAND_FastGet(&seed),
                RAND_FastGet(&seed), htons(80));
        S2IP_Hsk(&s2ip_ctrl, precver_http_cb, runner);
        for (j=0; j<count_data; j++) {
            S2IP_Data(&s2ip_ctrl, request, request_len, precver_http_cb, runner);
            S2IP_Switch(&s2ip_ctrl);
            S2IP_Data(&s2ip_ctrl, reply, reply_len, precver_http_cb, runner);
            S2IP_Switch(&s2ip_ctrl);
        }
        S2IP_Bye(&s2ip_ctrl, precver_http_cb, runner);
    }

    if (flag & PRECVER_HTTP_SLOW) {
        Sleep(1000);
    }

    return 0;
}

