/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2012-12-18
* Description: 
* History:     
******************************************************************************/
#include "bs.h"

#include "utl/mbuf_utl.h"
#include "inet/in_pub.h"

#include "protosw.h"
#include "domain.h"
#include "uipc_def.h"
#include "uipc_socket.h"
#include "uipc_sock_func.h"
#include "uipc_func.h"
#include "uipc_compat.h"


int uiomove(IN void *cp, IN int n, IN UIO_S *uio)
{
    struct iovec *iov;
    UINT cnt;
    int error = 0;

    BS_DBGASSERT(uio->uio_rw == UIO_READ || uio->uio_rw == UIO_WRITE);

    while (n > 0 && uio->uio_resid > 0)
    {
        iov = uio->uio_iov;
        cnt = (UINT)iov->iov_len;
        if (cnt == 0)
        {
            uio->uio_iov++;
            uio->uio_iovcnt--;
            continue;
        }

        if (cnt > (UINT)n)
        {
            cnt = (UINT)n;
        }

        if (uio->uio_rw == UIO_READ)
        {
            memcpy(iov->iov_base, cp, cnt);
        }
        else
        {
            memcpy(cp, iov->iov_base, cnt);
        }

        iov->iov_base = (char *)iov->iov_base + cnt;
        iov->iov_len -= cnt;
        uio->uio_resid -= (int)cnt;
        uio->uio_offset += (long)(int)cnt;
        cp = (char *)cp + cnt;
        n -= (int)cnt;
    }

    return (error);
}



MBUF_S *m_uiotombuf
(
    IN UIO_S *uio,
    IN int len,
    IN int align
)
{
    MBUF_S *m = NULL;
    MBUF_S *head = NULL;
    int total;
    struct iovec *iov;
    UINT cnt;

    
    if (len > 0)
    {
        total = MIN(uio->uio_resid, len);
    }
    else
    {
        total = uio->uio_resid;
    }

    m = MBUF_Create(MT_DATA, align);
    if (NULL == m)
    {
        return NULL;
    }

    while (total > 0 && uio->uio_resid > 0)
    {
        iov = uio->uio_iov;
        cnt = (UINT)iov->iov_len;
        if (cnt == 0)
        {
            uio->uio_iov++;
            uio->uio_iovcnt--;
            continue;
        }

        if (cnt > (UINT)total)
        {
            cnt = (UINT)total;
        }

        if (BS_OK != MBUF_CopyFromBufToMbuf(m, MBUF_TOTAL_DATA_LEN(m), iov->iov_base, cnt))
        {
            MBUF_Free(m);
            return NULL;
        }

        iov->iov_base = (char *)iov->iov_base + cnt;
        iov->iov_len -= cnt;
        uio->uio_resid -= (int)cnt;
        uio->uio_offset += (long)(int)cnt;
        total -= (int)cnt;
    }

    return m;
}

void bcopy(const void *src0, void *dst0, UINT length)
{
    (void)memcpy(dst0, src0, length);
    return;
}

