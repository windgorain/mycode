/*================================================================
*   Created by LiXingang
*   Description: 产生随机报文
*
================================================================*/
#include "bs.h"
#include "utl/rand_utl.h"
#include "utl/time_utl.h"
#include "utl/stream2ip.h"
#include "utl/getopt2_utl.h"
#include "comp/comp_precver.h"
#include "../../h/precver_def.h"
#include "../../h/precver_ev.h"
#include "../precver_impl_common.h"

#define PRECVER_RAND_SLOW 0x1

static int precver_rand_cb(void *eth_pkt, int pkt_len, void *ud)
{
    PRECVER_PKT_S pkt;
    PRECVER_RUNNER_S *runner = ud;

    pkt.data = (void*)eth_pkt;
    pkt.data_len = pkt_len;
    gettimeofday(&pkt.ts, NULL);

    PRecver_Ev_Publish(runner, &pkt);
    if (runner->need_stop) {
        return BS_STOP;
    }
    return 0;
}

static int _precver_rand_help(GETOPT2_NODE_S *opts)
{
    char buf[512];
    printf("%s\r\n", GETOPT2_BuildHelpinfo(opts, buf, sizeof(buf)));
    return 0;
}

static UINT _precver_rand_init(int argc, char **argv)
{
    UINT flag = 0;
    GETOPT2_NODE_S opts[] = {
        {'o', 's', "slow", 0, NULL, "slow down", 0},
        {0}
    };

    if (BS_OK != GETOPT2_ParseFromArgv0(argc, argv, opts)) {
        _precver_rand_help(opts);
        return 0;
    }

    if (GETOPT2_IsOptSetted(opts, 's', NULL)) {
        flag |= PRECVER_RAND_SLOW;
    }

    return flag;
}

PLUG_API int PRecverImpl_Init(PRECVER_RUNNER_S *runner, int argc, char **argv)
{
    UINT flag = _precver_rand_init(argc, argv);
    runner->recver_handle = UINT_HANDLE(flag);
    return 0;
}

PLUG_API int PRecverImpl_Run(PRECVER_RUNNER_S *runner)
{
    S2IP_S s2ip_ctrl;
    UCHAR data[1400];
    int i, j;
    UINT flag;
    int count = 10000;
    int count_data = 100;
    int tcp_hsk = 1;

    flag = HANDLE_UINT(runner->recver_handle);

    if (flag & PRECVER_RAND_SLOW) {
        count = 1;
        count_data = 1;
        tcp_hsk = 0;
    }

    memset(&s2ip_ctrl, 0, sizeof(s2ip_ctrl));

    for (i=0; i<count; i++) {
        S2IP_Init(&s2ip_ctrl, RAND_Get(), RAND_Get(), RAND_Get(), RAND_Get());

        if (tcp_hsk) {
            S2IP_Hsk(&s2ip_ctrl, precver_rand_cb, runner);
        }

        for (j=0; j<count_data; j++) {
            S2IP_Data(&s2ip_ctrl, data, sizeof(data), tcp_hsk, precver_rand_cb, runner);
        }

        if (tcp_hsk) {
            S2IP_Bye(&s2ip_ctrl, precver_rand_cb, runner);
        }
    }

    if (flag & PRECVER_RAND_SLOW) {
        Sleep(1000);
    }

    return 0;
}

