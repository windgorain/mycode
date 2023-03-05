/*********************************************************
*   Copyright (C) LiXingang
*   Description: 
*
*************************************************/
#include "klc/klc_base.h"
#include "drivers/dirver_netdev.h"
#include <net/netns/generic.h>
#include <net/fib_notifier.h>

typedef struct {
    char dev_name[64];
    struct dentry *nsim_ddir;
    struct dentry *nsim_sdev_ddir;
    struct bus_type nsim_bus;
    struct rtnl_link_ops nsim_link_ops;
    struct pernet_operations nsim_devlink_net_ops;
    unsigned int nsim_devlink_id;
    unsigned int nsim_fib_net_id;
    struct pernet_operations nsim_fib_net_ops;
    struct notifier_block nsim_fib_nb;
    struct net_device_ops nsim_netdev_ops;
}NETDEVSIM_S;

struct netdevsim_shared_dev {
	unsigned int refcnt;
	u32 switch_id;

	struct dentry *ddir;

	struct bpf_offload_dev *bpf_dev;

	struct dentry *ddir_bpf_bound_progs;
	u32 prog_id_gen;

	struct list_head bpf_bound_progs;
	struct list_head bpf_bound_maps;
};

struct netdevsim {
	struct net_device *netdev;

	u64 tx_packets;
	u64 tx_bytes;
	struct u64_stats_sync syncp;

	struct device dev;
	struct netdevsim_shared_dev *sdev;

	struct dentry *ddir;

	unsigned int num_vfs;
	struct nsim_vf_config *vfconfigs;

	struct bpf_prog	*bpf_offloaded;
	u32 bpf_offloaded_id;

	struct xdp_attachment_info xdp;
	struct xdp_attachment_info xdp_hw;

	bool bpf_bind_accept;
	u32 bpf_bind_verifier_delay;

	bool bpf_tc_accept;
	bool bpf_tc_non_bound_accept;
	bool bpf_xdpdrv_accept;
	bool bpf_xdpoffload_accept;

	bool bpf_map_accept;
#if IS_ENABLED(CONFIG_NET_DEVLINK)
	struct devlink *devlink;
#endif
	//struct nsim_ipsec ipsec;
};

struct nsim_fib_entry {
	u64 max;
	u64 num;
};

struct nsim_per_fib_data {
	struct nsim_fib_entry fib;
	struct nsim_fib_entry rules;
};

struct nsim_fib_data {
	struct nsim_per_fib_data ipv4;
	struct nsim_per_fib_data ipv6;
};

#define KLC_MODULE_NAME "drivers/ns"
KLC_DEF_MODULE_EXT(KLC_MODULE_NAME, sizeof(NETDEVSIM_S));

static inline struct netdevsim *to_nsim(struct device *ptr)
{
	return container_of(ptr, struct netdevsim, dev);
}

SEC("klc/namefunc/nsim_num_vf")
int nsim_num_vf(struct device *dev)
{
	struct netdevsim *ns = to_nsim(dev);
	return ns->num_vfs;
}

SEC("klc/namefunc/nsim_fib_netns_init")
int nsim_fib_netns_init(struct net *net)
{
    NETDEVSIM_S *ctrl = KLCHLP_GetSelfModuleData(sizeof(NETDEVSIM_S));
	struct nsim_fib_data *data = net_generic(net, ctrl->nsim_fib_net_id);

	data->ipv4.fib.max = (u64)-1;
	data->ipv4.rules.max = (u64)-1;

	data->ipv6.fib.max = (u64)-1;
	data->ipv6.rules.max = (u64)-1;

	return 0;
}

static inline int nsim_fib_rule_account(struct nsim_fib_entry *entry, bool add,
				 struct netlink_ext_ack *extack)
{
	int err = 0;

	if (add) {
		if (entry->num < entry->max) {
			entry->num++;
		} else {
			err = -ENOSPC;
		}
	} else {
		entry->num--;
	}

	return err;
}

static inline int nsim_fib_rule_event(struct fib_notifier_info *info, bool add)
{
    NETDEVSIM_S *ctrl = KLCHLP_GetSelfModuleData(sizeof(NETDEVSIM_S));
	struct nsim_fib_data *data = net_generic(info->net, ctrl->nsim_fib_net_id);
	struct netlink_ext_ack *extack = info->extack;
	int err = 0;

	switch (info->family) {
	case AF_INET:
		err = nsim_fib_rule_account(&data->ipv4.rules, add, extack);
		break;
	case AF_INET6:
		err = nsim_fib_rule_account(&data->ipv6.rules, add, extack);
		break;
	}

	return err;
}

static inline int nsim_fib_account(struct nsim_fib_entry *entry, bool add,
			    struct netlink_ext_ack *extack)
{
	int err = 0;

	if (add) {
		if (entry->num < entry->max) {
			entry->num++;
		} else {
			err = -ENOSPC;
		}
	} else {
		entry->num--;
	}

	return err;
}

static inline int nsim_fib_event(struct fib_notifier_info *info, bool add)
{
    NETDEVSIM_S *ctrl = KLCHLP_GetSelfModuleData(sizeof(NETDEVSIM_S));
	struct nsim_fib_data *data = net_generic(info->net, ctrl->nsim_fib_net_id);
	struct netlink_ext_ack *extack = info->extack;
	int err = 0;

	switch (info->family) {
	case AF_INET:
		err = nsim_fib_account(&data->ipv4.fib, add, extack);
		break;
	case AF_INET6:
		err = nsim_fib_account(&data->ipv6.fib, add, extack);
		break;
	}

	return err;
}

SEC("klc/namefunc/nsim_fib_event_nb")
int nsim_fib_event_nb(struct notifier_block *nb, unsigned long event, void *ptr)
{
	struct fib_notifier_info *info = ptr;
	int err = 0;

	switch (event) {
	case FIB_EVENT_RULE_ADD: /* fall through */
	case FIB_EVENT_RULE_DEL:
		err = nsim_fib_rule_event(info, event == FIB_EVENT_RULE_ADD);
		break;

	case FIB_EVENT_ENTRY_ADD:  /* fall through */
	case FIB_EVENT_ENTRY_DEL:
		err = nsim_fib_event(info, event == FIB_EVENT_ENTRY_ADD);
		break;
	}

	return notifier_from_errno(err);
}


SEC("klc/namefunc/nsim_fib1")
void nsim_fib1(struct notifier_block *nb)
{
    NETDEVSIM_S *ctrl = KLCHLP_GetSelfModuleData(sizeof(NETDEVSIM_S));
	struct nsim_fib_data *data;
	struct net *net;

	//TODO rcu_read_lock();
	for_each_net_rcu(net) {
		data = net_generic(net, ctrl->nsim_fib_net_id);

		data->ipv4.fib.num = 0ULL;
		data->ipv4.rules.num = 0ULL;

		data->ipv6.fib.num = 0ULL;
		data->ipv6.rules.num = 0ULL;
	}
	//TODO rcu_read_unlock();
}

SEC("klc/namefunc/nsim2")
int nsim2(struct net *net)
{
    NETDEVSIM_S *ctrl = KLCHLP_GetSelfModuleData(sizeof(NETDEVSIM_S));
	bool *reg_devlink = net_generic(net, ctrl->nsim_devlink_id);

	*reg_devlink = true;

	return 0;
}

SEC("klc/namefunc/nsim_free")
void nsim_free(struct net_device *dev)
{
	struct netdevsim *ns = netdev_priv(dev);

	DriverKlc_device_unregister(&ns->dev);
	/* netdev and vf state will be freed out of device_release() */
}

SEC("klc/namefunc/nsim_setup")
void nsim_setup(struct net_device *dev)
{
    NETDEVSIM_S *ctrl = KLCHLP_GetSelfModuleData(sizeof(NETDEVSIM_S));
	DriverKlc_ether_setup(dev);
	DriverKlc_eth_hw_addr_random(dev);

	dev->netdev_ops = &ctrl->nsim_netdev_ops;
	dev->priv_destructor = KLCHLP_GetFuncEntry(KLCHLP_GetLocalNameFunc("nsim_free"));

	dev->tx_queue_len = 0;
	dev->flags |= IFF_NOARP;
	dev->flags &= ~IFF_MULTICAST;
	dev->priv_flags |= IFF_LIVE_ADDR_CHANGE |
			   IFF_NO_QUEUE;
	dev->features |= NETIF_F_HIGHDMA |
			 NETIF_F_SG |
			 NETIF_F_FRAGLIST |
			 NETIF_F_HW_CSUM |
			 NETIF_F_TSO;
	dev->hw_features |= NETIF_F_HW_TC;
	dev->max_mtu = ETH_MAX_MTU;
}

SEC("klc/namefunc/nsim_validate")
int nsim_validate(struct nlattr *tb[], struct nlattr *data[], struct netlink_ext_ack *extack)
{
	if (tb[IFLA_ADDRESS]) {
		if (nla_len(tb[IFLA_ADDRESS]) != ETH_ALEN)
			return -EINVAL;
		if (!is_valid_ether_addr(nla_data(tb[IFLA_ADDRESS])))
			return -EADDRNOTAVAIL;
	}
	return 0;
}

SEC("klc/namefunc/nsim_newlink")
int nsim_newlink(struct net *src_net, struct net_device *dev,
			struct nlattr *tb[], struct nlattr *data[],
			struct netlink_ext_ack *extack)
{
    NETDEVSIM_S *ctrl = KLCHLP_GetSelfModuleData(sizeof(NETDEVSIM_S));
	struct netdevsim *ns = netdev_priv(dev);

	if (tb[IFLA_LINK]) {
		struct net_device *joindev;
		struct netdevsim *joinns;

		joindev = DriverKlc___dev_get_by_index(src_net,
					     nla_get_u32(tb[IFLA_LINK]));
		if (!joindev)
			return -ENODEV;
		if (joindev->netdev_ops != &ctrl->nsim_netdev_ops)
			return -EINVAL;

		joinns = netdev_priv(joindev);
		if (!joinns->sdev || !joinns->sdev->refcnt)
			return -EINVAL;
		ns->sdev = joinns->sdev;
	}

	return DriverKlc_register_netdevice(dev);
}

SEC("klc/namefunc/nsim_dellink")
void nsim_dellink(struct net_device *dev, struct list_head *head)
{
	DriverKlc_unregister_netdevice_queue(dev, head);
}

static inline int _mod_data_init(NETDEVSIM_S *ctrl)
{
    char name[] = "netdevsim";
    memcpy(ctrl->dev_name, name, sizeof(name));

    KLC_FUNC_S *func = KLCHLP_GetLocalNameFunc("nsim_num_vf");
    if (! func) {
        BPF_Print("Can't get function nsim_num_vf");
        return -1;
    }

    if (! KLCHLP_IsFuncJitted(func)) {
        BPF_Print("Function not jitted");
        return -1;
    }

    ctrl->nsim_bus.name = ctrl->dev_name;
    ctrl->nsim_bus.dev_name = ctrl->dev_name;
    ctrl->nsim_bus.num_vf = KLCHLP_GetFuncEntry(KLCHLP_GetLocalNameFunc("nsim_num_vf"));
    if (! ctrl->nsim_bus.num_vf) {
        return -1;
    }

    ctrl->nsim_fib_net_ops.init = KLCHLP_GetFuncEntry(KLCHLP_GetLocalNameFunc("nsim_fib_netns_init"));
    ctrl->nsim_fib_net_ops.id = &ctrl->nsim_fib_net_id;
    ctrl->nsim_fib_net_ops.size = sizeof(struct nsim_fib_data);
    if (! ctrl->nsim_fib_net_ops.init) {
        return -1;
    }

    ctrl->nsim_fib_nb.notifier_call = KLCHLP_GetFuncEntry(KLCHLP_GetLocalNameFunc("nsim_fib_event_nb"));

    ctrl->nsim_devlink_net_ops.init = KLCHLP_GetFuncEntry(KLCHLP_GetLocalNameFunc("nsim2"));
    ctrl->nsim_devlink_net_ops.id = &ctrl->nsim_devlink_id;
    ctrl->nsim_devlink_net_ops.size = sizeof(bool);

    ctrl->nsim_link_ops.kind = ctrl->dev_name;
    ctrl->nsim_link_ops.priv_size = sizeof(struct netdevsim);
    ctrl->nsim_link_ops.setup = KLCHLP_GetFuncEntry(KLCHLP_GetLocalNameFunc("nsim_setup"));
    ctrl->nsim_link_ops.validate = KLCHLP_GetFuncEntry(KLCHLP_GetLocalNameFunc("nsim_validata"));
    ctrl->nsim_link_ops.newlink = KLCHLP_GetFuncEntry(KLCHLP_GetLocalNameFunc("nsim_newlink"));
    ctrl->nsim_link_ops.dellink = KLCHLP_GetFuncEntry(KLCHLP_GetLocalNameFunc("nsim_dellink"));

    return 0;
}

static inline int nsim_fib_init(NETDEVSIM_S *ctrl)
{
	int err;

	err = DriverKlc_register_pernet_subsys(&ctrl->nsim_fib_net_ops);
	if (err < 0) {
		BPF_Print("Failed to register pernet subsystem\n");
		goto err_out;
	}

    return 0;

    void *fn = KLCHLP_GetFuncEntry(KLCHLP_GetLocalNameFunc("nsim_fib1"));

	err = KLCHLP_SysCall("register_fib_notifier", (long)&ctrl->nsim_fib_nb, (long)fn, 0);
	if (err < 0) {
		BPF_Print("Failed to register fib notifier\n");
		goto err_out;
	}

err_out:
	return err;
}

static inline void nsim_fib_exit(NETDEVSIM_S *ctrl)
{
    DriverKlc_unregister_pernet_subsys(&ctrl->nsim_fib_net_ops);
	KLCHLP_SysCall("unregister_fib_notifier", (long)&ctrl->nsim_fib_nb, 0, 0);
}

static inline int nsim_devlink_init(NETDEVSIM_S *ctrl)
{
	int err;

	err = nsim_fib_init(ctrl);
	if (err)
		goto err_out;

    return 0;

	err = DriverKlc_register_pernet_subsys(&ctrl->nsim_devlink_net_ops);
	if (err)
		nsim_fib_exit(ctrl);

err_out:
	return err;
}

SEC("klc/namefunc/nsim_init")
int nsim_module_init()
{
	int err;
    NETDEVSIM_S *ctrl = KLCHLP_GetSelfModuleData(sizeof(NETDEVSIM_S));

    if (! ctrl) {
		return -ENOMEM;
    }

    if (_mod_data_init(ctrl) < 0) {
		return -ENOMEM;
    }

    ctrl->nsim_ddir = DriverKlc_debugfs_create_dir("netdevsim");
    if (IS_ERR_OR_NULL(ctrl->nsim_ddir)) {
        return -ENOMEM;
    }

    BPF_PrintLn();

    ctrl->nsim_sdev_ddir = DriverKlc_debugfs_create_dir("netdevsim_sdev");
    if (IS_ERR_OR_NULL(ctrl->nsim_sdev_ddir)) {
        err = -ENOMEM;
        goto err_debugfs_destroy;
    }

    BPF_PrintLn();

    err = DriverKlc_bus_register(&ctrl->nsim_bus);
    if (err)
        goto err_sdir_destroy;

    BPF_PrintLn();

    err = nsim_devlink_init(ctrl);
    if (err)
        goto err_unreg_bus;

    BPF_PrintLn();

    return 0;

	err = DriverKlc_rtnl_link_register(&ctrl->nsim_link_ops);
	if (err)
		goto err_dl_fini;

    BPF_PrintLn();

	return 0;

err_dl_fini:
    KLCHLP_SysCall("nsim_devlink_exit", 0, 0, 0);
err_unreg_bus:
    DriverKlc_bus_unregister(&ctrl->nsim_bus);
err_sdir_destroy:
    DriverKlc_debugfs_remove_recursive(ctrl->nsim_sdev_ddir);
err_debugfs_destroy:
    DriverKlc_debugfs_remove_recursive(ctrl->nsim_ddir);
    BPF_PrintLn();
	return err;
}

/* 定义init段, 指定初始化调用函数 */
KLC_FUNC_INIT_S _init[] SEC("klc/init") = {
    {.func = "nsim_init"}
};
