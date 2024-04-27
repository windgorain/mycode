/*================================================================
*   Created by LiXingang
*   Description: 
*
================================================================*/
#include "klcko_lib.h"
#include "ko/ko_nl.h"
#include "ko/ko_utl.h"

typedef void (*PF_netlink_ack)(void *in_skb, void *nlh, int err, void *extack);

static PF_KLCKO_NL_MSG g_klcko_nl_type[KLC_NL_TYPE_MAX] = {0};
static DEFINE_MUTEX(g_klcko_nl_lock);

static inline void _klcko_nl_lock(void)
{
    mutex_lock(&g_klcko_nl_lock);
}

static inline void _klcko_nl_unlock(void)
{
    mutex_unlock(&g_klcko_nl_lock);
}

static int _klcko_process_cmd(int cmd, void *data, int data_len, void *reply, int reply_size)
{
    int type = KLC_NL_TYPE(cmd);
    int subcmd = KLC_NL_CMD(cmd);
    PF_KLCKO_NL_MSG func;

    func = g_klcko_nl_type[type];

    if (! func) {
        return KO_ERR_NO_SUCH;
    }

    return func(subcmd, data, data_len, reply, reply_size);
}

static void _klcko_rcv_skb(struct sk_buff *skb)
{
    int type, pid, flags, nlmsglen, skblen, ret = 0;
    struct nlmsghdr *nlh;
    NETLINK_MSG_S *msg_st;
    void *data;
    void *reply_ptr;
    int data_len;
    PF_netlink_ack ack_func;

    skblen = skb->len;
    if (skblen < sizeof(*nlh)) {
        return;
    }

    nlh = nlmsg_hdr(skb);
    nlmsglen = nlh->nlmsg_len;
    if (nlmsglen < sizeof(*nlh) || skblen < nlmsglen) {
        return;
    }

    pid = nlh->nlmsg_pid;
    flags = nlh->nlmsg_flags;

    if (flags & MSG_TRUNC) {
        return;
    }

    msg_st = (NETLINK_MSG_S *)nlh;
    type = msg_st->msg_type;
    data = msg_st->data;
    data_len = nlmsglen - sizeof(NETLINK_MSG_S);
    reply_ptr = msg_st->reply_ptr;

    ret = _klcko_process_cmd(type, data, data_len, reply_ptr, msg_st->reply_size);

    ack_func = (void*)netlink_ack;
    ack_func(skb, nlh, ret, NULL);

    return;
}

static int netlink_gen_doit(struct sk_buff *skb2, struct genl_info *info)
{ 
    _klcko_nl_lock();
    _klcko_rcv_skb(skb2);
    _klcko_nl_unlock();

    return 0;
}  

static struct genl_ops g_klcko_nl_ops[] = {  
    {
        .cmd = NETLINK_GEN_C_CMD,
        .flags = 0,
        .doit = netlink_gen_doit,
        .dumpit = NULL,
        .maxattr = 0,
    },
};

static struct genl_family g_klcko_nl_family = {
    .hdrsize = 0,  
    .name = NETLINK_GEN_NAME,
    .version = 1,
    .maxattr = 0, 
    .netnsok	= true,
    .ops = g_klcko_nl_ops,
    .n_ops = ARRAY_SIZE(g_klcko_nl_ops),
#ifdef GENL_ID_GENERATE
    .id = GENL_ID_GENERATE,
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6,0,0))
	.resv_start_op = NETLINK_GEN_C_MAX,
#endif
}; 

int KlcKoNl_Init(void)
{
    if (KO_NL_Reg(&g_klcko_nl_family, g_klcko_nl_ops, 1) < 0) {
        KO_Print("NL init failed \n");
        return -1;
    }  
    
    return 0;
}

void KlcKoNl_Fini(void)
{
    KO_NL_Unreg(&g_klcko_nl_family, g_klcko_nl_ops);
}

int KlcKoNl_Reg(unsigned int type, PF_KLCKO_NL_MSG func)
{
    if (type >= KLC_NL_TYPE_MAX) {
        return KO_ERR_BAD_PARAM;
    }

    g_klcko_nl_type[type] = func;

    return 0;
}
EXPORT_SYMBOL(KlcKoNl_Reg);

