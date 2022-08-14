/*================================================================
*   Created by LiXingang
*   Description: 产生随机报文
*
================================================================*/
#include "bs.h"
#include "utl/rand_utl.h"
#include "utl/time_utl.h"
#include "utl/stream2ip.h"
#include "comp/comp_precver.h"
#include "../../h/precver_def.h"
#include "../../h/precver_ev.h"
#include "../precver_impl_common.h"

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

PLUG_API int PRecverImpl_Init(PRECVER_RUNNER_S *runner, int argc, char **argv)
{
    return 0;
}

PLUG_API int PRecverImpl_Run(void *runner)
{
    S2IP_S s2ip_ctrl;
    UCHAR data[1400];
    int i, j;

    memset(&s2ip_ctrl, 0, sizeof(s2ip_ctrl));

    for (i=0; i<1000; i++) {
        S2IP_Init(&s2ip_ctrl, RAND_Get(), RAND_Get(), RAND_Get(), RAND_Get());
        S2IP_Hsk(&s2ip_ctrl, precver_rand_cb, runner);

        for (j=0; j<1000; j++) {
            S2IP_Data(&s2ip_ctrl, data, sizeof(data), precver_rand_cb, runner);
        }

        S2IP_Bye(&s2ip_ctrl, precver_rand_cb, runner);
    }

    return 0;
}
