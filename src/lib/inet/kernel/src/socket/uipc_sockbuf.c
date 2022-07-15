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

#define    SB_MAX        (256*1024)    /* default for max chars in sockbuf */

#define SBLINKRECORD(sb, m0) do {                    \
    if ((sb)->sb_lastrecord != NULL)                \
        MBUF_SET_NEXT_MBUF((sb)->sb_lastrecord, m0); \
    else                                \
        (sb)->sb_mb = (m0);                    \
    (sb)->sb_lastrecord = (m0);                    \
} while (/*CONSTCOND*/0)

static void    sbflush_internal(IN SOCKBUF_S *sb, IN SOCKET_S *so);

/* 返回0:正常; EWOULDBLOCK: 超时 */
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

/* 和socantsendmore功能类似，但是调用者需要已将发送缓存加锁，并在此接口中释放锁 */
void socantsendmore_locked(IN SOCKET_S *so)
{
    SOCKBUF_LOCK_ASSERT(&so->so_snd);

    so->so_snd.sb_state |= SBS_CANTSENDMORE;
    sowwakeup_locked(so);
    mtx_assert(SOCKBUF_MTX(&so->so_snd), MA_NOTOWNED);
}

/* 设置SOCKET对象为不可发送状态，并唤醒阻塞在发送缓存上的进程 */
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

/* 设置SOCKET对象为不可接收状态，并唤醒阻塞在接收缓存上的进程 */
void socantrcvmore(struct socket *so)
{

    SOCKBUF_LOCK(&so->so_rcv);
    socantrcvmore_locked(so);
    mtx_assert(SOCKBUF_MTX(&so->so_rcv), MA_NOTOWNED);
}

/* 在接收或者发送缓存上阻塞等待。有数据空间或者超时时，则会返回 */
int sbwait(struct sockbuf *sb)
{

    SOCKBUF_LOCK_ASSERT(sb);

    sb->sb_flags |= SB_WAIT;

    return (so_msleep(sb->sb_wait, sb->sb_mtx, sb->sb_timeo));
}

/*
对sockbuf信号量进行down操作。如果socket对象为阻塞方式，获取到信号
量后返回；若socket对象为不可阻塞方式，且没有获取到信号量，则返回
*/
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

/* 对sockbuf信号量进行up操作。 */
void sbunlock(IN SOCKBUF_S *sb)
{
    MUTEX_V(&sb->sb_sx);
}

/*
唤醒阻塞在SOCKET对象接收或者发送缓存的进程
*/
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

/* 为socket对象接收和发送缓存设置高水位，即设置了接收、发送缓存大小 */
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

/*
为socket对象接收或发送缓存设置高水位，即设置了接收或者发送缓存大
小，此函数和sbreserve的差别在于调用此接口时，调用者已经对SOCKBUF
加锁保护
*/
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

/*
为socket对象接收或发送缓存设置高水位，即设置接收或者发送缓存大小
*/
int sbreserve(IN SOCKBUF_S *sb, IN UINT cc, IN SOCKET_S *so)
{
    int error;

    SOCKBUF_LOCK(sb);
    error = sbreserve_locked(sb, cc, so);
    SOCKBUF_UNLOCK(sb);
    return (error);
}

/*
 * Free mbufs held by a socket, and reserved mbuf space.
 */
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

/*
将接收或者待发送的数据添加到接收缓存或者发送缓存的MBUF链中，此
接口和sbappend的区别则在于调用此接口时需要对缓存进行加锁
*/
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

    /* If datalen is 0, we don't append it. */
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

/* 此接口专门提供给面向流模式的协议使用，将接收或者待发送的数据添加
到接收缓存或者发送缓存的MBUF链中，和sbappend_locked的区别则在于因
为面向流的协议的缓存中，只有一条MBUF链表，直接添加到此链表后面即可 */
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
    /* If datalen is 0, we don't append it. */
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

/*
将接收或者待发送的MBUF作为一条新的MBUF链添加在接收或者发送缓存中,
此接口和sbappendrecord的区别则在于调用此接口时，调用者需要对缓存
加锁
*/
void sbappendrecord_locked(IN SOCKBUF_S *sb, IN MBUF_S *m0)
{
    UINT len;

    SOCKBUF_LOCK_ASSERT(sb);

    if (NULL == m0)
    {
        return;
    }
    len = MBUF_TOTAL_DATA_LEN(m0);
    /* If datalen is 0, we don't append it. */
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

/*
为传入的地址结构创建地址MBUF，并将地址MBUF、控制MBUF、和数据MBUF
串在一条MBUF链中，并将新的MBUF链添加在接收或者发送缓存中。此接口
和sbappendaddr的区别则在于调用此接口前需要对缓存加锁
*/
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

        /* something may be copied to m if needed */
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

    /*
     * Hayek: Now we release control and data mbuf, and the callers cann't
     * use control and m0 either.
     */
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

/*
将传入的控制MBUF、和数据MBUF串在一条MBUF链中，并将新的MBUF链添加
在接收或者发送缓存中，和sbappendcontrol的区别在于调用此接口前需要
对缓存加锁保护
*/
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
        /* something may be copied to m if needed */
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

/*
清空接收或者发送缓存中所有MBUF链中的所有数据，
此接口和sbflush的区别在于调用此接口前需要对缓存加锁保护
*/
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

/* 清空接收或者发送缓存中所有MBUF链中的所有数据 */
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

/* 在传入的接收或者发送缓存的MBUF链表前部，丢弃指定长度的数据 */
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

/* 释放接收或者发送缓存中第一条MBUF链 */
void sbdroprecord(struct sockbuf *sb)
{
    SOCKBUF_LOCK(sb);
    sbdroprecord_locked(sb);
    SOCKBUF_UNLOCK(sb);
}

/* 根据传入的数据长度创建控制MBUF结构，并将传入的数据、类型拷贝到MBUF */
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

