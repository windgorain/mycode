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
MAPP_ATTR_DEF(MAPP_CLASS_CLUSTER, MAPP_TYPE_TC, MAPP_TYPE_TC, "tc_ip_cluster", "");

SEC("classifier/mapp_pkt_input")
int mapp_pkt_input(struct __sk_buff *skb)
{
    KLCHLP_License();

    void *data = (void *)(long)skb->data;
    void *data_end = (void *)(long)skb->data_end;

    int ret = mapp_ip_cluster_input(skb, data, data_end);
    if (ret < 0) {
        return TC_ACT_OK;
    }

    return ret;
}

static char _license[] SEC("license") = "GPL";

