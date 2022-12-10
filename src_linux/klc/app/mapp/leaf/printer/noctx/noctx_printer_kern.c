/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      lixingang
* Description: 
* History:     
******************************************************************************/

#include "klc/klc_base.h"
#include "bpf/l4/l4_bpf.h"
#include "app/mapp/mapp_klc.h"

#define KLC_MODULE_NAME "micro_ip_printer"

MAPP_LEAF_MAPS();
MAPP_ATTR_DEF(MAPP_CLASS_LEAF, MAPP_TYPE_NOCTX, MAPP_TYPE_NONE, "printer", "");

static char _license[] SEC("license") = "GPL";

SEC("xdp/mapp_pkt_input")
int mapp_pkt_input(void)
{
    KLCHLP_License();

    MAPP_ATTR_S *attr = MAPP_KLC_GetAttr();

    if (attr) {
        BPF_Print("instance %s input \n", attr->instance_name);
    }

    return 0;
}

