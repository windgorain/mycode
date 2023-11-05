/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2012-11-21
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

#define    SB_MAX        (256*1024)    

#define SBLINKRECORD(sb, m0) do {                    \
    if ((sb)->sb_lastrecord != NULL)                \
        MBUF_SET_NEXT_MBUF((sb)->sb_lastrecord, m0); \
    else                                \
        (sb)->sb_mb = (m0);                    \
    (sb)->sb_lastrecord = (m0);                    \
} while (0)

static void    sbflush_internal(IN SOCKBUF_S *sb, IN SOCKET_S *so);


int so_msleep(IN SEM_HANDLE hSemToP, IN SEM_HANDLE hSemToV, IN int timeo)
{
    BS_STATUS eRet;
    
    eRet = SEM_PV(hSemToP, hSemToV, BS_WAIT, timeo);

    SEM_P(hSemToV, BS_WAIT, BS_WAIT_FOREVER);

    if (BS_OK == eRet)
    {
        return 0;
    }

    return EWOULDBLOCK;
}


void socantsendmore_locked(IN SOCKET_S *so)
{
    SOCKBUF_LOCK_ASSERT(&so->so_snd);

    so->so_snd.sb_state |= SBS_CANTSENDMORE;
    sowwakeup_locked(so);
    mtx_assert(SOCKBUF_MTX(&so->so_snd), MA_NOTOWNED);
}


void socantsendmore(IN SOCKET_S *so)
{
    SOCKBUF_LOCK(&so->so_snd);
    socantsendmore_locked(so);
    mtx_assert(SOCKBUF_MTX(&so->so_snd), MA_NOTOWNED);
}

void socantrcvmore_locked(IN SOCKET_S *so)
{
    SOCKBUF_LOCK_ASSERT(&so->so_rcv);

    so->so_rcv.sb_state |= SBS_CANTRCVMORE;
    sorwakeup_locked(so);
    mtx_assert(SOCKBUF_MTX(&so->so_rcv), MA_NOTOWNED);
}


void socantrcvmore(struct socket *so)
{

    SOCKBUF_LOCK(&so->so_rcv);
    socantrcvmore_locked(so);
    mtx_assert(SOCKBUF_MTX(&so->so_rcv), MA_NOTOWNED);
}


int sbwait(struct sockbuf *sb)
{

    SOCKBUF_LOCK_ASSERT(sb);

    sb->sb_flags |= SB_WAIT;

    return (so_msleep(sb->sb_wait, sb->sb_mtx, sb->sb_timeo));
}


int sblock(IN SOCKBUF_S *sb, IN int flags)
{
    BS_DBGASSERT((flags & SBL_VALID) == flags);

    if (flags & SBL_WAIT)
    {
        MUTEX_P(&sb->sb_sx);
    }
    else
    {
        if (MUTEX_TryP(&sb->sb_sx) == FALSE)
        {
            return (EWOULDBLOCK);
        }
    }

    return 0;
}


void sbunlock(IN SOCKBUF_S *sb)
{
    MUTEX_V(&sb->sb_sx);
}


void sowakeup(IN SOCKET_S *so, IN SOCKBUF_S *sb)
{
    SOCKBUF_LOCK_ASSERT(sb);

    if  ((sb->sb_flags & SB_WAIT)
        || (SEM_CountPending(sb->sb_wait)))
    {
        sb->sb_flags &= ~SB_WAIT;
        SEM_V(sb->sb_wait);
    }
    SOCKBUF_UNLOCK(sb);

    if (sb->sb_flags & SB_UPCALL)
    {
        (*so->so_upcall)(so, so->so_upcallarg, 0);
    }

    mtx_assert(SOCKBUF_MTX(sb), MA_NOTOWNED);
}


int soreserve(IN SOCKET_S *so, IN UINT sndcc, IN UINT rcvcc)
{
    SOCKBUF_LOCK(&so->so_snd);
    if (sbreserve_locked(&so->so_snd, sndcc, so) == 0)
        goto bad;
    if (so->so_snd.sb_lowat == 0)
        so->so_snd.sb_lowat = 512;
    if (so->so_snd.sb_lowat > so->so_snd.sb_hiwat)
        so->so_snd.sb_lowat = so->so_snd.sb_hiwat;
    SOCKBUF_UNLOCK(&so->so_snd);

    SOCKBUF_LOCK(&so->so_rcv);    
    if (sbreserve_locked(&so->so_rcv, rcvcc, so) == 0)
        goto bad2;
    if (so->so_rcv.sb_lowat == 0)
        so->so_rcv.sb_lowat = 1;
    SOCKBUF_UNLOCK(&so->so_rcv);

    return (0);
    
bad2:
    SOCKBUF_UNLOCK(&so->so_rcv);
    SOCKBUF_LOCK(&so->so_snd);
    sbrelease_locked(&so->so_snd, so);
bad:
    SOCKBUF_UNLOCK(&so->so_snd);
    return (ENOBUFS);
}


int sbreserve_locked(IN SOCKBUF_S *sb, IN UINT cc, IN SOCKET_S *so)
{
    SOCKBUF_LOCK_ASSERT(sb);

    if (cc > SB_MAX)
        return 0;

    sb->sb_hiwat = cc;

    if (sb->sb_lowat > sb->sb_hiwat)
        sb->sb_lowat = sb->sb_hiwat;
    return (1);
}


int sbreserve(IN SOCKBUF_S *sb, IN UINT cc, IN SOCKET_S *so)
{
    int error;

    SOCKBUF_LOCK(sb);
    error = sbreserve_locked(sb, cc, so);
    SOCKBUF_UNLOCK(sb);
    return (error);
}


static void sbrelease_internal(IN SOCKBUF_S *sb, IN SOCKET_S *so)
{
    sbflush_internal(sb, so);
    sb->sb_hiwat = 0;
}

void sbrelease_locked(IN SOCKBUF_S *sb, IN SOCKET_S *so)
{
    SOCKBUF_LOCK_ASSERT(sb);

    sbrelease_internal(sb, so);
}

void sbrelease(IN SOCKBUF_S *sb, IN SOCKET_S *so)
{
    SOCKBUF_LOCK(sb);
    sbrelease_locked(sb, so);
    SOCKBUF_UNLOCK(sb);
}

void sbdestroy(IN SOCKBUF_S *sb, IN SOCKET_S *so)
{
    sbrelease_internal(sb, so);
}


void sbappend_locked(IN SOCKBUF_S *sb, IN MBUF_S *m)
{
    UINT len = 0;
    BS_STATUS ret;

    SOCKBUF_LOCK_ASSERT(sb);

    if (NULL == m)
    {
        return;
    }
    SBLASTRECORDCHK(sb);
    len = MBUF_TOTAL_DATA_LEN(m);

    
    if (0 == len)
    {
        MBUF_Free(m);
        return;
    }
    if (NULL == sb->sb_lastrecord)
    {
        BS_DBGASSERT(NULL == sb->sb_mb);
        sb->sb_lastrecord = sb->sb_mb = m;
    }
    else
    {
        ret = MBUF_Cat(sb->sb_lastrecord, m);
        if (BS_OK != ret)
        {
            MBUF_Free(m);
            return;
        }
    }
    sballoc(sb, len);
    return;
}

void sbappend(IN SOCKBUF_S *sb, IN MBUF_S *m)
{
    SOCKBUF_LOCK(sb);
    sbappend_locked(sb, m);
    SOCKBUF_UNLOCK(sb);
}


void sbappendstream_locked(IN SOCKBUF_S *sb, IN MBUF_S *m)
{
    MBUF_S *nextmbuf = NULL;
    UINT len = 0;
    BS_STATUS ret;

    SOCKBUF_LOCK_ASSERT(sb);

    if (NULL == m)
    {
        return;
    }

    nextmbuf = MBUF_GET_NEXT_MBUF(m);
    BS_DBGASSERT(nextmbuf == NULL);
    BS_DBGASSERT(sb->sb_mb == sb->sb_lastrecord);

    len = MBUF_TOTAL_DATA_LEN(m);
    
    if (0 == len)
    {
        MBUF_Free(m);
        return;
    }
    if (NULL == sb->sb_mb)
    {
        sb->sb_mb = m;
    }
    else
    {
        ret = MBUF_Cat(sb->sb_mb, m);
        if (BS_OK != ret)
        {
            MBUF_Free(m);
            return;
        }
    }
    sballoc(sb, len);

    sb->sb_lastrecord = sb->sb_mb;
    SBLASTRECORDCHK(sb);

    return;
}

void sbappendstream(IN SOCKBUF_S *sb, IN MBUF_S *m)
{
    SOCKBUF_LOCK(sb);
    sbappendstream_locked(sb, m);
    SOCKBUF_UNLOCK(sb);
}


void sbappendrecord_locked(IN SOCKBUF_S *sb, IN MBUF_S *m0)
{
    UINT len;

    SOCKBUF_LOCK_ASSERT(sb);

    if (NULL == m0)
    {
        return;
    }
    len = MBUF_TOTAL_DATA_LEN(m0);
    
    if (0 == len)
    {
        (void)MBUF_Free(m0);
        return;
    }
    SBLASTRECORDCHK(sb);

    SBLINKRECORD(sb, m0);
    sballoc(sb, len);
    return;
}

void sbappendrecord(IN SOCKBUF_S *sb, IN MBUF_S *m0)
{

    SOCKBUF_LOCK(sb);
    sbappendrecord_locked(sb, m0);
    SOCKBUF_UNLOCK(sb);
}


int sbappendaddr_locked
(
    IN SOCKBUF_S *sb,
    IN SOCKADDR_S *asa,
    IN MBUF_S *m0,
    IN MBUF_S *control
)
{
    MBUF_S *m;
    UINT space = asa->sa_len;
    ULONG ret = 0UL;
    MBUF_S *tmpcontrol = NULL;
    MBUF_S *tmpm0 = NULL;

    if (m0)
    {
        space += MBUF_TOTAL_DATA_LEN(m0);
    }

    if (control)
    {
        space += MBUF_TOTAL_DATA_LEN(control);
    }

    if ((int)space > sbspace(sb))
    {
        return (0);
    }

    m = MBUF_CreateByCopyBuf(0, (UCHAR*)asa, asa->sa_len, 0);
    if (NULL == m)
    {
        return (0);
    }

    if (control)
    {
        tmpcontrol = MBUF_ReferenceCopy(control, 0, MBUF_TOTAL_DATA_LEN(control));
        if (NULL == tmpcontrol)
        {
            MBUF_Free(m);
            return (0);
        }
        ret = MBUF_Cat(m, tmpcontrol);
        if (BS_OK != ret)
        {
            MBUF_Free(m);
            MBUF_Free(tmpcontrol);
            return (0);
        }
    }

    if (m0)
    {
        tmpm0 = MBUF_ReferenceCopy(m0, 0, MBUF_TOTAL_DATA_LEN(m0));
        if (NULL == tmpm0)
        {
            MBUF_Free(m);
            return (0);
        }

        
        ret = MBUF_Cat(m, tmpm0);
        if (BS_OK != ret)
        {
            MBUF_Free(m);
            MBUF_Free(tmpm0);
            return (0);
        }
    }
    SBLINKRECORD(sb, m);
    
    sballoc(sb, MBUF_TOTAL_DATA_LEN(m));

    
    if (NULL != control)
    {
        MBUF_Free(control);
    }
    if (NULL != m0)
    {
        MBUF_Free(m0);
    }
    return (1);
}

int sbappendaddr
(
    IN SOCKBUF_S *sb,
    IN SOCKADDR_S *asa,
    IN MBUF_S *m0,
    IN MBUF_S *control
)
{
    int retval;

    SOCKBUF_LOCK(sb);
    retval = sbappendaddr_locked(sb, asa, m0, control);
    SOCKBUF_UNLOCK(sb);
    return (retval);
}


int sbappendcontrol_locked
(
    IN SOCKBUF_S *sb,
    IN MBUF_S *m0,
    IN MBUF_S *control
)
{
    UINT space;
    BS_STATUS ret;

    SOCKBUF_LOCK_ASSERT(sb);

    BS_DBGASSERT(NULL != control);

    space = MBUF_TOTAL_DATA_LEN(control);

    if (m0)
    {
        space += MBUF_TOTAL_DATA_LEN(m0);
    }

    if ((int)space > sbspace(sb))
        return (0);

    if (m0)
    {
        
        ret = MBUF_Cat(control, m0);
        if (BS_OK != ret)
        {
            return (0);
        }
    }

    SBLASTRECORDCHK(sb);

    SBLINKRECORD(sb, control);
    sballoc(sb, MBUF_TOTAL_DATA_LEN(control));

    SBLASTRECORDCHK(sb);
    return (1);
}

int sbappendcontrol
(
    IN SOCKBUF_S *sb,
    IN MBUF_S *m0,
    IN MBUF_S *control
)
{
    int retval;

    SOCKBUF_LOCK(sb);
    retval = sbappendcontrol_locked(sb, m0, control);
    SOCKBUF_UNLOCK(sb);
    return (retval);
}


static void sbflush_internal(IN SOCKBUF_S *sb, IN SOCKET_S *so)
{
    MBUF_S *m, *next;
    UINT mlen;

    m = sb->sb_mb;
    while (NULL != m)
    {
        mlen = MBUF_TOTAL_DATA_LEN(m);
        next = MBUF_GET_NEXT_MBUF(m);
        (void)MBUF_Free(m);
        sbfree(sb, mlen);
        m = next;
    }

    sb->sb_mb = sb->sb_lastrecord = NULL;    
}

void sbflush_locked(IN SOCKBUF_S *sb, IN SOCKET_S *so)
{
    SOCKBUF_LOCK_ASSERT(sb);
    sbflush_internal(sb, so);
}


void sbflush(struct sockbuf *sb, struct socket *so)
{

    SOCKBUF_LOCK(sb);
    sbflush_locked(sb, so);
    SOCKBUF_UNLOCK(sb);
}

static void sbdrop_internal(IN SOCKBUF_S *sb, IN int len)
{
    MBUF_S *m;
    MBUF_S *next;

    m = sb->sb_mb;
    while ((NULL != m) && (len > 0))
    {
        int mlen = MBUF_TOTAL_DATA_LEN(m);
        next = MBUF_GET_NEXT_MBUF(m);
        if (mlen <= len)
        {
            MBUF_Free(m);
            sbfree(sb, mlen);
            len -= mlen;
        }
        else
        {
            (void)MBUF_CutHead(m, (UINT)len);
            sbfree(sb, len);
            break;
        }
        m = next;
    }
    if (NULL != m)
    {
        sb->sb_mb = m;
    }
    else
    {
        sb->sb_mb = sb->sb_lastrecord = NULL;
    }
    SBLASTRECORDCHK(sb);
    return;
}

void sbdrop_locked(IN SOCKBUF_S *sb, IN int len)
{
    SOCKBUF_LOCK_ASSERT(sb);

    sbdrop_internal(sb, len);
}


void sbdrop(IN SOCKBUF_S *sb, IN int len)
{

    SOCKBUF_LOCK(sb);
    sbdrop_locked(sb, len);
    SOCKBUF_UNLOCK(sb);
}

void sbdroprecord_locked(IN SOCKBUF_S *sb)
{
    MBUF_S *m;

    SOCKBUF_LOCK_ASSERT(sb);

    m = sb->sb_mb;
    if (NULL != m)
    {
        sb->sb_mb = MBUF_GET_NEXT_MBUF(m);

        sbfree(sb, MBUF_TOTAL_DATA_LEN(m));
        MBUF_Free(m);
    }
    SB_EMPTY_FIXUP(sb);
    return;
}


void sbdroprecord(struct sockbuf *sb)
{
    SOCKBUF_LOCK(sb);
    sbdroprecord_locked(sb);
    SOCKBUF_UNLOCK(sb);
}


MBUF_S * sbcreatecontrol(IN VOID * p, IN int size, IN int type, IN int level)
{
    CMSGHDR_S *cp;
    MBUF_S *m;

    m = MBUF_CreateByCopyBuf(0, p, size, MT_CONTROL);
    if (NULL == m)
    {
        return NULL;
    }

    if (BS_OK != MBUF_MakeContinue(m, size))
    {
        MBUF_Free(m);
        return NULL;
    }

    cp = MBUF_MTOD(m);

    if (p != NULL)
    {
        void *data = CMSG_DATA(cp);
        memcpy(data, p, size);
    }

    cp->cmsg_len = CMSG_LEN(size);
    cp->cmsg_level = level;
    cp->cmsg_type = type;

    return (m);
}

