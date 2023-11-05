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

int ip6_v6only = 1;   
int ip6_auto_flowlabel = 0; 


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
    
    vrf = inp->inp_rvrf;
    if (VRF_ANY == vrf)
    {
        vrf = svrf;
    }

    if ((NULL == inp->inp_socket) || (0 == (inp->inp_socket->so_options & SO_DONTROUTE))) {
        
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
    
    
    if (INADDR_ANY == srcaddr) 
    {
        memset(&ipaddrinfo, 0, sizeof(IPADDR_INFO_S));
        
        (void)IN_IPAddr_GetFwdAddrInVrf(vrf, &ipaddrinfo);
        srcaddr = ipaddrinfo.stIPAddrKey.uiIPAddr;
    }

    
    if (IN_MULTICAST(ntohl(dstaddr))) 
    {
        if ((ipmoptions != NULL) && (0 != ipmoptions->imo_multicast_if))
        {
            outif = ipmoptions->imo_multicast_if;
            
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


INT in_pcb_listen_cmp(struct inpcb *pstInPcb1, struct inpcb *pstInPcb2)
{
    UCHAR ucVflag1, ucVflag2;
    INT iRet = 0;
    
    ucVflag1 = pstInPcb1->inp_vflag & INP_IPV6PROTO;
    ucVflag2 = pstInPcb2->inp_vflag & INP_IPV6PROTO;
    
    if (ucVflag1 == ucVflag2) {
        if (INP_IPV6PROTO != ucVflag1) {
            
            iRet = in_pcb4_listen_cmp(&pstInPcb1->inp_inc, &pstInPcb2->inp_inc);
        } else {
            
            iRet = in_pcb6_listen_cmp(&pstInPcb1->inp_inc, &pstInPcb2->inp_inc);
        }
    } else if (INP_IPV6PROTO != ucVflag1) {
        iRet = -1;
    } else {
        iRet = 1;
    }

    return iRet;
}


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


INT in_pcbcmp(struct inpcb *pstInPcb1, struct inpcb *pstInPcb2)
{
    UCHAR ucVflag1, ucVflag2;
    INT iRet = 0;
    
    ucVflag1 = pstInPcb1->inp_vflag & INP_IPV6PROTO;
    ucVflag2 = pstInPcb2->inp_vflag & INP_IPV6PROTO;
    
    if (ucVflag1 == ucVflag2) {
        if (INP_IPV6PROTO != ucVflag1) {
            
            iRet = in_pcb4cmp(&pstInPcb1->inp_inc, &pstInPcb2->inp_inc);
        } else {
            
            iRet = in_pcb6cmp(&pstInPcb1->inp_inc, &pstInPcb2->inp_inc);
        }
    } else if (INP_IPV6PROTO != ucVflag1) {
        iRet = -1;
    } else {
        iRet = 1;
    }

    return iRet;
}


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

    
    inp->inp_svrf = inp->inp_rvrf = VRF_ANY;

    if (INP_SOCKAF(so) == AF_INET6)
    {
        inver = INV6;
        inp->inp_vflag |= INP_IPV6PROTO;

        
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

    
    BS_DBGASSERT(0 != lport);

    if (0 != (vflag & INP_IPV6PROTO))
    {
        inver = INV6;
    }
    else
    {
        inver = INV4;
    }

    
    if (IN_MULTICAST(ntohl(laddr)))
    {
        
        if (0 != (inflags & INP_REUSEADDR))
        {
            reuseport = INP_REUSEADDR | INP_REUSEPORT;
        }
    }

    inp = in_pcblookup_local(pcbinfo, rvrf, laddr, lport, faddr, wild);

    
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

    
    if (NULL != nam)
    {
        sin = (struct sockaddr_in *)nam;
        if (nam->sa_len < sizeof (*sin))
        {
            return (EINVAL);
        }

        
        if (sin->sin_family != AF_INET)
        {
            return (EAFNOSUPPORT);
        }

        if (sin->sin_port != *lportp)
        {
            
            if (*lportp != 0)
            {
                return (EINVAL);
            }
            lport = sin->sin_port;
        }
        
        
        if (sin->sin_vrf != *rvrfp)
        {
            if (*rvrfp != VRF_ANY)
            {
                return (EINVAL);
            }
            rvrf = sin->sin_vrf;
        }

        laddr = sin->sin_addr;

        
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

    
    LIST_FOREACH(phd, pcbporthash, phd_hash)
    {
        if (phd->phd_port == inp->inp_lport)
        {
            break;
        }
    }
    
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



    return (0);
}


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

    
    if (wildcard > 0) {
        INPCB_PORT_HEAD_S *porthash;
        INPCB_PORT_S *phd;
        INPCB_S *match = NULL;
        
        
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
            
            LIST_FOREACH(inp, &phd->phd_pcblist, inp_portlist)
            {
                wild = 0;
                if ((inp->inp_vflag & INP_IPV4) == 0)
                {
                    continue;
                }

                
                if ((inp->inp_vflag & INP_IPV6) != 0)
                {
                    wildcard += INP_LOOKUP_MAPPED_PCB_COST;
                }
                
                
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
        
        
        head = &pcbinfo->ipi_hashbase[INP_PCBHASH(hashkey_faddr, lport,
            0, pcbinfo->ipi_hashmask)];

        LIST_FOREACH(inp, head, inp_hash)
        {
            if ((inp->inp_vflag & INP_IPV4) == 0)
            {
                continue;
            }
            
            
            if (inp->inp_faddr == faddr &&
                inp->inp_rvrf == rvrf &&
                inp->inp_laddr == laddr &&
                inp->inp_lport == lport)
            {
                
                return (inp);
            }
        }
        
        return (NULL);
    }
    else
    {
        INPCB_PORT_HEAD_S *porthash;
        INPCB_PORT_S *phd;
        INPCB_S *match = NULL;
        
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
            
            LIST_FOREACH(inp, &phd->phd_pcblist, inp_portlist)
            {
                wildcard = 0;
                if ((inp->inp_vflag & INP_IPV4) == 0)
                {
                    continue;
                }
                
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
                        
                    }
                }
                else
                {
                    if (faddr != INADDR_ANY)
                    {
                        wildcard++;
                    }
                }

                
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

    
    if (rvrf == VRF_ANY)
    {
        rvrf = svrf;
    }

    
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

