/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2012-12-2
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/mbuf_utl.h"
#include "utl/ip_utl.h"
#include "utl/in_checksum.h"
#include "inet/in_pub.h"

#include "protosw.h"
#include "domain.h"
#include "in.h"
#include "in_ipaddr.h"
#include "udp_func.h"
#include "ip_options.h"

#include "in_pcb.h"
#include "in_mcast.h"
#include "icmp.h"
#include "inet_ip.h"
#include "inet6_ip.h"

#define INP_LOOKUP_MAPPED_PCB_COST    4

int ip6_v6only = 1;   /* 缺省不支持IPv6和IPv4兼容 */
int ip6_auto_flowlabel = 0; /* 缺省不支持自动生成流标签 */

/*
传输层发送报文时，如果上层没有指定报文的源地址，调用此接口获取一个合适的地址。
获取源地址的原则是:
1. 从上层指定的接口上获取
2. 根据目的地址查找FIB，使用FIB表项中的Local地址
3. 如果还没有选择到，则在指定的VRF内选择一个地址
*/
UINT in_selectsrc
(
    IN INPCB_S *inp,
    IN SND_IF_S *sndif,
    IN IP_MOPTIONS_S *ipmoptions,
    IN VRF_INDEX svrf,
    IN UINT dst
)
{
    UINT srcaddr;
    UINT nexthopaddr;
    UINT dstaddr;
    IF_INDEX outif = 0;
    VRF_INDEX vrf;
    IPADDR_INFO_S ipaddrinfo;
    ULONG ret;
    
    srcaddr = INADDR_ANY;
    nexthopaddr = INADDR_ANY;
    dstaddr = dst;
    
    BS_DBGASSERT(NULL != inp);
    /*
     * 获取VRF Index。首先获取上层指定的接收VRF，如果没有指定接收VRF，
     * 则使用发送VRF
     */
    vrf = inp->inp_rvrf;
    if (VRF_ANY == vrf)
    {
        vrf = svrf;
    }

    if ((NULL == inp->inp_socket) || (0 == (inp->inp_socket->so_options & SO_DONTROUTE))) {
        /*
         * 如果上层已经通过出接口选项指定了本地地址，直接使用此地址。
         * 如果已经指定了下一跳的情况，则在接口上获取一个最匹配的地址。
         */
        if (NULL != sndif) 
        {
            if (INADDR_ANY != sndif->si_lcladdr) 
            {
                return sndif->si_lcladdr;
            }
        }

        ret = IN_IPAddr_SelectSrcAddr(dstaddr, vrf, outif, nexthopaddr, &srcaddr);
    }
    else 
    {
        memset(&ipaddrinfo, 0, sizeof(IPADDR_INFO_S));
        /*
         * 在VRF中查找和目的地址网段匹配的地址
         */
        ret = IN_IPAddr_GetAddrInVrf(vrf, dstaddr, IPADDR_TYPE_FWD, &ipaddrinfo);
        if (BS_OK != ret) 
        {
            ret = IN_IPAddr_MatchBestNetInVrf(vrf, dstaddr, IPADDR_TYPE_FWD, &ipaddrinfo);
        }
        
        if (BS_OK == ret) 
        {
            srcaddr = ipaddrinfo.stIPAddrKey.uiIPAddr;
        }
    }
    
    /* 如果此时还没有选择到合适的地址，只能从VRF内任意选择一个地址了 */
    if (INADDR_ANY == srcaddr) 
    {
        memset(&ipaddrinfo, 0, sizeof(IPADDR_INFO_S));
        /*
         * 在VRF内找一个任意地址。
         */
        (void)IN_IPAddr_GetFwdAddrInVrf(vrf, &ipaddrinfo);
        srcaddr = ipaddrinfo.stIPAddrKey.uiIPAddr;
    }

    /*
     * If the destination address is multicast and an outgoing
     * interface has been set as a multicast option, use the
     * address of that interface as our source address.
     */
    if (IN_MULTICAST(ntohl(dstaddr))) 
    {
        if ((ipmoptions != NULL) && (0 != ipmoptions->imo_multicast_if))
        {
            outif = ipmoptions->imo_multicast_if;
            /* 在接口上获取一个合适的地址 */
            memset(&ipaddrinfo, 0, sizeof(IPADDR_INFO_S));
            ret = IN_IPAddr_GetPriorAddr(outif, IPADDR_TYPE_FWD, &ipaddrinfo);
            if (BS_OK == ret) 
            {
                srcaddr = ipaddrinfo.stIPAddrKey.uiIPAddr;
            }
        }
    }

    return srcaddr;
}

/* IPv4 INPCB字典序比较函数, 比较本地地址、本地端口 */
STATIC INT in_pcb4_listen_cmp(IN IN_CONNINFO_S *pstInc1, IN IN_CONNINFO_S *pstInc2)
{
    INT iRet = 0;
    iRet = memcmp(&(pstInc1->inc_laddr), &(pstInc2->inc_laddr), sizeof(UINT));
    if (0 == iRet)
    {
        iRet = memcmp(&(pstInc1->inc_lport), &(pstInc2->inc_lport), sizeof(USHORT));
    }
   
    return iRet;
}

/* IPv6 LISTEN INPCB字典序比较函数, 比较本地地址、本地端口 */
STATIC INT in_pcb6_listen_cmp(struct in_conninfo *pstInc1, struct in_conninfo *pstInc2)
{
    INT iRet = 0;
    iRet = memcmp(&(pstInc1->inc6_laddr), &(pstInc2->inc6_laddr), sizeof(struct in6_addr));
    if (0 == iRet)
    {
        iRet = memcmp(&(pstInc1->inc_lport), &(pstInc2->inc_lport), sizeof(USHORT));
    }

    return iRet;
}

/* INPCB字典序比较函数, 先比较地址类型，然后比较本地地址、本地端口 */
INT in_pcb_listen_cmp(struct inpcb *pstInPcb1, struct inpcb *pstInPcb2)
{
    UCHAR ucVflag1, ucVflag2;
    INT iRet = 0;
    
    ucVflag1 = pstInPcb1->inp_vflag & INP_IPV6PROTO;
    ucVflag2 = pstInPcb2->inp_vflag & INP_IPV6PROTO;
    
    if (ucVflag1 == ucVflag2) {
        if (INP_IPV6PROTO != ucVflag1) {
            /* IPv4 address compare. */
            iRet = in_pcb4_listen_cmp(&pstInPcb1->inp_inc, &pstInPcb2->inp_inc);
        } else {
            /* IPv6 address compare. */
            iRet = in_pcb6_listen_cmp(&pstInPcb1->inp_inc, &pstInPcb2->inp_inc);
        }
    } else if (INP_IPV6PROTO != ucVflag1) {
        iRet = -1;
    } else {
        iRet = 1;
    }

    return iRet;
}

/* IPv4 INPCB字典序比较函数, 比较本地地址、本地端口、对端地址、对端端口 */
STATIC INT in_pcb4cmp(struct in_conninfo *pstInc1, struct in_conninfo *pstInc2)
{
    INT iRet = 0;
    iRet = memcmp(&(pstInc1->inc_laddr), &(pstInc2->inc_laddr), sizeof(UINT));
    if (0 == iRet)
    {
        iRet = memcmp(&(pstInc1->inc_lport), &(pstInc2->inc_lport), sizeof(USHORT));
    }
    if (0 == iRet)
    {
        iRet = memcmp(&(pstInc1->inc_faddr), &(pstInc2->inc_faddr), sizeof(UINT));    
    }
    if (0 == iRet)
    {
        iRet = memcmp(&(pstInc1->inc_fport), &(pstInc2->inc_fport), sizeof(USHORT));
    }    

    return iRet;
}

/* IPv6 INPCB字典序比较函数, 比较本地地址、本地端口、对端地址、对端端口 */
STATIC INT in_pcb6cmp(struct in_conninfo *pstInc1, struct in_conninfo *pstInc2)
{
    INT iRet = 0;
    iRet = memcmp(&(pstInc1->inc6_laddr), &(pstInc2->inc6_laddr), sizeof(struct in6_addr));
    if (0 == iRet)
    {
        iRet = memcmp(&(pstInc1->inc_lport), &(pstInc2->inc_lport), sizeof(USHORT));
    }
    if (0 == iRet)
    {
        iRet = memcmp(&(pstInc1->inc6_faddr), &(pstInc2->inc6_faddr), sizeof(struct in6_addr));    
    }
    if (0 == iRet)
    {
        iRet = memcmp(&(pstInc1->inc_fport), &(pstInc2->inc_fport), sizeof(USHORT));
    }    

    return iRet;
}

/* INPCB字典序比较函数, 先比较地址类型，然后比较本地地址、本地端口、对端地址、对端端口 */
INT in_pcbcmp(struct inpcb *pstInPcb1, struct inpcb *pstInPcb2)
{
    UCHAR ucVflag1, ucVflag2;
    INT iRet = 0;
    
    ucVflag1 = pstInPcb1->inp_vflag & INP_IPV6PROTO;
    ucVflag2 = pstInPcb2->inp_vflag & INP_IPV6PROTO;
    
    if (ucVflag1 == ucVflag2) {
        if (INP_IPV6PROTO != ucVflag1) {
            /* IPv4 address compare. */
            iRet = in_pcb4cmp(&pstInPcb1->inp_inc, &pstInPcb2->inp_inc);
        } else {
            /* IPv6 address compare. */
            iRet = in_pcb6cmp(&pstInPcb1->inp_inc, &pstInPcb2->inp_inc);
        }
    } else if (INP_IPV6PROTO != ucVflag1) {
        iRet = -1;
    } else {
        iRet = 1;
    }

    return iRet;
}

/* 创建TCP、UDP或RawIP类型的Socket时，需要创建Socket对象对应的协议控制块，调用本接口完成协议控制块的生成 */
int in_pcballoc(IN SOCKET_S *so, IN INPCB_INFO_S *pcbinfo)
{
    struct inpcb *inp = NULL;
    int error;
    enum inpcbversion inver = INV4;

    INP_INFO_WLOCK_ASSERT(pcbinfo);
    error = 0;
    inp = (struct inpcb *)MEM_ZMalloc(sizeof(INPCB_S));
    if (inp == NULL)
    {
        return (ENOBUFS);
    }
    inp->inp_pcbinfo = pcbinfo;
    inp->inp_socket = so;

    /*
     * J03845: init send_vrf and receive_vrf to be VRF_ANY.
     */
    inp->inp_svrf = inp->inp_rvrf = VRF_ANY;

    if (INP_SOCKAF(so) == AF_INET6)
    {
        inver = INV6;
        inp->inp_vflag |= INP_IPV6PROTO;

        /* 
         * Note: IN6P_IPV6_V6ONLY is default configuration
         */
        if (0 != ip6_v6only)
        {
            inp->inp_flags |= IN6P_IPV6_V6ONLY;
        }
    }

    LIST_INSERT_HEAD(pcbinfo->ipi_listhead, inp, inp_list);
    pcbinfo->ipi_count[inver]++;
    so->so_pcb = inp;

    if (0 != ip6_auto_flowlabel)
    {
        inp->inp_flags |= IN6P_AUTOFLOWLABEL;
    }
    INP_LOCK(inp);

    return (error);
}

/*  TCP或UDP进行bind操做时，调用本接口，实现地址和端口的绑定 */
int in_pcbbind(IN INPCB_S *inp, IN SOCKADDR_S *nam)
{
    int anonport, error;

    INP_INFO_WLOCK_ASSERT(inp->inp_pcbinfo);
    INP_LOCK_ASSERT(inp);

    if (inp->inp_lport != 0 || inp->inp_laddr != INADDR_ANY)
    {
        return (EINVAL);
    }

    anonport = (int)(inp->inp_lport == 0 && (nam == NULL || ((struct sockaddr_in *)nam)->sin_port == 0));

    error = in_pcbbind_setup(inp, nam, &inp->inp_rvrf, &inp->inp_laddr, &inp->inp_lport);
    if (error > 0)
    {
        return (error);
    }
    if (in_pcbinshash(inp) != 0)
    {
        inp->inp_laddr = INADDR_ANY;
        inp->inp_lport = 0;
        inp->inp_rvrf = VRF_ANY;
        return (EAGAIN);
    }

    if (anonport > 0)
    {
        inp->inp_flags |= INP_ANONPORT;
    }
    return (0);
}

int in_pcbbind_check
(
    INPCB_INFO_S *pcbinfo,
    UINT laddr,
    USHORT lport, 
    VRF_INDEX rvrf,
    UINT faddr,
    INT inflags,
    UCHAR vflag
)
{
    int wild = 0;
    int reuseport = (inflags & INP_REUSEPORT);
    enum inpcbversion inver;
    struct inpcb *inp;

    if ((inflags & (INP_REUSEADDR | INP_REUSEPORT)) == 0)
    {
        wild = INPLOOKUP_WILDCARD;
    }

    /* 此时port肯定不为0 */
    BS_DBGASSERT(0 != lport);

    if (0 != (vflag & INP_IPV6PROTO))
    {
        inver = INV6;
    }
    else
    {
        inver = INV4;
    }

    /* NB: lport is left as 0 if the port isn't being changed. */
    if (IN_MULTICAST(ntohl(laddr)))
    {
        /*
         * Treat SO_REUSEADDR as SO_REUSEPORT for multicast;
         * allow complete duplication of binding if
         * SO_REUSEPORT is set, or if SO_REUSEADDR is set
         * and a multicast address is bound on both
         * new and duplicated sockets.
         */
        if (0 != (inflags & INP_REUSEADDR))
        {
            reuseport = INP_REUSEADDR | INP_REUSEPORT;
        }
    }

    inp = in_pcblookup_local(pcbinfo, rvrf, laddr, lport, faddr, wild);

    /*
     * 删除BSD中if分支的原因:
     * 连接处于timewait状态后会释放socket, 释放后so_options保存在timewait里面,
     * 这里为了获取到so_options, 必须通过inpcb反查到timewait去获取so_options,
     * 现在我们inpcb里面直接记录了so_options, 这个处理就没有必要了
     */
    if ((inp != NULL) && (reuseport & inp->inp_flags) == 0)
    {
        return (EADDRINUSE);
    }

    return (0);
}

int in_pcbbind_setup
(
    INPCB_S *inp,
    SOCKADDR_S *nam,
    VRF_INDEX *rvrfp,
    UINT *laddrp,
    USHORT *lportp
)
{
    struct socket *so = inp->inp_socket;
    struct sockaddr_in *sin;
    struct inpcbinfo *pcbinfo = inp->inp_pcbinfo;
    UINT laddr;
    UINT faddr = inp->inp_faddr;
    USHORT lport = 0;
    int error;
    int result = -1;
    VRF_INDEX rvrf = *rvrfp;

    INP_INFO_WLOCK_ASSERT(pcbinfo);
    INP_LOCK_ASSERT(inp);

    laddr = *laddrp;
    if (nam != NULL && laddr != INADDR_ANY) {
        return (EINVAL);
    }

    if (0 != (so->so_options & SO_REUSEADDR)) 
    {
        inp->inp_flags |= INP_REUSEADDR;
    }

    /* 
     * 指定端口bind 
     */
    if (NULL != nam)
    {
        sin = (struct sockaddr_in *)nam;
        if (nam->sa_len < sizeof (*sin))
        {
            return (EINVAL);
        }

        /*
         * We should check the family, but old programs
         * incorrectly fail to initialize it.
         */
        if (sin->sin_family != AF_INET)
        {
            return (EAFNOSUPPORT);
        }

        if (sin->sin_port != *lportp)
        {
            /* Don't allow the port to change. */
            if (*lportp != 0)
            {
                return (EINVAL);
            }
            lport = sin->sin_port;
        }
        
        /* 需要检查一下VRF，不允许VRF变化 */
        if (sin->sin_vrf != *rvrfp)
        {
            if (*rvrfp != VRF_ANY)
            {
                return (EINVAL);
            }
            rvrf = sin->sin_vrf;
        }

        laddr = sin->sin_addr;

        /*
         * 如果应用程序指定的本地端口不为0，需要进行合法性检查。
         */
        if (0 != lport)
        {
            error = in_pcbbind_check(pcbinfo, laddr, lport, rvrf, faddr, inp->inp_flags, inp->inp_vflag);
            if (error != 0)
            {
                return (error);
            }
        }
    }

    if (*lportp != 0)
    {
        lport = *lportp;
    }

    /* 
     * 动态端口bind 
     */
    if (0 == lport)
    {
        USHORT uslastport = 1024;
        do {
			if (uslastport < 1024)
				return (EADDRNOTAVAIL);
			++uslastport;
			lport = htons(uslastport);
		} while (in_pcblookup_local(pcbinfo, rvrf, laddr, lport, 0, 0));
    }

    *laddrp = laddr;
    *lportp = lport;
    *rvrfp = rvrf;
    return (0);
}

/* 把输入的inpcb加入到全局协议控制信息中的哈希表中 */
int in_pcbinshash(struct inpcb *inp)
{
    INPCB_HEAD_S *pcbhash;
    INPCB_PORT_HEAD_S *pcbporthash;
    struct inpcbinfo *pcbinfo = inp->inp_pcbinfo;
    struct inpcbport *phd;
    UINT hashkey_faddr;
    enum inpcbversion inver;

    INP_INFO_WLOCK_ASSERT(pcbinfo);
    INP_LOCK_ASSERT(inp);

    if (0 != (inp->inp_vflag & INP_IPV6))
    {
        hashkey_faddr = inp->in6p_faddr.si6_addr32[3];
    }
    else
    {
        hashkey_faddr = inp->inp_faddr;
    }

    pcbhash = &pcbinfo->ipi_hashbase[
        INP_PCBHASH(hashkey_faddr, inp->inp_lport, inp->inp_fport, pcbinfo->ipi_hashmask)];

    pcbporthash = &pcbinfo->ipi_porthashbase[
        INP_PCBPORTHASH(inp->inp_lport, pcbinfo->ipi_porthashmask)];

    /*
     * Go through port list and look for a head for this lport.
     */
    LIST_FOREACH(phd, pcbporthash, phd_hash)
    {
        if (phd->phd_port == inp->inp_lport)
        {
            break;
        }
    }
    /*
     * If none exists, malloc one and tack it on.
     */
    if (phd == NULL) {
        phd = (struct inpcbport *)MEM_ZMalloc(sizeof(INPCB_PORT_S));
        if (phd == NULL)
        {
            return (ENOBUFS);
        }
        phd->phd_port = inp->inp_lport;
        LIST_INIT(&phd->phd_pcblist);
        LIST_INSERT_HEAD(pcbporthash, phd, phd_hash);
    }
    inp->inp_phd = phd;
    LIST_INSERT_HEAD(&phd->phd_pcblist, inp, inp_portlist);
    LIST_INSERT_HEAD(pcbhash, inp, inp_hash);
    
    if (0 != (inp->inp_vflag & INP_IPV6PROTO))
    {
        inver = INV6;
    }
    else
    {
        inver = INV4;
    }

//    in_portrefreshport(&(pcbinfo->ipi_portinfo[inver]), inp->inp_lport);

    return (0);
}

/*
实现inpcb的查找功能，根据指定的类型可实现精确或部分匹配查找.
目前的查找原则是：
1.(lp,la,*,*),则返回查找到inpcb;
2.(lp,*,*,*),需要进一步查找是否有更精确的匹配,没有则返回该inpcb;
后续需要支持TCP绑定远端peer地址的应用(可以防止land攻击,也可实现
不同邻居的同一端口的分别侦听)，需要增加如下查找规则：
3.(LA,LP;FA,*),则返回查找到inpcb
4.(*,LP,FA,*),需要进一步查找是否有更精确的匹配,没有则返回该inpcb
*/
INPCB_S * in_pcblookup_hash
(
    IN INPCB_INFO_S *pcbinfo, 
    IN UINT faddr,
    IN USHORT fport_arg, 
    IN VRF_INDEX rvrf,
    IN UINT laddr,
    IN USHORT lport_arg, 
    IN int wildcard
)
{
    INPCB_HEAD_S *head;
    INPCB_S *inp;
    USHORT fport = fport_arg, lport = lport_arg;
    int wild = 0;
    int wildmatch = INP_LOOKUP_MAPPED_PCB_COST;

    /*
     * First look for an exact match.
     */
    head = &pcbinfo->ipi_hashbase[INP_PCBHASH(faddr, lport, fport, pcbinfo->ipi_hashmask)];
    LIST_FOREACH(inp, head, inp_hash)
    {
        if ((inp->inp_vflag & INP_IPV4) == 0) {
            continue;
        }
        if (inp->inp_faddr == faddr &&
            inp->inp_laddr == laddr &&
            inp->inp_fport == fport &&
            inp->inp_lport == lport &&
            inp->inp_rvrf == rvrf)
        {
            return (inp);
        }
    }

    /*
     * Then look for a wildcard match, if requested.
     */
    if (wildcard > 0) {
        INPCB_PORT_HEAD_S *porthash;
        INPCB_PORT_S *phd;
        INPCB_S *match = NULL;
        
        /*
         * J03845: 在BSD中，查找的是三元组HASH，并将对端地址和对端端口都置为
         * 0，即查找的是侦听SOCKET。在Leopard中，因为支持了绑定对端地址，不能
         * 简单查找三元组HASH(如果要查找三元组HASH的话，需要查找两次，一次将对
         * 端地址设置为实际值，一次为0)，修改为查找端口HASH，并对对端地址、接收
         * VRF、本地地址进行通配处理。
         */
        porthash = &pcbinfo->ipi_porthashbase[INP_PCBPORTHASH(lport, pcbinfo->ipi_porthashmask)];
        LIST_FOREACH(phd, porthash, phd_hash)
        {
            if (phd->phd_port == lport)
            {
                break;
            }
        }
        
        if (phd != NULL)
        {
            /*
             * Port is in use by one or more PCBs. Look for best fit.
             */
            LIST_FOREACH(inp, &phd->phd_pcblist, inp_portlist)
            {
                wild = 0;
                if ((inp->inp_vflag & INP_IPV4) == 0)
                {
                    continue;
                }

                /*
                 * We never select the PCB that has
                 * INP_IPV6 flag and is bound to :: if
                 * we have another PCB which is bound
                 * to 0.0.0.0.  If a PCB has the
                 * INP_IPV6 flag, then we set its cost
                 * higher than IPv4 only PCBs.
                 *
                 * Note that the case only happens
                 * when a socket is bound to ::, under
                 * the condition that the use of the
                 * mapped address is allowed.
                 */
                if ((inp->inp_vflag & INP_IPV6) != 0)
                {
                    wildcard += INP_LOOKUP_MAPPED_PCB_COST;
                }
                
                /* 侦听socket对端端口应该为0 */
                if (inp->inp_fport != 0)
                {
                    continue;
                }

                if (inp->inp_rvrf != VRF_ANY)
                {
                    if (inp->inp_rvrf != rvrf)
                    {
                        continue;
                    }
                }
                else
                {
                    wild++;
                }
                
                if (inp->inp_laddr != INADDR_ANY)
                {
                    if (inp->inp_laddr != laddr)
                    {
                        continue;
                    }
                }
                else
                {
                    wild++;
                }

                if (inp->inp_faddr != INADDR_ANY)
                {
                    if (inp->inp_faddr != faddr)
                    {
                        continue;
                    }
                }
                else
                {
                    wild++;
                }

                /* 找一个最匹配的INPCB */
                if (wild < wildmatch)
                {
                    match = inp;
                    wildmatch = wild;
                    if (wildmatch == 0)
                    {
                        break;
                    }
                }
            }
        }
        
        return match;
    }

    return (NULL);
}

/*
实现inpcb的查找功能，根据指定的类型可实现精确或部分匹配查找
通过本地地址和端口号实现inpcb的最长匹配查找，具体原则如下：
1)精确匹配查找时，本地地址和端口号相等，且inpcb的远端连接地址为INADDR_ANY
 时，返回查找到inpcb
2)非精确查找时，找出最长匹配的inpcb
注意：本函数的查找不关注远端地址和端口号,inpcb的远端地址为INADDR_ANY，
即认为匹配，后续支持TCP绑定远端peer邻居地址时，需要关注该函数与
in_pcblookup_hash的应用情况
*/
INPCB_S * in_pcblookup_local
(
    IN INPCB_INFO_S *pcbinfo,
    IN VRF_INDEX rvrf,
    IN UINT laddr,
    IN USHORT lport,
    IN UINT faddr,
    int wild_okay
)
{
    INPCB_S *inp;
    int matchwild = INP_LOOKUP_MAPPED_PCB_COST + INP_LOOKUP_MAPPED_PCB_COST;
    int wildcard;
    UINT hashkey_faddr = faddr;

    INP_INFO_WLOCK_ASSERT(pcbinfo);

    if (0 == wild_okay)
    {
        INPCB_HEAD_S *head;
        
        /*
         * Look for an unconnected (wildcard foreign addr) PCB that
         * matches the local address and port we're looking for.
         */
        head = &pcbinfo->ipi_hashbase[INP_PCBHASH(hashkey_faddr, lport,
            0, pcbinfo->ipi_hashmask)];

        LIST_FOREACH(inp, head, inp_hash)
        {
            if ((inp->inp_vflag & INP_IPV4) == 0)
            {
                continue;
            }
            
            /*
             * 1. Receive vrf index should be checked.
             * 2. Foreign address should be checked.
             */
            if (inp->inp_faddr == faddr &&
                inp->inp_rvrf == rvrf &&
                inp->inp_laddr == laddr &&
                inp->inp_lport == lport)
            {
                /*
                 * Found.
                 */
                return (inp);
            }
        }
        /*
         * Not found.
         */
        return (NULL);
    }
    else
    {
        INPCB_PORT_HEAD_S *porthash;
        INPCB_PORT_S *phd;
        INPCB_S *match = NULL;
        /*
         * Best fit PCB lookup.
         *
         * First see if this local port is in use by looking on the
         * port hash list.
         */
        porthash = &pcbinfo->ipi_porthashbase[INP_PCBPORTHASH(lport,
            pcbinfo->ipi_porthashmask)];
        LIST_FOREACH(phd, porthash, phd_hash)
        {
            if (phd->phd_port == lport)
            {
                break;
            }
        }

        if (phd != NULL)
        {
            /*
             * Port is in use by one or more PCBs. Look for best
             * fit.
             */
            LIST_FOREACH(inp, &phd->phd_pcblist, inp_portlist)
            {
                wildcard = 0;
                if ((inp->inp_vflag & INP_IPV4) == 0)
                {
                    continue;
                }
                /*
                 * We never select the PCB that has
                 * INP_IPV6 flag and is bound to :: if
                 * we have another PCB which is bound
                 * to 0.0.0.0.  If a PCB has the
                 * INP_IPV6 flag, then we set its cost
                 * higher than IPv4 only PCBs.
                 *
                 * Note that the case only happens
                 * when a socket is bound to ::, under
                 * the condition that the use of the
                 * mapped address is allowed.
                 */
                if ((inp->inp_vflag & INP_IPV6) != 0)
                {
                    wildcard += INP_LOOKUP_MAPPED_PCB_COST;
                }
                if (inp->inp_faddr != INADDR_ANY)
                {
                    if (faddr == INADDR_ANY)
                    {
                        wildcard++;
                    }
                    else if (inp->inp_faddr != faddr)
                    {
                        continue;
                    }
                    else
                    {
                        /* go on */
                    }
                }
                else
                {
                    if (faddr != INADDR_ANY)
                    {
                        wildcard++;
                    }
                }

                /*
                 * Receive VRF index should be checked.
                 */
                if (inp->inp_rvrf != VRF_ANY)
                {
                    if (rvrf == VRF_ANY)
                    {
                        wildcard++;
                    }
                    else if (inp->inp_rvrf != rvrf)
                    {
                        continue;
                    }
                    else
                    {
                        /* go on */
                    }
                }
                else
                {
                    if (rvrf != VRF_ANY)
                    {
                        wildcard++;
                    }
                }

                if (inp->inp_laddr != INADDR_ANY)
                {
                    if (laddr == INADDR_ANY)
                    {
                        wildcard++;
                    }
                    else if (inp->inp_laddr != laddr)
                    {
                        continue;
                    }
                    else
                    {
                        /*just for remove warning*/
                    }
                }
                else
                {
                    if (laddr != INADDR_ANY)
                    {
                        wildcard++;
                    }
                }

                if (wildcard < matchwild)
                {
                    match = inp;
                    matchwild = wildcard;
                    if (matchwild == 0)
                    {
                        break;
                    }
                }
            }
        }
        return (match);
    }
}

void in_pcbrehash(IN INPCB_S *inp)
{
    INPCB_INFO_S *pcbinfo = inp->inp_pcbinfo;
    INPCB_HEAD_S *head;
    UINT hashkey_faddr;

    INP_INFO_WLOCK_ASSERT(pcbinfo);
    INP_LOCK_ASSERT(inp);

    if (0 != (inp->inp_vflag & INP_IPV6))
    {
        hashkey_faddr = inp->in6p_faddr.si6_addr32[3];
    }
    else
    {
        hashkey_faddr = inp->inp_faddr;
    }

    head = &pcbinfo->ipi_hashbase[INP_PCBHASH(hashkey_faddr,
        inp->inp_lport, inp->inp_fport, pcbinfo->ipi_hashmask)];

    LIST_REMOVE(inp, inp_hash);
    LIST_INSERT_HEAD(head, inp, inp_hash);
}

int in_pcbconnect_setup
(
    IN INPCB_S *inp,
    IN SOCKADDR_S *nam,
    IN VRF_INDEX *rvrfp,
    IN UINT *laddrp,
    IN USHORT *lportp,
    IN VRF_INDEX *svrfp,
    IN UINT *faddrp,
    IN USHORT *fportp,
    OUT INPCB_S **oinpp
)
{
    struct sockaddr_in *sin = (struct sockaddr_in *)nam;
    struct inpcb *oinp;
    UINT laddr, faddr;
    USHORT lport, fport;
    VRF_INDEX rvrf, svrf;
    int error;

    INP_INFO_WLOCK_ASSERT(inp->inp_pcbinfo);
    INP_LOCK_ASSERT(inp);

    if (oinpp != NULL)
    {
        *oinpp = NULL;
    }

    if (nam->sa_len < sizeof (*sin))
    {
        return (EINVAL);
    }

    if (sin->sin_family != AF_INET)
    {
        return (EAFNOSUPPORT);
    }
    
    if (sin->sin_port == 0)
    {
        return (EADDRNOTAVAIL);
    }
    
    /* 必须要指定发送VRF */
    if (sin->sin_vrf == VRF_ANY)
    {
        return (EINVAL);
    }
    laddr = *laddrp;
    lport = *lportp;
    rvrf = *rvrfp;
    faddr = sin->sin_addr;
    fport = sin->sin_port;
    svrf = sin->sin_vrf;

    /*
     * If user hasn't bind receive vrf index, set send vrf index
     * to receive's. Otherwise, local address can't be selected.
     */
    if (rvrf == VRF_ANY)
    {
        rvrf = svrf;
    }

    /*
     * 如果没有设置本地地址，则根据目的地址，选项等选择一个合适的地址；如果
     * 未找到，则需要返回错误
     */
    if (INADDR_ANY == laddr)
    {
        laddr = in_selectsrc(inp, inp->inp_outif, inp->inp_moptions, svrf, faddr);

        if (INADDR_ANY == laddr)
        {
            return (EADDRNOTAVAIL);
        }
    }

    oinp = in_pcblookup_hash(inp->inp_pcbinfo, faddr, fport, rvrf, laddr, lport, 0);
    if (oinp != NULL)
    {
        if (oinpp != NULL)
        {
            *oinpp = oinp;
        }
        return (EADDRINUSE);
    }
    if (lport == 0)
    {
        error = in_pcbbind_setup(inp, NULL, &rvrf, &laddr, &lport);
        if (error > 0)
        {
            return (error);
        }
    }
    
    *rvrfp = rvrf;
    *laddrp = laddr;
    *lportp = lport;
    *svrfp = svrf;
    *faddrp = faddr;
    *fportp = fport;

    return (0);
}

int in_pcbconnect(IN INPCB_S *inp, IN SOCKADDR_S *nam)
{
    USHORT lport, fport;
    UINT laddr, faddr;
    VRF_INDEX svrf, rvrf;
    int anonport, error;

    INP_INFO_WLOCK_ASSERT(inp->inp_pcbinfo);
    INP_LOCK_ASSERT(inp);

    lport = inp->inp_lport;
    laddr = inp->inp_laddr;
    rvrf = inp->inp_rvrf;
    anonport = (int)(lport == 0);
    error = in_pcbconnect_setup(inp, nam, &rvrf, &laddr, &lport, &svrf, &faddr, &fport, NULL);
    if (error > 0)
    {
        return (error);
    }

    /* Do the initial binding of the local address if required. */
    if (inp->inp_laddr == INADDR_ANY && inp->inp_lport == 0) {
        inp->inp_lport = lport;
        inp->inp_laddr = laddr;
        inp->inp_rvrf = rvrf;
        if (in_pcbinshash(inp) != 0) {
            inp->inp_laddr = INADDR_ANY;
            inp->inp_lport = 0;
            inp->inp_rvrf = VRF_ANY;
            return (EAGAIN);
        }
    }

    /* Commit the remaining changes. */
    inp->inp_lport = lport;
    inp->inp_laddr = laddr;
    inp->inp_faddr = faddr;
    inp->inp_fport = fport;
    inp->inp_svrf = svrf;

    in_pcbrehash(inp);

    if (anonport > 0)
    {
        inp->inp_flags |= INP_ANONPORT;
    }
    return (0);
}

void in_pcbdisconnect(IN INPCB_S *inp)
{
    INP_INFO_WLOCK_ASSERT(inp->inp_pcbinfo);
    INP_LOCK_ASSERT(inp);

    inp->inp_faddr = INADDR_ANY;
    inp->inp_fport = 0;
    inp->inp_svrf = VRF_ANY;

    in_pcbrehash(inp);
}

void in_pcbdetach(IN INPCB_S *inp)
{
    BS_DBGASSERT(inp->inp_socket != NULL);
    inp->inp_socket->so_pcb = NULL;
    inp->inp_socket = NULL;
}

void in_pcbremhash(struct inpcb *inp)
{
    struct inpcbinfo *pcbinfo = inp->inp_pcbinfo;

    INP_INFO_WLOCK_ASSERT(pcbinfo);
    INP_LOCK_ASSERT(inp);

    if (0 < inp->inp_lport)
    {
        INPCB_PORT_S *phd = inp->inp_phd;
        inp->inp_phd = NULL;
        LIST_REMOVE(inp, inp_hash);
        LIST_REMOVE(inp, inp_portlist);
        if (LIST_FIRST(&phd->phd_pcblist) == NULL)
        {
            LIST_REMOVE(phd, phd_hash);
            MEM_Free(phd);
        }
    }

    return ;
}

void in_pcbremlists(IN INPCB_S *inp)
{
    struct inpcbinfo *pcbinfo = inp->inp_pcbinfo;
    enum inpcbversion inver;

    in_pcbremhash(inp);

    LIST_REMOVE(inp, inp_list);
    
    if (0 != (inp->inp_vflag & INP_IPV6PROTO))
    {
        inver = INV6;
    }
    else
    {
        inver = INV4;
    }
    pcbinfo->ipi_count[inver]--;

    return;
}

void in_pcbfree(IN INPCB_S *inp)
{
    struct inpcbinfo *ipi = inp->inp_pcbinfo;

    INP_INFO_WLOCK_ASSERT(ipi);
    INP_LOCK_ASSERT(inp);

    in_pcbremlists(inp);

    if (NULL != inp->inp_options)
    {
        MBUF_Free(inp->inp_options);
    }

    if (NULL != inp->inp_moptions)
    {
        inp_freemoptions(inp->inp_moptions);
    }

    if (NULL != inp->inp_dmac)
    {
        MEM_Free(inp->inp_dmac);
    }

    if (NULL != inp->inp_outif)
    {
        MEM_Free(inp->inp_outif);
    }

    inp->inp_vflag = 0;

    INP_UNLOCK(inp);

    MEM_Free(inp);

    return;
}

SOCKADDR_S * in_sockaddr(IN USHORT port, IN UINT addr, IN VRF_INDEX vrf)
{
    SOCKADDR_IN_S *sin;

    sin = (SOCKADDR_IN_S *)MEM_ZMalloc(sizeof(SOCKADDR_IN_S));
    if(NULL == sin)
    {
        return NULL;
    }
    sin->sin_family = AF_INET;
    sin->sin_len = sizeof(*sin);
    sin->sin_addr = addr;
    sin->sin_port = port;
    sin->sin_vrf = vrf;

    return (SOCKADDR_S *)sin;
}


int in_getpeeraddr(IN SOCKET_S *so, OUT SOCKADDR_S **nam)
{
    struct inpcb *inp;
    UINT addr;
    USHORT port;
    VRF_INDEX vrf;

    inp = sotoinpcb(so);
    BS_DBGASSERT(inp != NULL);

    INP_LOCK(inp);
    port = inp->inp_fport;
    addr = inp->inp_faddr;
    vrf = inp->inp_svrf;
    INP_UNLOCK(inp);

    *nam = in_sockaddr(port, addr, vrf);

    return 0;
}

int in_getsockaddr(IN SOCKET_S *so, IN SOCKADDR_S **nam)
{
    struct inpcb *inp;
    UINT addr;
    USHORT port;
    VRF_INDEX vrf;

    inp = sotoinpcb(so);
    BS_DBGASSERT(inp != NULL);

    INP_LOCK(inp);
    port = inp->inp_lport;
    addr = inp->inp_laddr;
    vrf= inp->inp_rvrf;
    INP_UNLOCK(inp);

    *nam = in_sockaddr(port, addr, vrf);
    return 0;
}

void in_pcbsosetlabel(IN SOCKET_S *so)
{
    (void)so;
}

/* 通知应用程序tcp或udp发生某种差错，传输层通过notify进行处理 */
void in_pcbnotifyall
(
    IN INPCB_INFO_S *pcbinfo,
    IN UINT faddr,
    IN VRF_INDEX rvrf,
    IN int errorno,
    INPCB_S *(*notify)(struct inpcb *, int)
)
{
    struct inpcb *inp, *ninp;
    INPCB_HEAD_S *head;

    INP_INFO_WLOCK(pcbinfo);
    head = pcbinfo->ipi_listhead;
    for (inp = LIST_FIRST(head); inp != NULL; inp = ninp)
    {
        INP_LOCK(inp);
        ninp = LIST_NEXT(inp, inp_list);
        if ((inp->inp_vflag & INP_IPV4) == 0)
        {
            INP_UNLOCK(inp);
            continue;
        }
        if ((inp->inp_faddr != faddr)
            || (inp->inp_socket == NULL)
            || ((VRF_ANY != inp->inp_rvrf) && (rvrf != inp->inp_rvrf)))
        {
            INP_UNLOCK(inp);
            continue;
        }
        if (NULL != (*notify)(inp, errorno))
        {
            INP_UNLOCK(inp);
        }
    }
    INP_INFO_WUNLOCK(pcbinfo);

    return;
}

