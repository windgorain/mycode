/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "klcko_impl.h"
#include "ko/utl/klist_utl.h"
#include "linux/gfp.h"

static u64 _klcko_snprintf(char *buf, int size, char *fmt, IN KLC_PARAM_S *p)
{
    if (unlikely(p == NULL)) {
        return snprintf(buf, size, fmt);
    }

    return snprintf(buf, size, fmt, KLC_PARAM_LISTS(p));
}

static u64 _klcko_printk_string(char *string, u64 p1, u64 p2, u64 p3)
{
    printk_ratelimited(string, p1, p2, p3);
    return 0;
}

static u64 _klcko_load(void *mem, unsigned int count)
{
    u64 ret = 0;
    unsigned char *dst = (void*)&ret;
    unsigned char *src = mem;
    int i;

    if (count > 8) {
        return KLC_RET_ERR;
    }

    for (i=0; i<count; i++) {
        dst[i] = src[i];
    }

    return ret;
}

static u64 _klcko_do_base(u64 cmd, u64 p2, u64 p3, u64 p4, u64 p5)
{
    switch (cmd) {
        case KLCHELP_KO_VERSION:
            return KLC_KO_VERSION;
        case KLCHELP_KERNEL_VERSION:
            return LINUX_VERSION_CODE;
        case KLCHELP_DO_NOTHING:
            return 0;
        case KLCHELP_IS_ENABLE:
            return 1;
        case KLCHELP_SELF:
            return p2;
        case KLCHELP_LOAD:
            return _klcko_load((void*)p2, p3);
        case KLCHELP_KPRINT_STRING:
            return _klcko_printk_string((void*)p2, p3, p4, p5);
        case KLCHELP_SNPRINTF:
            return _klcko_snprintf((void*)p2, p3, (void*)p4, (void*)p5);
        case KLCHELP_CURRENT_MM:
            return (long)current->mm;

        default:
            return KLC_RET_ERR;
    }
}

static u64 _klcko_do_run(u64 cmd, u64 p2, u64 p3, u64 p4, u64 p5)
{
    static DEFINE_PER_CPU(int, call_depth);
    u64 ret;
    u64 new_cmd;

    
    if (unlikely(this_cpu_read(call_depth) >= 32)) {
        return KLC_RET_ERR;
    }

    new_cmd = KLCHELP_SET_CLASS(KLCHELP_CLASS_RUN_EXT, cmd);

    this_cpu_inc(call_depth);
    ret = klc_helper_agent(new_cmd, p2, p3, p4, p5);
    this_cpu_dec(call_depth);

    return ret;
}

static u64 _klcko_do_skb(u64 cmd, u64 p2, u64 p3, u64 p4, u64 p5)
{
    switch(cmd) {
        case KLCHELP_SKB_CONTINUE:
            return pskb_may_pull((void*)p2, p3);
        case KLCHELP_SKB_PUT:
    	    return (unsigned long)skb_put((void*)p2, p3);   
        case KLCHELP_SKB_RESERVE:
            skb_reserve((void*)p2, p3);
			return 0;
        case KLCHELP_SKB_RESET_NETWORK_HEADER:
            skb_reset_network_header((void*)p2);
			return 0;
        case KLCHELP_SKB_RESET_TRANSPORT_HEADER: 
            skb_reset_transport_header((void*)p2);
            return 0;
        case KLCHELP_SKB_SET_TRANSPORT_HEADER:
            skb_set_transport_header((void*)p2, p3);
            return 0; 
        default:
            return KLC_RET_ERR;
    }
}

int KlcKo_Init(void)
{
    KlcKo_SetBaseFunc(KLCHELP_CLASS_BASE, _klcko_do_base);
    KlcKo_SetBaseFunc(KLCHELP_CLASS_RUN, _klcko_do_run);
    KlcKo_SetBaseFunc(KLCHELP_CLASS_SKB, _klcko_do_skb);
    KlcKo_BaseEnable(1);
    return 0;
}

void KlcKo_Fini1(void)
{
    KlcKo_BaseEnable(0);
}

void KlcKo_Fini2(void)
{
    int i;

    for (i=0; i<KLCKO_BASE_FUNC_NUM; i++) {
        KlcKo_SetBaseFunc(i, NULL);
    }
}

