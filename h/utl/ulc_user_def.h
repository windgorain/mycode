/******************************************************************************
* Copyright (C), Xingang.Li
* Author:      Xingang.Li  Version: 1.0
* Description:
******************************************************************************/
#ifndef _ULC_USER_DEF_H_
#define _ULC_USER_DEF_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef IN_ULC_USER

#undef THREAD_LOCAL 
#define THREAD_LOCAL 

#undef PRINTFL
#define PRINTFL() do { \
    char _file[] = __FILE__; \
    printf("%s(%d) \n", _file, __LINE__); \
}while(0)

#define access(a,b) ulc_sys_access(a,b)
#define ftell(a) ulc_sys_ftell(a)
#define fseek(a,b,c) ulc_sys_fseek(a,b,c)
#define fopen(a,b) ulc_sys_fopen(a,b)
#define fread(a,b,c,d) ulc_sys_fread(a,b,c,d)
#define fclose(a) ulc_sys_fclose(a)
#define fgets(a,b,c) ulc_sys_fgets(a,b,c)
#define fwrite(a,b,c,d) ulc_sys_fwrite(a,b,c,d)
#define stat(a,b) ulc_sys_stat(a,b)
#undef stderr 
#define stderr ((void*)2)

#define fprintf(fp,fmt, ...) ({ \
    char _fmt[] = fmt; \
    int _count = BS_ARG_COUNT(__VA_ARGS__); \
    U64 _d[10]; \
    long _ret = -1; \
    switch (_count) { \
        case 10: _d[9]=(unsigned long long)BS_ARG_GET(10, ##__VA_ARGS__); \
        case 9: _d[8]=(unsigned long long)BS_ARG_GET(9, ##__VA_ARGS__); \
        case 8: _d[7]=(unsigned long long)BS_ARG_GET(8, ##__VA_ARGS__); \
        case 7: _d[6]=(unsigned long long)BS_ARG_GET(7, ##__VA_ARGS__); \
        case 6: _d[5]=(unsigned long long)BS_ARG_GET(6, ##__VA_ARGS__); \
        case 5: _d[4]=(unsigned long long)BS_ARG_GET(5, ##__VA_ARGS__); \
        case 4: _d[3]=(unsigned long long)BS_ARG_GET(4, ##__VA_ARGS__); \
        case 3: _d[2]=(unsigned long long)BS_ARG_GET(3, ##__VA_ARGS__); \
        case 2: _d[1]=(unsigned long long)BS_ARG_GET(2, ##__VA_ARGS__); \
        case 1: _d[0]=(unsigned long long)BS_ARG_GET(1, ##__VA_ARGS__); \
        case 0: break; \
    } \
    if (_count <= 10) { _ret = ulc_sys_fprintf(fp,fmt,_d,_count);} \
    _ret; \
})

#define time(a) ulc_sys_time(a)

#undef assert
#define assert(x) 

#define flush(x) 

#undef BS_WARNNING
#define BS_WARNNING(X) BPF_Print X

#define IC_Print(level,fmt,...) ulc_sys_printf(fmt, ##__VA_ARGS__)

#undef ATOM_BARRIER
#define ATOM_BARRIER() ulc_do_nothing()

#define BpfHelper_GetFunc(a,b) ulc_get_helper(a,b)
#define BpfHelper_BaseHelper() ulc_get_base_helpers()
#define BpfHelper_SysHelper() ulc_get_sys_helpers()
#define BpfHelper_UserHelper() ulc_get_user_helpers()

#define RcuEngine_Call ulc_sys_rcu_call
#define RcuEngine_Lock ulc_sys_rcu_lock
#define RcuEngine_UnLock ulc_sys_rcu_unlock

#define memset(a,b,c) ulc_sys_memset(a,b,c)
#undef MEM_ZMalloc
#define MEM_ZMalloc(x) ({void *p = ulc_sys_malloc(x); if (p) {ulc_sys_memset(p, 0, (x));} p;})
#undef MEM_Malloc
#define MEM_Malloc(x) ulc_sys_malloc(x)
#undef MEM_Free
#define MEM_Free(x) ulc_sys_free(x)
#define malloc(a) ulc_sys_malloc(a)
#define calloc(a,b) ulc_sys_calloc(a,b)
#define free(a) ulc_sys_free(a)
#define realloc(a,b) ulc_sys_realloc(a,b)

#undef strlcpy
#define strlcpy(a,b,c) ulc_sys_strlcpy(a,b,c)
#define TXT_Strdup(a) ulc_sys_strdup(a)
#define strdup(a) ulc_sys_strdup(a)
#define strtok_r(a,b,c) ulc_sys_strtok_r(a,b,c)
#define strcmp(a,b) ulc_sys_strcmp(a,b)
#define strnlen(a,b) ulc_sys_strnlen(a,b)
#define strlen(a) ulc_sys_strlen(a)
#define strncmp(a,b,c) ulc_sys_strncmp(a,b,c)
#define strcpy(a,b) ulc_sys_strcpy(a,b)
#undef tolower
#define tolower(a) ulc_sys_tolower(a)
#define strchr(a,b) ulc_sys_strchr(a,b)

#define memcpy(a,b,c) ulc_sys_memcpy(a,b,c)
#define memmove(dst,src,len) ulc_sys_memmove(dst,src,len)
#define memcmp(a,b,c) ulc_sys_memcmp(a,b,c)

#define socket(a,b,c) ulc_sys_socket(a,b,c)
#define bind(a,b,c) ulc_sys_bind(a,b,c)
#define connect(a,b,c) ulc_sys_connect(a,b,c)
#define listen(a,b) ulc_sys_listen(a,b)
#define accept(a,b,c) ulc_sys_accept(a,b,c)
#define recv(a,b,c,d) ulc_sys_recv(a,b,c,d)
#define send(a,b,c,d) ulc_sys_send(a,b,c,d)
#define close(a) ulc_sys_close(a)
#define setsockopt(a,b,c,d,e) ulc_sys_setsockopt(a,b,c,d,e)

#define pthread_create(a,b,c,d) ulc_pthread_create(a,b,c,d)

#undef errno
#define errno ulc_sys_get_errno()

#define mmap(a,b,c,d,e,f) ulc_mmap_map(a,b,(((U64)(c))<<32)|(d),e,f)
#define munmap(a,b) ulc_mmap_unmap(a,b)
#define mprotect(a,b,c) ulc_mmap_mprotect(a,b,c)

#endif

#ifdef __cplusplus
}
#endif
#endif //ULC_USER_DEF_H_
