/******************************************************************************
* Copyright (C) Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2012-11-23
* Description: 
* History:     
******************************************************************************/

#ifndef __IN_PCB_H_
#define __IN_PCB_H_

#include "utl/list_utl.h"
#include "utl/mutex_utl.h"

#ifdef __cplusplus
    extern "C" {
#endif 

#define INPCB_MSG_BLOCK     0    
#define INPCB_MSG_NOBLOCK   1    

#define    INPLOOKUP_WILDCARD    1
#define    sotoinpcb(so)    ((INPCB_S *)(so)->so_pcb)
#define    sotoin6pcb(so)    sotoinpcb(so)

#define INP_INFO_RLOCK(ipi) MUTEX_P(&(ipi)->ipi_lock)
#define INP_INFO_WLOCK(ipi) MUTEX_P(&(ipi)->ipi_lock)
#define INP_INFO_RUNLOCK(ipi)   MUTEX_V(&(ipi)->ipi_lock)
#define INP_INFO_WUNLOCK(ipi)   MUTEX_V(&(ipi)->ipi_lock)
#define INP_LOCK(inp)        MUTEX_P(&(inp)->inp_mtx)
#define INP_UNLOCK(inp)        MUTEX_V(&(inp)->inp_mtx)
#define INP_INFO_LOCK_ASSERT(ipi)
#define INP_INFO_RLOCK_ASSERT(ipi)
#define INP_INFO_WLOCK_ASSERT(ipi)
#define INP_INFO_UNLOCK_ASSERT(ipi)
#define INP_LOCK_ASSERT(inp)

#define INP_SOCKAF(so) so->so_proto->pr_domain->uiDomFamily

#define INP_PCBHASH(faddr, lport, fport, mask) \
    (((faddr) ^ ((faddr) >> 16) ^ ntohs((lport) ^ (fport))) & (mask))

#define INP_PCBPORTHASH(lport, mask) (ntohs((lport)) & (mask))


enum inpcbversion {
    INV4,      
    INV6,      
    INV_BUTT
};



typedef struct in_endpoints {
    USHORT ie_fport;        
    USHORT ie_lport;        
    VRF_INDEX    ie_svrf;      
    VRF_INDEX    ie_rvrf;      
    
    union {
        
        struct    in_addr_4in6 ie46_foreign;
        IN6_ADDR_S ie6_foreign;
    } ie_dependfaddr;
    union {
        
        struct    in_addr_4in6 ie46_local;
        IN6_ADDR_S ie6_local;
    } ie_dependladdr;

#define    ie_faddr    ie_dependfaddr.ie46_foreign.ia46_addr4
#define    ie_laddr    ie_dependladdr.ie46_local.ia46_addr4
#define    ie6_faddr    ie_dependfaddr.ie6_foreign
#define    ie6_laddr    ie_dependladdr.ie6_local
}IN_END_POINTS_S;


typedef struct in_conninfo {
    UCHAR    inc_flags;
    UCHAR    inc_len;
    USHORT   inc_pad;    
    IN_END_POINTS_S inc_ie;

#define    inc_fport    inc_ie.ie_fport
#define    inc_lport    inc_ie.ie_lport
#define    inc_svrf     inc_ie.ie_svrf
#define    inc_rvrf     inc_ie.ie_rvrf
#define    inc_faddr    inc_ie.ie_faddr
#define    inc_laddr    inc_ie.ie_laddr
#define    inc6_faddr    inc_ie.ie6_faddr
#define    inc6_laddr    inc_ie.ie6_laddr

}IN_CONNINFO_S;


#define    INP_IPV4    0x1
#define    INP_IPV6    0x2
#define    INP_IPV6PROTO    0x4        
#define    INP_ONESBCAST    0x10        
#define    INP_DROPPED    0x20        
#define    INP_SOCKREF    0x40        
#define    INP_DONTBLOCK    0x80       


#define INP_RECVOPTS       0x01    
#define INP_RECVRETOPTS    0x02    
#define INP_RECVDSTADDR    0x04    
#define INP_HDRINCL        0x08    
#define INP_REUSEADDR      0x10    
#define INP_REUSEPORT      0x20    
#define INP_ANONPORT       0x40    
#define INP_RECVIF         0x80    
#define INP_MTUDISC        0x100   
#define INP_FAITH          0x200   
#define INP_RECVTTL        0x400   
#define INP_DONTFRAG       0x800   
#define	INP_HIGHPORT       0x001000 
#define	INP_LOWPORT		   0x002000 
#define INP_RCVVLANID      0x004000
#define IN6P_IPV6_V6ONLY   0x008000 
#define IN6P_PKTINFO       0x010000 
#define IN6P_HOPLIMIT      0x020000 
#define IN6P_HOPOPTS       0x040000 
#define IN6P_DSTOPTS       0x080000 
#define IN6P_RTHDR         0x100000 
#define IN6P_RTHDRDSTOPTS  0x200000 
#define IN6P_TCLASS        0x400000 
#define IN6P_AUTOFLOWLABEL 0x800000 
#define IN6P_RFC2292       0x1000000 
#define IN6P_MTU           0x2000000 
#define INP_RCVMACADDR     0x4000000 
#define INP_SNDBYLSPV      0x8000000
#define INP_RECVTOS        0x10000000 
#define    INP_CONTROLOPTS        (INP_RECVOPTS|INP_RECVRETOPTS|INP_RECVDSTADDR|\
                 INP_RECVIF|INP_RECVTTL|INP_RCVVLANID|INP_RCVMACADDR|\
                 IN6P_PKTINFO|IN6P_HOPLIMIT|IN6P_HOPOPTS|\
                 IN6P_DSTOPTS|IN6P_RTHDR|IN6P_RTHDRDSTOPTS|\
                 IN6P_TCLASS|IN6P_AUTOFLOWLABEL|IN6P_RFC2292|\
                 IN6P_MTU|INP_RECVTOS)
#define    INP_UNMAPPABLEOPTS    (IN6P_HOPOPTS|IN6P_DSTOPTS|IN6P_RTHDR|\
                 IN6P_TCLASS|IN6P_AUTOFLOWLABEL)

typedef LIST_HEAD_DECLARE(inpcbhead, inpcb) INPCB_HEAD_S;
typedef LIST_HEAD_DECLARE(inpcbporthead, inpcbport) INPCB_PORT_HEAD_S;
typedef LIST_ENTRY(inpcb) INPCB_LIST_ENTRY_S;
typedef LIST_ENTRY(inpcbport) INPCB_LIST_PORT_S;

typedef struct inpcbport {
    INPCB_LIST_PORT_S phd_hash;
    INPCB_HEAD_S phd_pcblist;
    USHORT phd_port;
}INPCB_PORT_S;

typedef struct inpcb
{
    INPCB_LIST_ENTRY_S inp_hash;
    INPCB_LIST_ENTRY_S inp_list; 
    struct    inpcbinfo *inp_pcbinfo;    
    SOCKET_S *inp_socket;   
    MUTEX_S inp_mtx;

    UCHAR    inp_vflag;        
    UCHAR    inp_ip_ttl;       
    UCHAR    inp_ip_minttl;    
    UINT     inp_flags;        

	struct snd_mac *inp_dmac;
    SND_IF_S *inp_outif;

    IN_CONNINFO_S inp_inc;
#define    inp_fport    inp_inc.inc_fport
#define    inp_lport    inp_inc.inc_lport
#define    inp_faddr    inp_inc.inc_faddr
#define    inp_laddr    inp_inc.inc_laddr
#define    inp_svrf     inp_inc.inc_svrf
#define    inp_rvrf     inp_inc.inc_rvrf

#define    in6p_faddr    inp_inc.inc6_faddr
#define    in6p_laddr    inp_inc.inc6_laddr

    struct
    {
        UCHAR     inp4_ip_tos;        
        MBUF_S *inp4_options;    
        struct    ip_moptions *inp4_moptions; 
    } inp_depend4;
#define    inp_ip_tos    inp_depend4.inp4_ip_tos
#define    inp_options    inp_depend4.inp4_options
#define    inp_moptions    inp_depend4.inp4_moptions

    INPCB_LIST_ENTRY_S inp_portlist;
    INPCB_PORT_S *inp_phd;    
}INPCB_S;



typedef struct inpcbinfo
{
    INPCB_HEAD_S    *ipi_listhead;
    UINT            ipi_count[INV_BUTT];

    
    INPCB_HEAD_S    *ipi_hashbase;
    ULONG             ipi_hashmask;

    
    INPCB_PORT_HEAD_S    *ipi_porthashbase;
    ULONG             ipi_porthashmask;

    
    UINT64            ipi_gencnt;
    MUTEX_S           ipi_lock;

    
    USHORT   ipi_protocol;

}INPCB_INFO_S;

INPCB_S * in_pcblookup_hash
(
    IN INPCB_INFO_S *pcbinfo, 
    IN UINT faddr,
    IN USHORT fport_arg, 
    IN VRF_INDEX rvrf,
    IN UINT laddr,
    IN USHORT lport_arg, 
    IN int wildcard
);
int in_pcbbind_setup
(
    INPCB_S *inp,
    SOCKADDR_S *nam,
    VRF_INDEX *rvrfp,
    UINT *laddrp,
    USHORT *lportp
);

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
);

int in_pcbinshash(struct inpcb *inp);

INPCB_S * in_pcblookup_local
(
    IN INPCB_INFO_S *pcbinfo,
    IN VRF_INDEX rvrf,
    IN UINT laddr,
    IN USHORT lport,
    IN UINT faddr,
    int wild_okay
);

int in_pcballoc(IN SOCKET_S *so, IN INPCB_INFO_S *pcbinfo);

int in_pcbbind(IN INPCB_S *inp, IN SOCKADDR_S *nam);

int in_pcbconnect(IN INPCB_S *inp, IN SOCKADDR_S *nam);

void in_pcbdisconnect(IN INPCB_S *inp);

void in_pcbdetach(struct inpcb *inp);

int in_getpeeraddr(IN SOCKET_S *so, OUT SOCKADDR_S **nam);

int in_getsockaddr(IN SOCKET_S *so, IN SOCKADDR_S **nam);

void in_pcbsosetlabel(IN SOCKET_S *so);

void in_pcbfree(IN INPCB_S *inp);

void in_pcbnotifyall
(
    IN INPCB_INFO_S *pcbinfo,
    IN UINT faddr,
    IN VRF_INDEX rvrf,
    IN int errorno,
    INPCB_S *(*notify)(struct inpcb *, int)
);

#ifdef __cplusplus
    }
#endif 

#endif 


