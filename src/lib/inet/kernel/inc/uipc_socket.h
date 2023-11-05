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
#endif 

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


#define SS_NOFDREF         0x0001    
#define SS_ISCONNECTED     0x0002    
#define SS_ISCONNECTING    0x0004    
#define SS_ISDISCONNECTING 0x0008    
#define SS_NBIO            0x0100    
#define SS_ISCONFIRMING    0x0400    
#define SS_CANBIND         0x1000    
#define SS_ISDISCONNECTED  0x2000    
#define SS_PROTOREF        0x4000    


#define    SQ_INCOMP        0x0800    
#define    SQ_COMP            0x1000    



#define SO_DEBUG         0x0001        
#define SO_ACCEPTCONN    0x0002        
#define SO_REUSEADDR     0x0004        
#define SO_KEEPALIVE     0x0008        
#define SO_DONTROUTE     0x0010        
#define SO_BROADCAST     0x0020        
#define SO_TIMESTAMP     0x0400        
#define SO_ACCEPTFILTER  0x1000        
#define SO_TIMESTAMPNS   0x2000      


#define    SOCK_SEM(_so) SOCKBUF_MTX(&(_so)->so_rcv)
#define    SOCK_LOCK(_so) SOCKBUF_LOCK(&(_so)->so_rcv)
#define    SOCK_UNLOCK(_so) SOCKBUF_UNLOCK(&(_so)->so_rcv)



#define    SBS_CANTSENDMORE    0x0010    
#define    SBS_CANTRCVMORE        0x0020    
#define    SBS_RCVATMARK        0x0040    

typedef struct socket
{
    struct socket *so_head;
    UPIC_FD_S *so_file;
    INT iFd;
    USHORT so_type;
    USHORT so_state;
    SHORT  so_linger;     
    UINT   so_qstate;     
    void    *so_pcb;      
    UINT so_count;              
    PROTOSW_S *so_proto;
    TAILQ_HEAD(, socket) so_incomp;    
    TAILQ_HEAD(, socket) so_comp;      
    TAILQ_ENTRY(socket) so_list;
    USHORT    so_qlen;             
    USHORT    so_incqlen;          
    USHORT    so_qlimit;           
    SOCKBUF_S so_rcv;
    SOCKBUF_S so_snd;
    INT so_options;
    USHORT  so_error;           
    SEM_HANDLE so_wait;

    void (*so_upcall)(struct socket *, void *, int);
    void  *so_upcallarg;

    struct so_accf {
        struct    accept_filter *so_accept_filter;
        void    *so_accept_filter_arg;    
        char    *so_accept_filter_str;    
    } *so_accf;
}SOCKET_S;

VOID UIPC_Socket_AcceptLock();
VOID UIPC_Socket_AcceptUnLock();
SEM_HANDLE UIPC_Socket_GetSem();

#ifdef __cplusplus
    }
#endif 

#endif 


