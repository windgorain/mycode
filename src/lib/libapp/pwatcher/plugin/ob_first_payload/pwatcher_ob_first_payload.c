/*================================================================
*   Created by LiXingang
*   Description: tcp流量的首个payload报文处理
*
================================================================*/
#include "bs.h"
#include "utl/cff_utl.h"
#include "utl/plug_utl.h"
#include "../../h/pwatcher_session.h"
#include "../../h/pwatcher_def.h"
#include "../../h/pwatcher_event.h"
#include "../h/pwatcher_ob_common.h"

static int pwatcher_ob_first_payload_input(UINT point, PWATCHER_PKT_DESC_S *pkt, void *data);

static PWATCHER_EV_OB_S g_pwatcher_ob_node = {
    .ob_name="first-payload",
    .event_func=pwatcher_ob_first_payload_input
};

static PWATCHER_EV_POINT_S g_pwatcher_ob_points[] = {
    {.valid=1, .ob=&g_pwatcher_ob_node, .point=PWATCHER_EV_TCP},
    {.valid=0, .ob=&g_pwatcher_ob_node}
};

static inline void pwatcher_ob_first_payload_process_pkt(PWATCHER_PKT_DESC_S *pkt)
{
    PWATCHER_STREAM_S *stream = pkt->stream;

    if (! stream) {
        return;
    }

    if (stream->tcp_status.first_payload_recved) {
        return;
    }

    if (pkt->tcp_header->ucFlag & TCP_FLAG_SYN) {
        stream->tcp_status.first_payload_seq = ntohl(pkt->tcp_header->ulSequence) + 1;
        return;
    }

    if (! (stream->tcp_status.flags & TCP_FLAG_SYN)) {
        return; /* 还未收到syn,所以无法计算first payload */
    }

    if (pkt->l4_payload_len == 0) {
        return;
    }

    if (ntohl(pkt->tcp_header->ulSequence) != stream->tcp_status.first_payload_seq) {
        return;
    }

    stream->tcp_status.first_payload_recved = 1;

    PWatcherEvent_Notify(PWATCHER_EV_TCP_FIRST_PAYLOAD, pkt, NULL);

    return;
}

static int pwatcher_ob_first_payload_input(UINT point, PWATCHER_PKT_DESC_S *pkt, void *data)
{
    switch (point) {
        case PWATCHER_EV_TCP:
            pwatcher_ob_first_payload_process_pkt(pkt);
            break;
    }

    return 0;
}

static int pwatcher_ob_first_payload_init()
{
    PWatcherEvent_Reg(g_pwatcher_ob_points);

    return 0;
}

static void pwatcher_ob_first_payload_Finit()
{
    PWatcherEvent_UnReg(g_pwatcher_ob_points);
    Sleep(1000);
}

PLUG_API BOOL_T DllMain(PLUG_HDL hPlug, int reason, void *reserved)
{
    switch(reason) {
        case DLL_PROCESS_ATTACH:
            pwatcher_ob_first_payload_init();
            break;

        case DLL_PROCESS_DETACH:
            pwatcher_ob_first_payload_Finit();
            break;
    }

    return TRUE;
}

PLUG_ENTRY 

PWATCHER_OB_FUNCTIONS 

