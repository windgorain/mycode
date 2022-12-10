#include "klc/klc_base.h"
#include "helpers/skb_klc.h"
#include <linux/skbuff.h>

#define KLC_MODULE_NAME SKB_KLC_MODULE_NAME
KLC_DEF_MODULE();

SEC_NAME_FUNC(SKB_KLC_CREATE)
void * skb_klc_create_skb(int data_len)
{
    struct sk_buff *skb;
    skb = (void*)KLCSYM_alloc_skb(data_len + LL_MAX_HEADER, GFP_ATOMIC);
    if (! skb) {
        return NULL;
	}

    KLCHLP_SkbReserve(skb, LL_MAX_HEADER);
    KLCHLP_SkbResetNetworkHeader(skb);

    return skb;
}

SEC_NAME_FUNC(SKB_KLC_GET_STRUCT_INFO)
int skb_klc_get_struct_info(OUT KLC_SKB_STRUCT_INFO_S *info)
{
    info->struct_size = sizeof(struct sk_buff);
    info->cb_offset = offsetof(struct sk_buff, cb);
    info->mark_offset = offsetof(struct sk_buff, mark);
    info->priority_offset  = offsetof(struct sk_buff, priority);

    return 0;
}

SEC_NAME_FUNC(SKB_KLC_GET_SKB_INFO)
int skb_klc_get_skb_info(struct sk_buff *skb, OUT KLC_SKB_INFO_S *info)
{
    info->queue_mapping = skb->queue_mapping;
    info->protocol = skb->protocol;
    info->tc_index = skb->tc_index;
    info->vlan_tci = skb->vlan_tci;
    info->vlan_proto = skb->vlan_proto;
    info->len= skb->len;
    info->fragment_len= skb->data_len;
    info->head_len = skb->len - skb->data_len;
    info->pkt_type = skb->pkt_type;
    info->ingress_ifindex = skb->skb_iif;
    info->mark = skb->mark;
    info->hash = skb->hash;
    info->napi_id = skb->napi_id;
    info->priority = skb->priority;
    info->data = (u64)(unsigned long)skb->data;

    return 0;
}


