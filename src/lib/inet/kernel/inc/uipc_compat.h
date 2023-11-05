/******************************************************************************
* Copyright (C) LiXingang
* Author:      LiXingang  Version: 1.0  Date: 2012-12-18
* Description: 
* History:     
******************************************************************************/

#ifndef __UIPC_COMPAT_H_
#define __UIPC_COMPAT_H_

#ifdef __cplusplus
    extern "C" {
#endif 

#define caddr_t UCHAR*

#define so_wakeup SEM_VAll

#define M_PREPEND(m, plen) \
    do { \
        if (BS_OK != MBUF_Append(m, plen)) \
        { \
            (void)MBUF_Free(m); \
            m = NULL; \
        } \
    } while (0)

#define m_freem(m) \
    do { \
        if (NULL != m) { \
            (void)MBUF_Free(m); \
        } \
    } while (0)

MBUF_S *m_uiotombuf
(
    IN UIO_S *uio,
    IN int len,
    IN int align
);

void bcopy(const void *src0, void *dst0, UINT length);

#ifdef __cplusplus
    }
#endif 

#endif 


