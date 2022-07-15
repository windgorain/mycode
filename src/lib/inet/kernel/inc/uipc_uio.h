/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2012-11-21
* Description: 
* History:     
******************************************************************************/

#ifndef __UIPC_UIO_H_
#define __UIPC_UIO_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

/*
 * UIO types
 */
enum uio_rw
{
    UIO_READ,
    UIO_WRITE
};

typedef struct uio {
    struct iovec *uio_iov;        /* scatter/gather list */
    int    uio_iovcnt;            /* length of scatter/gather list */
    int    uio_offset;            /* offset in target object */
    int    uio_resid;             /* remaining bytes to process */
    enum   uio_rw uio_rw;         /* operation */
}UIO_S;


#ifdef __cplusplus
    }
#endif /* __cplusplus */

#endif /*__UIPC_UIO_H_*/


