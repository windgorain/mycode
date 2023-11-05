/* SPDX-License-Identifier: (LGPL-2.1 OR BSD-2-Clause) */

/*
 * NETLINK      Netlink attributes
 *
 * Copyright (c) 2003-2013 Thomas Graf <tgraf@suug.ch>
 */

#ifndef __LIBBPF_NLATTR_H
#define __LIBBPF_NLATTR_H

#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/genetlink.h>


#define __LINUX_NETLINK_H


enum {
	LIBBPF_NLA_UNSPEC,	
	LIBBPF_NLA_U8,		
	LIBBPF_NLA_U16,		
	LIBBPF_NLA_U32,		
	LIBBPF_NLA_U64,		
	LIBBPF_NLA_STRING,	
	LIBBPF_NLA_FLAG,	
	LIBBPF_NLA_MSECS,	
	LIBBPF_NLA_NESTED,	
	__LIBBPF_NLA_TYPE_MAX,
};

#define LIBBPF_NLA_TYPE_MAX (__LIBBPF_NLA_TYPE_MAX - 1)


struct libbpf_nla_policy {
	
	uint16_t	type;

	
	uint16_t	minlen;

	
	uint16_t	maxlen;
};

struct libbpf_nla_req {
	struct nlmsghdr nh;
	union {
		struct ifinfomsg ifinfo;
		struct tcmsg tc;
		struct genlmsghdr gnl;
	};
	char buf[128];
};


#define libbpf_nla_for_each_attr(pos, head, len, rem) \
	for (pos = head, rem = len; \
	     nla_ok(pos, rem); \
	     pos = nla_next(pos, &(rem)))


static inline void *libbpf_nla_data(const struct nlattr *nla)
{
	return (void *)nla + NLA_HDRLEN;
}

static inline uint8_t libbpf_nla_getattr_u8(const struct nlattr *nla)
{
	return *(uint8_t *)libbpf_nla_data(nla);
}

static inline uint16_t libbpf_nla_getattr_u16(const struct nlattr *nla)
{
	return *(uint16_t *)libbpf_nla_data(nla);
}

static inline uint32_t libbpf_nla_getattr_u32(const struct nlattr *nla)
{
	return *(uint32_t *)libbpf_nla_data(nla);
}

static inline uint64_t libbpf_nla_getattr_u64(const struct nlattr *nla)
{
	return *(uint64_t *)libbpf_nla_data(nla);
}

static inline const char *libbpf_nla_getattr_str(const struct nlattr *nla)
{
	return (const char *)libbpf_nla_data(nla);
}


static inline int libbpf_nla_len(const struct nlattr *nla)
{
	return nla->nla_len - NLA_HDRLEN;
}

int libbpf_nla_parse(struct nlattr *tb[], int maxtype, struct nlattr *head,
		     int len, struct libbpf_nla_policy *policy);
int libbpf_nla_parse_nested(struct nlattr *tb[], int maxtype,
			    struct nlattr *nla,
			    struct libbpf_nla_policy *policy);

int libbpf_nla_dump_errormsg(struct nlmsghdr *nlh);

static inline struct nlattr *nla_data(struct nlattr *nla)
{
	return (struct nlattr *)((void *)nla + NLA_HDRLEN);
}

static inline struct nlattr *req_tail(struct libbpf_nla_req *req)
{
	return (struct nlattr *)((void *)req + NLMSG_ALIGN(req->nh.nlmsg_len));
}

static inline int nlattr_add(struct libbpf_nla_req *req, int type,
			     const void *data, int len)
{
	struct nlattr *nla;

	if (NLMSG_ALIGN(req->nh.nlmsg_len) + NLA_ALIGN(NLA_HDRLEN + len) > sizeof(*req))
		return -EMSGSIZE;
	if (!!data != !!len)
		return -EINVAL;

	nla = req_tail(req);
	nla->nla_type = type;
	nla->nla_len = NLA_HDRLEN + len;
	if (data)
		memcpy(nla_data(nla), data, len);
	req->nh.nlmsg_len = NLMSG_ALIGN(req->nh.nlmsg_len) + NLA_ALIGN(nla->nla_len);
	return 0;
}

static inline struct nlattr *nlattr_begin_nested(struct libbpf_nla_req *req, int type)
{
	struct nlattr *tail;

	tail = req_tail(req);
	if (nlattr_add(req, type | NLA_F_NESTED, NULL, 0))
		return NULL;
	return tail;
}

static inline void nlattr_end_nested(struct libbpf_nla_req *req,
				     struct nlattr *tail)
{
	tail->nla_len = (void *)req_tail(req) - (void *)tail;
}

#endif 
