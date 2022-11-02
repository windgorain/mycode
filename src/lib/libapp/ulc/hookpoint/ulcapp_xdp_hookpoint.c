/*********************************************************
*   Copyright (C) LiXingang
*   Description: 
*
********************************************************/
#include "bs.h"
#include "utl/ulc_utl.h"
#include "comp/comp_event_hub.h"
#include "../h/ulcapp_hookpoint.h"

static int ulcapp_hookpoint_precver_pkt_input(void *ob, void *data)
{
    PRECVER_PKT_S *pkt = data;
    ULC_XDP_BUFF_S xdp_buf = {0};

    xdp_buf.data = pkt->data;
    xdp_buf.data_end = pkt->data + pkt->data_len;

    ULC_HookPoint_XdpInput(&xdp_buf);

    return 0;
}

int ULCAPP_HookPointXdp_Init()
{
    EHUB_Reg(EHUB_EV_PRECVER_PKT, "ulc", ulcapp_hookpoint_precver_pkt_input, NULL);
    return 0;
}
