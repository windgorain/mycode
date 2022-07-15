/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2012-11-23
* Description: 
* History:     
******************************************************************************/

#ifndef __IN_MCAST_H_
#define __IN_MCAST_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

/*
 * The imo_membership vector for each socket is now dynamically allocated at
 * run-time, bounded by USHRT_MAX, and is reallocated when needed, sized
 * according to a power-of-two increment.
 */
#define IP_MIN_MEMBERSHIPS  31
#define IP_MAX_MEMBERSHIPS  (32 * 1024 - 1)
#define IP_MAX_SOURCE_FILTER    1024    /* # of filters per socket, per group */



union sockunion {
    struct sockaddr_storage    ss;
    struct sockaddr        sa;
    struct sockaddr_in    sin;
    struct sockaddr_in6    sin6;
};
typedef union sockunion sockunion_t;

/*
 * Multicast source list entry.
 */
typedef struct in_msource {
    TAILQ_ENTRY(in_msource) ims_next;   /* next source */
    struct sockaddr_storage ims_addr;   /* address of this source */
}IN_MSOURCE_S;

/*
 * Internet multicast address structure.
 * We only record multicast address & if index.
 * Support IPv4 & IPv6
 */
typedef struct in_multi
{
    union
    {
        /* IP multicast address entry, convenience */
        struct    in_addr_4in6 ie46_multi;
        IN6_ADDR_S ie6_multi;
    } inm_dependfaddr;
    UINT inm_ifindex;       /* J03845: ifnet index */
#define    inm_addr    inm_dependfaddr.ie46_multi.ia46_addr4
#define    in6m_addr    inm_dependfaddr.ie6_multi
}IN_MULTI_S;


/*
 * Filter modes; also used to represent per-socket filter mode internally.
 */
#define MCAST_UNDEFINED 0   /* fmode: not yet defined */
#define MCAST_INCLUDE   1   /* fmode: include these source(s) */
#define MCAST_EXCLUDE   2   /* fmode: exclude these source(s) */


/*
 * Argument structure for IP_ADD_MEMBERSHIP and IP_DROP_MEMBERSHIP.
 */
struct ip_mreq {
    struct in_addr imr_multiaddr;  /* IP multicast address of group */
    IF_INDEX imr_interface;  /* J03845: interface index */
};

/*
 * Modified argument structure for IP_MULTICAST_IF, obtained from Linux.
 * This is used to specify an interface index for multicast sends, as
 * the IPv4 legacy APIs do not support this (unless IP_SENDIF is available).
 */
struct ip_mreqn {
    UINT imr_multiaddr;  /* IP multicast address of group */
    UINT imr_address;    /* local IP address of interface */
    IF_INDEX imr_ifindex;    /* Interface index; cast to uint32_t */
};

/*
 * Argument structure for IPv4 Multicast Source Filter APIs. [RFC3678]
 */
struct ip_mreq_source {
    UINT imr_multiaddr;  /* IP multicast address of group */
    UINT imr_sourceaddr; /* IP address of source */
    IF_INDEX imr_interface;  /* J03845: interface index */
};

/*
 * Argument structures for Protocol-Independent Multicast Source
 * Filter APIs. [RFC3678]
 */
struct group_req {
    IF_INDEX        gr_interface;   /* interface index */
    struct sockaddr_storage gr_group;   /* group address */
};

/*
 * Multicast filter descriptor; there is one instance per group membership
 * on a socket, allocated as an expandable vector hung off ip_moptions.
 * struct in_multi contains separate IPv4-stack-wide state for IGMPv3.
 */
typedef struct in_mfilter
{
    USHORT    imf_fmode;      /* filter mode for this socket/group */
    USHORT    imf_nsources;   /* # of sources for this socket/group */
    TAILQ_HEAD(, in_msource) imf_sources;   /* source list */
}IN_MFILTER_S;

/*
 * Structure attached to inpcb.ip_moptions and
 * passed to ip_output when IP multicast options are in use.
 * This structure is lazy-allocated.
 */
typedef struct ip_moptions {
    UINT   imo_multicast_if;            /* J03845: ifindex of outgoing multicasts */
    UCHAR  imo_multicast_ttl;           /* TTL for outgoing multicasts */
    UCHAR  imo_multicast_loop;          /* 1 => hear sends if a member */
    USHORT imo_num_memberships;         /* no. memberships this socket */
    USHORT imo_max_memberships;         /* max memberships this socket */
    USHORT imo_msmthcursor;             /* G03597: next mcast data index to be collected(for smooth). -1 is init */
    USHORT imo_mpullcursor;             /* G03597: next mcast data index to be collected(for pull). -1 is init */
    IN_MULTI_S **imo_membership;        /* group memberships */
    IN_MFILTER_S *imo_mfilters;         /* source filters */
}IP_MOPTIONS_S;

struct group_source_req {
    IF_INDEX        gsr_interface;  /* interface index */
    struct sockaddr_storage gsr_group;  /* group address */
    struct sockaddr_storage gsr_source; /* source address */
};

UINT imo_match_group(IN IP_MOPTIONS_S *imo, IN UINT ifindex, IN SOCKADDR_S *group);
IN_MSOURCE_S * imo_match_source(IN IP_MOPTIONS_S *imo, IN UINT gidx, IN SOCKADDR_S *src);
void inp_freemoptions(IN IP_MOPTIONS_S *imo);
int inp_setmoptions(IN INPCB_S *inp, IN SOCKOPT_S *sopt);
int inp_getmoptions(struct inpcb *inp, struct sockopt *sopt);

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__IN_MCAST_H_*/


