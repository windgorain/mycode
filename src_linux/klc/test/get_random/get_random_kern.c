/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      lixingang
* Description: 
* History:     
******************************************************************************/
#include "klc/klc_base.h"

#define KLC_MODULE_NAME "test_get_random"
KLC_DEF_MODULE();

char _license[] SEC("license") = "GPL";
u32 _version SEC("version") = LINUX_VERSION_CODE;

/* 定义一个name function, 用于处理事件 */
SEC("klc/namefunc/get_random")
u64 performance_test(unsigned char *data, int len, u64 p3)
{
    return bpf_get_prandom_u32();
}

SEC("kprobe/ip_output")
int kprobe__ip_output(struct pt_regs *ctx)
{
    KLCHLP_License();

    unsigned int rnd = KLCHLP_LocalNameRun("get_random", 0, 0, 0);

    BPF_Print("random: %u \n", rnd);

    return 0;
}

