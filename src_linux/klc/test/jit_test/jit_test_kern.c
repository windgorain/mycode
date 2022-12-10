/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      lixingang
* Description: 
* History:     
******************************************************************************/
#include "klc/klc_base.h"

#define KLC_MODULE_NAME "test_jit"
KLC_DEF_MODULE();

char _license[] SEC("license") = "GPL";
u32 _version SEC("version") = LINUX_VERSION_CODE;

SEC("klc/namefunc/jit_test")
u64 performance_test(u64 p1, u64 p2, u64 p3)
{
    return KLCHLP_KoVersion();
}

SEC("kprobe/ip_output")
int kprobe__ip_output(struct pt_regs *ctx)
{
    KLCHLP_License();

    unsigned int ret = KLCHLP_LocalNameRun("jit_test", 1, 2, 3);

    BPF_Print("ret: %u \n", ret);

    return 0;
}

