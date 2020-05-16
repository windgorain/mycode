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
#include "../h/precver_def.h"

static int precver_rand_cb(void *eth_pkt, int pkt_len, void *ud)
{
    PRECVER_PKT_S pkt;

    pkt.data = (void*)eth_pkt;
    pkt.data_len = pkt_len;
    gettimeofday(&pkt.ts, NULL);

    return PRecver_PktInput(ud, &pkt);
}

int PRecver_Rand_Run(void *runner, int argc, char **argv)
{
    S2IP_S s2ip_ctrl;
    UCHAR data[1500];
    int i;

    S2IP_Init(&s2ip_ctrl, RAND_Get(), RAND_Get(), RAND_Get(), RAND_Get());

    S2IP_Hsk(&s2ip_ctrl, precver_rand_cb, runner);

    for (i=0; i<10; i++) {
        Sleep(10);
        S2IP_Data(&s2ip_ctrl, data, sizeof(data), precver_rand_cb, runner);
    }

    S2IP_Bye(&s2ip_ctrl, precver_rand_cb, runner);

    return 0;
}

