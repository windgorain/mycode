/******************************************************************************
* Copyright (C) Xingang.Li
* Author:      Xingang.Li  Version: 1.0  Date: 2012-11-15
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "inet/in_pub.h"

#include "protosw.h"
#include "uipc_socket.h"
#include "uipc_uio.h"

#include "uipc_sock_func.h"
#include "uipc_fd_func.h"

int uipc_socket(IN UINT uiDomain, IN USHORT usType, IN USHORT usProtocol)
{
    INT iFd;
    UPIC_FD_S *pstFp;
    SOCKET_S *pstSocket;

    iFd = UIPC_FD_FAlloc(&pstFp);
    if (iFd < 0)
    {
        return iFd;
    }

    pstSocket = socreate(uiDomain, usType, usProtocol);
    if (pstSocket == NULL)
    {
        UIPC_FD_FClose(pstFp);
        iFd = -1;
    }
    else
    {
        UIPC_FD_Attach(pstFp, pstSocket);
    }

    return iFd;
}

int uipc_close(IN INT iFd)
{
    SOCKET_S *so;
    UPIC_FD_S *pstFp;

    pstFp = UIPC_FD_GetFp(iFd);
    if (pstFp == NULL)
    {
        return EBADF;
    }

    so = UIPC_FD_GetSocket(pstFp);

    return soclose(so);
}

static int kern_bind(IN INT fd, IN struct sockaddr *pstAddr)
{
    SOCKET_S *so;
    UPIC_FD_S *pstFp;
    int error;

    pstFp = UIPC_FD_GetFp(fd);
    if (pstFp == NULL)
    {
        return EBADF;
    }

    so = UIPC_FD_GetSocket(pstFp);

    error = sobind(so, pstAddr);

    return (error);
}

int uipc_bind(IN int s, IN struct sockaddr *addr, IN UINT addrlen)
{
    int error;

    error = kern_bind(s, addr);

    return (error);
}

int uipc_listen(IN int s, IN int iBackLog)
{
    SOCKET_S *so;
    int error;
    UPIC_FD_S *pstFp;

    pstFp = UIPC_FD_GetFp(s);
    if (pstFp == NULL)
    {
        return EBADF;
    }

    so = UIPC_FD_GetSocket(pstFp);

    error = solisten(so, iBackLog);

    return(error);
}

static int kern_accept(IN int s, OUT SOCKADDR_S **ppstName, INOUT socklen_t *namelen, OUT VOID **fp)
{
    struct sockaddr *sa = NULL;
    int error;
    SOCKET_S *head, *so;
    int fd;
    UPIC_FD_S *pstHeadFp;
    UPIC_FD_S *pstFq;

    if (ppstName != NULL)
    {
        *ppstName = NULL;
    }

    if (fp != NULL)
    {
        *fp = NULL;
    }

    pstHeadFp = UIPC_FD_GetFp(s);
    if (pstHeadFp == NULL)
    {
        return EBADF;
    }

    head = UIPC_FD_GetSocket(pstHeadFp);

    if ((head->so_options & SO_ACCEPTCONN) == 0)
    {
        return EINVAL;
    }

    fd = UIPC_FD_FAlloc(&pstFq);
    if (fd < 0)
    {
        return ENOTSOCK;
    }
    
    ACCEPT_LOCK();
    if (((pstHeadFp->f_flags & O_NONBLOCK) != 0)
        && (TAILQ_EMPTY(&head->so_comp)))
    {
        ACCEPT_UNLOCK();
        UIPC_FD_FClose(pstFq);
        return EWOULDBLOCK;
    }

    while ((TAILQ_EMPTY(&head->so_comp)) && (head->so_error == 0))
    {
        if ((head->so_rcv.sb_state & SBS_CANTRCVMORE) != 0)
        {
            head->so_error = ECONNABORTED;
            break;
        }

        so_msleep(head->so_wait, ACCEPT_SEM(), BS_WAIT_FOREVER);
    }

    if (head->so_error != 0)
    {
        error = head->so_error;
        head->so_error = 0;
        ACCEPT_UNLOCK();
        goto noconnection;
    }

    so = TAILQ_FIRST(&head->so_comp);

    /*
     * Before changing the flags on the socket, we have to bump the
     * reference count.  Otherwise, if the protocol calls sofree(),
     * the socket will be released due to a zero refcount.
     */
    SOCK_LOCK(so);            /* soref() and so_state update */
    soref(so);            /* file descriptor reference */

    TAILQ_REMOVE(&head->so_comp, so, so_list);
    head->so_qlen--;
    so->so_qstate &= ~SQ_COMP;
    so->so_head = NULL;

    SOCK_UNLOCK(so);
    ACCEPT_UNLOCK();

    UIPC_FD_Attach(pstFq, so);

    sa = 0;
    error = soaccept(so, &sa);
    if (error != 0)
    {
        /*
             * return a namelen of zero for older code which might
             * ignore the return value from accept.
             */
        if (ppstName != NULL)
        {
            *namelen = 0;
        }

        goto noconnection;
    }
    
    if (sa == NULL)
    {
        if (ppstName != NULL)
        {
            *namelen = 0;
        }
        goto done;
    }

    if (ppstName != NULL)
    {
        /* check sa_len before it is destroyed */
        if (*namelen > sa->sa_len)
        {
            *namelen = sa->sa_len;
        }
        *ppstName = sa;
        sa = NULL;
    }
    
noconnection:
    if (sa != NULL)
    {
        MEM_Free(sa);
    }

    /*
     * close the new descriptor, assuming someone hasn't ripped it
     * out from under us.
     */
    if (error != 0)
    {
        UIPC_FD_FClose(pstFq);
        pstFq = NULL;
    }

    /*
     * Release explicitly held references before returning.  We return
     * a reference on nfp to the caller on success if they request it.
     */
done:
    if (fp != NULL)
    {
        if (error == 0)
        {
            *fp = pstFq;
            pstFq = NULL;
        }
        else
        {
            *fp = NULL;
        }
    }

    if (pstFq != NULL)
    {
        UIPC_FD_FClose(pstFq);
        pstFq = NULL;
    }

    return (error);
}


int uipc_accept(IN int s, OUT SOCKADDR_S *pstAddr, INOUT INT *piAddrLen)
{
    struct file *fp;
    int error;
    SOCKADDR_S *pstRetAddr = NULL;

    if (pstAddr == NULL)
    {
        return (kern_accept(s, NULL, NULL, NULL));
    }

    error = kern_accept(s, &pstRetAddr, piAddrLen, &fp);
    if (error != 0)
    {
        return (error);
    }

    if ((NULL != pstRetAddr) && (*piAddrLen != 0))
    {
        MEM_Copy(pstAddr, pstRetAddr, *piAddrLen);
    }

    return (error);
}

static int kern_connect(IN int fd, IN SOCKADDR_S *pstAddr)
{
    SOCKET_S *so;
    int error;
    UPIC_FD_S *pstFp;

    pstFp = UIPC_FD_GetFp(fd);
    if (pstFp == NULL)
    {
        return EBADF;
    }
    so = UIPC_FD_GetSocket(pstFp);

    if ((so->so_state & SS_ISCONNECTING) == 0)
    {
        error = soconnect(so, pstAddr);
        if (error != 0)
        {
            return error;
        }
    }

    if (((pstFp->f_flags & O_NONBLOCK) != 0)
        && ((so->so_state & SS_ISCONNECTING) != 0))
    {
        return EINPROGRESS;
    }

    SOCK_LOCK(so);
    while (((so->so_state & SS_ISCONNECTING) != 0) && so->so_error == 0)
    {
        so_msleep(so->so_wait, SOCK_SEM(so), BS_WAIT_FOREVER);
    }

    error = so->so_error;
    so->so_error = 0;

    SOCK_UNLOCK(so);

    return (error);
}

int uipc_connect(IN int s, IN SOCKADDR_S *pstAddr, IN int iAddrLen)
{
    int error;

    error = kern_connect(s, pstAddr);

    return (error);
}

static int sockargs(OUT MBUF_S **ppstMbuf, IN UCHAR *pucBuf, IN INT iBufLen)
{
    MBUF_S *m;

    if (iBufLen > 512)
    {
        return (EINVAL);
    }

    m = MBUF_CreateByCopyBuf(0, pucBuf, iBufLen, 0);
    if (NULL == m)
    {
        return (ENOBUFS);
    }

    *ppstMbuf = m;

    return 0;
}

static int kern_sendit
(
    IN int s,
    IN MSGHDR_S *mp,
    IN int flags,
    IN MBUF_S *control
)
{
    UIO_S auio;
    IOVEC_S *iov;
    int i;
    int len, error;
    SOCKET_S *so;
    UPIC_FD_S *pstFp;

    pstFp = UIPC_FD_GetFp(s);
    if (pstFp == NULL)
    {
        return EBADF;
    }
    so = UIPC_FD_GetSocket(pstFp);

    auio.uio_iov = mp->msg_iov;
    auio.uio_iovcnt = (int)mp->msg_iovlen;
    auio.uio_rw = UIO_WRITE;
    auio.uio_offset = 0;
    auio.uio_resid = 0;
    iov = mp->msg_iov;
    for (i = 0; i < (int)mp->msg_iovlen; i++, iov++)
    {
        if ((auio.uio_resid += (int)iov->iov_len) < 0)
        {
            error = EINVAL;
            goto bad;
        }
    }
    
    len = auio.uio_resid;
    error = sosend(so, mp->msg_name, &auio, 0, control, flags);
    if (error != 0)
    {
        if (auio.uio_resid != len && (error == EINTR || error == EWOULDBLOCK))
        {
            error = 0;
        }
    }

    if (error == 0)
    {
        error = len - auio.uio_resid;
    }

bad:
    return (error);
}

static int sendit(int s, struct msghdr *mp, int flags)
{
    MBUF_S *control;
    SOCKADDR_S *to;
    SOCKET_S *so;
    int error;
    UPIC_FD_S *pstFp;

    pstFp = UIPC_FD_GetFp(s);
    if (pstFp == NULL)
    {
        return EBADF;
    }
    so = UIPC_FD_GetSocket(pstFp);

    if (mp->msg_name != NULL)
    {
        to = mp->msg_name;
    }
    else
    {
        to = NULL;
    }

    if (((mp->msg_control) != 0) && ((mp->msg_controllen) != 0))
    {
        if (mp->msg_controllen < sizeof(CMSGHDR_S))
        {
            return EINVAL;
        }

        error = sockargs(&control, mp->msg_control, (int)mp->msg_controllen);
        if (error != 0)
        {
            return error;
        }
    }
    else
    {
        control = NULL;
    }

    error = kern_sendit(s, mp, flags, control);

    return error;
}

int uipc_sendto
(
    IN int    s,
    IN UCHAR *buf,
    IN size_t len,
    IN int    flags,
    IN SOCKADDR_S *pstTo,
    IN int    tolen
)
{
    struct msghdr msg;
    struct iovec aiov;
    int error;

    msg.msg_name = pstTo;
    msg.msg_namelen = tolen;
    msg.msg_iov = &aiov;
    msg.msg_iovlen = 1;
    msg.msg_control = 0;
    aiov.iov_base = buf;
    aiov.iov_len = len;

    error = sendit(s, &msg, flags);

    return (error);
}

int uipc_sendmsg
(
    IN int s,
    IN MSGHDR_S *msg,
    IN int flags
)
{
    int error;

    error = sendit(s, msg, flags);

    return (error);
}

int uipc_send(IN int iFd, UCHAR *pcBuf, IN int nLen, IN int iFlags) 
{
    IOVEC_S     stIov;
    MSGHDR_S    stMsg;

    stIov.iov_base = pcBuf;
    stIov.iov_len = nLen;

    stMsg.msg_name = NULL;
    stMsg.msg_namelen = 0;
    stMsg.msg_iov = &stIov;
    stMsg.msg_iovlen = 1;
    stMsg.msg_control = NULL;
    stMsg.msg_controllen=0;
    stMsg.msg_flags = 0;

    return sendit(iFd, &stMsg, iFlags);
}

static int kern_recvit
(
    IN int s,
    INOUT MSGHDR_S *mp,
    OUT MBUF_S **controlp
)
{
    UIO_S auio;
    IOVEC_S *iov;
    int i;
    socklen_t len;
    MBUF_S *control = 0;
    SOCKADDR_S *fromsa = 0;
    SOCKET_S *so;
    int error;
    UPIC_FD_S *pstFp;

    if(controlp != NULL)
    {
        *controlp = 0;
    }

    pstFp = UIPC_FD_GetFp(s);
    if (pstFp == NULL)
    {
        return EBADF;
    }
    so = UIPC_FD_GetSocket(pstFp);

    auio.uio_iov = mp->msg_iov;
    auio.uio_iovcnt = (int)mp->msg_iovlen;
    auio.uio_rw = UIO_READ;
    auio.uio_offset = 0;
    auio.uio_resid = 0;
    iov = mp->msg_iov;

    for (i = 0; i < (int)mp->msg_iovlen; i++, iov++)
    {
        if ((auio.uio_resid += (int)iov->iov_len) < 0)
        {
            return (EINVAL);
        }
    }

    len = (socklen_t)auio.uio_resid;
    error = soreceive(so, &fromsa, &auio, 0,
        (mp->msg_control != NULL || controlp != NULL) ? &control : 0,
        &mp->msg_flags);
    if (error != 0)
    {
        if (auio.uio_resid != (int)len && (error == EINTR || error == EWOULDBLOCK))
        {
            error = 0;
        }
    }

    if (error != 0)
    {
        goto out;
    }

    error = (int)len - auio.uio_resid;
    
    if (mp->msg_name != NULL)
    {
        len = mp->msg_namelen;
        if (fromsa == 0)
        {
            len = 0;
        }
        else
        {
            len = MIN(len, fromsa->sa_len);
            MEM_Copy(mp->msg_name, fromsa, len);
        }
        mp->msg_namelen = len;
    }

    if (mp->msg_control != NULL && controlp == NULL)
    {
        if (NULL != control)
        {
            if (mp->msg_controllen < MBUF_TOTAL_DATA_LEN(control))
            {
                len = (socklen_t)mp->msg_controllen;
                mp->msg_flags |= MSG_CTRUNC;
            }
            else
            {
                len = MBUF_TOTAL_DATA_LEN(control);
            }

            MBUF_CopyFromMbufToBuf(control, 0, len, mp->msg_control);
            mp->msg_controllen = len;
        }
        else
        {
            mp->msg_controllen = 0;
        }
    }
out:
    if (fromsa != NULL)
    {
        MEM_Free(fromsa);
    }

    if (error >= 0 && controlp != NULL)
    {
        *controlp = control;
    }
    else if (control != NULL)
    {
        MBUF_Free(control);
    }

    return (error);
}

static int recvit
(
    IN int s,
    INOUT MSGHDR_S *mp,
    INOUT socklen_t *namelenp
)
{
    int error;

    error = kern_recvit(s, mp, NULL);
    if (error != 0)
    {
        return (error);
    }

    if (namelenp != NULL)
    {
        *namelenp = mp->msg_namelen;
    }
    return (error);
}

int uipc_recvfrom
(
    IN int s,
    OUT UCHAR* buf,
    IN int len,
    IN int flags,
    OUT SOCKADDR_S *from,
    INOUT int *fromlenaddr
)
{
    MSGHDR_S msg;
    IOVEC_S stIov;

    if (from == NULL)
    {
        msg.msg_namelen = 0;
    }

    msg.msg_name = from;
    msg.msg_iov = &stIov;
    msg.msg_iovlen = 1;
    stIov.iov_base = buf;
    stIov.iov_len = len;
    msg.msg_control = 0;
    msg.msg_flags = flags;

    return recvit(s, &msg, fromlenaddr);
}


int uipc_recvmsg
(
    IN int s,
    IN MSGHDR_S *msg,
    IN int    flags
)
{
    int error;

    msg->msg_flags = flags;
    error = recvit(s, msg, NULL);
    return (error);
}

int uipc_recv
(
    int iFd,
    CHAR *pBuf,
    int nLen,
    int iFlags
)
{
    MSGHDR_S    stMsg;
    IOVEC_S     stIov;

    stIov.iov_base = pBuf;
    stIov.iov_len = nLen;

    stMsg.msg_name = NULL;
    stMsg.msg_namelen = 0;
    stMsg.msg_iov = &stIov;
    stMsg.msg_iovlen = 1;
    stMsg.msg_control = NULL;
    stMsg.msg_controllen=0;
    stMsg.msg_flags = iFlags;

    return recvit(iFd,&stMsg,NULL);
}

static int kern_setsockopt
(
    IN int s,
    IN int level,
    IN int name,
    IN void *val,
    IN int valsize
)
{
    SOCKOPT_S sopt;
    SOCKET_S *so;
    int error;
    UPIC_FD_S *pstFp;

    pstFp = UIPC_FD_GetFp(s);
    if (pstFp == NULL)
    {
        return EBADF;
    }
    so = UIPC_FD_GetSocket(pstFp);

    if (val == NULL && valsize != 0)
    {
        return (EFAULT);
    }

    if (valsize < 0)
    {
        return (EINVAL);
    }

    sopt.sopt_dir = SOPT_SET;
    sopt.sopt_level = level;
    sopt.sopt_name = name;
    sopt.sopt_val = val;
    sopt.sopt_valsize = valsize;

    error = sosetopt(so, &sopt);

    return(error);
}

int uipc_setsockopt
(
    IN int s,
    IN int level,
    IN int name,
    IN VOID * val,
    IN int valsize
)
{
    return kern_setsockopt(s, level, name, val, valsize);
}

static int kern_getsockopt
(
    IN int s,
    IN int level,
    IN int name,
    OUT void *val,
    INOUT socklen_t *valsize
)
{
    int error;
    SOCKOPT_S sopt;
    SOCKET_S *so;
    UPIC_FD_S *pstFp;

    if (val == NULL)
    {
        *valsize = 0;
    }

    if ((int)*valsize < 0)
    {
        return (EINVAL);
    }

    sopt.sopt_dir = SOPT_GET;
    sopt.sopt_level = level;
    sopt.sopt_name = name;
    sopt.sopt_val = val;
    sopt.sopt_valsize = (size_t)*valsize; /* checked non-negative above */

    pstFp = UIPC_FD_GetFp(s);
    if (pstFp == NULL)
    {
        return EBADF;
    }

    so = UIPC_FD_GetSocket(pstFp);

    error = sogetopt(so, &sopt);

    *valsize = (socklen_t)sopt.sopt_valsize;

    return (error);
}

int uipc_getsockopt
(
    IN int s,
    IN int level,
    IN int name,
    OUT void * val,
    INOUT socklen_t *pvalsize
)
{
    int    error;

    error = kern_getsockopt(s, level, name, val, pvalsize);

    return (error);
}

static int kern_getsockname(IN int fd, OUT SOCKADDR_S **sa, INOUT socklen_t *alen)
{
    socklen_t len;
    int error;
    SOCKET_S *so;
    UPIC_FD_S *pstFp;

    if (*alen == 0)
    {
        return (EINVAL);
    }

    pstFp = UIPC_FD_GetFp(fd);
    if (pstFp == NULL)
    {
        return EBADF;
    }
    so = UIPC_FD_GetSocket(pstFp);

    *sa = NULL;
    error = sogetsockname(so, sa);
    if (error != 0)
    {
        goto bad;
    }

    if (*sa == NULL)
    {
        len = 0;
    }
    else
    {
        len = MIN(*alen, (*sa)->sa_len);
    }
    *alen = len;
bad:
    if (error != 0 && *sa != NULL)
    {
        MEM_Free(*sa);
        *sa = NULL;
    }
    return (error);
}

int uipc_getsockname
(
    IN int fd,
    OUT SOCKADDR_S * pstSa,
    INOUT socklen_t * pLen
)
{
    SOCKADDR_S *sa;
    socklen_t len = *pLen;
    int error;

    error = kern_getsockname(fd, &sa, &len);
    if (error != 0)
    {
        return (error);
    }

    if (len != 0)
    {
        MEM_Copy(pstSa, sa, len);
        MEM_Free(sa);
    }

    if (error == 0)
    {
        *pLen = len;
    }

    return (error);
}

static int kern_getpeername(IN int fd, OUT SOCKADDR_S **sa, INOUT socklen_t *alen)
{
    socklen_t len;
    int error;
    SOCKET_S *so;
    UPIC_FD_S *pstFp;

    if (*alen == 0)
    {
        return (EINVAL);
    }

    pstFp = UIPC_FD_GetFp(fd);
    if (pstFp == NULL)
    {
        return EBADF;
    }
    so = UIPC_FD_GetSocket(pstFp);

    if ((so->so_state & (SS_ISCONNECTED|SS_ISCONFIRMING)) == 0)
    {
        return ENOTCONN;
    }

    *sa = NULL;
    error = sogetpeername(so, sa);
    if (error != 0)
    {
        goto bad;
    }

    if (*sa == NULL)
    {
        len = 0;
    }
    else
    {
        len = MIN(*alen, (*sa)->sa_len);
    }
    *alen = len;

bad:
    if (error != 0 && *sa != NULL)
    {
        MEM_Free(*sa);
        *sa = NULL;
    }

    return (error);
}

int uipc_getpeername
(
    IN int fd,
    OUT SOCKADDR_S *pstSa,
    INOUT socklen_t *pLen
)
{
    SOCKADDR_S *sa;
    socklen_t len = *pLen;
    int error;

    error = kern_getpeername(fd, &sa, &len);
    if (error != 0)
    {
        return (error);
    }

    if (len != 0)
    {
        MEM_Copy(pstSa, sa, len);
        MEM_Free(sa);
    }
    if (error == 0) {
        *pLen = len;
    }
    return (error);
}


