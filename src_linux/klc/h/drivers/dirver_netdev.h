/*********************************************************
*   Copyright (C) LiXingang
*   Description: 
*
*************************************************/
#ifndef _DIRVER_NETDEV_H
#define _DIRVER_NETDEV_H

#include "klc/klc_syshelper.h"

#ifdef __cplusplus
extern "C"
{
#endif

static inline void * _DriverKlc_debugfs_create_dir(char *name)
{
    u64 ret;
    ret = klc_sys_call(0, debugfs_create_dir, name);
    return (void*)ret;
}

#define DriverKlc_debugfs_create_dir(name) ({ \
        char _name[] = name; \
        _DriverKlc_debugfs_create_dir(_name); \
    })

static inline void DriverKlc_debugfs_remove_recursive(void *ddir)
{
	klc_sys_call(0, debugfs_remove_recursive, ddir);
}

static inline int DriverKlc_bus_register(void *bus)
{
    return klc_sys_call(-1, bus_register, bus);
}

static inline void DriverKlc_bus_unregister(void *bus)
{
    klc_sys_call(0, bus_unregister, bus);
}

static inline int DriverKlc_rtnl_link_register(void *link_ops)
{
	return klc_sys_call(-1, rtnl_link_register, link_ops);
}

static inline void DriverKlc_rtnl_link_unregister(void *link_ops)
{
	klc_sys_call(0, rtnl_link_unregister, link_ops);
}

static inline int DriverKlc_register_pernet_subsys(void *devlink_net_ops)
{
	return klc_sys_call(-1, register_pernet_subsys, devlink_net_ops);
}

static inline void DriverKlc_unregister_pernet_subsys(void *devlink_net_ops)
{
	klc_sys_call(0, unregister_pernet_subsys, (long)devlink_net_ops);
}

static inline void DriverKlc_device_unregister(void *dev)
{
	klc_sys_call(0, device_unregister, (long)dev);
}

static inline void DriverKlc_ether_setup(void *dev)
{
	klc_sys_call(0, ether_setup, (long)dev);
}

static inline void * DriverKlc___dev_get_by_index(void *net, int ifindex)
{
    return (void*)(long)klc_sys_call(0, __dev_get_by_index, net, ifindex);
}

static inline int DriverKlc_register_netdevice(void *dev)
{
    u64 ret = klc_sys_call(0, register_netdevice, (long)dev);
    if (ret == KLC_RET_ERR) {
        return -1;
    }
    return ret;
}

static inline void DriverKlc_unregister_netdevice_queue(void *dev, void *head)
{
    klc_sys_call(0, unregister_netdevice_queue, dev, head);
}

static inline void DriverKlc_get_random_bytes(void *buf, int nbytes)
{
    klc_sys_call(0, get_random_bytes, buf, nbytes);
}

static inline void DriverKlc_eth_random_addr(u8 *addr)
{
	DriverKlc_get_random_bytes(addr, ETH_ALEN);
	addr[0] &= 0xfe;	
	addr[0] |= 0x02;	
}

static inline void DriverKlc_eth_hw_addr_random(struct net_device *dev)
{
	dev->addr_assign_type = NET_ADDR_RANDOM;
	DriverKlc_eth_random_addr(dev->dev_addr);
}

#ifdef __cplusplus
}
#endif
#endif 
