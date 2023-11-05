/*================================================================
* Author：LiXingang. Data: 2019.08.19
* Description: 路由变化通知
*
================================================================*/
#include "bs.h"

#define RT_BUFFER_LEN       8192    

#if 0
static void parse_nlmsg(struct nlmsghdr *hdr)
{
	simple_rt rt;
	struct rtmsg *rtm;
	int attr_len;
	char devname[IFNAMSIZ];
	char *vlanname = NULL;
	int vlanid;
	int i, j;

	memset(&rt, 0, sizeof(rt));
	attr_len = hdr->nlmsg_len - NLMSG_LENGTH(sizeof(struct rtmsg));
	if (attr_len < 0) {
		printf("attr_len %d wrong, exit!\n", attr_len);
		return;
	}

	rtm = NLMSG_DATA(hdr);
	if (rtm->rtm_family != AF_INET) {
		return;
	}
	if ((rtm->rtm_table != 255) && (rtm->rtm_table != 254)) {
		return;
	}

	switch (hdr->nlmsg_type) {
	case RTM_NEWROUTE:
	case RTM_DELROUTE:
		rtm = NLMSG_DATA(hdr);
		convert_rtm_to_rt(rtm, &rt, attr_len);
		
		break;
	case RTM_NEWNEIGH:
	case RTM_DELNEIGH:
	default:
		return;
	}

	if (IPADDR_A(rt.dst_ip.ip4) == 127)
		return;

	for (i = 0; i < rt.nh_cnt; i++) {
		if_indextoname(rt.nh[i].out_dev, devname);

		vlanname = NULL;
		for (j = 0; j < NF_MONITOR_DEV_MAX; j ++) {
			if (config_opt.monitor_info[j].type != NF_MONITOR_TYPE_ROUTE)
				continue;
			if (!strncmp(config_opt.monitor_info[j].linux_dev, devname, IFNAMSIZ)) {
				vlanname = config_opt.monitor_info[j].nf_dev;
				break;
			}
		}

		if (vlanname == NULL)
			return;

		vlanid = se_get_vlanid_by_name(vlanname);
		if (vlanid < 0)
			return;
		rt.nh[i].out_dev = vlanid;
	}


	if (rtm->rtm_type == RTN_LOCAL || rtm->rtm_type == RTN_UNICAST) {
		rt_send_to_dpdk(hdr->nlmsg_type, &rt);
	}

	return;
}


int RouteNotify_Run()
{
    int fd;
    struct sockaddr_nl sn;
    struct sockaddr_nl sc;
    int ret;
    char buffer[RT_BUFFER_LEN];
	struct iovec iov = {buffer, RT_BUFFER_LEN };
	struct msghdr msghr = { (struct sockaddr *) &sc,
        sizeof(sc), &iov, 1, NULL, 0, 0 };

	fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
	if (fd < 0) {
		printf("rt sk create error %d\n", rsk.fd);
		return -1;
	}

	memset(&sn, 0, sizeof(struct sockaddr_nl));
	sn.nl_family = PF_NETLINK;
	sn.nl_pid = getpid();
	sn.nl_groups = RTMGRP_IPV4_ROUTE;

	ret = bind(rsk.fd, (struct sockaddr *)&sn, sizeof(struct sockaddr_nl));
	if (ret < 0) {
		printf("bind sk error ret %d!\n", ret);
		close(fd);
		return ret;
	}

    while (1) {
        len = recvmsg(rsk.fd, &msghr, 0);
		if (len < 0)
			continue;

		parse_nlmsg((struct nlmsghdr *)buffer);
	}

	close(fd);

    return 0;
}
#endif

