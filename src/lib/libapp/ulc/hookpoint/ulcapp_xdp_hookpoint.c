/*********************************************************
*   Copyright (C) LiXingang
*   Description: 
*
********************************************************/
#include "bs.h"
#include "utl/mybpf_prog.h"
#include "utl/mybpf_hookpoint.h"
#include "comp/comp_event_hub.h"
#include "../h/ulcapp_hookpoint.h"

static int ulcapp_hookpoint_precver_pkt_input(void *ob, void *data)
{
    PRECVER_PKT_S *pkt = data;
    MYBPF_XDP_BUFF_S xdp_buf = {0};

    xdp_buf.data = pkt->data;
    xdp_buf.data_end = pkt->data + pkt->data_len;

    MYBPF_XdpInput(&xdp_buf);

    return 0;
}

int ULCAPP_HookPointXdp_Init()
{
    EHUB_Reg(EHUB_EV_PRECVER_PKT, "ulc", ulcapp_hookpoint_precver_pkt_input, NULL);
    return 0;
}
