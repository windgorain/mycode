/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      lixingang
* Description: 
* History:     
******************************************************************************/
#include "klc/klc_base.h"
#include "bpf/l4/l4_bpf.h"
#include "app/mapp/mapp_klc.h"
#include "../mapp_ip_printer_klc.h"

MAPP_LEAF_MAPS();
MAPP_ATTR_DEF(MAPP_CLASS_LEAF, MAPP_TYPE_XDP, MAPP_TYPE_NONE, "ip_printer", "");

static char _license[] SEC("license") = "GPL";

SEC("xdp/mapp_pkt_input")
int mapp_pkt_input(struct xdp_md *ctx)
{
    KLCHLP_License();

    void *data_end = (void *)(long)ctx->data_end;
    void *data = (void *)(long)ctx->data;

    mapp_ip_printer_input(data, data_end);

    return XDP_PASS;
}

