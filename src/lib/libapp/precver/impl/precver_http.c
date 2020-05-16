/*================================================================
*   Created by LiXingang
*   Description: 产生随机报文
*
================================================================*/
#include "bs.h"
#include "utl/time_utl.h"
#include "utl/rand_utl.h"
#include "comp/comp_precver.h"
#include "utl/stream2ip.h"
#include "../h/precver_def.h"

static struct timeval g_precver_http_ts;

static int psee_recver_http_cb(void *eth_pkt, int pkt_len, void *ud)
{
    PRECVER_PKT_S pkt;

    pkt.data = (void*)eth_pkt;
    pkt.data_len = pkt_len;
    pkt.ts = g_precver_http_ts;

    return PRecver_PktInput(ud, &pkt);
}

int PRecver_Http_Run(void *runner, int argc, char **argv)
{
    S2IP_S s2ip_ctrl;
    char *request =
        "GET / HTTP/1.1\r\n"
        "Host: test\r\n\r\n";
    char *reply=
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 10\r\n\r\n"
        "0123456789";
    int request_len = strlen(request);
    int reply_len = strlen(reply);
    int i;

    gettimeofday(&g_precver_http_ts, NULL);

    for (i=0; i<10000; i++) {
        S2IP_Init(&s2ip_ctrl, RAND_Get(), RAND_Get(), RAND_Get(), htons(80));
        S2IP_Hsk(&s2ip_ctrl, psee_recver_http_cb, runner);
        S2IP_Data(&s2ip_ctrl, request, request_len, psee_recver_http_cb, runner);
        S2IP_Switch(&s2ip_ctrl);
        S2IP_Data(&s2ip_ctrl, reply, reply_len, psee_recver_http_cb, runner);
        S2IP_Bye(&s2ip_ctrl, psee_recver_http_cb, runner);
    }

//    Sleep(1000);

    return 0;
}

