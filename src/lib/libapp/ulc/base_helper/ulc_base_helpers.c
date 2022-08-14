/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/rand_utl.h"
#include "utl/time_utl.h"
#include "utl/process_utl.h"
#include "../h/ulc_def.h"
#include "../h/ulc_map.h"
#include "../h/ulc_prog.h"
#include "../h/ulc_fd.h"
#include "../h/ulc_hookpoint.h"
#include "../h/ulc_osbase.h"
#include "../h/ulc_base_helpers.h"

typedef u64 (*PF_ULC_BPF_CALL_FUNC)(u64 r1, u64 r2, u64 r3, u64 r4, u64 r5);

typedef struct {
    void *func;
}ULC_BASE_HELP_S;

u64 ulc_bpf_call_base(u64 r1, u64 r2, u64 r3, u64 r4, u64 r5)
{
	return 0;
}

static long _ulc_bpf_probe_read(void *dst, u32 size, const void *unsafe_ptr)
{
    if ((! dst) || (! unsafe_ptr)) {
        return -1;
    }

    memcpy(dst, unsafe_ptr, size);

    return 0;
}

static u64 _ulc_bpf_ktime_get_ns(void)
{
    return TM_NsFromInit();
}

static long _ulc_bpf_trace_printk(char *fmt, u32 fmt_size, ...)
{
    va_list args;
	int len;
    char buf[1024];

	va_start(args, fmt_size);
    len = vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

    if (len > 0) {
        printf("%s", buf);
    }

	return len;
}

static u32 _ulc_bpf_get_prandom_u32(void)
{
    return RAND_Get();
}

static u32 _ulc_bpf_get_smp_processor_id(void)
{
#ifdef IN_LINUX
    return sched_getcpu();
#else
    return 0;
#endif
}

static long _ulc_bpf_skb_store_bytes(void *skb, u32 offset,
        void *from, u32 len, u64 flags)
{
    BS_WARNNING(("TODO"));
    return -1;
}

static long _ulc_bpf_l3_csum_replace(void *skb, u32 offset,
        u64 from, u64 to, u64 size)
{
    BS_WARNNING(("TODO"));
    return -1;
}

static long _ulc_bpf_l4_csum_replace(void *skb, u32 offset,
        u64 from, u64 to, u64 flags)
{
    BS_WARNNING(("TODO"));
    return -1;
}

static long _ulc_bpf_tail_call(void *ctx, void *prog_array_map, u32 index)
{
    BS_WARNNING(("TODO"));
    return -1;
}

static long _ulc_bpf_clone_redirect(void *skb, u32 ifindex, u64 flags)
{
    BS_WARNNING(("TODO"));
    return -1;
}

static u64 _ulc_bpf_get_current_pid_tgid(void)
{
    UINT64 tgid = PROCESS_GetPid();
    UINT64 tid = PROCESS_GetTid();
    return (tgid << 32) | tid;
}

static u64 _ulc_bpf_get_current_uid_gid(void)
{
    UINT64 gid = getgid();
    UINT64 uid = getuid();
    return (gid << 32) | uid;
}

static long _ulc_bpf_get_current_comm(OUT void *buf, u32 size_of_buf)
{
    char *self_name = SYSINFO_GetSlefName();
    if (! self_name) {
        return -1;
    }

    strlcpy(buf, self_name, size_of_buf);

    return 0;
}

static u32 _ulc_bpf_get_cgroup_classid(void *skb)
{
    BS_WARNNING(("TODO"));
    return -1;
}

static long _ulc_bpf_skb_vlan_push(void *skb, u16 vlan_proto, u16 vlan_tci)
{
    BS_WARNNING(("TODO"));
    return -1;
}

static long _ulc_bpf_skb_vlan_pop(void *skb)
{
    BS_WARNNING(("TODO"));
    return -1;
}

static ULC_BASE_HELP_S g_ulc_base_helpers[] = {
    {.func = NULL},
    {.func = ULC_MAP_LookupElem},
    {.func = ULC_MAP_UpdataElem},
    {.func = ULC_MAP_DeleteElem},
    {.func = _ulc_bpf_probe_read},
    {.func = _ulc_bpf_ktime_get_ns},
    {.func = _ulc_bpf_trace_printk},
    {.func = _ulc_bpf_get_prandom_u32},
    {.func = _ulc_bpf_get_smp_processor_id},
    {.func = _ulc_bpf_skb_store_bytes},
    {.func = _ulc_bpf_l3_csum_replace},
    {.func = _ulc_bpf_l4_csum_replace},
    {.func = _ulc_bpf_tail_call},
    {.func = _ulc_bpf_clone_redirect},
    {.func = _ulc_bpf_get_current_pid_tgid},
    {.func = _ulc_bpf_get_current_uid_gid},
    {.func = _ulc_bpf_get_current_comm},
    {.func = _ulc_bpf_get_cgroup_classid},
    {.func = _ulc_bpf_skb_vlan_push},
    {.func = _ulc_bpf_skb_vlan_pop},
};

static PF_ULC_BPF_CALL_FUNC _ulc_base_help_get_func(UINT imm)
{
    int count = ARRAY_SIZE(g_ulc_base_helpers);

    if (imm >= count) {
        return NULL;
    }

    return g_ulc_base_helpers[imm].func;
}

UINT ULC_BaseHelp_GetFunc(UINT imm)
{
    PF_ULC_BPF_CALL_FUNC func;

    func = _ulc_base_help_get_func(imm);
    if (! func) {
        BS_WARNNING(("Not support bpf helper %u yet", imm));
        return 0;
    }

    return func - ulc_bpf_call_base;
}

