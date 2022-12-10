/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      lixingang
* Description: 
* History:     
******************************************************************************/
#include "klc/klc_base.h"
#include "helpers/skb_klc.h"

#define KLC_MODULE_NAME "perf_test"
KLC_DEF_MODULE();

char _license[] SEC("license") = "GPL";
u32 _version SEC("version") = LINUX_VERSION_CODE;

static inline u64 performance_process_pkt(unsigned char *data, int len)
{
    int i;
    u64 count = 0;

    for (i=0; i<len; i++) {
        count += data[i];
    }

    return count;
}

/* 定义一个name function, 用于处理事件 */
SEC("klc/namefunc/perf_test")
u64 performance_test(unsigned char *data, int len, u64 p3)
{
    return performance_process_pkt(data, len);
}

SEC("kprobe/ip_output")
int kprobe__ip_output(struct pt_regs *ctx)
{
    KLCHLP_License();

    struct sk_buff *skb;
    u64 count1, count2 = 0;

    skb = (void*)PT_REGS_PARM3(ctx);
    if (! skb) {
        return 0;
    }

    KLC_SKB_INFO_S skbinfo = {0};
    if (SKBKLC_GetSkbInfo(skb, &skbinfo)) {
        return 0;
    }

    KLCHLP_SkbContinue(skb, skbinfo.len);

    if (SKBKLC_GetSkbInfo(skb, &skbinfo)) {
        return 0;
    }

    unsigned char *data = (void*)KLCHLP_Self((long)skbinfo.data);
    int head_len = KLCHLP_Self(skbinfo.head_len);

    if (head_len < 100) {
        return 0;
    }

    unsigned char mydata[100];
    if (bpf_probe_read(mydata, 100, data) != 0) {
        return 0;
    }

    unsigned long long time1 = bpf_ktime_get_ns();
    count1 = performance_process_pkt(mydata, 100);
    unsigned long long time2 = bpf_ktime_get_ns();

    unsigned long long time3 = bpf_ktime_get_ns();
    count2 = KLCHLP_LocalNameRun("perf_test", (long)mydata, 100, 0);
    unsigned long long time4 = bpf_ktime_get_ns();

    BPF_Print("ebpf ret: %llu, klc ret: %llu \n", count1, count2);
    BPF_Print("ebpf time: %llu, klc time: %llu \n", time2 - time1, time4 - time3);

    return 0;
}

