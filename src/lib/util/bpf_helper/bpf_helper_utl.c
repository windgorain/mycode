/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2017-10-3
* Description: 
*
******************************************************************************/
#include "bs.h"
#include "utl/arch_utl.h"
#include "utl/time_utl.h"
#include "utl/rand_utl.h"
#include "utl/process_utl.h"
#include "utl/bpf_helper_utl.h"
#include "utl/umap_utl.h"
#include "utl/mmap_utl.h"

static const void ** ulc_get_base_helpers(void);
static const void ** ulc_get_sys_helpers(void);
static const void ** ulc_get_user_helpers(void);

static long __bpfprobe_read(void *dst, U32 size, const void *unsafe_ptr)
{
    if ((! dst) || (! unsafe_ptr)) {
        return -1;
    }

    memcpy(dst, unsafe_ptr, size);

    return 0;
}

static U64 __bpfktime_get_ns(void)
{
    return TM_NsFromInit();
}

static long __bpftrace_printk(const char *fmt, U32 fmt_size, void *p1, void *p2, void *p3)
{
    (void)fmt_size;
    printf(fmt, p1, p2, p3);
    return 0;
}

static U32 __bpfget_prandom_u32(void)
{
    return RAND_Get();
}

static U32 __bpfget_smp_processor_id(void)
{
#ifdef IN_LINUX
    return sched_getcpu();
#else
    return 0;
#endif
}

static U64 __bpfget_current_pid_tgid(void)
{
    U64 tgid = PROCESS_GetPid();
    U64 tid = PROCESS_GetTid();
    return (tgid << 32) | tid;
}

static U64 __bpfget_current_uid_gid(void)
{
    U64 gid = getgid();
    U64 uid = getuid();
    return (gid << 32) | uid;
}

static long __bpfget_current_comm(void *buf, U32 size_of_buf)
{
    char *self_name = SYSINFO_GetSlefName();
    if (! self_name) {
        return -1;
    }

    strlcpy(buf, self_name, size_of_buf);

    return 0;
}

static long __bpfstrtol(const char *buf, size_t buf_len, U64 flags, long *res)
{
    char *end;
    (void)buf_len;
    *res = strtol(buf, &end, flags);
    return end - buf;
}

static long __bpfstrtoul(const char *buf, size_t buf_len, U64 flags, unsigned long *res)
{
    char *end;
    (void)buf_len;
    *res = strtoul(buf, &end, flags);
    return end - buf;
}

static long __bpf_snprintf(char *str, U32 str_size, const char *fmt, U64 *d, U32 d_len)
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

static void * ulc_sys_rcu_malloc(int size)
{
    return malloc(size);
}

static void ulc_sys_rcu_free(void *m)
{
    free(m);
}

static int ulc_sys_strlcpy(void *dst, void *src, int size)
{
    return strlcpy(dst, src, size);
}

static int ulc_sys_strnlen(void *a, int max_len)
{
    const char *end = memchr(a, '\0', max_len);
    return end ? end - (char*)a : max_len;
}

static int ulc_sys_puts(const char *str)
{
    return printf("%s\n", str);
}

static int ulc_sys_fprintf(void *fp, char *fmt, U64 *d, int count)
{
    switch (count) {
        case 0: return fprintf(fp,"%s",fmt);
        case 1: return fprintf(fp,fmt,d[0]);
        case 2: return fprintf(fp,fmt,d[0],d[1]);
        case 3: return fprintf(fp,fmt,d[0],d[1],d[2]);
        case 4: return fprintf(fp,fmt,d[0],d[1],d[2],d[3]);
        case 5: return fprintf(fp,fmt,d[0],d[1],d[2],d[3],d[4]);
        case 6: return fprintf(fp,fmt,d[0],d[1],d[2],d[3],d[4],d[5]);
        case 7: return fprintf(fp,fmt,d[0],d[1],d[2],d[3],d[4],d[5],d[6]);
        case 8: return fprintf(fp,fmt,d[0],d[1],d[2],d[3],d[4],d[5],d[6],d[7]);
        case 9: return fprintf(fp,fmt,d[0],d[1],d[2],d[3],d[4],d[5],d[6],d[7],d[8]);
        case 10: return fprintf(fp,fmt,d[0],d[1],d[2],d[3],d[4],d[5],d[6],d[7],d[8],d[9]);
        default: return -1;
    }
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

static void * ulc_get_helper(unsigned int id, const void **tmp_helpers)
{
    return BpfHelper_GetFunc(id, tmp_helpers);
}

static int ulc_get_local_arch(void)
{
    return ARCH_LocalArch();
}

static void * ulc_mmap_map(void *addr, U64 len, U64 flag, int fd, U64 off)
{
    int prot = flag >> 32;
    int flags = flag;
#ifdef IN_MAC
    if (flags & 0x20) {
        flags &= ~(0x20);
        flags |= MAP_ANONYMOUS;
    }
#endif
    return mmap(addr, len, prot, flags, fd, off);
}

static int ulc_sys_get_errno()
{
    return errno;
}


static const void * g_bpf_base_helpers[BPF_BASE_HELPER_COUNT] = {
    [0] = NULL,
    [1] = UMAP_LookupElem,
    [2] = UMAP_UpdateElem,
    [3] = UMAP_DeleteElem,
    [4] = __bpfprobe_read,
    [5] = __bpfktime_get_ns,
    [6] = __bpftrace_printk,
    [7] = __bpfget_prandom_u32,
    [8] = __bpfget_smp_processor_id,
    [14] = __bpfget_current_pid_tgid,
    [15] = __bpfget_current_uid_gid,
    [16] = __bpfget_current_comm,
    [105] = __bpfstrtol,
    [106] = __bpfstrtoul,
    [165] = __bpf_snprintf,
};


static const void * g_bpf_sys_helpers[BPF_SYS_HELPER_COUNT] = {
    [0] = NULL, 
    [1] = calloc,
    [2] = free,
    [3] = ulc_sys_rcu_malloc,
    [4] = ulc_sys_rcu_free,
    [5] = malloc,
    [8] = strncmp,
    [9] = strlen,
    [10] = ulc_sys_strnlen,
    [11] = strcmp,
    [12] = ulc_sys_strlcpy,
    [13] = strdup,
    [14] = strtok_r,
    [40] = memcpy,
    [41] = memset,
    [42] = memmove,
    [70] = printf,
    [71] = ulc_sys_puts,
    [100] = access,
    [101] = ulc_sys_fprintf,
    [102] = ftell,
    [103] = fseek,
    [104] = fopen,
    [105] = fread,
    [106] = fclose,
    [107] = fgets,
    [130] = time,
    [200] = socket,
    [201] = bind,
    [202] = connect,
    [203] = listen,
    [204] = accept,
    [205] = recv,
    [206] = send,
    [207] = close,
    [208] = setsockopt,
    [300] = pthread_create,
    [400] = ulc_set_trusteeship,
    [401] = ulc_get_trusteeship,
    [402] = ulc_sys_get_errno,
    [500] = ulc_mmap_map,
    [501] = munmap,
    [502] = mprotect,
    [507] = ulc_get_local_arch,
    [508] = ulc_get_helper,
    [509] = ulc_get_base_helpers,
    [510] = ulc_get_sys_helpers,
    [511] = ulc_get_user_helpers,
};


static const void * g_bpf_user_helpers[BPF_USER_HELPER_COUNT] = {
    [0] = NULL, 
};

static const void ** ulc_get_base_helpers(void)
{
    return g_bpf_base_helpers;
}

static const void ** ulc_get_sys_helpers(void)
{
    return g_bpf_sys_helpers;
}

static const void ** ulc_get_user_helpers(void)
{
    return g_bpf_user_helpers;
}

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


void * BpfHelper_GetFunc(unsigned int id, const void **tmp_helpers)
{
    if (id < BPF_BASE_HELPER_END) {
        return (void*)g_bpf_base_helpers[id];
    } else if ((id >= BPF_SYS_HELPER_START) && (id < BPF_SYS_HELPER_END)) {
        return (void*)g_bpf_sys_helpers[id - BPF_SYS_HELPER_START];
    } else if ((id >= BPF_USER_HELPER_START) && (id < BPF_USER_HELPER_END)) {
        return (void*)g_bpf_user_helpers[id - BPF_USER_HELPER_START];
    } else if ((id >= BPF_TMP_HELPER_START) && (id < BPF_TMP_HELPER_END) && (tmp_helpers)) {
        int idx = id - BPF_TMP_HELPER_START;
        if ((idx <= 0) || (idx >= *(U32*)tmp_helpers)) { 
            return NULL;
        }
        return (void*)tmp_helpers[idx];
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

