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

void * KlcKoComp_GetSkbInfo(struct sk_buff *skb, OUT KLC_SKBUFF_INFO_S *skb_info)
{
    if ((!skb) || (!skb_info)) {
        return NULL;
    }

    skb_info->struct_size = sizeof(struct sk_buff);

    skb_info->cb_offset = offsetof(struct sk_buff, cb);
    skb_info->mark_offset = offsetof(struct sk_buff, mark);
    skb_info->priority_offset  = offsetof(struct sk_buff, priority);

    skb_info->queue_mapping = skb->queue_mapping;
    skb_info->protocol = skb->protocol;
    skb_info->tc_index = skb->tc_index;
    skb_info->vlan_tci = skb->vlan_tci;
    skb_info->vlan_proto = skb->vlan_proto;
    skb_info->len= skb->len;
    skb_info->fragment_len= skb->data_len;
    skb_info->head_len = skb->len - skb->data_len;

    skb_info->pkt_type = skb->pkt_type;
    skb_info->ingress_ifindex = skb->skb_iif;
    skb_info->mark = skb->mark;
    skb_info->hash = skb->hash;
    skb_info->napi_id = skb->napi_id;
    skb_info->priority = skb->priority;

    skb_info->data = (u64)(unsigned long)skb->data;

    return skb_info;
}
EXPORT_SYMBOL(KlcKoComp_GetSkbInfo);

int KlcKoComp_GetXdpStructInfo(OUT KLC_XDP_STRUCT_INFO_S *info)
{
    if (!info) {
        return -1;
    }

    info->struct_size = sizeof(struct xdp_buff);

    info->data_offset = offsetof(struct xdp_buff, data);
    info->data_end_offset = offsetof(struct xdp_buff, data_end);
    info->data_meta_offset = offsetof(struct xdp_buff, data_meta);
    info->data_hard_start_offset = offsetof(struct xdp_buff, data_hard_start);

    return 0;
}
EXPORT_SYMBOL(KlcKoComp_GetXdpStructInfo);

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

