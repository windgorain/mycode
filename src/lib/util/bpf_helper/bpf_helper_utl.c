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

void * _bpf_map_lookup_elem(void *map, const void *key)
{
    return UMAP_LookupElem(map, key);
}

long _bpf_map_update_elem(void *map, const void *key, const void *value, U64 flags)
{
    return UMAP_UpdateElem(map, key, value, flags);
}

long _bpf_map_delete_elem(void *map, const void *key)
{
    return UMAP_DeleteElem(map, key);
}

long _bpf_probe_read(void *dst, U32 size, const void *unsafe_ptr)
{
    if ((! dst) || (! unsafe_ptr)) {
        return -1;
    }

    memcpy(dst, unsafe_ptr, size);

    return 0;
}

U64 _bpf_ktime_get_ns(void)
{
    return TM_NsFromInit();
}


#if ((defined IN_MAC) || (defined __ARM__))
long _bpf_trace_printk(const char *fmt, U32 fmt_size, void *p1, void *p2, void *p3)
{
    printf(fmt, p1, p2, p3);
    return 0;
}
#else
long _bpf_trace_printk(const char *fmt, U32 fmt_size, ...)
{
    va_list args;
	int len;

	va_start(args, fmt_size);
    len = vprintf(fmt, args);
	va_end(args);

	return len;
}
#endif

U32 _bpf_get_prandom_u32(void)
{
    return RAND_Get();
}

U32 _bpf_get_smp_processor_id(void)
{
#ifdef IN_LINUX
    return sched_getcpu();
#else
    return 0;
#endif
}

long _bpf_skb_store_bytes(void *skb, U32 offset, const void *from, U32 len, U64 flags)
{
    BS_WARNNING(("TODO"));
    return -1;
}

long _bpf_l3_csum_replace(void *skb, U32 offset, U64 from, U64 to, U64 size)
{
    BS_WARNNING(("TODO"));
    return -1;
}

long _bpf_l4_csum_replace(void *skb, U32 offset, U64 from, U64 to, U64 flags)
{
    BS_WARNNING(("TODO"));
    return -1;
}

long _bpf_tail_call(void *ctx, void *prog_array_map, U32 index)
{
    BS_WARNNING(("TODO"));
    return -1;
}

long _bpf_clone_redirect(void *skb, U32 ifindex, U64 flags)
{
    BS_WARNNING(("TODO"));
    return -1;
}

U64 _bpf_get_current_pid_tgid(void)
{
    UINT64 tgid = PROCESS_GetPid();
    UINT64 tid = PROCESS_GetTid();
    return (tgid << 32) | tid;
}

U64 _bpf_get_current_uid_gid(void)
{
    UINT64 gid = getgid();
    UINT64 uid = getuid();
    return (gid << 32) | uid;
}

long _bpf_get_current_comm(void *buf, U32 size_of_buf)
{
    char *self_name = SYSINFO_GetSlefName();
    if (! self_name) {
        return -1;
    }

    strlcpy(buf, self_name, size_of_buf);

    return 0;
}

U32 _bpf_get_cgroup_classid(void *skb)
{
    BS_WARNNING(("TODO"));
    return -1;
}

long _bpf_skb_vlan_push(void *skb, U16 vlan_proto, U16 vlan_tci)
{
    BS_WARNNING(("TODO"));
    return -1;
}

long _bpf_skb_vlan_pop(void *skb)
{
    BS_WARNNING(("TODO"));
    return -1;
}

long _bpf_skb_get_tunnel_key(void *skb, void *key, U32 size, U64 flags)
{
    BS_WARNNING(("TODO"));
    return -1;
}

long _bpf_skb_set_tunnel_key(void *skb, void *key, U32 size, U64 flags)
{
    BS_WARNNING(("TODO"));
    return -1;
}

U64 _bpf_perf_event_read(void *map, U64 flags)
{
    BS_WARNNING(("TODO"));
    return -1;
}

long _bpf_strtol(const char *buf, size_t buf_len, U64 flags, long *res)
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
    [4] = _bpf_probe_read,
    [5] = _bpf_ktime_get_ns,
    [6] = _bpf_trace_printk,
    [7] = _bpf_get_prandom_u32,
    [8] = _bpf_get_smp_processor_id,
    [9] = _bpf_skb_store_bytes,
    [10] = _bpf_l3_csum_replace,
    [11] = _bpf_l4_csum_replace,
    [12] = _bpf_tail_call,
    [13] = _bpf_clone_redirect,
    [14] = _bpf_get_current_pid_tgid,
    [15] = _bpf_get_current_uid_gid,
    [16] = _bpf_get_current_comm,
    [17] = _bpf_get_cgroup_classid,
    [18] = _bpf_skb_vlan_push,
    [19] = _bpf_skb_vlan_pop,
    [20] = _bpf_skb_get_tunnel_key,
    [21] = _bpf_skb_set_tunnel_key,
    [22] = _bpf_perf_event_read,
    [105] = _bpf_strtol,
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

static int ulc_call_back(UINT_FUNC_4 func, U64 p1, U64 p2, U64 p3, U64 p4)
{
    return func((void*)(long)p1, (void*)(long)p2, (void*)(long)p3, (void*)(long)p4);
}

static const void * g_bpf_sys_helpers[BPF_SYS_HELPER_COUNT] = {
    [0] = ulc_sys_malloc, 
    [1] = ulc_sys_calloc,
    [2] = ulc_sys_free,
    [3] = ulc_sys_strncmp,
    [4] = ulc_sys_memcpy,
    [5] = ulc_sys_memset,
    [6] = ulc_err_code_set,
    [7] = ulc_err_info_set,
    [8] = ulc_call_back,
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


void * BpfHelper_GetFunc(unsigned int id)
{
    if (id < BPF_BASE_HELPER_END) {
        return (void*)g_bpf_base_helpers[id];
    } else if ((id >= BPF_SYS_HELPER_START) && (id < BPF_SYS_HELPER_END)) {
        return (void*)g_bpf_sys_helpers[id - BPF_SYS_HELPER_START];
    } else if ((id >= BPF_USER_HELPER_START) && (id < BPF_USER_HELPER_END)) {
        return (void*)g_bpf_user_helpers[id - BPF_USER_HELPER_START];
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


