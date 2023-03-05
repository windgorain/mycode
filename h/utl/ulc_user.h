/*********************************************************
*   Copyright (C) LiXingang
*   Description: 给ULC用户使用的头文件
*
********************************************************/
#ifndef _ULC_USER_H
#define _ULC_USER_H
#ifdef __cplusplus
extern "C"
{
#endif

#ifndef noinline
#define noinline __attribute__((noinline))
#endif

#define SEC(NAME) __attribute__((section(NAME), used))

#define BPF_Print(fmt, ...) ({ \
    char msg[] = fmt; \
    bpf_trace_printk(msg, sizeof(msg), ##__VA_ARGS__); \
})


#ifndef NULL
#define NULL 0
#endif

#ifndef printf
#define printf BPF_Print
#endif

#ifndef memset
#define memset __builtin_memset
#endif

#ifndef memcpy
#define memcpy __builtin_memcpy
#endif

enum bpf_map_type {
	BPF_MAP_TYPE_UNSPEC,
	BPF_MAP_TYPE_HASH,
	BPF_MAP_TYPE_ARRAY,
};

enum xdp_action {
	XDP_ABORTED = 0,
	XDP_DROP,
	XDP_PASS,
	XDP_TX,
	XDP_REDIRECT,
};

struct bpf_map_def {
	unsigned int type;
	unsigned int key_size;
	unsigned int value_size;
	unsigned int max_entries;
	unsigned int map_flags;
};

struct xdp_md {
	unsigned int data;
	unsigned int data_end;
	unsigned int data_meta;
	/* Below access go through struct xdp_rxq_info */
	unsigned int ingress_ifindex; /* rxq->dev->ifindex */
	unsigned int rx_queue_index;  /* rxq->queue_index  */
	unsigned int egress_ifindex;  /* txq->dev->ifindex */
};

static void *(*bpf_map_lookup_elem)(void *map, const void *key) = (void *) 1;
static long (*bpf_map_update_elem)(void *map, const void *key, const void *value, unsigned long long flags) = (void *) 2;
static long (*bpf_map_delete_elem)(void *map, const void *key) = (void *) 3;
static long (*bpf_probe_read)(void *dst, unsigned int size, const void *unsafe_ptr) = (void *) 4;
static unsigned long long (*bpf_ktime_get_ns)(void) = (void *) 5;
static long (*bpf_trace_printk)(const char *fmt, unsigned int fmt_size, ...) = (void *) 6;
static unsigned int (*bpf_get_prandom_u32)(void) = (void *) 7;
static unsigned int  (*bpf_get_smp_processor_id)(void) = (void *) 8;

static unsigned long long (*bpf_get_current_pid_tgid)(void) = (void *) 14;
static unsigned long long (*bpf_get_current_uid_gid)(void) = (void *) 15;
static long (*bpf_get_current_comm)(void *buf, unsigned int size_of_buf) = (void *) 16;
static long (*bpf_strtol)(const char *buf, int buf_len, unsigned long long flags, long *res) = (void *) 105;

#ifdef __cplusplus
}
#endif
#endif //ULC_USER_H_
