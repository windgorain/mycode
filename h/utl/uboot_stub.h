/*********************************************************
*   Copyright (C) LiXingang
*   Description: 
*
********************************************************/
#ifndef _UBOOT_STUB_H
#define _UBOOT_STUB_H
#ifdef __cplusplus
extern "C"
{
#endif

#define IN_UBOOT 1

#define ATOM_BARRIER()
#define MEM_Malloc(x) malloc(x)
#define MEM_ZMalloc(x) calloc((x),1)
#define MEM_Free(x) free(x)
#define RcuEngine_GetMemcap() (NULL)
#define RcuEngine_Lock() (0)
#define RcuEngine_UnLock(x) (void)(x)
#define MemCap_Malloc(a,x) MEM_Malloc(x)
#define MemCap_ZMalloc(a,x) MEM_ZMalloc(x)
#define MemCap_Free(a,x) MEM_Free(x)
#define TXT_Strdup strdup
#define strtol(a,b,c) simple_strtol(a,b,c)
#define RCU_NODE_S int
#define RcuEngine_Call(a,b) b(a)
#define MEM_RcuZMalloc(x) MEM_ZMalloc(x)
#define MEM_RcuFree(x) MEM_Free(x)
#ifndef INT64_C
#define UINT32_C(x) (x)
#define INT64_C(c) (c ## LL)
#define UINT64_C(c) (c ## ULL)
#endif
#ifndef assert
#define assert(x)
#endif

static inline void * MemCap_Dup(void *mem_cap, void *data, int len)
{
    void *buf = MEM_Malloc(len);
    if (buf) {
        memcpy(buf, data, len);
    }
    return buf;
}

static inline void * MEM_RcuDup(void *d, int l) {
    char *buf = MEM_Malloc(l);
    if (buf) {
        memcpy(buf, d, l);
    }
    return buf;
}

#ifdef __cplusplus
}
#endif
#endif 
