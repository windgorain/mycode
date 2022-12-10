/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      lixingang
* Description: 
* History:     
******************************************************************************/
#include "klc/klc_base.h"
#include "helpers/ipacl_klc.h"
#include "bpf/l4/l4_bpf.h"
#include "utl/jhash_utl.h"
#include "app/mapp/mapp_klc.h"
#include "../mapp_ip_cluster_klc.h"

MAPP_CLUSTER_MAPS();
MAPP_ATTR_DEF(MAPP_CLASS_CLUSTER, MAPP_TYPE_XDP, MAPP_TYPE_XDP, "xdp_ip_cluster", "");

SEC("xdp/mapp_pkt_input")
int mapp_pkt_input(struct xdp_md *ctx)
{
    KLCHLP_License();

    void *data = (void *)(long)ctx->data;
    void *data_end = (void *)(long)ctx->data_end;

    int ret = mapp_ip_cluster_input(ctx, data, data_end);
    if (ret < 0) {
        return XDP_PASS;
    }

    return ret;
}

static char _license[] SEC("license") = "GPL";

