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


UINT  udp_sendspace = 9216UL;       
UINT  udp_recvspace = 40 * (1024 + sizeof(SOCKADDR_IN6_S));

UDP_CTRL_S g_stUdpUsrreq = {0};

PROTOSW_USER_REQUEST_S udp_usrreqs =
{
    0,
    udp_abort,          
    NULL,               
	udp_attach,         
    udp_bind,           
    udp_connect,        
    NULL,               
    NULL,               
    udp_detach,         
    udp_disconnect,     
    NULL,               
    in_getpeeraddr,     
    NULL,               
    NULL,               
    udp_send,           
    NULL,               
    udp_shutdown,       
    in_getsockaddr,     
    sosend_dgram,       
    NULL,               
    NULL,               
    in_pcbsosetlabel,   
    udp_close           
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
    UCHAR iphead[MAX_IPHEADLEN];   
    int needtrsend = 0;
    UINT trsendnum = 0;
    MBUF_S *n;
    UINT uiIfindex = 0;
    VRF_INDEX rvrf;

    iphead[0] = '\0';

    
    if (iphlen > sizeof(IP_HEAD_S))
    {
        IP_SaveSrcOption(m, iphead); 
        IP_StrIpOptions(m);
        iphlen = sizeof(IP_HEAD_S);
    }

    if (BS_OK != MBUF_MakeContinue(m, iphlen + sizeof(UDP_HEAD_S))) 
    {
        goto badunlocked;
    }

    
    ip = MBUF_MTOD(m);
    uh = (UDP_HEAD_S *)(void *)((UCHAR*)ip + iphlen);

    rvrf = MBUF_GET_OUTVPNID(m);

    
    Mem_Zero(&udp_in, sizeof(udp_in));
    udp_in.sin_len = sizeof(udp_in);
    udp_in.sin_family = AF_INET;
    udp_in.sin_port = uh->usSrcPort;
    udp_in.sin_addr = ip->unSrcIp.uiIp;
    udp_in.sin_vrf = rvrf;

    
    len = ntohs(uh->usDataLength);
    if (ip->usTotlelen != len)
    {
        if ((USHORT)(short)len > ip->usTotlelen || len < sizeof(UDP_HEAD_S))
        {
            goto badunlocked;
        }

        m_adj(m, len - (INT)ip->usTotlelen);
    }

    
    if (0 == g_stUdpUsrreq.udp_blackhole)
    {
        save_ip = *ip;
    }
    else
    {
        Mem_Zero(&save_ip, sizeof(save_ip));
    }

    
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
            
            if (inp->inp_fport != 0 &&
                inp->inp_fport != uh->usSrcPort)
            {
                continue;
            }

            INP_LOCK(inp);

            
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
                    
                    blocked++;
                }
                else
                {
                    
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

    
    inp = in_pcblookup_hash(&V_udbinfo, ip->unSrcIp.uiIp,
                    uh->usSrcPort, rvrf, ip->unDstIp.uiIp, uh->usDstPort, 1);
    if (NULL != inp)
    {
        INP_LOCK(inp);

        {
            
            
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
        
        INP_INFO_RUNLOCK(&V_udbinfo);

        
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
    so->so_state &= ~SS_ISCONNECTED;        
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
            
            while (MBUF_TOTAL_DATA_LEN(newcontrol) > 0)
            {
                
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

        
        if (inp->inp_laddr == INADDR_ANY && inp->inp_lport == 0)
        {
            
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

    
    ui = MBUF_MTOD(m);
    Mem_Zero(ui->ui_x1, sizeof(ui->ui_x1));    
    ui->ui_pr = IPPROTO_UDP;
    ui->ui_src = laddr;
    ui->ui_dst = faddr;
    ui->ui_sport = lport;
    ui->ui_dport = fport;
    ui->ui_ulen = htons((USHORT)len + (USHORT)sizeof(UDP_HEAD_S));
    
    ui->ui_len  = ui->ui_ulen;
    ui->ui_sum  = 0;
    ip = (IP_HEAD_S *)&ui->ui_i;

    
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
        ipflags |= IP_SENDONES;
    }

    
    outif = inp->inp_outif;
    if (NULL != outif) {
        
        MBUF_SET_SEND_IF_INDEX(m, outif->si_if);
        
        if (INADDR_ANY != outif->si_lcladdr)
        {
            ip->unSrcIp.uiIp = outif->si_lcladdr;
        }
        
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

    
    ip_setopt2mbuf(inp, control, m);

    
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
    
    
    if ((unsigned)cmd >= PRC_NCMDS || inetctlerrmap[cmd] == 0)
    {
        return;
    }

    
    if (PRC_IS_REDIRECT(cmd))
    {
        return;
    }

    rvrf = ((struct sockaddr_in *)sa)->sin_vrf;

    
    if (cmd == PRC_HOSTDEAD)
    {
        ip = NULL;
    }
    else
    {
        
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
            
            iptmp = MBUF_MTOD(inmbuf);
            icp = (struct icmp *)(void *)((char *)iptmp + ipheadlen);
            ip = (struct ip *)&icp->icmp_ip;
        }
    }

    if (ip != NULL)
    {
        
        if (NULL != inmbuf)
        {
            uh = (struct udphdr *)(void *)((UCHAR *)ip + (UCHAR)(ip->ucHLen << 2));
            
            INP_INFO_RLOCK(&V_udbinfo);
            inp = in_pcblookup_hash(&V_udbinfo, faddr, uh->usDstPort, rvrf,
                ip->unSrcIp.uiIp, uh->usSrcPort, 0);
            if (inp != NULL) 
            {
                INP_LOCK(inp);
                
                if (inp->inp_socket != NULL)
                {
                    (void)udp_notify(inp, inetctlerrmap[cmd]);
                }

                INP_UNLOCK(inp);
            }
            INP_INFO_RUNLOCK(&V_udbinfo);
        }
    }  else {
        

        in_pcbnotifyall(&V_udbinfo, faddr, rvrf, inetctlerrmap[cmd], udp_notify);
    }

    return;
}
