/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "bs.h"
#include "utl/time_utl.h"
#include "utl/rand_utl.h"
#include "utl/process_utl.h"
#include "utl/bpf_helper_utl.h"
#include "utl/umap_utl.h"

static long _bpf_helper_probe_read(void *dst, U32 size, const void *unsafe_ptr)
{
    if ((! dst) || (! unsafe_ptr)) {
        return -1;
    }

    memcpy(dst, unsafe_ptr, size);

    return 0;
}

static U64 _bpf_helper_ktime_get_ns(void)
{
    return TM_NsFromInit();
}

/* macos-arm系统调用约定和arm标准不一致, 需要特殊处理 */
#if ((defined IN_MAC) || (defined __ARM__))
static long _bpf_helper_trace_printk(char *fmt, U32 fmt_size, void *p1, void *p2, void *p3)
{
    printf(fmt, p1, p2, p3);
    return 0;
}
#else
static long _bpf_helper_trace_printk(char *fmt, U32 fmt_size, ...)
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
#endif

static U32 _bpf_helper_get_prandom_u32(void)
{
    return RAND_Get();
}

static U32 _bpf_helper_get_smp_processor_id(void)
{
#ifdef IN_LINUX
    return sched_getcpu();
#else
    return 0;
#endif
}

static long _bpf_helper_skb_store_bytes(void *skb, U32 offset, void *from, U32 len, U64 flags)
{
    BS_WARNNING(("TODO"));
    return -1;
}

static long _bpf_helper_l3_csum_replace(void *skb, U32 offset, U64 from, U64 to, U64 size)
{
    BS_WARNNING(("TODO"));
    return -1;
}

static long _bpf_helper_l4_csum_replace(void *skb, U32 offset, U64 from, U64 to, U64 flags)
{
    BS_WARNNING(("TODO"));
    return -1;
}

static long _bpf_helper_tail_call(void *ctx, void *prog_array_map, U32 index)
{
    BS_WARNNING(("TODO"));
    return -1;
}

static long _bpf_helper_clone_redirect(void *skb, U32 ifindex, U64 flags)
{
    BS_WARNNING(("TODO"));
    return -1;
}

static U64 _bpf_helper_get_current_pid_tgid(void)
{
    UINT64 tgid = PROCESS_GetPid();
    UINT64 tid = PROCESS_GetTid();
    return (tgid << 32) | tid;
}

static U64 _bpf_helper_get_current_uid_gid(void)
{
    UINT64 gid = getgid();
    UINT64 uid = getuid();
    return (gid << 32) | uid;
}

static long _bpf_helper_get_current_comm(OUT void *buf, U32 size_of_buf)
{
    char *self_name = SYSINFO_GetSlefName();
    if (! self_name) {
        return -1;
    }

    strlcpy(buf, self_name, size_of_buf);

    return 0;
}

static U32 _bpf_helper_get_cgroup_classid(void *skb)
{
    BS_WARNNING(("TODO"));
    return -1;
}

static long _bpf_helper_skb_vlan_push(void *skb, U16 vlan_proto, U16 vlan_tci)
{
    BS_WARNNING(("TODO"));
    return -1;
}

static long _bpf_helper_skb_vlan_pop(void *skb)
{
    BS_WARNNING(("TODO"));
    return -1;
}


void * g_bpf_user_helpers[BPF_USER_HELPER_COUNT];

const void * g_bpf_base_helpers[BPF_BASE_HELPER_MAX] = {
    [0] = NULL,
    [1] = UMAP_LookupElem,
    [2] = UMAP_UpdateElem,
    [3] = UMAP_DeleteElem,
    [4] = _bpf_helper_probe_read,
    [5] = _bpf_helper_ktime_get_ns,
    [6] = _bpf_helper_trace_printk,
    [7] = _bpf_helper_get_prandom_u32,
    [8] = _bpf_helper_get_smp_processor_id,
    [9] = _bpf_helper_skb_store_bytes,
    [10] = _bpf_helper_l3_csum_replace,
    [11] = _bpf_helper_l4_csum_replace,
    [12] = _bpf_helper_tail_call,
    [13] = _bpf_helper_clone_redirect,
    [14] = _bpf_helper_get_current_pid_tgid,
    [15] = _bpf_helper_get_current_uid_gid,
    [16] = _bpf_helper_get_current_comm,
    [17] = _bpf_helper_get_cgroup_classid,
    [18] = _bpf_helper_skb_vlan_push,
    [19] = _bpf_helper_skb_vlan_pop,
};

/* 返回bpf helper的table指针 */
void * BpfHelper_BaseHelper(void)
{
    return g_bpf_base_helpers;
}

/* 返回base helper table的size, 表示最多可以容纳多少个元素 */
UINT BpfHelper_BaseSize(void)
{
    return BPF_BASE_HELPER_MAX;
}

/* 返回user helper table的size, 表示最多可以容纳多少个元素 */
UINT BpfHelper_UserSize(void)
{
    return BPF_USER_HELPER_COUNT;
}

/* 根据id获取helper函数指针 */
PF_BPF_HELPER_FUNC BpfHelper_GetFunc(UINT id)
{
    if (id < BPF_BASE_HELPER_MAX) {
        return g_bpf_base_helpers[id];
    }

    if ((id >= BPF_USER_HELPER_MIN) && (id < BPF_USER_HELPER_MAX)) {
        return g_bpf_user_helpers[id - BPF_USER_HELPER_MIN];
    }

    return NULL;
}

int BpfHelper_SetUserFunc(UINT id, void *func)
{
    /* 低空间留给base help, 不可以被set */
    if (id < BPF_USER_HELPER_MIN) {
        RETURN(BS_BAD_PARA);
    }

    if (id >= BPF_USER_HELPER_MAX) { 
        RETURN(BS_BAD_PARA);
    }

    g_bpf_user_helpers[id - BPF_USER_HELPER_MIN] = func;

    return 0;
}


