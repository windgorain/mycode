/*********************************************************
*   Copyright (C) LiXingang
*   Description: 给ULC用户使用的头文件
*
********************************************************/
#ifndef _ULC_USER_SYS_H
#define _ULC_USER_SYS_H

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef IN_ULC_USER

static void * (*ulc_sys_calloc)(int nitems, int size) = (void *)1000001;
static void (*ulc_sys_free)(void *m) = (void *)1000002;
static void * (*ulc_sys_rcu_malloc)(int size) = (void *)1000003;
static void (*ulc_sys_rcu_free)(void *m) = (void *)1000004;
static void * (*ulc_sys_malloc)(int size) = (void *)1000005;

static int (*ulc_sys_strncmp)(void *a, void *b, int len) = (void*)1000008;
static int (*ulc_sys_strlen)(void *a) = (void*)1000009;
static int (*ulc_sys_strnlen)(void *a, int max_len) = (void*)1000010;
static int (*ulc_sys_strcmp)(void *a, void *b) = (void*)1000011;
static int (*ulc_sys_strlcpy)(void *dst, void *src, int size) = (void*)1000012;
static char * (*ulc_sys_strdup)(void *s) = (void*)1000013;
static char * (*ulc_sys_strtok_r)(char *str, const char *sep, char **ctx) = (void*)1000014;


static void (*ulc_sys_memcpy)(void *d, const void *s, int len) = (void*)1000040;
static void (*ulc_sys_memset)(void *d, int c, int len) = (void*)1000041;
static void * (*ulc_sys_memmove)(void *str1, const void *str2, int n) = (void*)1000042;

static int (*ulc_sys_printf)(char *fmt, ...) = (void*)1000070;
static int (*ulc_sys_puts)(const char *str) =  (void*)1000071;

static int (*ulc_sys_access)(const char *pathname, int mode) = (void*)1000100;
static int (*ulc_sys_fprintf)(void *fp, char *fmt, U64 *d, int count) = (void*)1000101;
static long (*ulc_sys_ftell)(void *fp) = (void*)1000102;
static int (*ulc_sys_fseek)(void *fp, long int offset, int whence) = (void*)1000103;
static void * (*ulc_sys_fopen)(const char *filename, const char *mode) = (void*)1000104;
static long (*ulc_sys_fread)(void *ptr, long size, long nmemb, void *stream) = (void*)1000105;
static int (*ulc_sys_fclose)(void *stream) = (void*)1000106;
static char * (*ulc_sys_fgets)(void *str, int n, void *fp) = (void*)1000107;

static U64 (*ulc_sys_time)(U64 *seconds) = (void*)1000130;

static int (*ulc_sys_socket)(int af, int type, int protocol) = (void*)1000200;
static int (*ulc_sys_bind)(int sock, void *addr, int addrlen) = (void*)1000201;
static int (*ulc_sys_connect)(int sock, void *serv_addr, int addrlen) = (void*)1000202;
static int (*ulc_sys_listen)(int sock, int backlog) = (void*)1000203;
static int (*ulc_sys_accept)(int sock, void *addr, int *addrlen) = (void*)1000204;
static int (*ulc_sys_recv)(int s, char *buf, int len, int flags) = (void*)1000205;
static int (*ulc_sys_send)(int s, char *buf, int len, int flags) = (void*)1000206;
static int (*ulc_sys_close)(int s) = (void*)1000207;
static int (*ulc_sys_setsockopt)(int sockfd, int level, int optname, const void *optval, int optlen) = (void*)1000208;

static int (*ulc_pthread_create)(void *thread, const void *attr, void *fn, void *arg) = (void*)1000300;

static int (*ulc_set_trusteeship)(unsigned int id, void *ptr) = (void*)1000400;
static void * (*ulc_get_trusteeship)(unsigned int id) = (void*)1000401;

static int (*ulc_sys_get_errno)(void) = (void*)1000402;

static void * (*ulc_mmap_map)(void *addr, U64 len, U64 flag, int fd, U64 off) = (void*)1000500;
static void (*ulc_mmap_unmap)(void *buf, int size) = (void*)1000501;
static int (*ulc_mmap_mprotect)(void *buf, int size, U32 flag) = (void*)1000502;

static int (*ulc_get_local_arch)(void) = (void*)1000507;
static void * (*ulc_get_helper)(unsigned int id, const void **tmp_helpers) = (void*)1000508;
static const void ** (*ulc_get_base_helpers)(void) = (void*)1000509;
static const void ** (*ulc_get_sys_helpers)(void) = (void*)1000510;
static const void ** (*ulc_get_user_helpers)(void) = (void*)1000511;

#endif

#ifdef __cplusplus
}
#endif
#endif
