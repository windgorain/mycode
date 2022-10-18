/*********************************************************
*   Copyright (C) LiXingang
*   Description: 
*
*************************************************/
#ifndef _DIRVER_NETDEV_H
#define _DIRVER_NETDEV_H
#ifdef __cplusplus
extern "C"
{
#endif

static inline void * _DriverKlc_debugfs_create_dir(char *name)
{
    u64 ret;

    ret = KLCHLP_SysCall("debugfs_create_dir", (long)name, 0, 0);
    if (ret == KLC_RET_ERR) {
        return NULL;
    }

    return (void*)ret;
}

#define DriverKlc_debugfs_create_dir(name) ({ \
        char _name[] = name; \
        _DriverKlc_debugfs_create_dir(_name); \
    })

static inline void DriverKlc_debugfs_remove_recursive(void *ddir)
{
	KLCHLP_SysCall("debugfs_remove_recursive", (long)ddir, 0, 0);
}

static inline int DriverKlc_bus_register(void *bus)
{
    return KLCHLP_SysCall("bus_register", (long)bus, 0, 0);
}

static inline void DriverKlc_bus_unregister(void *bus)
{
    KLCHLP_SysCall("bus_unregister", (long)bus, 0, 0);
}

static inline int DriverKlc_rtnl_link_register(void *link_ops)
{
	return KLCHLP_SysCall("rtnl_link_register", (long)link_ops, 0, 0);
}

static inline void DriverKlc_rtnl_link_unregister(void *link_ops)
{
	KLCHLP_SysCall("rtnl_link_unregister", (long)link_ops, 0, 0);
}

static inline int DriverKlc_register_pernet_subsys(void *devlink_net_ops)
{
	return KLCHLP_SysCall("register_pernet_subsys", (long)devlink_net_ops, 0, 0);
}

static inline void DriverKlc_unregister_pernet_subsys(void *devlink_net_ops)
{
	KLCHLP_SysCall("unregister_pernet_subsys", (long)devlink_net_ops, 0, 0);
}

static inline void DriverKlc_device_unregister(void *dev)
{
	KLCHLP_SysCall("device_unregister", (long)dev, 0, 0);
}

static inline void DriverKlc_ether_setup(void *dev)
{
	KLCHLP_SysCall("ether_setup", (long)dev, 0, 0);
}

static inline void * DriverKlc___dev_get_by_index(void *net, int ifindex)
{
    u64 ret = KLCHLP_SysCall("__dev_get_by_index", (long)net, ifindex, 0);
    if (ret == KLC_RET_ERR) {
        return NULL;
    }
    return (void*)ret;
}

static inline int DriverKlc_register_netdevice(void *dev)
{
    u64 ret = KLCHLP_SysCall("register_netdevice", (long)dev, 0, 0);
    if (ret == KLC_RET_ERR) {
        return -1;
    }
    return ret;
}

static inline void DriverKlc_unregister_netdevice_queue(void *dev, void *head)
{
    KLCHLP_SysCall("unregister_netdevice_queue", (long)dev, (long)head, 0);
}

static inline void DriverKlc_get_random_bytes(void *buf, int nbytes)
{
    KLCHLP_SysCall("get_random_bytes", (long)buf, nbytes, 0);
}

static inline void DriverKlc_eth_random_addr(u8 *addr)
{
	DriverKlc_get_random_bytes(addr, ETH_ALEN);
	addr[0] &= 0xfe;	/* clear multicast bit */
	addr[0] |= 0x02;	/* set local assignment bit (IEEE802) */
}

static inline void DriverKlc_eth_hw_addr_random(struct net_device *dev)
{
	dev->addr_assign_type = NET_ADDR_RANDOM;
	DriverKlc_eth_random_addr(dev->dev_addr);
}

#ifdef __cplusplus
}
#endif
#endif //DIRVER_NETDEV_H_
