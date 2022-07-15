/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2012-11-20
* Description: 
* History:     
******************************************************************************/

#ifndef __UIPC_SOCKET_H_
#define __UIPC_SOCKET_H_

#include "utl/sem_utl.h"
#include "utl/mutex_utl.h"
#include "utl/mbuf_utl.h"
#include "utl/list_utl.h"

#include "uipc_sockbuf.h"
#include "uipc_fd.h"

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

#define    ACCEPT_LOCK()          UIPC_Socket_AcceptLock()
#define    ACCEPT_UNLOCK()        UIPC_Socket_AcceptUnLock()
#define    ACCEPT_SEM()           UIPC_Socket_GetSem()

#define soref(so) do { \
    ++(so)->so_count; \
} while (0)

#define sorele(so) do { \
    if (--(so)->so_count == 0) { \
        sofree(so); \
    } else { \
        SOCK_UNLOCK(so); \
        ACCEPT_UNLOCK(); \
    } \
} while (0)

/*
 * Do we need to notify the other side when I/O is possible?
 */
#define    sb_notify(sb)    ((0 != SEM_CountPending(((sb)->sb_wait))) || \
    (((sb)->sb_flags & (SB_WAIT | SB_SEL | SB_ASYNC | SB_UPCALL | SB_AIO | SB_KNOTE)) != 0)) 


#define sorwakeup_locked(so) do { \
        if (sb_notify(&(so)->so_rcv)) { \
            sowakeup((so), &(so)->so_rcv); \
        } else { \
            SOCKBUF_UNLOCK(&(so)->so_rcv); \
        } \
    } while (0)

#define sowwakeup_locked(so) do { \
    SOCKBUF_LOCK_ASSERT(&(so)->so_snd); \
    if (sb_notify(&(so)->so_snd)) { \
        sowakeup((so), &(so)->so_snd); \
    } else { \
        SOCKBUF_UNLOCK(&(so)->so_snd); \
    } \
} while (0)

#define sorwakeup(so) do { \
    SOCKBUF_LOCK(&(so)->so_rcv); \
    sorwakeup_locked(so); \
} while (0)

#define sowwakeup(so) do { \
    SOCKBUF_LOCK(&(so)->so_snd); \
    sowwakeup_locked(so); \
} while (0)

struct accept_filter {
    char    accf_name[16];
    void    (*accf_callback)
        (struct socket *so, void *arg, int waitflag);
    void *    (*accf_create)
        (struct socket *so, char *arg);
    void    (*accf_destroy)
        (struct socket *so);
    SLIST_ENTRY(accept_filter) accf_next;
};

/*
 * Socket state bits.
 *
 * Historically, this bits were all kept in the so_state field.  For
 * locking reasons, they are now in multiple fields, as they are
 * locked differently.  so_state maintains basic socket state protected
 * by the socket lock.  so_qstate holds information about the socket
 * accept queues.  Each socket buffer also has a state field holding
 * information relevant to that socket buffer (can't send, rcv).  Many
 * fields will be read without locks to improve performance and avoid
 * lock order issues.  However, this approach must be used with caution.
 */
#define SS_NOFDREF         0x0001    /* no file table ref any more */
#define SS_ISCONNECTED     0x0002    /* socket connected to a peer */
#define SS_ISCONNECTING    0x0004    /* in process of connecting to peer */
#define SS_ISDISCONNECTING 0x0008    /* in process of disconnecting */
#define SS_NBIO            0x0100    /* non-blocking ops(in Leopard,this won't be used) */
#define SS_ISCONFIRMING    0x0400    /* deciding to accept connection req */
#define SS_CANBIND         0x1000    /* can bind */
#define SS_ISDISCONNECTED  0x2000    /* socket disconnected from peer */
#define SS_PROTOREF        0x4000    /* strong protocol reference */

/*
 * Socket state bits stored in so_qstate.
 */
#define    SQ_INCOMP        0x0800    /* unaccepted, incomplete connection */
#define    SQ_COMP            0x1000    /* unaccepted, complete connection */


/*
 * Socket Level Options
 */
#define SO_DEBUG         0x0001        /* turn on debugging info recording */
#define SO_ACCEPTCONN    0x0002        /* socket has had listen() */
#define SO_REUSEADDR     0x0004        /* allow local address reuse */
#define SO_KEEPALIVE     0x0008        /* keep connections alive */
#define SO_DONTROUTE     0x0010        /* just use interface addresses */
#define SO_BROADCAST     0x0020        /* permit sending of broadcast msgs */
#define SO_TIMESTAMP     0x0400        /* timestamp received dgram traffic */
#define SO_ACCEPTFILTER  0x1000        /* there is an accept filter */
#define SO_TIMESTAMPNS   0x2000      /* timestamp received dgram traffic(ns) */


#define    SOCK_SEM(_so) SOCKBUF_MTX(&(_so)->so_rcv)
#define    SOCK_LOCK(_so) SOCKBUF_LOCK(&(_so)->so_rcv)
#define    SOCK_UNLOCK(_so) SOCKBUF_UNLOCK(&(_so)->so_rcv)


/*
 * Socket state bits now stored in the socket buffer state field.
 */
#define    SBS_CANTSENDMORE    0x0010    /* can't send more data to peer */
#define    SBS_CANTRCVMORE        0x0020    /* can't receive more data from peer */
#define    SBS_RCVATMARK        0x0040    /* at mark on input */

typedef struct socket
{
    struct socket *so_head;
    UPIC_FD_S *so_file;
    INT iFd;
    USHORT so_type;
    USHORT so_state;
    SHORT  so_linger;     /* time to linger while closing */
    UINT   so_qstate;     /* (e) internal state flags SQ_* */
    void    *so_pcb;      /* protocol control block */
    UINT so_count;              /* 引用计数 */
    PROTOSW_S *so_proto;
    TAILQ_HEAD(, socket) so_incomp;    /* queue of partial unaccepted connections */
    TAILQ_HEAD(, socket) so_comp;      /* queue of complete unaccepted connections */
    TAILQ_ENTRY(socket) so_list;
    USHORT    so_qlen;             /* (e) number of unaccepted connections */
    USHORT    so_incqlen;          /* (e) number of unaccepted incomplete connections */
    USHORT    so_qlimit;           /* (e) max number queued connections */
    SOCKBUF_S so_rcv;
    SOCKBUF_S so_snd;
    INT so_options;
    USHORT  so_error;           /* error affecting connection */
    SEM_HANDLE so_wait;

    void (*so_upcall)(struct socket *, void *, int);
    void  *so_upcallarg;

    struct so_accf {
        struct    accept_filter *so_accept_filter;
        void    *so_accept_filter_arg;    /* saved filter args */
        char    *so_accept_filter_str;    /* saved user args */
    } *so_accf;
}SOCKET_S;

VOID UIPC_Socket_AcceptLock();
VOID UIPC_Socket_AcceptUnLock();
SEM_HANDLE UIPC_Socket_GetSem();

#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__UIPC_SOCKET_H_*/


