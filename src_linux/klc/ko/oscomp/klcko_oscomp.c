/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "klcko_lib.h"
#include <linux/bpf_verifier.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,18,0)
#define KLCKO_OSBASE_BPF_OPS 
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,9,0)
#define KLCKO_OSBASE_BPF_PROG
#endif

#ifdef BPF_MAX_SUBPROGS
#undef KLCKO_OSBASE_BPF_OPS 
#define KLCKO_OSBASE_BPF_OPS 
#undef KLCKO_OSBASE_BPF_PROG
#define KLCKO_OSBASE_BPF_PROG
#endif


typedef void* (*PF_bpf_map_inc)(void *map, bool uref);
typedef void (*PF_bpf_map_put)(void *map);

static PF_bpf_map_inc g_klcko_oscomp_map_inc = NULL;
static PF_bpf_map_put g_klcko_oscomp_map_put = NULL;

int KlcKoComp_GetProgLen(struct bpf_prog *prog)
{
	return prog->len;
}
EXPORT_SYMBOL(KlcKoComp_GetProgLen);

void * KlcKoComp_GetProgInsn(struct bpf_prog *prog)
{
    return prog->insnsi;
}
EXPORT_SYMBOL(KlcKoComp_GetProgInsn);

void * KlcKoComp_GetProg(void *env)
{
#ifdef KLCKO_OSBASE_BPF_PROG
    struct bpf_verifier_env *tmp = env;
    return tmp->prog;
#else
    struct verifier_env {
        struct bpf_prog *prog;
    };
    struct verifier_env *tmp = env;
    return tmp->prog;
#endif
}
EXPORT_SYMBOL(KlcKoComp_GetProg);

void * KlcKoComp_GetOps(void *env)
{
#ifdef KLCKO_OSBASE_BPF_OPS
    struct bpf_verifier_env *tmp = env;
    return &tmp->ops;
#else
    struct bpf_prog *prog = KlcKoComp_GetProg(env);
    return &prog->aux->ops;
#endif
}
EXPORT_SYMBOL(KlcKoComp_GetOps);

int KlcKoComp_GetOpsSize(void)
{
    return sizeof(struct bpf_verifier_ops);
}
EXPORT_SYMBOL(KlcKoComp_GetOpsSize);

void * KlcKoComp_GetCurrentMm(void)
{
    return current->mm;
}
EXPORT_SYMBOL(KlcKoComp_GetCurrentMm);

u64 KlcKoComp_TaskGetClassID(void *skb)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,4,0)
    return task_get_classid(skb);
#else
    return KLC_RET_ERR;
#endif
}
EXPORT_SYMBOL(KlcKoComp_TaskGetClassID);

noinline void * klcko_base_get_map_self(void *map)
{
    return map;
}

int KlcKoComp_MapInc(void *map)
{
    if (!g_klcko_oscomp_map_inc) {
        return -1;
    }

	g_klcko_oscomp_map_inc(map, true);

    return 0;
}
EXPORT_SYMBOL(KlcKoComp_MapInc);

void KlcKoComp_MapDec(void *map)
{
    if (g_klcko_oscomp_map_put) {
        g_klcko_oscomp_map_put(map);
    }
}
EXPORT_SYMBOL(KlcKoComp_MapDec);

int KlcKoComp_Init(void)
{
    g_klcko_oscomp_map_inc = (void*)kallsyms_lookup_name("bpf_map_inc");
    g_klcko_oscomp_map_put = (void*)kallsyms_lookup_name("bpf_map_put_with_uref");

    return 0;
}

