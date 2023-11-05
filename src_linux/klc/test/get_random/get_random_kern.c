/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      lixingang
* Description: 
* History:     
******************************************************************************/
#include "klc/klc_base.h"

char _license[] SEC("license") = "GPL";
u32 _version SEC("version") = LINUX_VERSION_CODE;

SEC("kprobe/ip_output")
int kprobe__ip_output(struct pt_regs *ctx)
{
    KLCHLP_License();

    unsigned int rnd = KLCHLP_NameRun("test_get_random.get_random", 0, 0, 0);

    BPF_Print("random: %u", rnd);

    return 0;
}

