/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/net.h"
#include "comp/comp_precver.h"
#include "comp/comp_event_hub.h"
#include "../h/precver_def.h"
#include "../h/precver_worker.h"

static inline USHORT precver_sample_get(PRECVER_RUNNER_S *runner, void *pkt)
{
    USHORT sample_rate = runner->sample_rate;

    if (sample_rate == 0) {
        PRECVER_RUNNER_S *global_runner = PRecver_Worker_GetRunner(0);
        sample_rate = global_runner->sample_rate;
    }

    return sample_rate;
}

static inline BOOL_T precver_sample_process(PRECVER_RUNNER_S *runner, PRECVER_PKT_S *pkt,
        USHORT sample_rate, OUT MAC_ADDR_S *old)
{
    runner->sample_count = (runner->sample_count + 1) % sample_rate;
    if (runner->sample_count != 0) {
        return FALSE;
    }

    memcpy(old, pkt->data, sizeof(MAC_ADDR_S));

    pkt->data[0] = 0xcb;
    pkt->data[1] = 0x40;
    pkt->data[2] = 2;
    memcpy(pkt->data + 3, &sample_rate, sizeof(uint16_t));

    return TRUE;
}

void PRecver_Ev_Publish(void *in_runner, void *rpkt)
{
    MAC_ADDR_S dmac;
    PRECVER_PKT_S *pkt = rpkt;
    PRECVER_RUNNER_S *runner = in_runner;
    BOOL_T sampled = FALSE;

    USHORT sample_rate = precver_sample_get(runner, pkt);

    if (sample_rate) {
        sampled = precver_sample_process(runner, pkt, sample_rate, &dmac);
        if (! sampled) {
            return;
        }
    }

    EHUB_Publish(EHUB_EV_PRECVER_PKT, pkt);

    if (sample_rate) {
        memcpy(pkt->data, &dmac, sizeof(dmac));
    }

    return;
}

void PRecver_Ev_TimeStep(void *runner)
{
    EHUB_Publish(EHUB_EV_WORKER_TIME_STEP, runner);
}

