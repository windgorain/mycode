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
#endif 


enum uio_rw
{
    UIO_READ,
    UIO_WRITE
};

typedef struct uio {
    struct iovec *uio_iov;        
    int    uio_iovcnt;            
    int    uio_offset;            
    int    uio_resid;             
    enum   uio_rw uio_rw;         
}UIO_S;


#ifdef __cplusplus
    }
#endif 

#endif 


