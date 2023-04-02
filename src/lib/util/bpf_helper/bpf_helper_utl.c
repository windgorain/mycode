/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2017-10-3
* Description: 
*
******************************************************************************/
#include "bs.h"
#include "utl/time_utl.h"
#include "utl/rand_utl.h"
#include "utl/process_utl.h"
#include "utl/bpf_helper_utl.h"
#include "utl/umap_utl.h"

static void * g_bpf_user_helpers[BPF_USER_HELPER_COUNT];

void * bpf_map_lookup_elem(void *map, const void *key)
{
    return UMAP_LookupElem(map, key);
}

long bpf_map_update_elem(void *map, const void *key, const void *value, u64 flags)
{
    return UMAP_UpdateElem(map, key, value, flags);
}

long bpf_map_delete_elem(void *map, const void *key)
{
    return UMAP_DeleteElem(map, key);
}

long bpf_probe_read(void *dst, U32 size, const void *unsafe_ptr)
{
    if ((! dst) || (! unsafe_ptr)) {
        return -1;
    }

    memcpy(dst, unsafe_ptr, size);

    return 0;
}

u64 bpf_ktime_get_ns(void)
{
    return TM_NsFromInit();
}

/* macos-arm系统调用约定和arm标准不一致, 需要特殊处理 */
#if ((defined IN_MAC) || (defined __ARM__))
long bpf_trace_printk(const char *fmt, u32 fmt_size, void *p1, void *p2, void *p3)
{
    printf(fmt, p1, p2, p3);
    return 0;
}
#else
long bpf_trace_printk(const char *fmt, u32 fmt_size, ...)
{
    va_list args;
	int len;

	va_start(args, fmt_size);
    len = vprintf(fmt, args);
	va_end(args);

	return len;
}
#endif

u32 bpf_get_prandom_u32(void)
{
    return RAND_Get();
}

u32 bpf_get_smp_processor_id(void)
{
#ifdef IN_LINUX
    return sched_getcpu();
#else
    return 0;
#endif
}

long bpf_skb_store_bytes(void *skb, u32 offset, const void *from, u32 len, u64 flags)
{
    BS_WARNNING(("TODO"));
    return -1;
}

long bpf_l3_csum_replace(void *skb, u32 offset, u64 from, u64 to, u64 size)
{
    BS_WARNNING(("TODO"));
    return -1;
}

long bpf_l4_csum_replace(void *skb, u32 offset, u64 from, u64 to, u64 flags)
{
    BS_WARNNING(("TODO"));
    return -1;
}

long bpf_tail_call(void *ctx, void *prog_array_map, u32 index)
{
    BS_WARNNING(("TODO"));
    return -1;
}

long bpf_clone_redirect(void *skb, u32 ifindex, u64 flags)
{
    BS_WARNNING(("TODO"));
    return -1;
}

u64 bpf_get_current_pid_tgid(void)
{
    UINT64 tgid = PROCESS_GetPid();
    UINT64 tid = PROCESS_GetTid();
    return (tgid << 32) | tid;
}

u64 bpf_get_current_uid_gid(void)
{
    UINT64 gid = getgid();
    UINT64 uid = getuid();
    return (gid << 32) | uid;
}

long bpf_get_current_comm(void *buf, u32 size_of_buf)
{
    char *self_name = SYSINFO_GetSlefName();
    if (! self_name) {
        return -1;
    }

    strlcpy(buf, self_name, size_of_buf);

    return 0;
}

u32 bpf_get_cgroup_classid(void *skb)
{
    BS_WARNNING(("TODO"));
    return -1;
}

long bpf_skb_vlan_push(void *skb, u16 vlan_proto, u16 vlan_tci)
{
    BS_WARNNING(("TODO"));
    return -1;
}

long bpf_skb_vlan_pop(void *skb)
{
    BS_WARNNING(("TODO"));
    return -1;
}

long bpf_skb_get_tunnel_key(void *skb, void *key, u32 size, u64 flags)
{
    BS_WARNNING(("TODO"));
    return -1;
}

long bpf_skb_set_tunnel_key(void *skb, void *key, u32 size, u64 flags)
{
    BS_WARNNING(("TODO"));
    return -1;
}

u64 bpf_perf_event_read(void *map, u64 flags)
{
    BS_WARNNING(("TODO"));
    return -1;
}

long bpf_strtol(const char *buf, size_t buf_len, u64 flags, long *res)
{
    char *end;
    *res = strtol(buf, &end, flags);
    return end - buf;
}

static const void * g_bpf_base_helpers[BPF_BASE_HELPER_END] = {
    [0] = NULL,
    [1] = UMAP_LookupElem,
    [2] = UMAP_UpdateElem,
    [3] = UMAP_DeleteElem,
    [4] = bpf_probe_read,
    [5] = bpf_ktime_get_ns,
    [6] = bpf_trace_printk,
    [7] = bpf_get_prandom_u32,
    [8] = bpf_get_smp_processor_id,
    [9] = bpf_skb_store_bytes,
    [10] = bpf_l3_csum_replace,
    [11] = bpf_l4_csum_replace,
    [12] = bpf_tail_call,
    [13] = bpf_clone_redirect,
    [14] = bpf_get_current_pid_tgid,
    [15] = bpf_get_current_uid_gid,
    [16] = bpf_get_current_comm,
    [17] = bpf_get_cgroup_classid,
    [18] = bpf_skb_vlan_push,
    [19] = bpf_skb_vlan_pop,
    [20] = bpf_skb_get_tunnel_key,
    [21] = bpf_skb_set_tunnel_key,
    [22] = bpf_perf_event_read,
    [105] = bpf_strtol,
};

void * ulc_sys_malloc(int size)
{
    return malloc(size);
}

void * ulc_sys_calloc(int nitems, int size)
{
    return calloc(nitems, size);
}

void ulc_sys_free(void *m)
{
    free(m);
}

int ulc_sys_strncmp(void *a, void *b, int len)
{
    return strncmp(a, b, len);
}

void ulc_sys_memcpy(void *d, void *s, int len)
{
    memcpy(d, s, len);
}

void ulc_sys_memset(void *d, int c, int len)
{
    memset(d, c, len);
}

static void ulc_err_code_set(int err_code, const char *file_name, const char *func_name, int line)
{
    ErrCode_Set(err_code, NULL, file_name, func_name, line);
}

/* macos-arm系统调用约定和arm标准不一致, 需要特殊处理 */
#if ((defined IN_MAC) || (defined __ARM__))
static void ulc_err_info_set(const char *fmt, void *p1, void *p2, void *p3, void *p4)
{
    char buf[256];
    snprintf(buf, sizeof(buf), fmt, p1, p2, p3, p4);
    ErrCode_SetInfo(buf);
    return;
}
#else
static void ulc_err_info_set(const char *fmt, ...)
{
    va_list args;
    char buf[256];

	va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

    ErrCode_SetInfo(buf);

	return;
}
#endif

static const void * g_bpf_sys_helpers[BPF_SYS_HELPER_COUNT] = {
    [0] = ulc_sys_malloc, /* 10000 */
    [1] = ulc_sys_calloc,
    [2] = ulc_sys_free,
    [3] = ulc_sys_strncmp,
    [4] = ulc_sys_memcpy,
    [5] = ulc_sys_memset,
    [6] = ulc_err_code_set,
    [7] = ulc_err_info_set,
};

const void ** BpfHelper_BaseHelper(void)
{
    return g_bpf_base_helpers;
}

const void ** BpfHelper_SysHelper(void)
{
    return g_bpf_sys_helpers;
}

const void ** BpfHelper_UserHelper(void)
{
    return (const void **)g_bpf_user_helpers;
}

PF_BPF_HELPER_FUNC BpfHelper_GetFunc(UINT id)
{
    if ((id > 0) && (id < BPF_BASE_HELPER_END)) {
        return g_bpf_base_helpers[id];
    } else if ((id >= BPF_SYS_HELPER_START) && (id < BPF_SYS_HELPER_END)) {
        return g_bpf_sys_helpers[id - BPF_SYS_HELPER_START];
    } else if ((id >= BPF_USER_HELPER_START) && (id < BPF_USER_HELPER_END)) {
        return g_bpf_user_helpers[id - BPF_USER_HELPER_START];
    }

    return NULL;
}

int BpfHelper_SetUserFunc(UINT id, void *func)
{
    if ((BPF_USER_HELPER_START <= id) && (id < BPF_USER_HELPER_END)) {
        g_bpf_user_helpers[id - BPF_USER_HELPER_START] = func;
    } else {
        RETURN(BS_BAD_PARA);
    }

    return 0;
}


