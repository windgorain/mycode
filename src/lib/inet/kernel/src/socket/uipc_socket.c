/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2012-11-20
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/mutex_utl.h"
#include "inet/in_pub.h"

#include "protosw.h"
#include "domain.h"
#include "uipc_def.h"
#include "uipc_socket.h"
#include "uipc_sock_func.h"
#include "uipc_func.h"
#include "uipc_compat.h"

static SEM_HANDLE g_hUipcSocketAcceptSem = NULL;

VOID UIPC_Socket_Init()
{
    g_hUipcSocketAcceptSem = SEM_CCreate("uipc_accept", 1);
}

static int pru_notsupp()
{
    return BS_NOT_SUPPORT;
}

static SOCKET_S * soalloc()
{
    SOCKET_S *pstSo;

    pstSo = MEM_ZMalloc(sizeof(SOCKET_S));
    if (pstSo == NULL)
    {
        return (NULL);
    }

    pstSo->so_rcv.sb_wait = SEM_CCreate("so_rcv", 1);
    pstSo->so_snd.sb_wait = SEM_CCreate("so_snd", 1);

    TAILQ_INIT(&pstSo->so_comp);
    TAILQ_INIT(&pstSo->so_incomp);

    pstSo->so_count = 1;

    return pstSo;
}

static void sodealloc(IN SOCKET_S *pstSocket)
{
    SEM_Destory(pstSocket->so_rcv.sb_wait);
    SEM_Destory(pstSocket->so_snd.sb_wait);
    
    MEM_Free(pstSocket);
}

VOID UIPC_Socket_AcceptLock()
{
    SEM_P(g_hUipcSocketAcceptSem, BS_WAIT, BS_WAIT_FOREVER);
}

VOID UIPC_Socket_AcceptUnLock()
{
    SEM_V(g_hUipcSocketAcceptSem);
}

SEM_HANDLE UIPC_Socket_GetSem()
{
    return g_hUipcSocketAcceptSem;
}

SOCKET_S * socreate(IN UINT uiDomain, IN USHORT usType, IN USHORT usProtocol)
{
    PROTOSW_S *prp;
    SOCKET_S *so;
    int error;

    if (usProtocol)
    {
        prp = DOMAIN_FindProto(uiDomain, usProtocol, usType);
    }
    else
    {
        prp = DOMAIN_FindType(uiDomain, usType);
    }

    if ((prp == NULL)
       || (prp->pr_usrreqs->pru_attach == NULL)
       || (prp->pr_usrreqs->pru_attach == (VOID*)pru_notsupp))
    {
        return NULL;
    }

    so = soalloc();
    if (so == NULL)
    {
        return NULL;
    }

    so->so_type = usType;
    so->so_proto = prp;

    error = (*prp->pr_usrreqs->pru_attach)(so, usProtocol);
    if (error)
    {
        so->so_count = 0;
        sodealloc(so);
        return NULL;
    }

    return so;
}

void sofree(IN SOCKET_S *so)
{
    PROTOSW_S *pr = so->so_proto;
    SOCKET_S *head;

    if ((so->so_state & SS_NOFDREF) == 0 || so->so_count != 0 ||
        (so->so_state & SS_PROTOREF) || (so->so_qstate & SQ_COMP))
    {
        SOCK_UNLOCK(so);
        ACCEPT_UNLOCK();
        return;
    }

    head = so->so_head;
    if (head != NULL)
    {
        BS_DBGASSERT((so->so_qstate & SQ_COMP) != 0 || (so->so_qstate & SQ_INCOMP) != 0);
        BS_DBGASSERT((so->so_qstate & SQ_COMP) == 0 || (so->so_qstate & SQ_INCOMP) == 0);
        TAILQ_REMOVE(&head->so_incomp, so, so_list);
        head->so_incqlen--;
        so->so_qstate &= ~SQ_INCOMP;
        so->so_head = NULL;
    }
    
    BS_DBGASSERT((so->so_qstate & SQ_COMP) == 0 && (so->so_qstate & SQ_INCOMP) == 0);

    if (so->so_options & SO_ACCEPTCONN)
    {
        BS_DBGASSERT(TAILQ_EMPTY(&so->so_comp));
        BS_DBGASSERT(TAILQ_EMPTY(&so->so_incomp));
    }
    SOCK_UNLOCK(so);
    ACCEPT_UNLOCK();

    if (pr->pr_flags & PR_RIGHTS && pr->pr_domain->dom_dispose != NULL)
    {
        (*pr->pr_domain->dom_dispose)(so->so_rcv.sb_mb);
    }

    if (pr->pr_usrreqs->pru_detach != NULL)
    {
        (*pr->pr_usrreqs->pru_detach)(so);
    }

    /*
     * From this point on, we assume that no other references to this
     * socket exist anywhere else in the stack.  Therefore, no locks need
     * to be acquired or held.
     *
     * We used to do a lot of socket buffer and socket locking here, as
     * well as invoke sorflush() and perform wakeups.  The direct call to
     * dom_dispose() and sbrelease_internal() are an inlining of what was
     * necessary from sorflush().
     *
     * Notice that the socket buffer and kqueue state are torn down
     * before calling pru_detach.  This means that protocols shold not
     * assume they can perform socket wakeups, etc, in their detach code.
     */
    sbdestroy(&so->so_snd, so);
    sbdestroy(&so->so_rcv, so);

    SEM_VAll(so->so_wait);
    SEM_VAll(so->so_rcv.sb_wait);
    SEM_VAll(so->so_snd.sb_wait);

    sodealloc(so);
}

/*
丢弃一个新连接的接口。此接口用于新连接未完成时而发生了某些异常情况时，可以直接调用此接口丢弃此连接
*/
void soabort(IN SOCKET_S *so)
{
    BS_DBGASSERT(so->so_count == 0);
    BS_DBGASSERT((so->so_state & SS_PROTOREF) == 0);
    BS_DBGASSERT(so->so_state & SS_NOFDREF);
    BS_DBGASSERT((so->so_state & SQ_COMP) == 0);
    BS_DBGASSERT((so->so_state & SQ_INCOMP) == 0);

    if (so->so_proto->pr_usrreqs->pru_abort != NULL)
    {
        (*so->so_proto->pr_usrreqs->pru_abort)(so);
    }
    ACCEPT_LOCK();
    SOCK_LOCK(so);
    sofree(so);
}

int soclose(IN SOCKET_S *so)
{
    int error = 0;

    if (so->so_state & SS_ISCONNECTED)
    {
        if ((so->so_state & SS_ISDISCONNECTING) == 0)
        {
            error = sodisconnect(so);
            if (error)
            {
                goto drop;
            }
        }

        if (so->so_options & SO_LINGER)
        {
            if ((so->so_state & SS_ISDISCONNECTING)
                &&((so->so_file) && (so->so_file->f_flags & O_NONBLOCK)))
            {
                goto drop;
            }

            /*对sock接收缓冲加锁，避免被软中断抢占后,so_state状态已经断开，导致soclose长时间无法唤醒*/
            SOCK_LOCK(so);
            while (so->so_state & SS_ISCONNECTED)
            {
                if (BS_TIME_OUT == so_msleep(so->so_wait, SOCK_SEM(so), so->so_linger))
                {
                    error = EINTR;
                }

                if (error)
                {
                    break;
                }
            }
            SOCK_UNLOCK(so);
        }
    }

drop:
    if (so->so_proto->pr_usrreqs->pru_close != NULL)
    {
        (*so->so_proto->pr_usrreqs->pru_close)(so);
    }

    if (so->so_options & SO_ACCEPTCONN)
    {
        SOCKET_S *sp;
        ACCEPT_LOCK();
        while ((sp = TAILQ_FIRST(&so->so_incomp)) != NULL)
        {
            TAILQ_REMOVE(&so->so_incomp, sp, so_list);
            so->so_incqlen--;
            sp->so_qstate &= ~SQ_INCOMP;
            sp->so_head = NULL;
            ACCEPT_UNLOCK();
            soabort(sp);
            ACCEPT_LOCK();
        }

        while ((sp = TAILQ_FIRST(&so->so_comp)) != NULL)
        {
            TAILQ_REMOVE(&so->so_comp, sp, so_list);
            so->so_qlen--;
            sp->so_qstate &= ~SQ_COMP;
            sp->so_head = NULL;
            ACCEPT_UNLOCK();
            soabort(sp);
            ACCEPT_LOCK();
        }
        ACCEPT_UNLOCK();
    }
    ACCEPT_LOCK();
    SOCK_LOCK(so);
    BS_DBGASSERT((so->so_state & SS_NOFDREF) == 0);
    so->so_state |= SS_NOFDREF;
    so->so_file = NULL;
    sorele(so);
    return (error);
}

int sobind(IN SOCKET_S *so, IN SOCKADDR_S *nam)
{
    so->so_state &= ~SS_CANBIND;

    return ((*so->so_proto->pr_usrreqs->pru_bind)(so, nam));
}

int solisten(IN SOCKET_S *so, IN int iBackLog)
{
    return ((*so->so_proto->pr_usrreqs->pru_listen)(so, iBackLog));
}

int soaccept(IN SOCKET_S *so, OUT SOCKADDR_S **ppNam)
{
    int error;

    SOCK_LOCK(so);
    so->so_state &= ~SS_NOFDREF;
    SOCK_UNLOCK(so);
    error = (*so->so_proto->pr_usrreqs->pru_accept)(so, ppNam);
    return (error);
}

int soconnect(IN SOCKET_S *so, SOCKADDR_S *nam)
{
    int error;

    if (so->so_options & SO_ACCEPTCONN)
        return (EOPNOTSUPP);
    /*
     * If protocol is connection-based, can only connect once.
     * Otherwise, if connected, try to disconnect first.  This allows
     * user to disconnect by connecting to, e.g., a null address.
     */
    if (so->so_state & (SS_ISCONNECTED|SS_ISCONNECTING))
    {
        if (so->so_proto->pr_flags & PR_CONNREQUIRED)
        {
            return (EISCONN);
        }
        else
        {
            error = sodisconnect(so);
            if (error)
            {
                return (EISCONN);
            }
        }
    }

    /*
     * Prevent accumulated error from previous connection from
     * biting us.
     */
    so->so_error = 0;
    error = (*so->so_proto->pr_usrreqs->pru_connect)(so, nam);

    return (error);
}

int sodisconnect(IN SOCKET_S *so)
{
    int error;

    if ((so->so_state & SS_ISCONNECTED) == 0)
    {
        return (ENOTCONN);
    }
    
    if (so->so_state & SS_ISDISCONNECTING)
    {
        return (EALREADY);
    }
    
    error = (*so->so_proto->pr_usrreqs->pru_disconnect)(so);
    
    return (error);
}

int sosend
(
    IN SOCKET_S *so,
    IN SOCKADDR_S *addr,
    IN UIO_S *uio,
    IN MBUF_S *top,
    IN MBUF_S *control,
    IN int flags
)
{
    return (so->so_proto->pr_usrreqs->pru_sosend(so, addr, uio, top, control, flags));
}

int soreceive
(
    IN SOCKET_S *so,
    OUT SOCKADDR_S **psa,
    OUT UIO_S *uio,
    OUT MBUF_S **mp0,
    OUT MBUF_S **controlp,
    INOUT int *flagsp
)
{
    return (so->so_proto->pr_usrreqs->pru_soreceive(so, psa, uio, mp0, controlp, flagsp));
}

int sosetopt(IN SOCKET_S *so, IN SOCKOPT_S *sopt)
{
    int    error, optval;
    struct linger *l;
    KEEP_ALIVE_ARG_S keep_alive = {0};
    struct timeval *pTv;
    UINT val;

    if (sopt->sopt_level != SOL_SOCKET)
    {
        if (so->so_proto && so->so_proto->pr_ctloutput)
        {
            return ((*so->so_proto->pr_ctloutput)(so, sopt));
        }

        return ENOPROTOOPT;
    }

    switch (sopt->sopt_name)
    {
    case SO_LINGER:
        l = sopt->sopt_val;
        SOCK_LOCK(so);
        so->so_linger = (short)l->l_linger;
        if (l->l_onoff)
            so->so_options |= SO_LINGER;
        else
            so->so_options &= ~SO_LINGER;
        SOCK_UNLOCK(so);
        break;

    case SO_DEBUG:
    case SO_KEEPALIVE:
    case SO_DONTROUTE:
    case SO_USELOOPBACK:
    case SO_BROADCAST:
    case SO_REUSEADDR:
    case SO_OOBINLINE:
        optval = *((int*)sopt->sopt_val);
        SOCK_LOCK(so);
        if (optval)
            so->so_options |= (short)sopt->sopt_name;
        else
            so->so_options &= (short)~(u_int)(sopt->sopt_name);
        SOCK_UNLOCK(so);
        break;

    case SO_SNDBUF:
    case SO_RCVBUF:
    case SO_SNDLOWAT:
    case SO_RCVLOWAT:
        optval = *((int*)sopt->sopt_val);
        if (optval < 1)
        {
            error = EINVAL;
            goto bad;
        }

        switch (sopt->sopt_name)
        {
        case SO_SNDBUF:
        case SO_RCVBUF:
            if (sbreserve(sopt->sopt_name == SO_SNDBUF ?
                &so->so_snd : &so->so_rcv, (u_long)(long)optval,
                so) == 0)
            {
                error = ENOBUFS;
                goto bad;
            }

            (sopt->sopt_name == SO_SNDBUF ? &so->so_snd : &so->so_rcv)->sb_flags &= ~SB_AUTOSIZE;
            break;

        /*
         * Make sure the low-water is never greater than the
         * high-water.
         */
        case SO_SNDLOWAT:
            SOCKBUF_LOCK(&so->so_snd);
            so->so_snd.sb_lowat =
                ((u_int)optval > so->so_snd.sb_hiwat) ?
                so->so_snd.sb_hiwat : (u_int)optval;
            SOCKBUF_UNLOCK(&so->so_snd);
            break;
        case SO_RCVLOWAT:
            SOCKBUF_LOCK(&so->so_rcv);
            so->so_rcv.sb_lowat =
                ((u_int)optval > so->so_rcv.sb_hiwat) ?
                so->so_rcv.sb_hiwat : (u_int)optval;
            SOCKBUF_UNLOCK(&so->so_rcv);
            break;

        default:
            break;
        }
        break;

    case SO_SNDTIMEO:
    case SO_RCVTIMEO:
        pTv = sopt->sopt_val;
        val = pTv->tv_sec * 1000 + pTv->tv_usec * 1000;
        if (val == 0 && pTv->tv_usec != 0)
        {
            val = 1UL;
        }
        switch (sopt->sopt_name)
        {
        case SO_SNDTIMEO:
            so->so_snd.sb_timeo = (int)val;
            break;
        case SO_RCVTIMEO:
            so->so_rcv.sb_timeo = (int)val;
            break;
        default:
            break;
        }
        break;
    default:
        error = ENOPROTOOPT;
        break;
    }

    if (error == 0 && so->so_proto != NULL && so->so_proto->pr_ctloutput != NULL)
    {
        (void) ((*so->so_proto->pr_ctloutput)(so, sopt));
    }

bad:
    return (error);
}

int sogetopt(IN SOCKET_S *so, INOUT SOCKOPT_S *sopt)
{
    int  error;
    int  optval;
    struct linger *pL;
    struct timeval *pTv;
    INT *piOpt;

    error = 0;
    if (sopt->sopt_level != SOL_SOCKET)
    {
        if (so->so_proto && so->so_proto->pr_ctloutput)
        {
            return ((*so->so_proto->pr_ctloutput)(so, sopt));
        }

        return (ENOPROTOOPT);
    }

    piOpt = sopt->sopt_val;

    switch (sopt->sopt_name)
    {
    case SO_LINGER:
        pL = sopt->sopt_val;
        SOCK_LOCK(so);
        pL->l_onoff = so->so_options & SO_LINGER;
        pL->l_linger = so->so_linger;
        SOCK_UNLOCK(so);
        break;

    case SO_USELOOPBACK:
    case SO_DONTROUTE:
    case SO_DEBUG:
    case SO_KEEPALIVE:
    case SO_REUSEADDR:
    case SO_BROADCAST:
    case SO_OOBINLINE:
    case SO_ACCEPTCONN:
        *piOpt = so->so_options & sopt->sopt_name;
        break;

    case SO_TYPE:
        *piOpt = so->so_type;
        break;

    case SO_ERROR:
        SOCK_LOCK(so);
        *piOpt = so->so_error;
        so->so_error = 0;
        SOCK_UNLOCK(so);
        break;

    case SO_SNDBUF:
        *piOpt = (int)so->so_snd.sb_hiwat;
        break;

    case SO_RCVBUF:
        *piOpt = (int)so->so_rcv.sb_hiwat;
        break;

    case SO_SNDLOWAT:
        *piOpt = (int)so->so_snd.sb_lowat;
        break;

    case SO_RCVLOWAT:
        *piOpt = (int)so->so_rcv.sb_lowat;
        break;

    case SO_SNDTIMEO:
    case SO_RCVTIMEO:
        pTv = sopt->sopt_val;
        optval = (sopt->sopt_name == SO_SNDTIMEO ? so->so_snd.sb_timeo : so->so_rcv.sb_timeo);
        pTv->tv_sec = (long)optval / 1000;
        pTv->tv_usec = (optval % 1000) * 1000;
        break;

    default:
        error = ENOPROTOOPT;
        break;
    }

    return (error);
}

int sogetsockname(IN SOCKET_S *so, OUT SOCKADDR_S **ppstSa)
{
    return (*so->so_proto->pr_usrreqs->pru_sockaddr)(so, ppstSa);
}

int sogetpeername(IN SOCKET_S *so, OUT SOCKADDR_S **ppstSa)
{
    return (*so->so_proto->pr_usrreqs->pru_peeraddr)(so, ppstSa);
}

void soisconnected(IN SOCKET_S *so)
{
    SOCKET_S *head;

    ACCEPT_LOCK();
    SOCK_LOCK(so);
    so->so_state &= ~(SS_ISCONNECTING|SS_ISDISCONNECTING|SS_ISCONFIRMING);
    so->so_state |= SS_ISCONNECTED;
    head = so->so_head;
    if (head != NULL && (so->so_qstate & SQ_INCOMP))
    {
        if ((so->so_options & SO_ACCEPTFILTER) == 0)
        {
            SOCK_UNLOCK(so);
            TAILQ_REMOVE(&head->so_incomp, so, so_list);
            head->so_incqlen--;
            so->so_qstate &= ~SQ_INCOMP;
            TAILQ_INSERT_TAIL(&head->so_comp, so, so_list);
            head->so_qlen++;
            so->so_qstate |= SQ_COMP;
            ACCEPT_UNLOCK();
            sorwakeup(head);
            SEM_V(head->so_wait);
        }
        else
        {
            ACCEPT_UNLOCK();
            so->so_upcall = head->so_accf->so_accept_filter->accf_callback;
            so->so_upcallarg = head->so_accf->so_accept_filter_arg;
            so->so_rcv.sb_flags |= SB_UPCALL;
            so->so_options &= ~SO_ACCEPTFILTER;
            SOCK_UNLOCK(so);
            so->so_upcall(so, so->so_upcallarg, M_DONTWAIT);
        }
        return;
    }
    SOCK_UNLOCK(so);
    ACCEPT_UNLOCK();
    SEM_VAll(so->so_wait);
    sorwakeup(so);
    sowwakeup(so);
}

int sosend_dgram
(
    IN SOCKET_S *so,
    IN SOCKADDR_S *addr,
    IN UIO_S *uio,
    IN MBUF_S *top, 
    IN MBUF_S *control,
    IN int flags
)
{
    int space, resid;
    int clen = 0, error, dontroute;

    BS_DBGASSERT(so->so_type == SOCK_DGRAM);
    BS_DBGASSERT(so->so_proto->pr_flags & PR_ATOMIC);

    if (uio != NULL)
    {
        resid = uio->uio_resid;
    }
    else
    {
        resid = MBUF_TOTAL_DATA_LEN(top);
    }
    
    /*
     * In theory resid should be unsigned.  However, space must be
     * signed, as it might be less than 0 if we over-committed, and we
     * must use a signed comparison of space and resid.  On the other
     * hand, a negative resid causes us to loop sending 0-length
     * segments to the protocol.
     *
     * Also check to make sure that MSG_EOR isn't used on SOCK_STREAM
     * type sockets since that's an error.
     */
    if (resid < 0)
    {
        error = EINVAL;
        goto out;
    }

    if ((flags & MSG_DONTROUTE) && (so->so_options & SO_DONTROUTE) == 0)
    {
        dontroute = 1;
    }
    else
    {
        dontroute = 0;
    }

    if (control != NULL)
    {
        clen = (int)MBUF_TOTAL_DATA_LEN(control);
    }

    SOCKBUF_LOCK(&so->so_snd);
    if (so->so_snd.sb_state & SBS_CANTSENDMORE)
    {
        SOCKBUF_UNLOCK(&so->so_snd);
        error = EPIPE;
        goto out;
    }

    if (so->so_error)
    {
        error = so->so_error;
        so->so_error = 0;
        SOCKBUF_UNLOCK(&so->so_snd);
        goto out;
    }
    
    if ((so->so_state & SS_ISCONNECTED) == 0)
    {
        /*
         * `sendto' and `sendmsg' is allowed on a connection-based
         * socket if it supports implied connect.  Return ENOTCONN if
         * not connected and no address is supplied.
         */
        if ((so->so_proto->pr_flags & PR_CONNREQUIRED) &&
            (so->so_proto->pr_flags & PR_IMPLOPCL) == 0)
        {
            if ((so->so_state & SS_ISCONFIRMING) == 0 && !(resid == 0 && clen != 0))
            {
                SOCKBUF_UNLOCK(&so->so_snd);
                error = ENOTCONN;
                goto out;
            }
        }
        else if (addr == NULL)
        {
            if (so->so_proto->pr_flags & PR_CONNREQUIRED)
            {
                error = ENOTCONN;
            }
            else
            {
                error = EDESTADDRREQ;
            }
            SOCKBUF_UNLOCK(&so->so_snd);
            goto out;
        }
    }

    /*
     * Do we need MSG_OOB support in SOCK_DGRAM?  Signs here may be a
     * problem and need fixing.
     */
    space = sbspace(&so->so_snd);
    if (flags & MSG_OOB)
    {
        space += 1024;
    }
    space -= clen;
    SOCKBUF_UNLOCK(&so->so_snd);
    if (resid > space)
    {
        error = EMSGSIZE;
        goto out;
    }
    if (uio == NULL)
    {
        resid = 0;
        /* MSG_EOR will not supported int Leopard
        if (flags & MSG_EOR)
            top->m_flags |= M_EOR;
        */
    }
    else
    {
        /*
         * Copy the data from userland into a mbuf chain.
         * If no data is to be copied in, a single empty mbuf
         * is returned.
         */
        top = m_uiotombuf(uio, space, max_hdr);
        if (top == NULL)
        {
            error = EFAULT;    /* only possible error */
            goto out;
        }
        space -= resid - uio->uio_resid;
        resid = uio->uio_resid;
    }
    BS_DBGASSERT(resid == 0);
    /*
     * XXXRW: Frobbing SO_DONTROUTE here is even worse without sblock
     * than with.
     */
    if (dontroute)
    {
        SOCK_LOCK(so);
        so->so_options |= SO_DONTROUTE;
        SOCK_UNLOCK(so);
    }
    /*
     * XXX all the SBS_CANTSENDMORE checks previously done could be out
     * of date.  We could have recieved a reset packet in an interrupt or
     * maybe we slept while doing page faults in uiomove() etc.  We could
     * probably recheck again inside the locking protection here, but
     * there are probably other places that this also happens.  We must
     * rethink this.
     */
    error = (*so->so_proto->pr_usrreqs->pru_send)(so,
            (flags & MSG_OOB) ? PRUS_OOB :((flags & MSG_EOF) && (so->so_proto->pr_flags & PR_IMPLOPCL) && (resid <= 0)) ? PRUS_EOF : (resid > 0 && space > 0) ? PRUS_MORETOCOME : 0,
            top, addr, control);
    if (dontroute)
    {
        SOCK_LOCK(so);
        so->so_options &= ~SO_DONTROUTE;
        SOCK_UNLOCK(so);
    }
    clen = 0;
    control = NULL;
    top = NULL;
out:
    if (top != NULL)
        MBUF_Free(top);
    if (control != NULL)
        MBUF_Free(control);
    return (error);
}

void soisdisconnected(IN SOCKET_S *so)
{

    /*
     * Note: This code assumes that SOCK_LOCK(so) and
     * SOCKBUF_LOCK(&so->so_rcv) are the same.
     */
    SOCKBUF_LOCK(&so->so_rcv);
    so->so_state &= ~(SS_ISCONNECTING|SS_ISCONNECTED|SS_ISDISCONNECTING);
    so->so_state |= SS_ISDISCONNECTED;
    so->so_rcv.sb_state |= SBS_CANTRCVMORE;
    sorwakeup_locked(so);
    SOCKBUF_LOCK(&so->so_snd);
    so->so_snd.sb_state |= SBS_CANTSENDMORE;
    sbdrop_locked(&so->so_snd, (int)so->so_snd.sb_cc);
    sowwakeup_locked(so);
    so_wakeup(so->so_wait);
}

int sooptcopyin(IN struct sockopt *sopt, OUT void *buf, IN UINT len, IN UINT minlen)
{
    UINT valsize;

    /*
     * If the user gives us more than we wanted, we ignore it, but if we
     * don't get the minimum length the caller wants, we return EINVAL.
     * On success, sopt->sopt_valsize is set to however much we actually
     * retrieved.
     */
    if ((valsize = sopt->sopt_valsize) < minlen)
    {
        return EINVAL;
    }
    
    if (valsize > len)
    {
        sopt->sopt_valsize = valsize = len;
    }

    bcopy(sopt->sopt_val, buf, valsize);
    return (0);
}

int sooptcopyout(OUT struct sockopt *sopt, IN const void *buf, IN UINT len)
{
    int    error;
    UINT    valsize;

    error = 0;

    /*
     * Documented get behavior is that we always return a value, possibly
     * truncated to fit in the user's buffer.  Traditional behavior is
     * that we always tell the user precisely how much we copied, rather
     * than something useful like the total amount we had available for
     * her.  Note that this interface is not idempotent; the entire
     * answer must generated ahead of time.
     */
    valsize = MIN(len, (UINT)sopt->sopt_valsize);
    sopt->sopt_valsize = valsize;
    if (sopt->sopt_val != NULL)
    {
        bcopy(buf, sopt->sopt_val, valsize);
    }
    return (error);
}
