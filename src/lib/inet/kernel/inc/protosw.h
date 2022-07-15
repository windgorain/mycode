/******************************************************************************
* Copyright (C) Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2012-11-15
* Description: 
* History:     
******************************************************************************/

#ifndef __PROTOSW_H_
#define __PROTOSW_H_

#include "utl/mbuf_utl.h"
#include "inet/in_pub.h"

#include "uipc_def.h"
#include "uipc_uio.h"

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

/*
 * Values for pr_flags.
 * PR_ADDR requires PR_ATOMIC;
 * PR_ADDR and PR_CONNREQUIRED are mutually exclusive.
 * PR_IMPLOPCL means that the protocol allows sendto without prior connect,
 *    and the protocol understands the MSG_EOF flag.  The first property is
 *    is only relevant if PR_CONNREQUIRED is set (otherwise sendto is allowed
 *    anyhow).
 */
#define    PR_ATOMIC    0x01        /* exchange atomic messages only */
#define    PR_ADDR        0x02        /* addresses given with messages */
#define    PR_CONNREQUIRED    0x04        /* connection required by protocol */
#define    PR_WANTRCVD    0x08        /* want PRU_RCVD calls */
#define    PR_RIGHTS    0x10        /* passes capabilities */
#define    PR_IMPLOPCL    0x20        /* implied open/close */
#define    PR_LASTHDR    0x40        /* enforce ipsec policy; last header */


/*
 * The arguments to the ctlinput routine are
 *    (*protosw[].pr_ctlinput)(cmd, sa, arg);
 * where cmd is one of the commands below, sa is a pointer to a sockaddr,
 * and arg is a `void *' argument used within a protocol family.
 */
#define    PRC_IFDOWN        0    /* interface transition */
#define    PRC_ROUTEDEAD        1    /* select new route if possible ??? */
#define    PRC_IFUP        2     /* interface has come back up */
#define    PRC_QUENCH2        3    /* DEC congestion bit says slow down */
#define    PRC_QUENCH        4    /* some one said to slow down */
#define    PRC_MSGSIZE        5    /* message size forced drop */
#define    PRC_HOSTDEAD        6    /* host appears to be down */
#define    PRC_HOSTUNREACH        7    /* deprecated (use PRC_UNREACH_HOST) */
#define    PRC_UNREACH_NET        8    /* no route to network */
#define    PRC_UNREACH_HOST    9    /* no route to host */
#define    PRC_UNREACH_PROTOCOL    10    /* dst says bad protocol */
#define    PRC_UNREACH_PORT    11    /* bad port # */
/* was    PRC_UNREACH_NEEDFRAG    12       (use PRC_MSGSIZE) */
#define    PRC_UNREACH_SRCFAIL    13    /* source route failed */
#define    PRC_REDIRECT_NET    14    /* net routing redirect */
#define    PRC_REDIRECT_HOST    15    /* host routing redirect */
#define    PRC_REDIRECT_TOSNET    16    /* redirect for type of service & net */
#define    PRC_REDIRECT_TOSHOST    17    /* redirect for tos & host */
#define    PRC_TIMXCEED_INTRANS    18    /* packet lifetime expired in transit */
#define    PRC_TIMXCEED_REASS    19    /* lifetime expired on reass q */
#define    PRC_PARAMPROB        20    /* header incorrect */
#define    PRC_UNREACH_ADMIN_PROHIB    21    /* packet administrativly prohibited */

#define    PRC_NCMDS        22

#define    PRC_IS_REDIRECT(cmd)    \
    ((cmd) >= PRC_REDIRECT_NET && (cmd) <= PRC_REDIRECT_TOSHOST)

struct ifnet
{
    IF_INDEX ifindex;
};

typedef struct
{
    double  __Break_the_struct_layout_for_now;
    void   (*pru_abort)(VOID *so);
    int    (*pru_accept)(VOID *so, SOCKADDR_S **ppNam);
    int    (*pru_attach)(VOID *so, int proto);
    int    (*pru_bind)(VOID *so, SOCKADDR_S *nam);
    int    (*pru_connect)(VOID *so, SOCKADDR_S *nam);
    int    (*pru_connect2)(VOID *so1, VOID *so2);
    int    (*pru_control)(VOID *so, UINT cmd, VOID *data, struct ifnet ifp);
    void   (*pru_detach)(VOID *so);
    int    (*pru_disconnect)(VOID *so);
    int    (*pru_listen)(VOID *so, int backlog);
    int    (*pru_peeraddr)(VOID *so, SOCKADDR_S **nam);
    int    (*pru_rcvd)(VOID *so, int flags);
    int    (*pru_rcvoob)(VOID *so, MBUF_S *m, int flags);
    int    (*pru_send)(VOID *so, IN int flags, IN MBUF_S *m, IN SOCKADDR_S *addr, IN MBUF_S *control);
#define    PRUS_OOB    0x1
#define    PRUS_EOF    0x2
#define    PRUS_MORETOCOME    0x4
    int    (*pru_sense)(VOID *so, VOID *sb);
    int    (*pru_shutdown)(VOID *so);
    int    (*pru_sockaddr)(VOID *so, SOCKADDR_S **nam);
    int    (*pru_sosend)(VOID *so, SOCKADDR_S *addr, UIO_S *uio, MBUF_S *top, MBUF_S *control, int flags);
    int    (*pru_soreceive)(VOID *so, SOCKADDR_S **paddr, UIO_S *uio, MBUF_S **mp0, MBUF_S **controlp, int *flagsp);
    UINT   (*pru_sopoll)(VOID *file, VOID *so, VOID *wait);
    void   (*pru_sosetlabel)(VOID *so);
    void   (*pru_close)(VOID *so);
}PROTOSW_USER_REQUEST_S;

typedef VOID (*pr_input_t)(IN MBUF_S *pstMbuf, IN UINT uiPayLoadOffset);
typedef int  (*pr_ctloutput_t)(VOID *pstSocket, SOCKOPT_S * pstSockOpt);
typedef void (*pr_ctlinput_t) (int, SOCKADDR_S *, void *);
typedef int  (*pr_output_t) (struct mbuf *, struct socket *);

typedef void  (*pr_init_t) ();
typedef int   (*pr_servinit_t) ();
typedef void  (*pr_fasttimo_t) ();
typedef void  (*pr_slowtimo_t) ();
typedef void  (*pr_drain_t) ();

struct domain_s;

typedef struct protosw
{
    struct domain_s *pr_domain;
    USHORT pr_type;
    USHORT pr_protocol;
    USHORT pr_flags;

    /* protocol-protocol hooks */
    pr_input_t pr_input;
    pr_output_t pr_output;
    pr_ctlinput_t pf_ctlinput;
    pr_ctloutput_t pr_ctloutput;

    /* utility hooks */
    pr_init_t pr_init;
    pr_servinit_t pr_servinit;
    pr_fasttimo_t pr_fasttimo;    /* fast timeout (200ms) */
    pr_slowtimo_t pr_slowtimo;    /* slow timeout (500ms) */
    pr_drain_t pr_drain;        /* flush any excess space possible */
    
    PROTOSW_USER_REQUEST_S *pr_usrreqs;
}PROTOSW_S;

VOID IN_Proto_Init();

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__PROTOSW_H_*/


