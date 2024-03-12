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
#include "utl/mmap_utl.h"

void * __bpfmap_lookup_elem(void *map, const void *key)
{
    return UMAP_LookupElem(map, key);
}

long __bpfmap_update_elem(void *map, const void *key, const void *value, U64 flags)
{
    return UMAP_UpdateElem(map, key, value, flags);
}

long __bpfmap_delete_elem(void *map, const void *key)
{
    return UMAP_DeleteElem(map, key);
}

long __bpfprobe_read(void *dst, U32 size, const void *unsafe_ptr)
{
    if ((! dst) || (! unsafe_ptr)) {
        return -1;
    }

    memcpy(dst, unsafe_ptr, size);

    return 0;
}

U64 __bpfktime_get_ns(void)
{
    return TM_NsFromInit();
}


#if ((defined IN_MAC) && (defined __ARM__))
long __bpftrace_printk(const char *fmt, U32 fmt_size, void *p1, void *p2, void *p3)
{
    printf(fmt, p1, p2, p3);
    return 0;
}
#else
long __bpftrace_printk(const char *fmt, U32 fmt_size, ...)
{
    va_list args;
	int len;

	va_start(args, fmt_size);
    len = vprintf(fmt, args);
	va_end(args);

	return len;
}
#endif

U32 __bpfget_prandom_u32(void)
{
    return RAND_Get();
}

U32 __bpfget_smp_processor_id(void)
{
#ifdef IN_LINUX
    return sched_getcpu();
#else
    return 0;
#endif
}

long __bpfskb_store_bytes(void *skb, U32 offset, const void *from, U32 len, U64 flags)
{
    BS_WARNNING(("TODO"));
    return -1;
}

long __bpfl3_csum_replace(void *skb, U32 offset, U64 from, U64 to, U64 size)
{
    BS_WARNNING(("TODO"));
    return -1;
}

long __bpfl4_csum_replace(void *skb, U32 offset, U64 from, U64 to, U64 flags)
{
    BS_WARNNING(("TODO"));
    return -1;
}

long __bpftail_call(void *ctx, void *prog_array_map, U32 index)
{
    BS_WARNNING(("TODO"));
    return -1;
}

long __bpfclone_redirect(void *skb, U32 ifindex, U64 flags)
{
    BS_WARNNING(("TODO"));
    return -1;
}

U64 __bpfget_current_pid_tgid(void)
{
    U64 tgid = PROCESS_GetPid();
    U64 tid = PROCESS_GetTid();
    return (tgid << 32) | tid;
}

U64 __bpfget_current_uid_gid(void)
{
    U64 gid = getgid();
    U64 uid = getuid();
    return (gid << 32) | uid;
}

long __bpfget_current_comm(void *buf, U32 size_of_buf)
{
    char *self_name = SYSINFO_GetSlefName();
    if (! self_name) {
        return -1;
    }

    strlcpy(buf, self_name, size_of_buf);

    return 0;
}

U32 __bpfget_cgroup_classid(void *skb)
{
    BS_WARNNING(("TODO"));
    return -1;
}

long __bpfskb_vlan_push(void *skb, U16 vlan_proto, U16 vlan_tci)
{
    BS_WARNNING(("TODO"));
    return -1;
}

long __bpfskb_vlan_pop(void *skb)
{
    BS_WARNNING(("TODO"));
    return -1;
}

long __bpfskb_get_tunnel_key(void *skb, void *key, U32 size, U64 flags)
{
    BS_WARNNING(("TODO"));
    return -1;
}

long __bpfskb_set_tunnel_key(void *skb, void *key, U32 size, U64 flags)
{
    BS_WARNNING(("TODO"));
    return -1;
}

U64 __bpfperf_event_read(void *map, U64 flags)
{
    BS_WARNNING(("TODO"));
    return -1;
}

long __bpfstrtol(const char *buf, size_t buf_len, U64 flags, long *res)
{
    char *end;
    *res = strtol(buf, &end, flags);
    return end - buf;
}

long __bpfstrtoul(const char *buf, size_t buf_len, U64 flags, unsigned long *res)
{
    char *end;
    *res = strtoul(buf, &end, flags);
    return end - buf;
}

long __bpf_snprintf(char *str, U32 str_size, const char *fmt, U64 *d, U32 d_len)
{
    switch (d_len) {
        case 0: return snprintf(str,str_size,"%s",fmt);
        case 8: return snprintf(str,str_size,fmt,d[0]);
        case 16: return snprintf(str,str_size,fmt,d[0],d[1]);
        case 24: return snprintf(str,str_size,fmt,d[0],d[1],d[2]);
        case 32: return snprintf(str,str_size,fmt,d[0],d[1],d[2],d[3]);
        case 40: return snprintf(str,str_size,fmt,d[0],d[1],d[2],d[3],d[4]);
        case 48: return snprintf(str,str_size,fmt,d[0],d[1],d[2],d[3],d[4],d[5]);
        case 56: return snprintf(str,str_size,fmt,d[0],d[1],d[2],d[3],d[4],d[5],d[6]);
        case 64: return snprintf(str,str_size,fmt,d[0],d[1],d[2],d[3],d[4],d[5],d[6],d[7]);
        case 72: return snprintf(str,str_size,fmt,d[0],d[1],d[2],d[3],d[4],d[5],d[6],d[7],d[8]);
        case 80: return snprintf(str,str_size,fmt,d[0],d[1],d[2],d[3],d[4],d[5],d[6],d[7],d[8],d[9]);
        default: return -1;
    }
}

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

int ulc_sys_strcmp(void *a, void *b)
{
    return strcmp(a, b);
}

int ulc_sys_strncmp(void *a, void *b, int len)
{
    return strncmp(a, b, len);
}

int ulc_sys_strlen(void *a)
{
    return strlen(a);
}

int ulc_sys_strlcpy(void *dst, void *src, int size)
{
    return strlcpy(dst, src, size);
}

int ulc_sys_strnlen(void *a, int max_len)
{
    return strnlen(a, max_len);
}

void ulc_sys_memcpy(void *d, void *s, int len)
{
    memcpy(d, s, len);
}

void ulc_sys_memset(void *d, int c, int len)
{
    memset(d, c, len);
}

static void ulc_err_code_set(int err_code, char *info, const char *file_name, const char *func_name, int line)
{
    ErrCode_Set(err_code, info, file_name, func_name, line);
}


#if ((defined IN_MAC) && (defined __ARM__))
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

static void * ulc_mmap_map(void *buf, int buf_size, int head_size)
{
    return MMAP_Map(buf, buf_size, head_size);
}

static void ulc_mmap_unmap(void *buf, int size)
{
    MMAP_Unmap(buf, size);
}

static int ulc_mmap_make_exe(void *buf, int size)
{
    return MMAP_MakeExe(buf, size);
}

static void * g_bpf_helper_trusteeship[16];

static int ulc_set_trusteeship(unsigned int id, void *ptr)
{
    if (id >= ARRAY_SIZE(g_bpf_helper_trusteeship)) {
        return -1;
    }
    g_bpf_helper_trusteeship[id] = ptr;
    return 0;
}

static void * ulc_get_trusteeship(unsigned int id)
{
    if (id >= ARRAY_SIZE(g_bpf_helper_trusteeship)) {
        return NULL;
    }
    return g_bpf_helper_trusteeship[id];
}


static const void * g_bpf_base_helpers[BPF_BASE_HELPER_END] = {
    [0] = NULL,
    [1] = UMAP_LookupElem,
    [2] = UMAP_UpdateElem,
    [3] = UMAP_DeleteElem,
    [4] = __bpfprobe_read,
    [5] = __bpfktime_get_ns,
    [6] = __bpftrace_printk,
    [7] = __bpfget_prandom_u32,
    [8] = __bpfget_smp_processor_id,
    [9] = __bpfskb_store_bytes,
    [10] = __bpfl3_csum_replace,
    [11] = __bpfl4_csum_replace,
    [12] = __bpftail_call,
    [13] = __bpfclone_redirect,
    [14] = __bpfget_current_pid_tgid,
    [15] = __bpfget_current_uid_gid,
    [16] = __bpfget_current_comm,
    [17] = __bpfget_cgroup_classid,
    [18] = __bpfskb_vlan_push,
    [19] = __bpfskb_vlan_pop,
    [20] = __bpfskb_get_tunnel_key,
    [21] = __bpfskb_set_tunnel_key,
    [22] = __bpfperf_event_read,
    [105] = __bpfstrtol,
    [106] = __bpfstrtoul,
    [165] = __bpf_snprintf,
};


static const void * g_bpf_sys_helpers[BPF_SYS_HELPER_COUNT] = {
    [0] = ulc_sys_malloc, 
    [1] = ulc_sys_calloc,
    [2] = ulc_sys_free,
    [3] = ulc_sys_memcpy,
    [4] = ulc_sys_memset,
    [5] = ulc_err_code_set,
    [6] = ulc_err_info_set,
    [7] = ulc_call_back,
    [8] = ulc_sys_strncmp,
    [9] = ulc_sys_strlen,
    [10] = ulc_sys_strnlen,
    [11] = ulc_sys_strcmp,
    [12] = ulc_sys_strlcpy,
    [13] = ulc_mmap_map,
    [14] = ulc_mmap_unmap,
    [15] = ulc_mmap_make_exe,
    [16] = ulc_set_trusteeship,
    [17] = ulc_get_trusteeship,
};


static const void * g_bpf_user_helpers[BPF_USER_HELPER_COUNT];

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

int BpfHelper_RegFunc(U32 id, void *func)
{
    if (id < BPF_BASE_HELPER_END) {
        g_bpf_base_helpers[id] = func;
    } else if ((id >= BPF_SYS_HELPER_START) && (id < BPF_SYS_HELPER_END)) {
        g_bpf_sys_helpers[id - BPF_SYS_HELPER_START] = func;
    } else if ((BPF_USER_HELPER_START <= id) && (id < BPF_USER_HELPER_END)) {
        g_bpf_user_helpers[id - BPF_USER_HELPER_START] = func;
    } else {
        RETURN(BS_BAD_PARA);
    }

    return 0;
}

