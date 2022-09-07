/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "../klcko_lib.h"
#include "ko/ko_netfilter.h"

static atomic_t g_klcko_nf_enabled[NF_INET_NUMHOOKS] = {0};

static inline int _klcko_nf_input(void *net, int hooknum, void *skb)
{
    KLC_EVENT_S ev = {.event=KLC_SYS_EVENT_NETFILTER};
    KLC_SYSTEM_EVENT_NF_S param = {0};

    ev.action = NF_ACCEPT;
    param.state = hooknum;
    param.skb = skb;
    param.net = net;
    ev.sub_event = hooknum;

    KlcKo_SystemEventPublish(&ev, (long)(void*)&param, 0);

    return ev.action;
}

static unsigned int _klcko_nf_hook_event(KO_NF_HOOK_PARAMS)
{
	struct net *net = KO_NF_PARAM_GET_NET();
    int hooknum = KO_NF_PARAM_GET_HOOKNUM();

    return _klcko_nf_input(net, hooknum, skb);
}

static struct nf_hook_ops g_klcko_nf_prerouting_ops[] __read_mostly = {
    KO_NF_DEF_OPS(_klcko_nf_hook_event, NFPROTO_IPV4, NF_INET_PRE_ROUTING, NF_IP_PRI_FILTER+1)
};

static struct nf_hook_ops g_klcko_nf_localin_ops[] __read_mostly = {
    KO_NF_DEF_OPS(_klcko_nf_hook_event, NFPROTO_IPV4, NF_INET_LOCAL_IN, NF_IP_PRI_FILTER-10),
};

static struct nf_hook_ops g_klcko_nf_localout_ops[] __read_mostly = {
    KO_NF_DEF_OPS(_klcko_nf_hook_event, NFPROTO_IPV4, NF_INET_LOCAL_OUT, NF_IP_PRI_FILTER+10),
};

static struct nf_hook_ops g_klcko_nf_forward_ops[] __read_mostly = {
    KO_NF_DEF_OPS(_klcko_nf_hook_event, NFPROTO_IPV4, NF_INET_FORWARD, NF_IP_PRI_FILTER+1)
};

static struct nf_hook_ops g_klcko_nf_postrouting_ops[] __read_mostly = {
    KO_NF_DEF_OPS(_klcko_nf_hook_event, NFPROTO_IPV4, NF_INET_POST_ROUTING, NF_IP_PRI_FILTER+1)
};

int KlcKoNf_Start(unsigned int hooknum)
{
    int ret;

    if (hooknum >= NF_INET_NUMHOOKS){
        return KO_ERR_BAD_PARAM;
    }

    if (atomic_inc_return(&g_klcko_nf_enabled[hooknum]) != 1) {
        return 0;
    }

    switch (hooknum) {
        case NF_INET_PRE_ROUTING:
            ret = KO_NF_REG_HOOKS(g_klcko_nf_prerouting_ops);
            break;
        case NF_INET_LOCAL_IN:
            ret = KO_NF_REG_HOOKS(g_klcko_nf_localin_ops);
            break;
        case NF_INET_FORWARD:
            ret = KO_NF_REG_HOOKS(g_klcko_nf_forward_ops);
            break;
        case NF_INET_LOCAL_OUT:
            ret = KO_NF_REG_HOOKS(g_klcko_nf_localout_ops);
            break;
        case NF_INET_POST_ROUTING:
            ret = KO_NF_REG_HOOKS(g_klcko_nf_postrouting_ops);
            break;
        default:
            ret = -1;
            break;
    }

    if (ret != 0) {
        atomic_dec(&g_klcko_nf_enabled[hooknum]);
        return -1;
    }

    return 0;
}
EXPORT_SYMBOL(KlcKoNf_Start);

int KlcKoNf_Stop(unsigned int hooknum)
{
    int ret = 0;

    if (hooknum >= NF_INET_NUMHOOKS){
        return KO_ERR_BAD_PARAM;
    }

    if (atomic_dec_return(&g_klcko_nf_enabled[hooknum]) != 0) {
        return 0;
    }

    switch (hooknum) {
        case NF_INET_PRE_ROUTING:
            KO_NF_UNREG_HOOKS(g_klcko_nf_prerouting_ops);
            break;
        case NF_INET_LOCAL_IN:
            KO_NF_UNREG_HOOKS(g_klcko_nf_localin_ops);
            break;
        case NF_INET_FORWARD:
            KO_NF_UNREG_HOOKS(g_klcko_nf_forward_ops);
            break;
        case NF_INET_LOCAL_OUT:
            KO_NF_UNREG_HOOKS(g_klcko_nf_localout_ops);
            break;
        case NF_INET_POST_ROUTING:
            KO_NF_UNREG_HOOKS(g_klcko_nf_postrouting_ops);
            break;
        default:
            ret = -1;
            break;
    }

    return 0;
}
EXPORT_SYMBOL(KlcKoNf_Stop);

static void klcko_nf_stop_force(unsigned int hooknum)
{
    if (hooknum >= NF_INET_NUMHOOKS){
        return;
    }

    if (atomic_read(&g_klcko_nf_enabled[hooknum]) == 0) {
        return;
    }

    switch (hooknum) {
        case NF_INET_PRE_ROUTING:
            KO_NF_UNREG_HOOKS(g_klcko_nf_prerouting_ops);
            break;
        case NF_INET_LOCAL_IN:
            KO_NF_UNREG_HOOKS(g_klcko_nf_localin_ops);
            break;
        case NF_INET_FORWARD:
            KO_NF_UNREG_HOOKS(g_klcko_nf_forward_ops);
            break;
        case NF_INET_LOCAL_OUT:
            KO_NF_UNREG_HOOKS(g_klcko_nf_localout_ops);
            break;
        case NF_INET_POST_ROUTING:
            KO_NF_UNREG_HOOKS(g_klcko_nf_postrouting_ops);
            break;
        default:
            break;
    }

    atomic_set(&g_klcko_nf_enabled[hooknum], 0);
}

int KlcKoNf_Init(void)
{
    return 0;
}

void KlcKoNf_Fini(void)
{
    klcko_nf_stop_force(NF_INET_PRE_ROUTING);
    klcko_nf_stop_force(NF_INET_LOCAL_IN);
    klcko_nf_stop_force(NF_INET_LOCAL_OUT);
    klcko_nf_stop_force(NF_INET_FORWARD);
    klcko_nf_stop_force(NF_INET_POST_ROUTING);
}

