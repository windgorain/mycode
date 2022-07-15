/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2012-11-15
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/mbuf_utl.h"
#include "utl/ip_utl.h"
#include "utl/udp_utl.h"
#include "utl/icmp_utl.h"
#include "utl/in_checksum.h"
#include "inet/in_pub.h"

#include "protosw.h"
#include "domain.h"
#include "in.h"
#include "udp_func.h"
#include "ip_options.h"
#include "ip_fwd.h"
#include "uipc_func.h"
#include "uipc_compat.h"
#include "in_ip_output.h"

#include "../in_pcb.h"
#include "../in_mcast.h"
#include "../icmp.h"
#include "../inet_ip.h"
#include "../inet6_ip.h"
#include "../udp_var.h"

#include "udp_def.h"

STATIC int udp_attach(IN SOCKET_S *so, IN int proto);
STATIC int udp_bind(IN SOCKET_S *so, IN SOCKADDR_S *nam);
STATIC int udp_connect(IN SOCKET_S *so, IN SOCKADDR_S *nam);
STATIC int udp_disconnect(IN SOCKET_S *so);
STATIC int udp_send
(
    IN SOCKET_S *so,
    IN int flags,
    IN MBUF_S *m,
    IN SOCKADDR_S *addr,
    IN MBUF_S *control
);
STATIC void udp_abort(IN SOCKET_S *so);
STATIC void udp_detach(IN VOID *so);
STATIC int udp_shutdown(IN SOCKET_S *so);
STATIC void udp_close(IN SOCKET_S *so);
STATIC int udp_output
(
    IN INPCB_S *inp,
    IN MBUF_S *m,
    IN SOCKADDR_S *addr,
    IN MBUF_S *control
);


UINT  udp_sendspace = 9216UL;       /* really max datagram size */
UINT  udp_recvspace = 40 * (1024 + sizeof(SOCKADDR_IN6_S));

UDP_CTRL_S g_stUdpUsrreq = {0};

PROTOSW_USER_REQUEST_S udp_usrreqs =
{
    0,
    udp_abort,          /* pru_abort */
    NULL,               /* pru_accept */
	udp_attach,         /* pru_attach */
    udp_bind,           /* pru_bind */
    udp_connect,        /* pru_connect */
    NULL,               /* pru_connect2 */
    NULL,               /* pru_control */
    udp_detach,         /* pru_detach */
    udp_disconnect,     /* pru_disconnect */
    NULL,               /* pru_listen */
    in_getpeeraddr,     /* pru_peeraddr */
    NULL,               /* pru_rcvd */
    NULL,               /* pru_rcvoob */
    udp_send,           /* pru_send */
    NULL,               /* pru_sense */
    udp_shutdown,       /* pru_shutdown */
    in_getsockaddr,     /* pru_sockaddr */
    sosend_dgram,       /* pru_sosend */
    NULL,               /* pru_soreceive */
    NULL,               /* pru_sopoll */
    in_pcbsosetlabel,   /* pru_sosetlabel */
    udp_close           /* pru_close */
};

VOID udp_Init()
{
    return;
}

int udp_servinit(void)
{
    return 0;
}

void udp_slowtimo(void)
{
    return;
}

void m_adj(IN MBUF_S *m, IN INT len)
{
    if (len >= 0)
    {
        MBUF_CutHead(m, len);
    }
    else
    {
        MBUF_CutTail(m, (UINT)(-len));
    }
}

/*
接收到的UDP报文,根据查找到的inpcb,如果inpcb设置了接收控制选项或inpcb
对应的socket对象设置了时间选项,则调用ip_savecontrol从接收到的mbuf报文链中
提取控制选项，生成控制mbuf，增加在数据mbuf的前面；最终生成
(报文源addr+control选项+数据)mbuf链,存放在该inpcb对应的socket接收缓冲区内
*/
/*
 * Subroutine of udp_input(), which appends the provided mbuf chain to the
 * passed pcb/socket.  The caller must provide a sockaddr_in via udp_in that
 * contains the source address.  If the socket ends up being an IPv6 socket,
 * udp_append() will convert to a sockaddr_in6 before passing the address
 * into the socket code.
 */
STATIC void udp_append
(
    IN INPCB_S *inp,
    IN IP_HEAD_S *ip,
    IN MBUF_S *n,
    IN int off,
    IN SOCKADDR_IN_S *udp_in,
    IN UCHAR *ip_head
)
{
    SOCKADDR_S *append_sa;
    SOCKET_S *so;
    MBUF_S *opts = 0;
    SOCKADDR_IN6_S udp_in6;

    if ((0 != ((UINT)(inp->inp_flags) & INP_CONTROLOPTS))
        || (0 != (inp->inp_socket->so_options & (SO_TIMESTAMP | SO_TIMESTAMPNS))))
    {
        if (0 != (inp->inp_vflag & INP_IPV6))
        {
            int savedflags;

            savedflags = inp->inp_flags;
            inp->inp_flags &= ~INP_UNMAPPABLEOPTS;
            ip6_savecontrol(inp, n, &opts);
            inp->inp_flags = savedflags;
        }
        else
        {
            ip_savecontrol(inp, &opts, ip, n, ip_head);
        }
    }
    if (0 != (inp->inp_vflag & INP_IPV6)) {
        Mem_Zero(&udp_in6, sizeof(udp_in6));
        udp_in6.sin6_len = sizeof(udp_in6);
        udp_in6.sin6_family = AF_INET6;
        in6_sin_2_v4mapsin6(udp_in, &udp_in6);
        append_sa = (struct sockaddr *)&udp_in6;
    } else {
        append_sa = (struct sockaddr *)udp_in;
    }
    m_adj(n, off);

    so = inp->inp_socket;
    SOCKBUF_LOCK(&so->so_rcv);
    if (sbappendaddr_locked(&so->so_rcv, append_sa, n, opts) == 0)
    {
        SOCKBUF_UNLOCK(&so->so_rcv);
        MBUF_Free(n);
        if (NULL != opts)
        {
            MBUF_Free(opts);
        }
    } 
    else
    {
        sorwakeup_locked(so);
    }
}

VOID udp_input(IN MBUF_S *m, IN UINT uiPayloadOffset)
{
    IP_HEAD_S *ip;
    UDP_HEAD_S *uh;
    INPCB_S *inp;
    int len;
    UINT iphlen = uiPayloadOffset;
    IP_HEAD_S save_ip;
    SOCKADDR_IN_S udp_in;
    int broadcast = 0;
    UCHAR iphead[MAX_IPHEADLEN];   /*IP头长最大为60字节，用以存储原报文的IP头，包括选项。用数组避免分支释放内存*/
    int needtrsend = 0;
    UINT trsendnum = 0;
    MBUF_S *n;
    UINT uiIfindex = 0;
    VRF_INDEX rvrf;

    iphead[0] = '\0';

    /*
     * Strip IP options, if any; should skip this, make available to
     * user, and use on returned packets, but we don't yet have a way to
     * check the checksum with options still present.
     */
    if (iphlen > sizeof(IP_HEAD_S))
    {
        IP_SaveSrcOption(m, iphead); /*只针对选项报文提取源路由*/
        IP_StrIpOptions(m);
        iphlen = sizeof(IP_HEAD_S);
    }

    if (BS_OK != MBUF_MakeContinue(m, iphlen + sizeof(UDP_HEAD_S))) 
    {
        goto badunlocked;
    }

    /*
     * Get IP and UDP header together in first mbuf.
     */
    ip = MBUF_MTOD(m);
    uh = (UDP_HEAD_S *)(void *)((UCHAR*)ip + iphlen);

    rvrf = MBUF_GET_OUTVPNID(m);

    /*
     * Construct sockaddr format source address.  Stuff source address
     * and datagram in user buffer.
     */
    Mem_Zero(&udp_in, sizeof(udp_in));
    udp_in.sin_len = sizeof(udp_in);
    udp_in.sin_family = AF_INET;
    udp_in.sin_port = uh->usSrcPort;
    udp_in.sin_addr = ip->unSrcIp.uiIp;
    udp_in.sin_vrf = rvrf;

    /*
     * Make mbuf data length reflect UDP length.  If not enough data to
     * reflect UDP length, drop.
     */
    len = ntohs(uh->usDataLength);
    if (ip->usTotlelen != len)
    {
        if ((USHORT)(short)len > ip->usTotlelen || len < sizeof(UDP_HEAD_S))
        {
            goto badunlocked;
        }

        m_adj(m, len - (INT)ip->usTotlelen);
    }

    /*
     * Save a copy of the IP header in case we want restore it for
     * sending an ICMP error message in response.
     */
    if (0 == g_stUdpUsrreq.udp_blackhole)
    {
        save_ip = *ip;
    }
    else
    {
        Mem_Zero(&save_ip, sizeof(save_ip));
    }

    /*
     * Checksum extended UDP header and data.
     */
    if (0 != uh->usCrc)
    {
        USHORT uh_sum;
        char b[9];
        MEM_Copy(b, ((struct ipovly *)ip)->ih_x1, 9);
        Mem_Zero(((struct ipovly *)ip)->ih_x1, 9);
        ((struct ipovly *)ip)->ih_len = uh->usDataLength;
        if (BS_OK != MBUF_MakeContinue(m, len + sizeof (IP_HEAD_S)))
        {
            goto badunlocked;
        }
        ip = MBUF_MTOD(m);
        uh_sum = IN_CHKSUM_CheckSum((UCHAR*)ip, len + sizeof (IP_HEAD_S));
        MEM_Copy(((struct ipovly *)ip)->ih_x1, b, 9);
        if (0 != uh_sum)
        {
            goto badunlocked;
        }
    }

    uiIfindex = MBUF_GET_RECV_IF_INDEX(m);
    broadcast = MBUF_GET_IP_PKTTYPE(m) & (IP_PKT_IIFBRAODCAST | IP_PKT_OIFBRAODCAST);
    INP_INFO_RLOCK(&V_udbinfo);
    if ((broadcast != 0) || (IN_MULTICAST(ntohl(ip->unDstIp.uiIp))))
    {
        INPCB_S *last;
        struct ip_moptions *imo;

        last = NULL;
        LIST_FOREACH(inp, &g_stUdpUsrreq.stAllPcbList, inp_list)
        {
            if (inp->inp_inc.inc_ie.ie_lport != uh->usDstPort)
            {
                continue;
            }
            if ((inp->inp_vflag & INP_IPV4) == 0)
            {
                continue;
            }

            if ((inp->inp_rvrf != VRF_ANY) &&
                (inp->inp_rvrf != rvrf))
            {
                continue;
            }

            if ((inp->inp_laddr != INADDR_ANY) &&
                (inp->inp_laddr != ip->unDstIp.uiIp))
            {
                continue;
            }
            if ((inp->inp_faddr != INADDR_ANY) &&
                (inp->inp_faddr != ip->unSrcIp.uiIp))
            {
                continue;
            }
            /*
             * XXX: Do not check source port of incoming datagram
             * unless inp_connect() has been called to bind the
             * fport part of the 4-tuple; the source could be
             * trying to talk to us with an ephemeral port.
             */
            if (inp->inp_fport != 0 &&
                inp->inp_fport != uh->usSrcPort)
            {
                continue;
            }

            INP_LOCK(inp);

            /*
             * Handle socket delivery policy for any-source
             * and source-specific multicast. [RFC3678]
             */
            imo = inp->inp_moptions;
            if ((imo != NULL) && IN_MULTICAST(ntohl(ip->unDstIp.uiIp)))
            {
                SOCKADDR_IN_S   sin;
                struct in_msource   *ims;
                int          blocked, mode;
                UINT         idx;

                Mem_Zero(&sin, sizeof(SOCKADDR_IN_S));
                sin.sin_len = sizeof(SOCKADDR_IN_S);
                sin.sin_family = AF_INET;
                sin.sin_addr = ip->unDstIp.uiIp;

                blocked = 0;
                idx = imo_match_group(imo, uiIfindex, (struct sockaddr *)&sin);
                if (idx == (UINT)(-1))
                {
                    /*
                     * No group membership for this socket.
                     * Do not bump udps_noportbcast, as
                     * this will happen further down.
                     */
                    blocked++;
                }
                else
                {
                    /*
                     * Check for a multicast source filter
                     * entry on this socket for this group.
                     * MCAST_EXCLUDE is the default
                     * behaviour.  It means default accept;
                     * entries, if present, denote sources
                     * to be excluded from delivery.
                     */
                    ims = imo_match_source(imo, idx,
                        (struct sockaddr *)&udp_in);
                    mode = imo->imo_mfilters[idx].imf_fmode;
                    if ((ims != NULL &&
                         mode == MCAST_EXCLUDE) ||
                        (ims == NULL &&
                         mode == MCAST_INCLUDE))
                    {
                        blocked++;
                    }
                }
                if (blocked != 0) {
                    INP_UNLOCK(inp);
                    continue;
                }
            }
            if (last != NULL) {

                n = MBUF_ReferenceCopy(m, 0, MBUF_TOTAL_DATA_LEN(m));
                if (n != NULL)
                {
                    udp_append(last, ip, n, iphlen + (int)sizeof(UDP_HEAD_S), &udp_in, iphead);
                }
                INP_UNLOCK(last);
            }
            last = inp;
            /*
             * Don't look for additional matches if this one does
             * not have either the SO_REUSEPORT or SO_REUSEADDR
             * socket options set.  This heuristic avoids
             * searching through all pcbs in the common case of a
             * non-shared port.  It assumes that an application
             * will never clear these options after setting them.
             */
            if ((last->inp_socket->so_options & SO_REUSEADDR) == 0)
            {
                break;
            }
        }

        if (NULL != last)
        {
            udp_append(last, ip, m, iphlen + (int)sizeof(UDP_HEAD_S), &udp_in, iphead);
            INP_UNLOCK(last);
            m = NULL;
        }
        INP_INFO_RUNLOCK(&V_udbinfo);
        goto badunlocked;
    }

    /*
     * Locate pcb for datagram.
     */
    inp = in_pcblookup_hash(&V_udbinfo, ip->unSrcIp.uiIp,
                    uh->usSrcPort, rvrf, ip->unDstIp.uiIp, uh->usDstPort, 1);
    if (NULL != inp)
    {
        INP_LOCK(inp);

        {
            /* 本地上送处理 */
            /*
             * Check the minimum TTL for socket.
             */
            if ((0 == inp->inp_ip_minttl) || (inp->inp_ip_minttl <= ip->ucTtl))
            {
                udp_append(inp, ip, m, (int)iphlen + (int)sizeof(UDP_HEAD_S), &udp_in,iphead);
                m = NULL;
            }
        }

        INP_UNLOCK(inp);
        INP_INFO_RUNLOCK(&V_udbinfo);
        goto badunlocked;
    }
    else
    {
        /* 这里可以直接放掉大锁 */
        INP_INFO_RUNLOCK(&V_udbinfo);

        /* 没有找到匹配的INPCB，需要回应ICMP差错报文 */
        if (0 != (MBUF_GET_IP_PKTTYPE(m) & (IP_PKT_ETHBCAST | IP_PKT_ETHMCAST)))
        {
            goto badunlocked;
        }
        if (0 != g_stUdpUsrreq.udp_blackhole)
        {
            goto badunlocked;
        }
        if (badport_bandlim(BANDLIM_ICMP_UNREACH) < 0)
        {
            goto badunlocked;
        }
        *ip = save_ip;
        ip->usTotlelen += (USHORT)(short)iphlen;
        icmp_error(m, ICMP_UNREACH, ICMP_UNREACH_PORT, 0, 0);
        m = NULL;
        goto badunlocked;
    }

badunlocked:
    if (NULL != m) {
        MBUF_Free(m);
    }
}

STATIC int udp_attach(IN SOCKET_S *so, IN int proto)
{
    struct inpcb *inp;
    int error;

    inp = sotoinpcb(so);
    BS_DBGASSERT(inp == NULL);
    error = soreserve(so, udp_sendspace, udp_recvspace);
    if (error > 0)
    {
        return (error);
    }
    INP_INFO_WLOCK(&V_udbinfo);
    error = in_pcballoc(so, &V_udbinfo);
    if (error > 0) {
        INP_INFO_WUNLOCK(&V_udbinfo);
        return (error);
    }

    inp = (struct inpcb *)so->so_pcb;
    INP_INFO_WUNLOCK(&V_udbinfo);
    inp->inp_vflag |= INP_IPV4;
    inp->inp_ip_ttl = IP_GetDefTTL();
    INP_UNLOCK(inp);

    return (0);
}

STATIC int udp_bind(IN SOCKET_S *so, IN SOCKADDR_S *nam)
{
    struct inpcb *inp;
    int error;
    inp = sotoinpcb(so);
    BS_DBGASSERT(inp != NULL);
    INP_INFO_WLOCK(&V_udbinfo);
    INP_LOCK(inp);
    error = in_pcbbind(inp, nam);
    INP_UNLOCK(inp);
    INP_INFO_WUNLOCK(&V_udbinfo);

    return (error);
}

STATIC int udp_connect(IN SOCKET_S *so, IN SOCKADDR_S *nam)
{
    INPCB_S *inp;
    int error;
    SOCKADDR_IN_S *sin;

    inp = sotoinpcb(so);
    BS_DBGASSERT(inp != NULL);
    INP_INFO_WLOCK(&V_udbinfo);
    INP_LOCK(inp);
    if (inp->inp_faddr != INADDR_ANY)
    {
        INP_UNLOCK(inp);
        INP_INFO_WUNLOCK(&V_udbinfo);
        return (EISCONN);
    }
    sin = (struct sockaddr_in *)nam;
    error = in_pcbconnect(inp, nam);
    if (error == 0)
    {
        soisconnected(so);
    }
    INP_UNLOCK(inp);
    INP_INFO_WUNLOCK(&V_udbinfo);

    return (error);
}

STATIC int udp_disconnect(IN SOCKET_S *so)
{
    struct inpcb *inp;

    inp = sotoinpcb(so);
    BS_DBGASSERT(inp != NULL);
    INP_INFO_WLOCK(&V_udbinfo);
    INP_LOCK(inp);
    if (inp->inp_faddr == INADDR_ANY)
    {
        INP_INFO_WUNLOCK(&V_udbinfo);
        INP_UNLOCK(inp);
        return (ENOTCONN);
    }

    in_pcbdisconnect(inp);
    inp->inp_laddr = INADDR_ANY;
    SOCK_LOCK(so);
    so->so_state &= ~SS_ISCONNECTED;        /* XXX */
    SOCK_UNLOCK(so);
    INP_UNLOCK(inp);
    INP_INFO_WUNLOCK(&V_udbinfo);
    return (0);
}

STATIC int udp_send
(
    IN SOCKET_S *so,
    IN int flags,
    IN MBUF_S *m,
    IN SOCKADDR_S *addr,
    IN MBUF_S *control
)
{
    INPCB_S *inp;
    int error;

    inp = sotoinpcb(so);
    BS_DBGASSERT(inp != NULL);

    error = udp_output(inp, m, addr, control);
    if (NULL != control)
    {
        MBUF_Free(control);
    }

    return error;
}

STATIC void udp_abort(IN SOCKET_S *so)
{
    struct inpcb *inp;

    inp = sotoinpcb(so);
    BS_DBGASSERT(inp != NULL);
    INP_INFO_WLOCK(&V_udbinfo);
    INP_LOCK(inp);
    if (inp->inp_faddr != INADDR_ANY)
    {
        in_pcbdisconnect(inp);
        inp->inp_laddr = INADDR_ANY;
        soisdisconnected(so);
    }
    INP_UNLOCK(inp);
    INP_INFO_WUNLOCK(&V_udbinfo);
}

STATIC void udp_detach(IN SOCKET_S *so)
{
    struct inpcb *inp;

    inp = sotoinpcb(so);
    BS_DBGASSERT(inp != NULL);
    BS_DBGASSERT(inp->inp_faddr == INADDR_ANY);
    INP_INFO_WLOCK(&V_udbinfo);
    INP_LOCK(inp);
    in_pcbdetach(inp);
    in_pcbfree(inp);
    INP_INFO_WUNLOCK(&V_udbinfo);
}

STATIC int udp_shutdown(IN SOCKET_S *so)
{
    INPCB_S *inp;

    inp = sotoinpcb(so);
    BS_DBGASSERT(inp != NULL);
    INP_LOCK(inp);
    socantsendmore(so);
    INP_UNLOCK(inp);
    return (0);
}

STATIC void udp_close(IN SOCKET_S *so)
{
    INPCB_S *inp;

    inp = sotoinpcb(so);
    BS_DBGASSERT(inp != NULL);
    INP_INFO_WLOCK(&V_udbinfo);
    INP_LOCK(inp);
    if (inp->inp_faddr != INADDR_ANY)
    {
        in_pcbdisconnect(inp);
        inp->inp_laddr = INADDR_ANY;
        soisdisconnected(so);
    }
    INP_UNLOCK(inp);
    INP_INFO_WUNLOCK(&V_udbinfo);
}

STATIC int udp_output
(
    IN INPCB_S *inp,
    IN MBUF_S *m,
    IN SOCKADDR_S *addr,
    IN MBUF_S *control
)
{
    MBUF_S *newcontrol = NULL;
    struct udpiphdr *ui;
    IP_HEAD_S *ip;
    UINT len = MBUF_TOTAL_DATA_LEN( m );
    UINT contol_len = 0;
    UINT faddr, laddr;
    CMSGHDR_S *cm;
    SOCKADDR_IN_S *sin, src;
    int error = 0;
    long ipflags;
    USHORT fport, lport;
    VRF_INDEX rvrf, svrf;
    int unlock_udbinfo;
    struct snd_if *outif;
    int opt;
    ULONG ret;

    /*
     * udp_output() may need to temporarily bind or connect the current
     * inpcb.  As such, we don't know up front whether we will need the
     * pcbinfo lock or not.  Do any work to decide what is needed up
     * front before acquiring any locks.
     */
    if (len + sizeof(struct udpiphdr) > IP_MAXPACKET)
    {
        MBUF_Free(m);
        return (EMSGSIZE);
    }

    src.sin_family = 0;
    if (control != NULL)
    {
        newcontrol = MBUF_ReferenceCopy(control, 0, MBUF_TOTAL_DATA_LEN(control));
        if (NULL != newcontrol)
        {
            /*  We DON'T assume all the optional information is stored in a single mbuf. */
            while (MBUF_TOTAL_DATA_LEN(newcontrol) > 0)
            {
                /* First pullup, length is sizeof struct cmsghdr */
                ret = MBUF_MakeContinue(newcontrol, sizeof(CMSGHDR_S));
                if (BS_OK != ret)
                {
                    error = ENOMEM;
                    break;
                }
                
                cm = MBUF_MTOD(newcontrol);
                contol_len = MBUF_TOTAL_DATA_LEN(newcontrol);
                if ((contol_len < sizeof(*cm))
                    || (cm->cmsg_len == 0)
                    || (cm->cmsg_len > contol_len))
                {
                    error = EINVAL;
                    break;
                }
                
                if (cm->cmsg_level != IPPROTO_IP)
                {
                    MBUF_CutHead(newcontrol, CMSG_ALIGN(cm->cmsg_len));
                    continue;
                }
                
                /* Second pullup, length is CMSG_ALIGN(cm->cmsg_len) */
                ret = MBUF_MakeContinue(newcontrol, CMSG_ALIGN(cm->cmsg_len));
                if (BS_OK != ret)
                {
                    error = ENOMEM;
                    break;
                }
                cm = MBUF_MTOD(newcontrol);
                opt = cm->cmsg_type;
                if (IP_SENDSRCADDR == opt)
                {
                    if (cm->cmsg_len != CMSG_LEN(sizeof(UINT)))
                    {
                        error = EINVAL;
                    }
                    else
                    {
                        Mem_Zero(&src, sizeof(src));
                        src.sin_family = AF_INET;
                        src.sin_len = sizeof(src);
                        src.sin_port = inp->inp_lport;
                        src.sin_vrf = inp->inp_rvrf;
                        src.sin_addr = *(UINT *)CMSG_DATA(cm);
                    }
                    break;
                }
                MBUF_CutHead(newcontrol, CMSG_ALIGN(cm->cmsg_len));
            }
            MBUF_Free(newcontrol);
        }
        else
        {
            error = ENOBUFS;
        }
    }

    if (error > 0)
    {
        MBUF_Free(m);
        return (error);
    }

    if (src.sin_family == AF_INET || addr != NULL)
    {
        INP_INFO_WLOCK(&V_udbinfo);
        unlock_udbinfo = 1;
    }
    else
    {
        unlock_udbinfo = 0;
    }
    INP_LOCK(inp);

    /*
     * If the IP_SENDSRCADDR control message was specified, override the
     * source address for this datagram.  Its use is invalidated if the
     * address thus specified is incomplete or clobbers other inpcbs.
     */
    laddr = inp->inp_laddr;
    lport = inp->inp_lport;
    rvrf = inp->inp_rvrf;

    if (NULL != addr)
    {
        sin = (SOCKADDR_IN_S *)addr;

        if (inp->inp_faddr != INADDR_ANY)
        {
            error = EISCONN;
            goto release;
        }

        error = in_pcbconnect_setup(inp, addr, &rvrf, &laddr, &lport, &svrf, &faddr, &fport, NULL);
        if (error > 0)
        {
            goto release;
        }

        /* Commit the local port if newly assigned. */
        if (inp->inp_laddr == INADDR_ANY && inp->inp_lport == 0)
        {
            /*
                     * Remember addr if jailed, to prevent rebinding.
                     */
            inp->inp_lport = lport;
            if (in_pcbinshash(inp) != 0)
            {
                inp->inp_lport = 0;
                error = EAGAIN;
                goto release;
            }
            inp->inp_flags |= INP_ANONPORT;
        }
    } 
    else
    {
        faddr = inp->inp_faddr;
        fport = inp->inp_fport;
        svrf = inp->inp_svrf;
        if (faddr == INADDR_ANY)
        {
            error = ENOTCONN;
            goto release;
        }
    }
    
    if (src.sin_family == AF_INET)
    {
        if (lport == 0)
        {
            error = EINVAL;
            goto release;
        }
        laddr = src.sin_addr;
    }

    M_PREPEND(m, sizeof(struct udpiphdr));
    if (m == NULL)
    {
        error = ENOBUFS;
        goto release;
    }

    if (BS_OK != MBUF_MakeContinue(m, sizeof(struct udpiphdr)))
    {
        error = ENOBUFS;
        goto release;
    }

    /*
     * Fill in mbuf with extended UDP header and addresses and length put
     * into network format.
     */
    ui = MBUF_MTOD(m);
    Mem_Zero(ui->ui_x1, sizeof(ui->ui_x1));    /* XXX still needed? */
    ui->ui_pr = IPPROTO_UDP;
    ui->ui_src = laddr;
    ui->ui_dst = faddr;
    ui->ui_sport = lport;
    ui->ui_dport = fport;
    ui->ui_ulen = htons((USHORT)len + (USHORT)sizeof(UDP_HEAD_S));
    /*伪头部中需要计算UDP报文的长度,以便计算checksum*/
    ui->ui_len  = ui->ui_ulen;
    ui->ui_sum  = 0;
    ip = (IP_HEAD_S *)&ui->ui_i;

    /*
     * Set the Don't Fragment bit in the IP header.
     */
    if (0 != (inp->inp_flags & INP_DONTFRAG))
    {
        ip->usOff |= IP_DF;
    }

    ipflags = 0L;
    if (0 != (inp->inp_socket->so_options & SO_DONTROUTE))
    {
        ipflags |= IP_ROUTETOIF;
    }
    if (0 != (inp->inp_socket->so_options & SO_BROADCAST))
    {
        ipflags |= IP_ALLOWBROADCAST;
    }
    if (0 != (inp->inp_flags & INP_ONESBCAST))
    {
        ipflags |= IP_SENDONES;/*标志由IP转发定义，目前不使用*/
    }

    /* J03845: If user set output interface index, set it in mbuf. */
    outif = inp->inp_outif;
    if (NULL != outif) {
        /* set output if index */
        MBUF_SET_SEND_IF_INDEX(m, outif->si_if);
        /* set src address */
        if (INADDR_ANY != outif->si_lcladdr)
        {
            ip->unSrcIp.uiIp = outif->si_lcladdr;
        }
        /* set nexthop address */
        if (INADDR_ANY != outif->si_nxthop)
        {
            MBUF_SET_NEXT_HOP(m, outif->si_nxthop);
        }
        ipflags |= IP_SENDDATAIF;
    }
    
    if (0 != (inp->inp_flags & INP_SNDBYLSPV))
    {
        ipflags |= IP_SENDBY_LSPV;
    }

    /*
     * Set up checksum and output datagram.
     */
    /*checksum修改，必须保证已经有源地址，在in_pcbconnect_setup已获取了源地址,
    每次发送报文时，都需要通过此函数查找FIB表，进行源地址选择*/
    if ( 0 != V_udp_cksum) 
    {
        if ( 0 != (inp->inp_flags & INP_ONESBCAST))
        {
            faddr = INADDR_BROADCAST;
        }
        if ((ui->ui_sum = IN_Cksum(m , sizeof(struct udpiphdr)+len)) == 0)
        {
            ui->ui_sum = 0xffff;
        }
    }
    else
    {
        ui->ui_sum = 0;
    }
    ((IP_HEAD_S *)ui)->usTotlelen = (sizeof (struct udpiphdr) + len);
    ((IP_HEAD_S *)ui)->ucTtl = inp->inp_ip_ttl; 
    ((IP_HEAD_S *)ui)->ucTos = inp->inp_ip_tos; 
 
    if (unlock_udbinfo > 0)
    {
        INP_INFO_WUNLOCK(&V_udbinfo);
    }

    /* set ip opts */
    ip_setopt2mbuf(inp, control, m);

    /*
     * J03845: Assign send vrf index in mbuf.
     */
    MBUF_SET_RAWINVPNID(m, svrf);
    MBUF_SET_INVPNID(m, svrf);

    error = (int)IN_IP_Output( m, inp->inp_options, ipflags, (IPMOPTIONS_S*)inp->inp_moptions);
    INP_UNLOCK(inp);
    return (error);

release:
    INP_UNLOCK(inp);
    if (unlock_udbinfo > 0)
    {
        INP_INFO_WUNLOCK(&V_udbinfo);
    }
    m_freem(m);
    return (error);
}

INPCB_S * udp_notify(IN INPCB_S *inp, IN int errorno)
{

    inp->inp_socket->so_error = (USHORT)errorno;
    sorwakeup(inp->inp_socket);
    sowwakeup(inp->inp_socket);
    return (inp);
}

void udp_ctlinput
(
    IN int cmd,
    IN SOCKADDR_S *sa,
    IN void *vip
)
{
    IP_HEAD_S *ip = NULL;
    IP_HEAD_S *iptmp = NULL;
    UINT ipheadlen;
    ICMP_S *icp = NULL;
    UDP_HEAD_S *uh;
    UINT faddr;
    INPCB_S *inp;
    VRF_INDEX rvrf;
    MBUF_S *outmbuf = NULL;
    MBUF_S *inmbuf = NULL;
    USHORT minslotno = 0;
    USHORT maxslotno = 0;
    ULONG index = 0;
    UINT datalen = 0;

    faddr = ((struct sockaddr_in *)sa)->sin_addr;
    if (sa->sa_family != AF_INET || faddr == INADDR_ANY)
    {
        return;
    }
    
    /* Check whether cmd valid or not. */
    if ((unsigned)cmd >= PRC_NCMDS || inetctlerrmap[cmd] == 0)
    {
        return;
    }

    /*
     * Redirects don't need to be handled up here.
     */
    if (PRC_IS_REDIRECT(cmd))
    {
        return;
    }

    rvrf = ((struct sockaddr_in *)sa)->sin_vrf;

    /*
     * Hostdead is ugly because it goes linearly through all PCBs.
     *
     * XXX: We never get this from ICMP, otherwise it makes an excellent
     * DoS attack on machines with many connections.
     */
    if (cmd == PRC_HOSTDEAD)
    {
        ip = NULL;
    }
    else
    {
        /*
         * G03597: 协议栈支持分布式后，CTLINPUT函数也需要支持分布式
         * 在Socket-Dist-Project项目中，修改了CTLINPUT函数第三个参数(void *vip)
         * 的含义，传入的是mbuf的指针。所以在实际处理前需要解析mbuf。
         */
        if (NULL != vip)
        {
            inmbuf = (MBUF_S *)vip;

            if (MBUF_MakeContinue(inmbuf, sizeof(IP_HEAD_S)) != BS_OK)
            {
                return ;
            }

            iptmp = MBUF_MTOD(inmbuf);
            ipheadlen = (((UINT)iptmp->ucHLen) << 2);
            datalen = ipheadlen + ICMP_ADVLENMIN;
            if (MBUF_MakeContinue(inmbuf, datalen) != BS_OK)
            {
                return ;
            }
            /* 重新获取一下ip头指针 */
            iptmp = MBUF_MTOD(inmbuf);
            icp = (struct icmp *)(void *)((char *)iptmp + ipheadlen);
            ip = (struct ip *)&icp->icmp_ip;
        }
    }

    if (ip != NULL)
    {
        /* 这种情况下，MBUF不可能为NULL */
        if (NULL != inmbuf)
        {
            uh = (struct udphdr *)(void *)((UCHAR *)ip + (UCHAR)(ip->ucHLen << 2));
            
            INP_INFO_RLOCK(&V_udbinfo);
            inp = in_pcblookup_hash(&V_udbinfo, faddr, uh->usDstPort, rvrf,
                ip->unSrcIp.uiIp, uh->usSrcPort, 0);
            if (inp != NULL) 
            {
                INP_LOCK(inp);
                /*
                 * G03597: 如果找到了匹配的INPCB，首先检查是否为本板生成的，如果是本板
                 * 生成的，则上送处理；如果INPCB不是本板生成的，且报文是从本板接收的
                 * 需要将报文拷贝一份，并透传到对应的单板进行处理。
                 */
                if (inp->inp_socket != NULL)
                {
                    (void)udp_notify(inp, inetctlerrmap[cmd]);
                }

                INP_UNLOCK(inp);
            }
            INP_INFO_RUNLOCK(&V_udbinfo);
        }
    }  else {
        /*
         * G03597: 通知所有源地址匹配的INPCB。Leopard支持分布式后，需要通知到
         * 所有单板，所以需要透传处理。
         * 在接口板上报文上送时，不能确定其他单板是否在位，只能直接透传，可能
         * 导致向不在位的单板也进行了透传，但是这样也不会导致问题。后续如果存在
         * 问题，可以考虑先透传到主控板，再透传到其他单板。
         */

        in_pcbnotifyall(&V_udbinfo, faddr, rvrf, inetctlerrmap[cmd], udp_notify);
    }

    return;
}
